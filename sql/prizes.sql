/****************************************************************************************
	抢红包
****************************************************************************************/
drop function if exists format_interval(interval);
drop function if exists grab_prizes(integer,bigint);
drop table if exists prizes;
drop table if exists prizes_conf;

/****************************************************************************************
	红包配置
	insert into prizes_conf(price,num,expiration) values(500,30,'[2018-11-26 15:30, 2018-11-26 15:40]')
	insert into prizes_conf(price,num,start,done) values(500,30,'2018-11-26 15:30:00','2018-11-26 15:40:00')
****************************************************************************************/
create table prizes_conf(
	objectid serial not null,										--唯一编号
	price bigint not null,											--红包总金额,精确至分
	num integer not null,										--红包数量
	--expiration tstzrange not null,							--有效期
	start timestamptz not null,								--开始时间
	done timestamptz not null,								--结束时间
	constraint pk_prizes_conf_objectid primary key(objectid) with (fillfactor=100)
)with (fillfactor=100);
/****************************************************************************************
	中奖信息
****************************************************************************************/
create table prizes(
	objectid bigserial not null,									--唯一编号
	confid integer not null,										--配置编号
	userid bigint not null,										--用户信息
	price bigint not null,											--中奖金额
	generate timestamptz default now() not null	,	--抽奖时间
	constraint pk_prizes_objectid primary key(objectid) with (fillfactor=100),
	constraint fk_prizes_confid foreign key(confid)  references prizes_conf(objectid) on delete cascade
)with (fillfactor=100);

create or replace function format_interval(interval) 
    returns text 
as $$
	declare
		v_day bigint;
		v_hour bigint;
		v_min bigint;
		v_sec bigint;
	begin
		--day=86400,hour=3600;min=60;
		select extract(epoch from $1) into v_sec;
		v_day := cast(v_sec/86400 as bigint);
		v_sec := v_sec - (v_day * 86400);
		v_hour := cast(v_sec/3600 as bigint);
		v_sec := v_sec - (v_hour * 3600);
		v_min :=  cast(v_sec/60 as bigint);
		v_sec := v_sec - (v_min * 60);
		if (0 = v_day and 0 = v_hour and 0 = v_min) then
			return format('%s秒',v_sec);
		elsif (0 = v_day and 0 = v_hour and  v_min > 0) then
			return format('%s分%s秒',v_min,v_sec);
		elsif (0 = v_day and v_hour>0 and  v_min > 0) then
			return format('%s小时%s分%s秒',v_hour,v_min,v_sec);
		else
			return format('%s天%s小时%s分%s秒',v_day,v_hour,v_min,v_sec);
		end if;
	end;
$$ language plpgsql strict;

/****************************************************************************************
	将第x个bit位设置为1,ibit的值范围0-31
00000000000000000000000000000000
****************************************************************************************/
create or replace function SE_flag_add(iflag integer, ibit integer) 
    returns integer 
as $$
	select iflag + (1<<ibit);
$$ language sql strict; 
/****************************************************************************************
	将第x个bit位设置为0,ibit的值范围0-31
00000000000000000000000000000000
****************************************************************************************/
create or replace function SE_flag_remove(iflag integer, ibit integer) 
    returns integer 
as $$
	select iflag - (1<<ibit);
$$ language sql strict;
/****************************************************************************************
	凑数第x个bit位是否为1,ibit的值范围0-31
00000000000000000000000000000000
****************************************************************************************/
create or replace function SE_flag_bitisone(iflag integer, ibit integer) 
    returns boolean 
as $$
	select  (case ((iflag >> ibit) & 1) when 1 then
		true
	else
		false
	end);
$$ language sql strict; 



create or replace function grab_prizes_lock(integer,bigint)
	returns bigint
as $$
	declare
		v_allmoney bigint;--红包大小
		v_allnum integer; --红包数量

		v_start timestamptz;
		v_done timestamptz;
		v_current timestamptz;

		v_money bigint;--剩余的钱
		v_num integer; --剩余红包数量

		v_flag integer;	--锁标志
	begin
		v_flag := 0;
		select  pg_advisory_lock($1::bigint),SE_flag_add(v_flag,0);
		--获取配置信息
		v_current := now();
		select price,num,start,done into v_allmoney,v_allnum,v_start,v_done from prizes_conf where objectid=$1;

		if( v_allmoney is null or v_allnum is null or  v_start is null or  v_done is null) then
			select pg_advisory_unlock($1::bigint),SE_flag_remove(v_flag,0);
			raise exception '本期抢红包信息未配置!';
		end if;

		if(v_current < v_start) then
			select pg_advisory_unlock($1::bigint),SE_flag_remove(v_flag,0);
			raise exception '本期抢红包还有"%s"开始!',format_interval(v_start -v_current);
		end if;

		if( v_current > v_done ) then
			select pg_advisory_unlock($1::bigint),SE_flag_remove(v_flag,0);
			raise exception '本期抢红包已经结束!';
		end if;

		select v_allmoney - coalesce(sum(price),0),v_allnum - count(*) into v_money,v_num from prizes where confid=$1;
		if( v_num < 1 or v_num >v_allnum ) then
			--红包已经抢完
			v_money := 0;
		elsif (1=v_num) then
			--最后一个红包,什么也不做,直接将剩余的钱给当前用户
		else
			v_money := cast((random() * (v_money / v_num * 2)) as bigint);
			if( v_money < 1 ) then
				v_money := 1;
			end if;
		end if;
		--保存抢红包信息
		insert into prizes(confid,userid,price) values($1,$2,v_money);
		if( v_money > 0 ) then
			select pg_advisory_lock($2),SE_flag_remove(v_flag,1);
			--更新用户钱包的中金额

			select pg_advisory_unlock($2),SE_flag_remove(v_flag,1);
		end if;
		select pg_advisory_unlock($1::bigint),SE_flag_remove(v_flag,0);
		return v_money;
		exception
			when others then
				if(SE_flag_bitisone(v_flag,0) ) then
					perform pg_advisory_unlock($1::bigint);
				end if;
				if(SE_flag_bitisone(v_flag,1) ) then
					perform pg_advisory_unlock($2);
				end if;
				raise exception '%', sqlerrm;				
	end;
$$ language plpgsql strict;


select grab_prizes_lock(1,1);