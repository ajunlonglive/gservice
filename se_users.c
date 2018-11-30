#include "github/jwt.h"
#include "./core/se_utils.h"
#include "se_users.h"

void user_login(struct SE_WORKINFO *arg) {
	struct SEPQ_EXECPARAMS *params = NULL;
	struct SE_PGITEM *pg;
	PGresult *result = NULL;

	char *bin_result;
	int64_t userid;
	time_t current_tm;

	//获取请求的参数
	const char *username = (const char *)get_req_param_value(arg, "username", SOAP_TYPE__string);
	const  char *password = (const char *)get_req_param_value(arg, "password", SOAP_TYPE__string);

	//检查必填参数
	SE_request_arg_isnotset(username, arg, "username");
	SE_request_arg_isnotset(password, arg, "password");
	//获取一个只读的数据库连接,如果没配置只读数据库则使用读写数据库
	SE_send_conn_null((pg = get_dbserver(arg, false)), arg);
	//开始一个只读事务
	SE_throw(SEPQ_begin(arg, READ_COMMITTED_READ_ONLY));

	resetStringBuilder(arg->sql);
	appendStringBuilder(arg->sql, "select test_users_login($1,$2)");

	SE_throw(SEPQ_params_create(arg, 2, &params));
	SE_throw(SEPQ_params_add_varchar(arg, params, 0, username));
	SE_throw(SEPQ_params_add_varchar(arg, params, 1, password));

	result = PQexecParams(pg->conn, arg->sql->data, params->count, params->types, (const char *const *)params->values, params->lengths, params->formats, 1);
	//输出记录集
	SE_throw(SE_soap_generate_recordset(arg, result));
	//获取用户编号
	bin_result = PQgetvalue(result, 0, 0);
	userid = (int64_t)pg_hton64(*((const uint64_t *)bin_result));
	SE_PQclear(result);
	current_tm = time(NULL);
	SE_throw(SE_make_token(arg, userid, current_tm, NULL));
	//提交事务
	SE_throw(SEPQ_commit(arg));
	return;
SE_ERROR_CLEAR:
	SE_PQclear(result);
	//回滚事务
	SEPQ_rollback(arg);
	return;
}

//抢红包
extern void grab_prizes(struct SE_WORKINFO *arg) {
	int64_t userid, expired, last;
	struct SEPQ_EXECPARAMS *params = NULL;
	struct SE_PGITEM *pg;
	PGresult *result = NULL;
	//获取请求的参数
	const int64_t *prizes = (const int64_t *)get_req_param_value(arg, "prizes", SOAP_TYPE__int);
	const char *token = (const char *)get_req_param_value(arg, "token", SOAP_TYPE__string);

	//检查必填参数
	SE_request_arg_isnotset(prizes, arg, "prizes");
	SE_request_arg_isnotset(token, arg, "token");
	SE_throw(SE_parse_token(arg, token, &userid, &expired, &last));
	//获取一个只读的数据库连接,如果没配置只读数据库则使用读写数据库
	SE_send_conn_null((pg = get_dbserver(arg, false)), arg);
	//开始一个读写事务,已经在函数grab_prizes_lock中实现锁了,因此用READ_COMMITTED即可
	SE_throw(SEPQ_begin(arg, READ_COMMITTED_READ_WRITE));

	resetStringBuilder(arg->sql);
	appendStringBuilder(arg->sql, "select grab_prizes_lock($1,$2)");

	SE_throw(SEPQ_params_create(arg, 2, &params));
	SE_throw(SEPQ_params_add_int32(arg, params, 0, (int32_t)(*prizes)));
	SE_throw(SEPQ_params_add_int64(arg, params, 1, userid));

	result = PQexecParams(pg->conn, arg->sql->data, params->count, params->types, (const char *const *)params->values, params->lengths, params->formats, 1);
	SE_throw(SE_soap_generate_recordset(arg, result));
	SE_PQclear(result);

	//更新token并发送到客户端
	SE_throw(SE_make_token(arg, userid, expired, &last));

	//提交事务
	SE_throw(SEPQ_commit(arg));
	return;
SE_ERROR_CLEAR:
	SE_PQclear(result);
	//回滚事务
	SEPQ_rollback(arg);
	return;
}