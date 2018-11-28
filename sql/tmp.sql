select to_timestamp(1542353998),to_timestamp(1542353998),to_timestamp(1542326400)
1542353998
select * from pg_timezone_names;

select make_timestamptz(1970,1,1,0,0,0) - make_timestamp(1970,1,1,0,0,0);

show timezone;

select * from pg_timezone_names where abbrev = current_setting('timezone');

select extract(timezone from make_timestamptz(1970,1,1,0,0,0));

test_sqlarray

select * from pg_type where oid=any(array[700,701]);

select * from pg_type where oid=any(array[18,25,1042,1043,2950]);
test_sqltype

61A7FDAB-03C9-4E3B-9860-5F243FA26727