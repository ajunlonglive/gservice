#include "../github/jwt.h"
#include "se_utils.h"



static const char* get_isolation_level(SEPQ_ISOLATION_LEVEL level) {
	switch (level) {
	case READ_UNCOMMITTED_READ_WRITE:
	case READ_COMMITTED_READ_WRITE: return "BEGIN TRANSACTION ISOLATION LEVEL READ COMMITTED READ WRITE";
	case READ_UNCOMMITTED_READ_ONLY:
	case READ_COMMITTED_READ_ONLY: return "BEGIN TRANSACTION ISOLATION LEVEL READ COMMITTED READ ONLY";
	case REPEATABLE_READ_READ_WRITE: return "BEGIN TRANSACTION ISOLATION LEVEL REPEATABLE READ READ WRITE";
	case REPEATABLE_READ_READ_ONLY: return "BEGIN TRANSACTION ISOLATION LEVEL REPEATABLE READ READ ONLY";
	case SERIALIZABLE_READ_WRITE: return "BEGIN TRANSACTION ISOLATION LEVEL SERIALIZABLE READ WRITE";
	case SERIALIZABLE_READ_ONLY: return "BEGIN TRANSACTION ISOLATION LEVEL SERIALIZABLE READ ONLY";
	case SERIALIZABLE_READ_ONLY_DEFERRABLE: return "BEGIN TRANSACTION ISOLATION LEVEL SERIALIZABLE READ ONLY DEFERRABLE";
	default:return "BEGIN TRANSACTION ISOLATION LEVEL READ COMMITTED READ ONLY";
	}
}


bool SEAPI SE_b64soap_encrypt(struct SE_WORKINFO *arg, const char *bin_data, int32_t bin_memlen, char **ptr, int32_t *base64_len) {
	char *b64_data = NULL;
	int32_t rc = 0, b64_len = 0;

	SE_function_arg_invalide(arg);
	SE_send_arg_validate(bin_data, arg, SE_2STR(SE_base64_encrypt));
	SE_send_arg_zero(bin_memlen, arg, SE_2STR(SE_base64_encrypt));

	b64_len = pg_b64_enc_len(bin_memlen);
	SE_send_malloc_fail((b64_data = (char *)soap_malloc(arg->tsoap, b64_len + 1)), arg, SE_2STR(SE_base64_encrypt));
	rc = pg_b64_encode(bin_data, bin_memlen, b64_data);
	if (-1 == rc)
		goto SE_ERROR_CLEAR;
	b64_data[rc] = 0;
	*base64_len = rc;
	*ptr = b64_data;
	return true;
SE_ERROR_CLEAR:
	if (!SE_PTR_ISNULL(b64_data))
		soap_dealloc(arg->tsoap, b64_data);
	(*ptr) = NULL;
	return false;
}

bool SEAPI SE_b64soap_decrypt(struct SE_WORKINFO *arg, const char *base64, int32_t b64_len, char **ptr, int32_t *bin_memlen) {
	char *bin_data = NULL;
	int32_t rc = 0, src_len = 0;

	SE_function_arg_invalide(arg);
	SE_send_arg_validate(base64, arg, SE_2STR(SE_base64_decrypt));
	SE_send_arg_zero(b64_len, arg, SE_2STR(SE_base64_decrypt));

	src_len = pg_b64_dec_len(b64_len);
	SE_send_malloc_fail((bin_data = (char *)soap_malloc(arg->tsoap, src_len + 1)), arg, SE_2STR(SE_b64soap_decrypt));
	rc = pg_b64_decode(base64, b64_len, bin_data);
	if (-1 == rc)
		goto SE_ERROR_CLEAR;
	bin_data[rc] = 0;
	*bin_memlen = rc;
	(*ptr) = bin_data;
	return true;
SE_ERROR_CLEAR:
	if (!SE_PTR_ISNULL(bin_data))
		soap_dealloc(arg->tsoap, bin_data);
	(*ptr) = NULL;
	return false;
}

/*生成发送给客户端错误结构体*/
void  SEAPI SE_soap_generate_error(struct SE_WORKINFO *arg, const char *err) {
	const char *txt = "{\"status\":500,\"error\":\"";
	if (!SE_PTR_ISNULL(arg) && !SE_PTR_ISNULL(arg->tsoap) && !SE_PTR_ISNULL(err)) {
		resetStringBuilder(arg->sys);
		appendBinaryStringBuilderNT(arg->sys, txt, strlen(txt));
		appendBinaryStringBuilderJsonStr(arg->sys, err,strlen(err));
		appendStringBuilderChar(arg->sys, '\"');
	}
}
/*生成发送给客户端消息结构体*/
void  SEAPI SE_soap_generate_message(struct SE_WORKINFO *arg, const char *msg) {
	const char *txt = "{\"status\":200,\"msg\":\"";
	if (!SE_PTR_ISNULL(arg) && !SE_PTR_ISNULL(arg->tsoap) && !SE_PTR_ISNULL(msg)) {
		resetStringBuilder(arg->sys);
		appendBinaryStringBuilderNT(arg->sys, txt, strlen(txt));
		appendBinaryStringBuilderJsonStr(arg->sys, msg, strlen(msg));
		appendStringBuilderChar(arg->sys, '\"');
	}
}

void  SEAPI SE_soap_generate_redirect(struct SE_WORKINFO *arg, const char *msg, const char *url) {
	const char *txt = "{\"status\":302,\"msg\":\"";
	if (!SE_PTR_ISNULL(arg) && !SE_PTR_ISNULL(arg->tsoap) && !SE_PTR_ISNULL(msg)
		&& !SE_PTR_ISNULL(url)) {
		resetStringBuilder(arg->sys);
		appendBinaryStringBuilderNT(arg->sys, txt, strlen(txt));
		appendBinaryStringBuilderJsonStr(arg->sys, msg, strlen(msg));
		appendBinaryStringBuilderNT(arg->sys, "\",\"url\":\"", 9);
		appendBinaryStringBuilderJsonStr(arg->sys, url, strlen(url));
		appendStringBuilderChar(arg->sys, '\"');
	}
}

const void *SEAPI get_req_param_value(struct SE_WORKINFO *arg, const char * const name, uint32_t desired_val_type) {
	int32_t index = 0;
	struct value *param_val;
	const int64_t *int64_val; const double *double_val; const bool *bool_val;
	const char *string_val; const struct value *array_val; const struct value *struct_val;
	const char *datetime_val; const struct _base64 *base64_val;

	//检查输入参数有效性
	SE_function_arg_invalide(arg);
	SE_send_arg_validate(name, arg, SE_2STR(get_req_param_value));

	//检查指定的参数是否存在
	index = nth_at(arg->request, name);
	if (-1 == index) {//指定的参数不存在
		return NULL;
	} else {
		param_val = nth_value(arg->request, index);
		//参数存在但是值为NULL
		if (is_null(param_val))
			return NULL;
		//开始检查类型
		if (is_number(param_val)) {
			int64_val = int_of(param_val);
			return  (desired_val_type == SOAP_TYPE__i4 || desired_val_type == SOAP_TYPE__int) ? (const void *)int64_val : NULL;
		} else if (is_double(param_val)) {
			double_val = double_of(param_val);
			return (desired_val_type == SOAP_TYPE__double) ? (const void *)double_val : NULL;
		} else if (is_string(param_val)) {
			string_val = *string_of(param_val);
			if ('\0' == string_val[0]) //字符串还需要检查是否为空值	
				return NULL;
			return (desired_val_type == SOAP_TYPE__string) ? (const void *)string_val : NULL;
		} else if (is_bool(param_val)) {
			bool_val = bool_of(param_val);
			return (desired_val_type == SOAP_TYPE__boolean) ? (const void *)bool_val : NULL;
		} else if (is_array(param_val)) {
			array_val = param_val;
			return (desired_val_type == SOAP_TYPE__array) ? (const void *)array_val : NULL;
		} else if (is_struct(param_val)) {
			struct_val = param_val;
			return (desired_val_type == SOAP_TYPE__struct) ? (const void *)struct_val : NULL;
		} else if (is_dateTime(param_val)) {
			datetime_val = *dateTime_of(param_val);
			if ('\0' == datetime_val[0]) //字符串还需要检查是否为空值			
				return NULL;
			return (desired_val_type == SOAP_TYPE__dateTime_DOTiso8601) ? (const void *)datetime_val : NULL;
		} else if (is_base64(param_val)) {
			base64_val = base64_of(param_val);
			return (desired_val_type == SOAP_TYPE__base64) ? (const void *)base64_val : NULL;
		} else {
			return NULL;
		}
	}
	return NULL;
SE_ERROR_CLEAR:
	return NULL;
}

bool SEAPI SEPQ_begin(struct SE_WORKINFO *arg, SEPQ_ISOLATION_LEVEL level) {
	PGresult *result = NULL;

	SE_function_arg_invalide(arg);
	SE_function_arg_invalide(arg->pg->conn);

	result = PQexec(arg->pg->conn, get_isolation_level(level));
	SE_check_pqexec_fail_send(PGRES_COMMAND_OK, result, arg);
	SE_PQclear(result);
	return true;
SE_ERROR_CLEAR:
	SE_PQclear(result);
	return false;
}

bool SEAPI SEPQ_commit(struct SE_WORKINFO *arg) {
	PGresult *result = NULL;

	SE_function_arg_invalide(arg);
	SE_function_arg_invalide(arg->pg->conn);

	result = PQexec(arg->pg->conn, "COMMIT");
	SE_check_pqexec_fail_send(PGRES_COMMAND_OK, result, arg);
	SE_PQclear(result);
	return true;
SE_ERROR_CLEAR:
	SE_PQclear(result);
	return false;
}

void SEAPI SEPQ_rollback(struct SE_WORKINFO *arg) {
	PGresult *result = NULL;

	SE_function_arg_invalide(arg);
	SE_function_arg_invalide(arg->pg);
	SE_function_arg_invalide(arg->pg->conn);

	result = PQexec(arg->pg->conn, "ROLLBACK");
	SE_check_pqexec_fail_send(PGRES_COMMAND_OK, result, arg);
	SE_PQclear(result);
	return;
SE_ERROR_CLEAR:
	SE_PQclear(result);
	return;
}

#define SE_jwt_free(jwt_) do{\
	if(!SE_PTR_ISNULL(jwt_) ){\
		jwt_free(jwt_);jwt_=NULL;\
	}\
}while (0)

bool SEAPI SE_make_token(struct SE_WORKINFO *arg, int64_t userid, time_t current_tm, const int64_t *last) {
	/*
	*	{"userid":1,"expired":1543373542,"last":1543402360}
	*	userid:用户唯一编号
	*	expired:Token过期时间,UNIX时间,使用时会不断更新这个时间
	*	last:一个token的整个生命周期为8小时,使用超过8小时后必须重新登录,UNIX时间
	*/
	jwt_t *jwt = NULL;
	char *txt = NULL;
	const struct  SE_CONF *conf;

	SE_function_arg_invalide(arg);

	conf = get_conf();
	if (SE_PTR_ISNULL(conf))
		SE_send_unknown_err(arg);

	//设置用户编号
	resetStringBuilder(arg->buf);
	appendStringBuilderString(arg->buf, "{\"userid\":");
#if defined(_MSC_VER)		
	appendStringBuilder(arg->buf, "%I64d", userid);
#else
#	ifdef __x86_64__
	appendStringBuilder(arg->buf, "%ldd", userid);
#	elif __i386__
	appendStringBuilder(arg->buf, "%lld", userid);
#	endif
#endif


	//Token保持时间,验证token时修改为最新的时间
	appendStringBuilderString(arg->buf, ",\"expired\":");
#if defined(_MSC_VER)		
	appendStringBuilder(arg->buf, "%I64d", current_tm + conf->token_keep * 60);
#else
#	ifdef __x86_64__
	appendStringBuilder(arg->buf, "%ldd", current_tm + conf->token_keep * 60);
#	elif __i386__
	appendStringBuilder(arg->buf, "%lld", current_tm + conf->token_keep * 60);
#	endif
#endif
	if (SE_PTR_ISNULL(last)) {
		//token生命周期(一个token的整个生命周期为8小时,使用超过8小时后必须重新登录)
		appendStringBuilderString(arg->buf, ",\"last\":");
#if defined(_MSC_VER)		
		appendStringBuilder(arg->buf, "%I64d", current_tm + 3600 * 8);
#else
#	ifdef __x86_64__
		appendStringBuilder(arg->buf, "%ldd", current_tm + 3600 * 8);
#	elif __i386__
		appendStringBuilder(arg->buf, "%lld", current_tm + 3600 * 8);
#	endif
#endif
	}
	appendStringBuilderChar(arg->buf, '}');

	SE_send_check_rc(jwt_new(&jwt), arg, SE_2STR(jwt_new));
	SE_send_check_rc(jwt_set_alg(jwt, JWT_ALG_HS256, (const uint8_t *)get_jwt_key(), 16), arg, SE_2STR(jwt_set_alg));
	SE_send_check_rc(jwt_add_grants_json(jwt, arg->buf->data), arg, SE_2STR(jwt_add_grants_json));
	SE_send_malloc_fail((txt = jwt_encode_str(jwt)), arg, SE_2STR(jwt_encode_str));

	appendStringBuilderString(arg->sys, ",\"token\":\"");
	appendStringBuilderString(arg->sys, txt);
	appendStringBuilderCharNT(arg->sys, '\"');
	SE_free(&txt);
	SE_jwt_free(jwt);
	return true;
SE_ERROR_CLEAR:
	SE_free(&txt);
	SE_jwt_free(jwt);
	return false;
}

bool SEAPI SE_parse_token(struct SE_WORKINFO *arg, const char *token_base64, int64_t *userid, int64_t *expired, int64_t *last) {
	jwt_t *jwt = NULL;
	int64_t iuserid = 0, iexpired = 0, ilast = 0;
	time_t current_tm;
	const struct  SE_CONF *conf;

	SE_function_arg_invalide(arg);
	SE_send_arg_validate(token_base64, arg, SE_2STR(SE_parse_token));

	conf = get_conf();
	if (SE_PTR_ISNULL(conf))
		SE_send_unknown_err(arg);

	SE_send_check_rc(jwt_decode(&jwt, token_base64, (const uint8_t *)get_jwt_key(), 16), arg, SE_2STR(jwt_decode));
	SE_get_jwtval_check((iuserid = jwt_get_grant_int(jwt, "userid")), arg);
	SE_get_jwtval_check((iexpired = jwt_get_grant_int(jwt, "expired")), arg);
	SE_get_jwtval_check((ilast = jwt_get_grant_int(jwt, "last")), arg);

	current_tm = time(NULL);
	if (current_tm <= iexpired && current_tm <= ilast) {
		//更新过期时间
		*userid = iuserid;
		*expired = current_tm + conf->token_keep * 60;
		*last = ilast;
	} else {
		//登录超时,要求客户端重定向
		*userid = 0;
		*expired = 0;
		*last = 0;
		SE_soap_generate_redirect(arg, "login timeout.", conf->redirect_url);
		goto SE_ERROR_CLEAR;
	}
	SE_jwt_free(jwt);
	return true;
SE_ERROR_CLEAR:
	SE_jwt_free(jwt);
	return false;
}