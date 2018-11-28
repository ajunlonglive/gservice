drop function if exists test_sqltype();
drop function if exists test_sqlarray();
drop function if exists test_json();
drop table if exists test_001;
drop table if exists test_002;
drop table if exists test_003;
drop table if exists test_users;
drop function if exists users_login(varchar(16),varchar(64));
/****************************************************************************************
	创建测试表
****************************************************************************************/
create table test_001(
	f1 boolean,
	f2 smallint,
	f3 integer,
	f4 bigint,
	f5 money,
	f6 float4,
	f7 float8,
	f8 numeric,
	f9 decimal,
	f10 char,
	f11 char(32),
	f12 varchar(32),
	f13 text,
	f14 bytea,
	f15 uuid,
	f16 timestamp,
	f17 timestamptz,
	f18 date,
	f19 time,
	f20 timetz,
	f21 interval
);

create table test_002(
	f1 smallint[],
	f2 integer[],
	f3 bigint[],
	f4 float4[],
	f5 float8[],
	f6 char[],
	f7 char(16)[],
	f8 varchar(32)[],
	f9 text[],
	f10 uuid[]
);

create table test_003(
	f1 json,
	f2 jsonb
)

--用户表
create table test_users(
	objectid bigserial not null,									--唯一编号
	uname varchar(16) not null,								--用户名,唯一,不能修改
	pwd varchar(64) not null,									--密码
	name varchar(16) not null,								--昵称或姓名
	generate timestamptz default now() not null,	--创建日期
	--用户名只能是字母和数字组合
	--constraint ck_test_users_uname check(uname~'^[0-9a-zA-Z_]{4,16}$' and uname !~ '^[0-9]+$' ),
	constraint pk_test_users_objectid primary key(objectid) with (fillfactor=80)
)with (fillfactor=80);


/****************************************************************************************
****************************************************************************************/
create or replace function test_users_login(
	iidentity varchar(16),ipwd varchar(64)
) returns bigint
as $$
	declare
		v_hisid bigint;
		v_userid bigint;
		v_pwd text;
	begin	
		select objectid,pwd into v_userid,v_pwd from test_users where uname=iidentity;
		if( v_userid is null or v_pwd is null ) then
			raise exception '用户名不存在!';
		end if;
		if( v_pwd != ipwd ) then
			raise exception '登录密码错误!';
		end if;		
		return v_userid;
	end;
$$ language plpgsql strict;

/****************************************************************************************
drop function if exists test_sqltype();
select * from test_sqltype();
****************************************************************************************/
create or replace function test_sqltype()
    returns table(f1 smallint,f2 int,f3 bigint,f4 decimal,f5 numeric,
		f6 float4,f7 float8,f8 money,
		f9 char(3),f10 text,f11 varchar,f12 char,
		f13 integer,f14 bytea,f15 boolean,
		f16 timestamp,f17 timestamptz,f18 timestamptz,
		f19 date,f20 time,f21 timetz,f22 interval,f23 interval)
as $$
	select cast(1 as smallint),cast(2 as int),cast(3 as bigint),cast(4 as decimal),cast(5 as numeric),
		cast(6.1 as float4), cast(7.1 as float8), cast(8.20 as money),
		cast('abc' as char(3)),'abc',cast('abc' as varchar),'x',
		null::integer,'abc'::bytea,true,
		'2018-11-16 15:39:58'::timestamp,'2018-11-16 15:39:58'::timestamptz, make_timestamptz(2018,11,16,15,39,58,'Asia/Tokyo'),
		'2018-11-16'::date,'23:18:18'::time,'23:18:18'::timetz, '1 months 1 day 2 hours'::interval,make_interval(hours =>2)
	union all
	select cast(-1 as smallint),cast(-2 as int),cast(-3 as bigint),cast(-4 as decimal),cast(-5 as numeric),
		cast(-6.1 as float4), cast(-7.1 as float8), cast(-8.20 as money),
		cast('abc' as char(3)),'中国',cast('中国' as varchar),'f',
		null::integer,'中国'::bytea,false,
		'2018-11-16 15:39:58'::timestamp,'2018-11-16 15:39:58'::timestamptz, make_timestamptz(2018,11,16,15,39,58,'Asia/Tokyo'),
		'2018-11-16'::date,'23:18:18'::time,'23:18:18'::timetz, '1 months 1 day 2 hours'::interval,make_interval(hours =>2);
$$ language sql;

/****************************************************************************************
drop function if exists test_sqlarray();
select * from test_sqlarray();
****************************************************************************************/
create or replace function test_sqlarray()
    returns table(f0 text[],f1 smallint[],f2 int[],f3 bigint[],f4 float4[],f5 float8[],
	f6 char(3)[],f7 varchar[],f8 text[],f9 char[],f10 uuid,f11 uuid[])
as $$
	select array[null::text,'2'::text]
		,array[1::smallint,2::smallint,3::smallint],
		array[4,5,6],
		array[7::bigint,8::bigint,9::bigint],
		array[10.1::float4,11.2::float4],
		array[12.3::float8,13.4::float8],
		array['1ab'::char(3),'2ab'::char(3)],
		array['3ab'::varchar,'4ab'::varchar],
		array['5ab'::text,'6ab'::text],
		array['c'::char,'d'::char],
		'2CEF3AFE-D0C6-4A03-A972-4D6BD7A9F9C1'::uuid,
		array['06125647-45BD-45D9-A61E-547AABF572F5'::uuid]
$$ language sql;


create or replace function test_json()
	returns table(f0 json,f1 jsonb)
as $$
	select '{"国家":"美帝"}'::json,'{"国家":"中国"}'::jsonb
$$ language sql;
