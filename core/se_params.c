#include "se_utils.h"

/* uuid size in bytes */
#define UUID_LEN 16

typedef struct pg_uuid_t {
	unsigned char data[UUID_LEN];
} pg_uuid_t;

static bool string_to_uuid(const char *source, pg_uuid_t *uuid) {
	const char *src = source;
	bool		braces = false;
	int			i;

	if (src[0] == '{') {
		src++;
		braces = true;
	}

	for (i = 0; i < UUID_LEN; i++) {
		char		str_buf[3];

		if (src[0] == '\0' || src[1] == '\0')
			goto SE_ERROR_CLEAR;
		memcpy(str_buf, src, 2);
		if (!isxdigit((unsigned char)str_buf[0]) ||
			!isxdigit((unsigned char)str_buf[1]))
			goto SE_ERROR_CLEAR;

		str_buf[2] = '\0';
		uuid->data[i] = (unsigned char)strtoul(str_buf, NULL, 16);
		src += 2;
		if (src[0] == '-' && (i % 2) == 1 && i < UUID_LEN - 1)
			src++;
	}
	if (braces) {
		if (*src != '}')
			goto SE_ERROR_CLEAR;
		src++;
	}
	if (*src != '\0')
		goto SE_ERROR_CLEAR;
	return true;
SE_ERROR_CLEAR:
	return false;
}


void SEAPI SEPQ_params_free(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS **ptr) {
	struct SEPQ_EXECPARAMS *params = *ptr;
	int32_t i = 0;

	if (!SE_PTR_ISNULL(params)) {
		if (!SE_PTR_ISNULL(params->types))
			soap_dealloc(arg->tsoap, params->types);
		if (!SE_PTR_ISNULL(params->values)) {
			for (i = 0; i < params->count; ++i) {
				if (!SE_PTR_ISNULL(params->values[i]))
					soap_dealloc(arg->tsoap, params->values[i]);
			}
			soap_dealloc(arg->tsoap, params->values);
		}
		if (!SE_PTR_ISNULL(params->lengths))
			soap_dealloc(arg->tsoap, params->lengths);
		if (!SE_PTR_ISNULL(params->formats))
			soap_dealloc(arg->tsoap, params->formats);
		soap_dealloc(arg->tsoap, params);
	}
	*ptr = NULL;
}

bool SEAPI SEPQ_params_create(struct SE_WORKINFO *arg, int32_t params_count, struct SEPQ_EXECPARAMS **ptr) {
	struct SEPQ_EXECPARAMS *params = NULL;

	SE_function_arg_invalide(arg);
	SE_send_arg_less1(params_count, arg, SE_2STR(SE_pqparam_create));

	SE_send_malloc_fail((params = (struct SEPQ_EXECPARAMS *)soap_malloc(arg->tsoap, sizeof(struct SEPQ_EXECPARAMS))), arg, SE_2STR(SE_pqparam_create));
	memset(params, 0, sizeof(struct SEPQ_EXECPARAMS));
	if (params_count > 0) {
		params->count = params_count;
		SE_send_malloc_fail((params->types = (Oid *)soap_malloc(arg->tsoap, params_count * sizeof(Oid))), arg, SE_2STR(SEPQ_params_create));
		SE_send_malloc_fail((params->values = (char **)soap_malloc(arg->tsoap, params_count * sizeof(char *))), arg, SE_2STR(SEPQ_params_create));
		SE_send_malloc_fail((params->lengths = (int32_t  *)soap_malloc(arg->tsoap, params_count * sizeof(int32_t))), arg, SE_2STR(SEPQ_params_create));
		SE_send_malloc_fail((params->formats = (int32_t  *)soap_malloc(arg->tsoap, params_count * sizeof(int32_t))), arg, SE_2STR(SEPQ_params_create));

		memset(params->types, 0, params_count * sizeof(Oid));
		memset(params->values, 0, params_count * sizeof(char *));
		memset(params->lengths, 0, params_count * sizeof(int32_t));
		memset(params->formats, 0, params_count * sizeof(int32_t));
	}
	(*ptr) = params;
	return true;
SE_ERROR_CLEAR:
	SEPQ_params_free(arg, &params);
	return false;
}

bool SEAPI SEPQ_params_add_null(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index) {
	SE_function_arg_invalide(arg);
	SE_send_arg_validate(params, arg, SE_2STR(SEPQ_params_add_null));
	SE_send_check_array_index(index, params->count, arg, SE_2STR(SEPQ_params_add_null));
	SE_send_param_has_set(params, index, arg, SE_2STR(SEPQ_params_add_null));

	params->values[index] = NULL;
	params->lengths[index] = 0;
	params->types[index] = 0;
	params->formats[index] = 1;
	return true;
SE_ERROR_CLEAR:
	return false;
}

bool SEAPI SEPQ_params_add_bool(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, bool val) {
	SE_function_arg_invalide(arg);
	SE_send_arg_validate(params, arg, SE_2STR(SEPQ_params_add_bool));
	SE_send_check_array_index(index, params->count, arg, SE_2STR(SEPQ_params_add_bool));
	SE_send_param_has_set(params, index, arg, SE_2STR(SEPQ_params_add_bool));

	SE_send_malloc_fail((params->values[index] = (char *)soap_malloc(arg->tsoap, 1)), arg, SE_2STR(SEPQ_params_add_bool));
	SE_memcpy(params->values[index], 1, val ? "\1" : "\0", 1);
	params->lengths[index] = 1;
	params->types[index] = PQ_BOOLOID;
	params->formats[index] = 1;
	return true;
SE_ERROR_CLEAR:
	if (!SE_PTR_ISNULL(params->values[index]))
		soap_dealloc(arg->tsoap, params->values[index]);
	return false;
}

bool SEAPI SEPQ_params_add_bytea(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, const char *b64_val) {
	SE_function_arg_invalide(arg);
	SE_send_arg_validate(params, arg, SE_2STR(SEPQ_params_add_bytea));
	SE_send_arg_validate(b64_val, arg, SE_2STR(SEPQ_params_add_bytea));
	SE_send_check_array_index(index, params->count, arg, SE_2STR(SEPQ_params_add_bytea));
	SE_send_param_has_set(params, index, arg, SE_2STR(SEPQ_params_add_bytea));

	SE_throw(SE_b64soap_decrypt(arg, b64_val, strlen(b64_val), &params->values[index], &params->lengths[index]));
	params->types[index] = PQ_BYTEAOID;
	params->formats[index] = 1;
	return true;
SE_ERROR_CLEAR:
	if (!SE_PTR_ISNULL(params->values[index]))
		soap_dealloc(arg->tsoap, params->values[index]);
	return false;
}

bool SEAPI SEPQ_params_add_int16(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, int16_t int16_val) {
	int16_t bin_data;
	int32_t bin_memlen = 2;

	SE_function_arg_invalide(arg);
	SE_send_arg_validate(params, arg, SE_2STR(SEPQ_params_add_int16));
	SE_send_check_array_index(index, params->count, arg, SE_2STR(SEPQ_params_add_int16));
	SE_send_param_has_set(params, index, arg, SE_2STR(SEPQ_params_add_int16));


	SE_send_malloc_fail((params->values[index] = (char *)soap_malloc(arg->tsoap, bin_memlen)), arg, SE_2STR(SEPQ_params_add_int16));
	bin_data = pg_bswap16(int16_val);
	SE_memcpy(params->values[index], bin_memlen, &bin_data, bin_memlen);
	params->lengths[index] = bin_memlen;
	params->types[index] = PQ_INT2OID;
	params->formats[index] = 1;
	return true;
SE_ERROR_CLEAR:
	if (!SE_PTR_ISNULL(params->values[index]))
		soap_dealloc(arg->tsoap, params->values[index]);
	return false;
}

bool SEAPI SEPQ_params_add_int32(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, int32_t int32_val) {
	int32_t bin_data;
	int32_t bin_memlen = 4;

	SE_function_arg_invalide(arg);
	SE_send_arg_validate(params, arg, SE_2STR(SEPQ_params_add_int32));
	SE_send_check_array_index(index, params->count, arg, SE_2STR(SEPQ_params_add_int32));
	SE_send_param_has_set(params, index, arg, SE_2STR(SEPQ_params_add_int32));


	SE_send_malloc_fail((params->values[index] = (char *)soap_malloc(arg->tsoap, bin_memlen)), arg, SE_2STR(SEPQ_params_add_int32));
	bin_data = pg_bswap32(int32_val);
	SE_memcpy(params->values[index], bin_memlen, &bin_data, bin_memlen);
	params->lengths[index] = bin_memlen;
	params->types[index] = PQ_INT4OID;
	params->formats[index] = 1;
	return true;
SE_ERROR_CLEAR:
	if (!SE_PTR_ISNULL(params->values[index]))
		soap_dealloc(arg->tsoap, params->values[index]);
	return false;
}

bool SEAPI SEPQ_params_add_int64(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, int64_t int64_val) {
	int64_t bin_data;
	int32_t bin_memlen = 8;

	SE_function_arg_invalide(arg);
	SE_send_arg_validate(params, arg, SE_2STR(SEPQ_params_add_int64));
	SE_send_check_array_index(index, params->count, arg, SE_2STR(SEPQ_params_add_int64));
	SE_send_param_has_set(params, index, arg, SE_2STR(SEPQ_params_add_int64));


	SE_send_malloc_fail((params->values[index] = (char *)soap_malloc(arg->tsoap, bin_memlen)), arg, SE_2STR(SEPQ_params_add_int64));
	bin_data = pg_bswap64(int64_val);
	SE_memcpy(params->values[index], bin_memlen, &bin_data, bin_memlen);
	params->lengths[index] = bin_memlen;
	params->types[index] = PQ_INT8OID;
	params->formats[index] = 1;
	return true;
SE_ERROR_CLEAR:
	if (!SE_PTR_ISNULL(params->values[index]))
		soap_dealloc(arg->tsoap, params->values[index]);
	return false;
}



bool SEAPI SEPQ_params_add_money(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, int64_t money_val) {
	int64_t bin_data;
	int32_t bin_memlen = 8;

	SE_function_arg_invalide(arg);
	SE_send_arg_validate(params, arg, SE_2STR(SEPQ_params_add_money));
	SE_send_check_array_index(index, params->count, arg, SE_2STR(SEPQ_params_add_money));
	SE_send_param_has_set(params, index, arg, SE_2STR(SEPQ_params_add_money));


	SE_send_malloc_fail((params->values[index] = (char *)soap_malloc(arg->tsoap, bin_memlen)), arg, SE_2STR(SEPQ_params_add_money));
	bin_data = pg_bswap64(money_val);
	SE_memcpy(params->values[index], bin_memlen, &bin_data, bin_memlen);
	params->lengths[index] = bin_memlen;
	params->types[index] = PQ_MONEYOID;
	params->formats[index] = 1;
	return true;
SE_ERROR_CLEAR:
	if (!SE_PTR_ISNULL(params->values[index]))
		soap_dealloc(arg->tsoap, params->values[index]);
	return false;
}

bool SEAPI SEPQ_params_add_float4(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, float float_val) {
	int32_t bin_data;
	int32_t bin_memlen = 4;

	SE_function_arg_invalide(arg);
	SE_send_arg_validate(params, arg, SE_2STR(SEPQ_params_add_float4));
	SE_send_check_array_index(index, params->count, arg, SE_2STR(SEPQ_params_add_float4));
	SE_send_param_has_set(params, index, arg, SE_2STR(SEPQ_params_add_float4));


	SE_send_malloc_fail((params->values[index] = (char *)soap_malloc(arg->tsoap, bin_memlen)), arg, SE_2STR(SEPQ_params_add_float4));
	bin_data = pg_bswap32((*((int32_t *)(&float_val))));
	SE_memcpy(params->values[index], bin_memlen, &bin_data, bin_memlen);
	params->lengths[index] = bin_memlen;
	params->types[index] = PQ_FLOAT4OID;
	params->formats[index] = 1;
	return true;
SE_ERROR_CLEAR:
	if (!SE_PTR_ISNULL(params->values[index]))
		soap_dealloc(arg->tsoap, params->values[index]);
	return false;
}



bool SEAPI SEPQ_params_add_float8(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, double double_val) {
	int64_t bin_data;
	int32_t bin_memlen = 8;

	SE_function_arg_invalide(arg);
	SE_send_arg_validate(params, arg, SE_2STR(SEPQ_params_add_float8));
	SE_send_check_array_index(index, params->count, arg, SE_2STR(SEPQ_params_add_float8));
	SE_send_param_has_set(params, index, arg, SE_2STR(SEPQ_params_add_float8));


	SE_send_malloc_fail((params->values[index] = (char *)soap_malloc(arg->tsoap, bin_memlen)), arg, SE_2STR(SEPQ_params_add_float8));
	bin_data = pg_bswap64((*((int64_t *)(&double_val))));
	SE_memcpy(params->values[index], bin_memlen, &bin_data, bin_memlen);
	params->lengths[index] = bin_memlen;
	params->types[index] = PQ_FLOAT8OID;
	params->formats[index] = 1;
	return true;
SE_ERROR_CLEAR:
	if (!SE_PTR_ISNULL(params->values[index]))
		soap_dealloc(arg->tsoap, params->values[index]);
	return false;
}


static bool SEPQ_params_add_string(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, const char *data, bool isdatetime, bool isjsonb) {
	int32_t bin_memlen = 0;
	char *ptr_tmp;

	SE_function_arg_invalide(arg);
	SE_send_arg_validate(params, arg, SE_2STR(SEPQ_params_add_string));
	SE_send_check_array_index(index, params->count, arg, SE_2STR(SEPQ_params_add_string));
	SE_send_arg_validate(data, arg, SE_2STR(SEPQ_params_add_string));
	SE_send_param_has_set(params, index, arg, SE_2STR(SEPQ_params_add_string));

	bin_memlen = strlen(data) + (isjsonb ? 1 : 0);
	if (bin_memlen > 32 && isdatetime) {
		resetStringBuilder(arg->buf);
		appendStringBuilder(arg->buf, "Invalid ISO 8601 datetime format");
		SE_soap_generate_error(arg, arg->buf->data);
		goto SE_ERROR_CLEAR;
	}

	SE_send_malloc_fail((params->values[index] = (char *)soap_malloc(arg->tsoap, bin_memlen + (isjsonb ? 1 : 0) + 1)), arg, SE_2STR(SEPQ_params_add_datetime));
	ptr_tmp = params->values[index];
	if (isjsonb) {
		ptr_tmp[0] = 1;
		++ptr_tmp;
	}
	SE_memcpy(ptr_tmp, bin_memlen, data, bin_memlen);
	ptr_tmp[bin_memlen] = '\0';
	params->lengths[index] = bin_memlen;
	return true;
SE_ERROR_CLEAR:
	return false;
}

//numeric和decimal完全相同,这里不解析,直接使用字符串方式提交给pg由pg解析
bool SEAPI SEPQ_params_add_numeric(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, const char *bin_data) {
	bool isok = SEPQ_params_add_string(arg, params, index, bin_data, false,false);
	if (isok) {
		params->types[index] = PQ_NUMERICOID;
		params->formats[index] = 0;
	}
	return isok;
}
bool SEAPI SEPQ_params_add_decimal(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, const char *bin_data) {
	bool isok = SEPQ_params_add_string(arg, params, index, bin_data, false, false);
	if (isok) {
		params->types[index] = PQ_NUMERICOID;
		params->formats[index] = 0;
	}
	return isok;
}


bool SEAPI SEPQ_params_add_char(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, const char *bin_data) {
	bool isok = SEPQ_params_add_string(arg, params, index, bin_data, false, false);
	if (isok) {
		params->types[index] = PQ_BPCHAROID;
		params->formats[index] = 1;
	}
	return isok;
}

bool SEAPI SEPQ_params_add_varchar(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, const char *bin_data) {
	bool isok = SEPQ_params_add_string(arg, params, index, bin_data, false, false);
	if (isok) {
		params->types[index] = PQ_VARCHAROID;
		params->formats[index] = 1;
	}
	return isok;
}

bool SEAPI SEPQ_params_add_text(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, const char *bin_data) {
	bool isok = SEPQ_params_add_string(arg, params, index, bin_data, false, false);
	if (isok) {
		params->types[index] = PQ_TEXTOID;
		params->formats[index] = 1;
	}
	return isok;
}
bool SEAPI SEPQ_params_add_json(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, const char *bin_data) {
	bool isok = SEPQ_params_add_string(arg, params, index, bin_data, false, false);
	if (isok) {
		params->types[index] = PQ_JSON;
		params->formats[index] = 1;
	}
	return isok;
}
bool SEAPI SEPQ_params_add_jsonb(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, const char *bin_data) {
	bool isok = SEPQ_params_add_string(arg, params, index, bin_data, false, true);
	if (isok) {
		params->types[index] = PQ_JSONB;
		params->formats[index] = 1;
	}
	return isok;
}

bool SEAPI SEPQ_params_add_uuid(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, const char *uuid_str) {
	int32_t bin_memlen = 0;
	pg_uuid_t uuid;

	SE_function_arg_invalide(arg);
	SE_send_arg_validate(params, arg, SE_2STR(SEPQ_params_add_uuid));
	SE_send_check_array_index(index, params->count, arg, SE_2STR(SEPQ_params_add_uuid));
	SE_send_arg_validate(uuid_str, arg, SE_2STR(SEPQ_params_add_uuid));
	SE_send_param_has_set(params, index, arg, SE_2STR(SEPQ_params_add_uuid));

	bin_memlen = strlen(uuid_str);
	memset(uuid.data, 0, UUID_LEN);
	SE_send_malloc_fail((params->values[index] = (char *)soap_malloc(arg->tsoap, 16)), arg, SE_2STR(SEPQ_params_add_uuid));
	//接受两种格式"{6CEDF74F-8125-49B9-B24C-852A6655C183}"和"6CEDF74F-8125-49B9-B24C-852A6655C183"
	if (bin_memlen >= (2 * UUID_LEN + 4))
		string_to_uuid(uuid_str, &uuid);
	SE_memcpy(params->values[index], UUID_LEN, uuid.data, UUID_LEN);
	params->lengths[index] = UUID_LEN;
	params->types[index] = PQ_UUIDOID;
	params->formats[index] = 1;
	return true;
SE_ERROR_CLEAR:
	return false;
}

bool SEAPI SEPQ_params_add_timestamp(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, const char *bin_data) {
	bool isok = SEPQ_params_add_string(arg, params, index, bin_data, true, false);
	if (isok) {
		params->types[index] = PQ_TIMESTAMPOID;
		params->formats[index] = 0;
	}
	return isok;
}
bool SEAPI SEPQ_params_add_timestamptz(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, const char *bin_data) {
	bool isok = SEPQ_params_add_string(arg, params, index, bin_data, true, false);
	if (isok){
		params->types[index] = PQ_TIMESTAMPTZOID;
		params->formats[index] = 0;
	}
	return isok;
}
bool SEAPI SEPQ_params_add_date(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, const char *bin_data) {
	bool isok = SEPQ_params_add_string(arg, params, index, bin_data, true, false);
	if (isok) {
		params->types[index] = PQ_DATEOID;
		params->formats[index] = 0;
	}
	return isok;
}
bool SEAPI SEPQ_params_add_time(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, const char *bin_data) {
	bool isok = SEPQ_params_add_string(arg, params, index, bin_data, true, false);
	if (isok){
		params->types[index] = PQ_TIMEOID;
		params->formats[index] = 0;
	}
	return isok;
}
bool SEAPI SEPQ_params_add_timetz(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, const char *bin_data) {
	bool isok = SEPQ_params_add_string(arg, params, index, bin_data, true, false);
	if (isok){
		params->types[index] = PQ_TIMETZOID;
		params->formats[index] = 0;
	}
	return isok;
}
bool SEAPI SEPQ_params_add_interval(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, const char *bin_data) {
	bool isok = SEPQ_params_add_string(arg, params, index, bin_data, true, false);
	if (isok){
		params->types[index] = PQ_INTERVAL;	
		params->formats[index] = 0;
	}
	return isok;
}











static bool SEAPI SEPQ_params_add_array(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, const char *values, int32_t array_size, int32_t oid) {
	int32_t i, bin_all_memlen = sizeof(int32_t) * 5;
	int32_t bin_memlen = 0;
	int16_t tmpi16_val;
	int32_t tmpi32_val;
	int64_t tmpi64_val;
	bool isnull = false;
	char *ptr_tmp;

	SE_function_arg_invalide(arg);
	SE_send_arg_validate(params, arg, SE_2STR(SEPQ_params_add_array));
	SE_send_check_array_index(index, params->count, arg, SE_2STR(SEPQ_params_add_array));
	SE_send_arg_validate(values, arg, SE_2STR(SEPQ_params_add_array));
	SE_send_arg_zero(array_size, arg, SE_2STR(SEPQ_params_add_array));
	SE_send_param_has_set(params, index, arg, SE_2STR(SEPQ_params_add_array));

	//计算数组所需的内存	
	switch (oid) {
	case PQ_BPCHAROID:
	case PQ_CHAROID:
		bin_memlen = sizeof(char);
		bin_all_memlen += ((sizeof(int32_t) + bin_memlen) * array_size);
		break;
	case PQ_INT2OID:
		bin_memlen = sizeof(int16_t);
		bin_all_memlen += ((sizeof(int32_t) + bin_memlen) * array_size);
		break;
	case PQ_FLOAT4OID:
	case PQ_INT4OID:
		bin_memlen = sizeof(int32_t);
		bin_all_memlen += ((sizeof(int32_t) + bin_memlen) * array_size);
		break;
	case PQ_FLOAT8OID:
	case PQ_INT8OID:
		bin_memlen = sizeof(int64_t);
		bin_all_memlen += ((sizeof(int32_t) + bin_memlen) * array_size);
		break;
	default:
		SE_send_not_support_type(arg, SE_2STR(SEPQ_params_add_array));
		break;
	}
	//为参数值分配空间
	SE_send_malloc_fail((params->values[index] = (char *)soap_malloc(arg->tsoap, bin_all_memlen)), arg, SE_2STR(SEPQ_params_add_array));
	ptr_tmp = params->values[index];
	//设置维数,只支持一维数组
	tmpi32_val = pg_hton32(1);
	SE_memcpy(ptr_tmp, sizeof(int32_t), &tmpi32_val, sizeof(int32_t));
	ptr_tmp += sizeof(int32_t);
	//设置数组中是否有NULL元素,定长数组没有NULL元素
	tmpi32_val = pg_hton32(isnull ? 1 : 0);
	SE_memcpy(ptr_tmp, sizeof(int32_t), &tmpi32_val, sizeof(int32_t));
	ptr_tmp += sizeof(int32_t);
	//设置数组OID
	tmpi32_val = pg_hton32(oid);
	SE_memcpy(ptr_tmp, sizeof(int32_t), &tmpi32_val, sizeof(int32_t));
	ptr_tmp += sizeof(int32_t);
	//设置数组第一维大小
	tmpi32_val = pg_hton32(array_size);
	SE_memcpy(ptr_tmp, sizeof(int32_t), &tmpi32_val, sizeof(int32_t));
	ptr_tmp += sizeof(int32_t);
	//设置数组下边界
	tmpi32_val = pg_hton32(1);
	SE_memcpy(ptr_tmp, sizeof(int32_t), &tmpi32_val, sizeof(int32_t));
	ptr_tmp += sizeof(int32_t);
	//设置数组元素的内容
	for (i = 0; i < array_size; ++i) {
		//元素内存大小
		tmpi32_val = pg_hton32(bin_memlen);
		SE_memcpy(ptr_tmp, sizeof(int32_t), &tmpi32_val, sizeof(int32_t));
		ptr_tmp += sizeof(int32_t);

		switch (oid) {
		case PQ_BPCHAROID:
		case PQ_CHAROID:
			SE_memcpy(ptr_tmp, bin_memlen, &values[0], bin_memlen);
			values += bin_memlen;
			ptr_tmp += bin_memlen;
			break;
		case PQ_INT2OID:
			tmpi16_val = *((int16_t *)values);
			values += bin_memlen;
			tmpi16_val = pg_hton16(tmpi16_val);
			SE_memcpy(ptr_tmp, bin_memlen, &tmpi16_val, bin_memlen);
			ptr_tmp += bin_memlen;
			break;
		case PQ_FLOAT4OID:
		case PQ_INT4OID:
			tmpi32_val = *((int32_t *)values);
			values += bin_memlen;
			tmpi32_val = pg_hton32(tmpi32_val);
			SE_memcpy(ptr_tmp, bin_memlen, &tmpi32_val, bin_memlen);
			ptr_tmp += bin_memlen;
			break;
		case PQ_FLOAT8OID:
		case PQ_INT8OID:
			tmpi64_val = *((int64_t *)values);
			values += bin_memlen;
			tmpi64_val = pg_hton64(tmpi64_val);
			SE_memcpy(ptr_tmp, bin_memlen, &tmpi64_val, bin_memlen);
			ptr_tmp += bin_memlen;
			break;
		}
	}
	params->lengths[index] = bin_all_memlen;
	return true;
SE_ERROR_CLEAR:
	return false;
}

bool SEAPI SEPQ_params_add_int16_array(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, const int16_t *array_vals, int32_t array_size) {
	bool isok = SEPQ_params_add_array(arg, params, index, (const char *)array_vals, array_size, PQ_INT2OID);
	if (isok) {
		params->types[index] = PQ_INT2ARRAYOID;
		params->formats[index] = 1;
	}
	return isok;
}

bool SEAPI SEPQ_params_add_int32_array(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, const int32_t *array_vals, int32_t array_size) {
	bool isok = SEPQ_params_add_array(arg, params, index, (const char *)array_vals, array_size, PQ_INT4OID);
	if (isok) {
		params->types[index] = PQ_INT4ARRAYOID;
		params->formats[index] = 1;
	}
	return isok;
}

bool SEAPI SEPQ_params_add_int64_array(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, const int64_t *array_vals, int32_t array_size) {
	bool isok = SEPQ_params_add_array(arg, params, index, (const char *)array_vals, array_size, PQ_INT8OID);
	if (isok) {
		params->types[index] = PQ_INT8ARRAYOID;
		params->formats[index] = 1;
	}
	return isok;
}

bool SEAPI SEPQ_params_add_float4_array(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, const float *array_vals, int32_t array_size) {
	bool isok = SEPQ_params_add_array(arg, params, index, (const char *)array_vals, array_size, PQ_FLOAT4OID);
	if (isok) {
		params->types[index] = PQ_FLOAT4ARRAYOID;
		params->formats[index] = 1;
	}
	return isok;
}

bool SEAPI SEPQ_params_add_float8_array(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, const double *array_vals, int32_t array_size) {
	bool isok = SEPQ_params_add_array(arg, params, index, (const char *)array_vals, array_size, PQ_FLOAT8OID);
	if (isok) {
		params->types[index] = PQ_FLOAT8ARRAYOID;
		params->formats[index] = 1;
	}
	return isok;
}

extern bool SEAPI SEPQ_params_add_char_one_array(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, const char *array_vals, int32_t array_size) {
	bool isok = SEPQ_params_add_array(arg, params, index, (const char *)array_vals, array_size, PQ_BPCHAROID);
	if (isok) {
		params->types[index] = PQ_BPCHARARRAYOID;
		params->formats[index] = 1;
	}
	return isok;
}


static bool SEAPI SEPQ_params_add_strarray(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, const char * const *array_vals, int32_t array_size, int32_t oid) {
	int32_t i, bin_all_memlen = sizeof(int32_t) * 5;
	int32_t *items_bin_memlen = NULL;
	int32_t tmpi32_val;
	bool isnull = false;
	char *ptr_tmp;
	pg_uuid_t uuid;

	SE_function_arg_invalide(arg);
	SE_send_arg_validate(params, arg, SE_2STR(SEPQ_params_add_strarray));
	SE_send_check_array_index(index, params->count, arg, SE_2STR(SEPQ_params_add_strarray));
	SE_send_arg_validate(array_vals, arg, SE_2STR(SEPQ_params_add_strarray));
	SE_send_arg_zero(array_size, arg, SE_2STR(SEPQ_params_add_strarray));
	SE_send_param_has_set(params, index, arg, SE_2STR(SEPQ_params_add_strarray));


	//计算数组所需的内存
	SE_send_malloc_fail((items_bin_memlen = (int32_t *)soap_malloc(arg->tsoap, array_size * sizeof(int32_t))), arg, SE_2STR(SEPQ_params_add_strarray));
	for (i = 0; i < array_size; ++i) {
		if (SE_PTR_ISNULL(array_vals[i])) {
			items_bin_memlen[i] = 0;
			isnull = true;
			bin_all_memlen += (sizeof(int32_t) + 0);
		} else {
			switch (oid) {
			case PQ_BPCHAROID:
			case PQ_CHAROID:
			case PQ_VARCHAROID:
			case PQ_TEXTOID:
				items_bin_memlen[i] = strlen(array_vals[i]);
				bin_all_memlen += (sizeof(int32_t) + items_bin_memlen[i]);
				break;
			case PQ_UUIDOID:
				items_bin_memlen[i] = strlen(array_vals[i]);
				bin_all_memlen += (sizeof(int32_t) + UUID_LEN);
				break;
			default:
				SE_send_not_support_type(arg, SE_2STR(SEPQ_params_add_strarray));
				break;
			}
		}
	}

	SE_send_malloc_fail((params->values[index] = (char *)soap_malloc(arg->tsoap, bin_all_memlen)), arg, SE_2STR(SEPQ_params_add_strarray));
	ptr_tmp = params->values[index];
	//设置维数
	tmpi32_val = pg_hton32(1);
	SE_memcpy(ptr_tmp, sizeof(int32_t), &tmpi32_val, sizeof(int32_t));
	ptr_tmp += sizeof(int32_t);
	//设置数组中是否有NULL元素
	tmpi32_val = pg_hton32(isnull ? 1 : 0);
	SE_memcpy(ptr_tmp, sizeof(int32_t), &tmpi32_val, sizeof(int32_t));
	ptr_tmp += sizeof(int32_t);
	//设置数组OID
	tmpi32_val = pg_hton32(oid);
	SE_memcpy(ptr_tmp, sizeof(int32_t), &tmpi32_val, sizeof(int32_t));
	ptr_tmp += sizeof(int32_t);
	//设置数组第一维大小
	tmpi32_val = pg_hton32(array_size);
	SE_memcpy(ptr_tmp, sizeof(int32_t), &tmpi32_val, sizeof(int32_t));
	ptr_tmp += sizeof(int32_t);
	//设置数组下边界
	tmpi32_val = pg_hton32(1);
	SE_memcpy(ptr_tmp, sizeof(int32_t), &tmpi32_val, sizeof(int32_t));
	ptr_tmp += sizeof(int32_t);
	//设置数组元素的内容
	for (i = 0; i < array_size; ++i) {
		if (items_bin_memlen[i] > 0) {
			switch (oid) {
			case PQ_BPCHAROID:
			case PQ_CHAROID:
			case PQ_VARCHAROID:
			case PQ_TEXTOID:
				//元素内存大小
				tmpi32_val = pg_hton32(items_bin_memlen[i]);
				SE_memcpy(ptr_tmp, sizeof(int32_t), &tmpi32_val, sizeof(int32_t));
				ptr_tmp += sizeof(int32_t);
				//元素内容
				SE_memcpy(ptr_tmp, items_bin_memlen[i], array_vals[i], items_bin_memlen[i]);
				ptr_tmp += items_bin_memlen[i];
				break;
			case PQ_UUIDOID:
				//元素内存大小
				tmpi32_val = pg_hton32(UUID_LEN);
				SE_memcpy(ptr_tmp, sizeof(int32_t), &tmpi32_val, sizeof(int32_t));
				ptr_tmp += sizeof(int32_t);
				//元素内容
				if (items_bin_memlen[i] >= (2 * UUID_LEN + 4))
					string_to_uuid(array_vals[i], &uuid);
				SE_memcpy(ptr_tmp, UUID_LEN, uuid.data, UUID_LEN);
				ptr_tmp += UUID_LEN;
				break;
			}
		}
	}
	params->lengths[index] = bin_all_memlen;
	params->formats[index] = 1;
	if (!SE_PTR_ISNULL(items_bin_memlen))
		soap_dealloc(arg->tsoap, items_bin_memlen);
	return true;
SE_ERROR_CLEAR:
	if (!SE_PTR_ISNULL(items_bin_memlen))
		soap_dealloc(arg->tsoap, items_bin_memlen);
	return false;
}


bool SEAPI SEPQ_params_add_char_more_array(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, const char * const *array_vals, int32_t array_size) {
	bool isok = SEPQ_params_add_strarray(arg, params, index, array_vals, array_size, PQ_BPCHAROID);
	if (isok) {
		params->types[index] = PQ_BPCHARARRAYOID;
	}
	return isok;
}

bool SEAPI SEPQ_params_add_varchar_array(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, const char * const *array_vals, int32_t array_size) {
	bool isok = SEPQ_params_add_strarray(arg, params, index, array_vals, array_size, PQ_VARCHAROID);
	if (isok) {
		params->types[index] = PQ_VARCHARARRAYOID;
	}
	return isok;
}

bool SEAPI SEPQ_params_add_text_array(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, const char * const *array_vals, int32_t array_size) {
	bool isok = SEPQ_params_add_strarray(arg, params, index, array_vals, array_size, PQ_TEXTOID);
	if (isok) {
		params->types[index] = PQ_TEXTARRAYOID;
	}
	return isok;
}


bool SEAPI SEPQ_params_add_uuid_array(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, const char * const *array_vals, int32_t array_size) {
	bool isok = SEPQ_params_add_strarray(arg, params, index, array_vals, array_size, PQ_UUIDOID);
	if (isok) {
		params->types[index] = PQ_UUIDARRAYOID;
	}
	return isok;
}