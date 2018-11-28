
#include "se_method.h"
#include "se_handler.h"
#include "se_users.h"

static void test_msg(struct SE_WORKINFO *arg);
static void test_error(struct SE_WORKINFO *arg);
static void test_redirect(struct SE_WORKINFO *arg);

static void hello_world(struct SE_WORKINFO *arg);
static void test_sqltype(struct SE_WORKINFO *arg);
static void test_sqlarray(struct SE_WORKINFO *arg);
static void test_insert(struct SE_WORKINFO *arg);
static void test_array_insert(struct SE_WORKINFO *arg);
static void test_json(struct SE_WORKINFO *arg);
static void test_json_insert(struct SE_WORKINFO *arg);

void user_process(struct SE_WORKINFO *arg) {
	uint32_t result = 0;
	int32_t method_len;

	const char *method = (const char *)get_req_param_value(arg, "method", SOAP_TYPE__string);
	SE_request_arg_isnotset(method, arg, "method");

	method_len = strlen(method);
	result = crc32(method, method_len);
	switch (result) {
	case SEM_TEST_MSG:
		test_msg(arg);
		break;
	case SEM_TEST_ERROR:
		test_error(arg);
		break;
	case SEM_TEST_REDIRECT:
		test_redirect(arg);
		break;
	case SEM_HELLO_WORLD:
		hello_world(arg);
		break;
	case SEM_TEST_SQLTYPE:
		test_sqltype(arg);
		break;
	case SEM_TEST_SQLARRAY:
		test_sqlarray(arg);
		break;
	case SEM_TEST_INSERT:
		test_insert(arg);
		break;
	case SEM_TEST_ARRAY_INSERT:
		test_array_insert(arg);
		break;
	case SEM_TEST_JSON:
		test_json(arg);
		break;
	case SEM_TEST_JSON_INSERT:
		test_json_insert(arg);
		break;
	case SEM_USER_LOGIN:
		user_login(arg);
		break;
	case SEM_GRAB_PRIZES:
		grab_prizes(arg);
		break;
	default:
		resetStringBuilder(arg->buf);
		appendStringBuilder(arg->buf, "The method you requested does not exist.");
		SE_soap_generate_error(arg, arg->buf->data);
		break;
	}
SE_ERROR_CLEAR:
	return;
}

static void test_msg(struct SE_WORKINFO *arg) {
	SE_soap_generate_message(arg, "ok");
}
static void test_error(struct SE_WORKINFO *arg) {
	SE_soap_generate_error(arg, "error");
}
static void test_redirect(struct SE_WORKINFO *arg) {
	SE_soap_generate_redirect(arg, "redirect", "https://localhost:8080");
}

static void hello_world(struct SE_WORKINFO *arg) {
	struct SE_PGITEM *pg;
	//获取请求的参数
	const char *name = (const char *)get_req_param_value(arg, "name", SOAP_TYPE__string);
	const int64_t *age = (const int64_t *)get_req_param_value(arg, "age", SOAP_TYPE__int);
	//检查必填参数
	SE_request_arg_isnotset(name, arg, "name");
	SE_request_arg_isnotset(age, arg, "age");

	//获取可用的数据库连接

	SE_send_conn_null((pg = get_dbserver(arg, false)), arg);
	appendStringBuilder(arg->sys, "\"%s\": %d,", "pid", arg->id);
	appendStringBuilder(arg->sys, "\"%s\": %d,", "serdbid", pg->serverid);
	appendStringBuilder(arg->sys, "\"%s\": %d,", "connid", pg->id);

	//*int_of(value_at(arg->response, "pid")) = arg->id;						//处理进程编号
	//*int_of(value_at(arg->response, "serdbid")) = pg->serverid;		//database server id
	//*int_of(value_at(arg->response, "connid")) = pg->id;				//database connect id
	//resetStringBuilder(arg->buf);
#if defined(_MSC_VER)		
	appendStringBuilder(arg->sys, "\"msg\":\"%s hello world age(%I64d)!\"", name, *age);
#else
#	ifdef __x86_64__
	appendStringBuilder(arg->sys, "\"msg\":\"%s hello world age(%ldd)!\",", name, *age);
#	elif __i386__
	appendStringBuilder(arg->sys, "\"msg\":\"%s hello world age(%lld)!\",", name, *age);
#	endif
#endif
	return;
SE_ERROR_CLEAR:
	return;
}


static void test_sqltype(struct SE_WORKINFO *arg) {
	struct SE_PGITEM *pg;
	PGresult *result = NULL;
	SE_send_conn_null((pg = get_dbserver(arg, false)), arg);

	//开始一个事务
	SE_throw(SEPQ_begin(arg, READ_COMMITTED_READ_ONLY));

	result = PQexecParams(pg->conn, "select * from test_sqltype()", 0, NULL, NULL, NULL, NULL, 1);
	SE_throw(SE_soap_generate_recordset(arg, result));
	SE_PQclear(result);

	result = PQexecParams(pg->conn, "select * from test_sqltype()", 0, NULL, NULL, NULL, NULL, 1);
	SE_throw(SE_soap_generate_recordset(arg, result));
	SE_PQclear(result);

	//提交事务
	SE_throw(SEPQ_commit(arg));
	return;
SE_ERROR_CLEAR:
	SE_PQclear(result);
	//回滚事务
	SEPQ_rollback(arg);
	return;
}

static void test_sqlarray(struct SE_WORKINFO *arg) {
	struct SE_PGITEM *pg;
	PGresult *result = NULL;
	SE_send_conn_null((pg = get_dbserver(arg, false)), arg);

	//开始一个事务
	SE_throw(SEPQ_begin(arg, READ_COMMITTED_READ_ONLY));

	result = PQexecParams(pg->conn, "select * from test_sqlarray()", 0, NULL, NULL, NULL, NULL, 1);
	SE_throw(SE_soap_generate_recordset(arg, result));
	SE_PQclear(result);

	result = PQexecParams(pg->conn, "select * from test_sqlarray()", 0, NULL, NULL, NULL, NULL, 1);
	SE_throw(SE_soap_generate_recordset(arg, result));
	SE_PQclear(result);

	//提交事务
	SE_throw(SEPQ_commit(arg));
	return;
SE_ERROR_CLEAR:
	SE_PQclear(result);
	//回滚事务
	SEPQ_rollback(arg);
	return;
}

static bool test_insertex(struct SE_WORKINFO *arg, PGconn *conn) {
	struct SEPQ_EXECPARAMS *params = NULL;
	PGresult *result = NULL;
	//汉字"中".注意这样的声明方法必须包含字符串结束标记0
	const char char1_val[4] = { 0xE4,0xB8,0xAD, 0x00 };
	//编写SQL	
	const char *sql = "insert into test_001(f1,f2,f3,f4,f5,f6,f7,f8,f9,f10,f11,f12,f13,f14,f15,f16,f17,f18,f19,f20,f21)"\
		" values($1,$2,$3,$4,$5,$6,$7,$8,$9,$10,$11,$12,$13,$14,$15,$16,$17,$18,$19,$20,$21);";
	//设置SQL参数
	SE_throw(SEPQ_params_create(arg, 21, &params));
	SE_throw(SEPQ_params_add_bool(arg, params, 0, true));
	SE_throw(SEPQ_params_add_int16(arg, params, 1, (int16_t)1));
	SE_throw(SEPQ_params_add_int32(arg, params, 2, (int32_t)2));
	SE_throw(SEPQ_params_add_int64(arg, params, 3, (int64_t)3));
	SE_throw(SEPQ_params_add_money(arg, params, 4, (int64_t)414)); //money精确至分
	SE_throw(SEPQ_params_add_float4(arg, params, 5, (float)5.1415926));
	SE_throw(SEPQ_params_add_float8(arg, params, 6, 6.1415926));
	SE_throw(SEPQ_params_add_numeric(arg, params, 7, "7.1415926"));
	SE_throw(SEPQ_params_add_decimal(arg, params, 8, "8.1415926"));
	SE_throw(SEPQ_params_add_char(arg, params, 9, char1_val));
	SE_throw(SEPQ_params_add_char(arg, params, 10, "a123"));
	SE_throw(SEPQ_params_add_varchar(arg, params, 11, "b456"));
	SE_throw(SEPQ_params_add_text(arg, params, 12, "c789"));
	SE_throw(SEPQ_params_add_bytea(arg, params, 13, "R0lGODlhAQABAIAAAAAAAP///ywAAAAAAQABAAACAUwAOw=="));
	SE_throw(SEPQ_params_add_uuid(arg, params, 14, "{6CEDF74F-8125-49B9-B24C-852A6655C183}"));
	SE_throw(SEPQ_params_add_timestamp(arg, params, 15, "2018-01-01T01:01:01Z"));
	SE_throw(SEPQ_params_add_timestamptz(arg, params, 16, "2018-01-01T01:01:01+08"));
	SE_throw(SEPQ_params_add_date(arg, params, 17, "2018-01-01"));
	SE_throw(SEPQ_params_add_time(arg, params, 18, "22:59:18Z"));
	SE_throw(SEPQ_params_add_timetz(arg, params, 19, "22:59:18+08:00"));
	SE_throw(SEPQ_params_add_interval(arg, params, 20, "P0M0DT2H0M0S"));
	//执行sql
	result = PQexecParams(conn, sql, params->count, params->types, (const char *const *)params->values, params->lengths, params->formats, 1);
	//释放参数,不释放也没关系,本次请求完成后自动释放
	SEPQ_params_free(arg, &params);
	//检查命令是否成功
	SE_check_pqexec_fail_send(PGRES_COMMAND_OK, result, arg);
	//清理libpq的资源
	SE_PQclear(result);
	return true;
SE_ERROR_CLEAR:
	SEPQ_params_free(arg, &params);
	SE_PQclear(result);
	return false;
}

static void test_insert(struct SE_WORKINFO *arg) {
	struct SE_PGITEM *pg;
	//获取可用的连接
	SE_send_conn_null((pg = get_dbserver(arg, true)), arg);
	//开始一个事务
	SE_throw(SEPQ_begin(arg, READ_COMMITTED_READ_WRITE));
	//执行插入sql
	SE_throw(test_insertex(arg, pg->conn));
	//提交事务
	SE_throw(SEPQ_commit(arg));
	return;
SE_ERROR_CLEAR:
	//回滚事务
	SEPQ_rollback(arg);
	return;
}


static bool test_array_insertex(struct SE_WORKINFO *arg, PGconn *conn) {
	struct SEPQ_EXECPARAMS *params = NULL;
	PGresult *result = NULL;
	int16_t int16_vals[2] = { 1,2 };
	int32_t int32_vals[3] = { 3,4,5 };
	int64_t int64_vals[4] = { 6,7,8,9 };
	float float_vals[2] = { 3.14f,4.14f };
	double double_vals[3] = { 5.14,6.14,7.14 };
	char char1_vals[3] = { 'a','b','c' };
	char *char2_vals[3] = { "a123","b123","c123" };
	char *varchar_vals[3] = { "d123","e123","f123" };
	char *text_vals[3] = { "k123","j123","m123" };
	char *uuid_vals[3] = { "{0DEA742A-8022-4769-A0D6-3076F4385A76}","{D6FC351E-0467-4DD7-8308-FE0AC8BD563F}","{F7C15BF9-74B3-4EBA-9A8E-28A536234973}" };

	//编写SQL	
	const char *sql = "insert into test_002(f1,f2,f3,f4,f5,f6,f7,f8,f9,f10) values($1,$2,$3,$4,$5,$6,$7,$8,$9,$10) returning *";
	//设置SQL参数
	SE_throw(SEPQ_params_create(arg, 10, &params));
	SE_throw(SEPQ_params_add_int16_array(arg, params, 0, int16_vals, 2));
	SE_throw(SEPQ_params_add_int32_array(arg, params, 1, int32_vals, 3));
	SE_throw(SEPQ_params_add_int64_array(arg, params, 2, int64_vals, 4));
	SE_throw(SEPQ_params_add_float4_array(arg, params, 3, float_vals, 2));
	SE_throw(SEPQ_params_add_float8_array(arg, params, 4, double_vals,3));
	SE_throw(SEPQ_params_add_char_one_array(arg, params, 5, char1_vals, 3));
	SE_throw(SEPQ_params_add_char_more_array(arg, params, 6, (const char * const *)char2_vals, 3));
	SE_throw(SEPQ_params_add_varchar_array(arg, params, 7, (const char * const *)varchar_vals, 3));
	SE_throw(SEPQ_params_add_text_array(arg, params, 8, (const char * const *)text_vals, 3));
	SE_throw(SEPQ_params_add_uuid_array(arg, params, 9, (const char * const *)uuid_vals, 3));
	
	//执行sql
	result = PQexecParams(conn, sql, params->count, params->types, (const char *const *)params->values, params->lengths, params->formats, 1);
	//输出插入的信息
	SE_throw(SE_soap_generate_recordset(arg, result));
	//清理libpq的资源
	SE_PQclear(result);
	return true;
SE_ERROR_CLEAR:
	SE_PQclear(result);
	return false;
}

static void test_array_insert(struct SE_WORKINFO *arg) {
	struct SE_PGITEM *pg;
	//获取可用的连接
	SE_send_conn_null((pg = get_dbserver(arg, true)), arg);
	//开始一个事务
	SE_throw(SEPQ_begin(arg, READ_COMMITTED_READ_WRITE));
	//执行插入sql
	SE_throw(test_array_insertex(arg, pg->conn));
	//提交事务
	SE_throw(SEPQ_commit(arg));
	return;
SE_ERROR_CLEAR:
	//回滚事务
	SEPQ_rollback(arg);
	return;
}



static void test_json(struct SE_WORKINFO *arg) {
	struct SE_PGITEM *pg;
	PGresult *result = NULL;
	SE_send_conn_null((pg = get_dbserver(arg, false)), arg);

	//开始一个事务
	SE_throw(SEPQ_begin(arg, READ_COMMITTED_READ_ONLY));

	result = PQexecParams(pg->conn, "select * from test_json()", 0, NULL, NULL, NULL, NULL, 1);
	SE_throw(SE_soap_generate_recordset(arg, result));
	SE_PQclear(result);

	//提交事务
	SE_throw(SEPQ_commit(arg));
	return;
SE_ERROR_CLEAR:
	SE_PQclear(result);
	//回滚事务
	SEPQ_rollback(arg);
	return;
}


static bool test_json_insertex(struct SE_WORKINFO *arg, PGconn *conn) {
	struct SEPQ_EXECPARAMS *params = NULL;
	PGresult *result = NULL;
	//编写SQL	
	const char *sql = "insert into test_003(f1,f2)  values($1,$2) returning *;";
	//设置SQL参数
	SE_throw(SEPQ_params_create(arg, 2, &params));
	SE_throw(SEPQ_params_add_json(arg, params, 0, "{\"a\":1}"));
	SE_throw(SEPQ_params_add_jsonb(arg, params, 1, "{\"b\":\"c\"}"));
	
	//执行sql
	result = PQexecParams(conn, sql, params->count, params->types, (const char *const *)params->values, params->lengths, params->formats, 1);
	//释放参数,不释放也没关系,本次请求完成后自动释放
	SEPQ_params_free(arg, &params);
	//输出插入的信息
	SE_throw(SE_soap_generate_recordset(arg, result));
	//清理libpq的资源
	SE_PQclear(result);
	return true;
SE_ERROR_CLEAR:
	SEPQ_params_free(arg, &params);
	SE_PQclear(result);
	return false;
}

static void test_json_insert(struct SE_WORKINFO *arg) {
	struct SE_PGITEM *pg;
	//获取可用的连接
	SE_send_conn_null((pg = get_dbserver(arg, true)), arg);
	//开始一个事务
	SE_throw(SEPQ_begin(arg, READ_COMMITTED_READ_WRITE));
	//执行插入sql
	SE_throw(test_json_insertex(arg, pg->conn));
	//提交事务
	SE_throw(SEPQ_commit(arg));
	return;
SE_ERROR_CLEAR:
	//回滚事务
	SEPQ_rollback(arg);
	return;
}