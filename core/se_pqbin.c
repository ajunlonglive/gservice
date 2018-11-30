#include <time.h>
#include "../pg/pg_bswap.h"
#include "se_utils.h"
#include "se_numeric.h"
/*************************************************************************************
*	解析PQexecParams二进制查询结果
*************************************************************************************/

#define USECS_PER_SEC   INT64CONST(1000000)/*
*	pg时间和UNIX时间差值,分别为年\分钟\秒
*	postgresql 日期和时间均从2000-01-01 00:00:00开始计算
*/
#define  SE_PQUNIX_DAY_DIFF	 (-10957)
#define  SE_PQUNIX_SEC_DIFF	 (-946684800)
/*pg时间转换为UNIX时间*/
#define  SE_PQDATE_UNIXTM(x)	((x-SE_PQUNIX_DAY_DIFF)*24*3600)
#define  SE_PQTM_UNIXTM(x)	((x/USECS_PER_SEC) - SE_PQUNIX_SEC_DIFF)


//base64编码
static bool SE_parse_base64(struct SE_WORKINFO *arg, const char *const bin_data, int32_t bin_memlen) {
	char *b64_data = NULL;
	int32_t b64_len = 0;
	SE_throw(SE_b64soap_encrypt(arg, bin_data, bin_memlen, &b64_data, &b64_len));
	appendStringBuilderChar(arg->sys, '\"');
	appendBinaryStringBuilderNT(arg->sys, b64_data, b64_len);
	appendStringBuilderChar(arg->sys, '\"');
	return true;
SE_ERROR_CLEAR:
	if (!SE_PTR_ISNULL(b64_data))
		soap_dealloc(arg->tsoap, b64_data);
	return false;
}
/* UUID字节大小 */
#define UUID_LEN 16
//UUID编码
static void SE_parse_uuid(struct SE_WORKINFO *arg, const uint8_t *bin_data) {
	static const char hex_chars[] = "0123456789abcdef";
	int32_t i, hi, lo;
	//36 = 16 * 2 + 4
	appendStringBuilderCharNT(arg->sys, '\"');
	for (i = 0; i < UUID_LEN; i++) {
		hi = 0, lo = 0;
		if (i == 4 || i == 6 || i == 8 || i == 10)
			appendStringBuilderCharNT(arg->sys, '-');
		hi = bin_data[i] >> 4;
		lo = bin_data[i] & 0x0F;
		appendStringBuilderCharNT(arg->sys, hex_chars[hi]);
		appendStringBuilderCharNT(arg->sys, hex_chars[lo]);
	}
	appendStringBuilderCharNT(arg->sys, '\"');
}

//ISO 8601 DateTime 8字节
static void SE_parse_timestamp(struct SE_WORKINFO *arg, const char *bin_result, bool time_zone) {
	struct tm tt, *pt = &tt;
	int64_t tmp64_val = (int64_t)pg_hton64(*((const uint64_t *)bin_result));
	int32_t tzhour, tzminute;

	tmp64_val = SE_PQTM_UNIXTM(tmp64_val) + (time_zone ? arg->pg->time_zone : 0);
	pt = gmtime(&tmp64_val);
	appendStringBuilderCharNT(arg->sys, '\"');
	if (SE_PTR_ISNULL(pt)) {
		appendBinaryStringBuilderNT(arg->sys, "1970-01-01T00:00:00Z", 20);
	} else {
		if (time_zone) {
			tzhour = arg->pg->time_zone / 3600;	//输出时区
			tzminute = arg->pg->time_zone % 3600;//检查时期是否可以整除3600,如不能整除还需要输出分钟部份
			if (0 == tzminute)//只输出小时
				appendStringBuilder(arg->sys, "%04d-%02d-%02dT%02d:%02d:%02d%s%02d", pt->tm_year + 1900, pt->tm_mon + 1, pt->tm_mday,
					pt->tm_hour, pt->tm_min, pt->tm_sec, (arg->pg->time_zone > 0 ? "+" : "-"), abs(tzhour));
			else//输出分钟部份
				appendStringBuilder(arg->sys, "%04d-%02d-%02dT%02d:%02d:%02d%s%02d:%02d", pt->tm_year + 1900, pt->tm_mon + 1, pt->tm_mday,
					pt->tm_hour, pt->tm_min, pt->tm_sec, (arg->pg->time_zone > 0 ? "+" : "-"), abs(tzhour), abs(tzminute / 60));
		} else {
			appendStringBuilder(arg->sys, "%04d-%02d-%02dT%02d:%02d:%02dZ", pt->tm_year + 1900, pt->tm_mon + 1, pt->tm_mday, pt->tm_hour, pt->tm_min, pt->tm_sec);
		}
	}
	appendStringBuilderCharNT(arg->sys, '\"');
}

//ISO 8601 Date 4字节
static void SE_parse_date(struct SE_WORKINFO *arg, const char *bin_result) {
	struct tm tt, *pt = &tt;
	int32_t tmpi32_val;
	int64_t tmpi64_val;

	tmpi32_val = (int32_t)pg_hton32(*((const uint32_t *)bin_result));
	tmpi64_val = SE_PQDATE_UNIXTM(tmpi32_val);
	pt = gmtime(&tmpi64_val);
	appendStringBuilderCharNT(arg->sys, '\"');
	if (SE_PTR_ISNULL(pt)) {
		appendBinaryStringBuilderNT(arg->sys, "1970-01-01", 10);
	} else {
		appendStringBuilder(arg->sys, "%04d-%02d-%02d", pt->tm_year + 1900, pt->tm_mon + 1, pt->tm_mday);
	}
	appendStringBuilderCharNT(arg->sys, '\"');
}

//ISO 8601  time 8字节 timetz 12字节
static void SE_parse_time(struct SE_WORKINFO *arg, const char *bin_result, bool time_zone) {
	int32_t hour, minute, second, tzhour, tzminute;;
	int32_t tzoffset = 0;
	int64_t tmpi64_val;
	const char *tmpstr_ptr = bin_result;


	tmpi64_val = pg_hton64(*((const uint64_t *)tmpstr_ptr));
	tmpi64_val /= USECS_PER_SEC;
	if (time_zone) {
		tmpstr_ptr += sizeof(int64_t);
		tzoffset = pg_hton32(*((const uint32_t *)tmpstr_ptr));
		tmpi64_val += tzoffset;
		tmpi64_val += arg->pg->time_zone;
	}

	hour = (int32_t)(tmpi64_val / 3600);
	minute = (int32_t)(tmpi64_val % 3600 / 60);
	second = (int32_t)(tmpi64_val - (hour * 3600) - (minute * 60));

	tzhour = arg->pg->time_zone / 3600;	//输出时区
	tzminute = arg->pg->time_zone % 3600;//检查时期是否可以整除3600,如不能整除还需要输出分钟部份

	appendStringBuilderCharNT(arg->sys, '\"');
	if (time_zone) {
		appendStringBuilder(arg->sys, "%02d:%02d:%02d%s%02d:%02d",
			hour, minute, second, (arg->pg->time_zone > 0 ? "+" : "-"), abs(tzhour), abs(tzminute / 60));
	} else {
		appendStringBuilder(arg->sys, "%02d:%02d:%02dZ", hour, minute, second);
	}
	appendStringBuilderCharNT(arg->sys, '\"');
}

//ISO 8601  interval 16字节
static void SE_parse_interval(struct SE_WORKINFO *arg, const char *bin_result) {
	int64_t time;
	int32_t day, month;
	int32_t hour, minute, second;
	const char *tmpstr_ptr = bin_result;

	time = pg_hton64(*((const uint64_t *)tmpstr_ptr));
	time /= USECS_PER_SEC;
	tmpstr_ptr += sizeof(int64_t);
	day = pg_hton32(*((const uint32_t *)tmpstr_ptr));
	tmpstr_ptr += sizeof(int32_t);
	month = pg_hton32(*((const uint32_t *)tmpstr_ptr));


	hour = (int32_t)(time / 3600);
	minute = (int32_t)(time % 3600 / 60);
	second = (int32_t)(time - (hour * 3600) - (minute * 60));

	appendStringBuilderCharNT(arg->sys, '\"');
	appendStringBuilder(arg->sys, "P%dM%dDT%dH%dM%dS", month, day, hour, minute, second);
	appendStringBuilderCharNT(arg->sys, '\"');
}

/*****************************************************************************
*	POSTGRESQL一维数组头信息
*	目前只支持一维数组
*****************************************************************************/
//struct SEPQ_ONE_ARRTYPE {
//	//数组维度
//	int32_t ndim;
//	//数组是否包含有NULL值.bool类型
//	int32_t dataoffset;
//	//数组中元素的类型
//	Oid elemtype;
//	//数组元素数量
//	int32_t array_size;
//	//数组下边界.永远为1,没有用到这个参数
//	int32_t lower_bnds;
//};
static void SEPQ_parse_one_array_elem(struct SE_WORKINFO *arg, int32_t array_size, Oid elemtype, const char * ptrtmp) {
	int32_t i, elem_memlen;
	int16_t tmp16_val;
	int32_t tmp32_val;
	int64_t tmp64_val;

	for (i = 0; i < array_size; ++i) {
		elem_memlen = (int32_t)pg_hton32(*((const uint32_t *)ptrtmp));
		ptrtmp += sizeof(int32_t);
		if (elem_memlen > 0) { //数组中的元素不为NULL
			switch (elemtype) {
			case PQ_INT2OID:
				tmp16_val = (int16_t)pg_hton16(*((const uint16_t *)ptrtmp));
				appendStringBuilder(arg->sys, "%d", tmp16_val);
				break;
			case PQ_INT4OID:
				tmp32_val = (int32_t)pg_hton32(*((const uint32_t *)ptrtmp));
				appendStringBuilder(arg->sys, "%d", tmp32_val);
				break;
			case PQ_INT8OID:
				tmp64_val = (int64_t)pg_hton64(*((const uint64_t *)ptrtmp));
#if defined(_MSC_VER)		
				appendStringBuilder(arg->sys, "%I64d", tmp64_val);
#else
#	ifdef __x86_64__
				appendStringBuilder(arg->sys, "%ld", tmp64_val);
#	elif __i386__
				appendStringBuilder(arg->sys, "%lld", tmp64_val);
#	endif
#endif
				break;
			case PQ_FLOAT4OID:
				tmp32_val = (int32_t)pg_hton32(*((const uint32_t *)ptrtmp));
				appendStringBuilder(arg->sys, global_shared->conf->double_format, *((float *)(&tmp32_val)));
				break;
			case PQ_FLOAT8OID:
				tmp64_val = (int64_t)pg_hton64(*((const uint64_t *)ptrtmp));
				appendStringBuilder(arg->sys, global_shared->conf->double_format, *((double *)(&tmp64_val)));
				break;
			case PQ_BPCHAROID:
			case PQ_CHAROID:
			case PQ_VARCHAROID:
			case PQ_TEXTOID:
				appendStringBuilderChar(arg->sys, '\"');
				appendBinaryStringBuilderJsonStr(arg->sys, ptrtmp, elem_memlen);
				appendStringBuilderChar(arg->sys, '\"');
				break;
			case PQ_UUIDOID:
				SE_parse_uuid(arg, (const uint8_t  *)ptrtmp);
				break;			
			default: /*不支持的类型*/
				appendBinaryStringBuilderNT(arg->sys, "null", 4);
			}
			appendStringBuilderChar(arg->sys, ',');
			ptrtmp += elem_memlen;	//前进至下一元素
		} else {
			appendBinaryStringBuilderNT(arg->sys, "null,", 5);
		}
		//ptrtmp += (elem_memlen < 1 ? 0 : elem_memlen);
	}
	--arg->sys->len;
}
static bool SEPQ_parse_one_array(struct SE_WORKINFO *arg, const char *const data) {
	/*****************************************************************************
	第一个整（4字节）数是维数
		00 00 00 02　　表示二维数组
	第二个整数（4字节）表示数组中是否有为NULL的元素,bool类型判断
		00 00 00 00　　表示数组中的元素是否没有为NULL的元素,否则为 00 00 00 01
	第三个整数是数组类型的OID
		00 00 00 17　　转换成十进制后值为23（23，INT4OID）表示数组类型是INT4　　
	第四个整数是第一个维度中元素数量
		00 00 00 02　　转换成十进制后值为2，表示第一维包含二个元素
	第五个整数（永远为1），每个维度的下边界
		00 00 00 01
	如是是一维数组
		之后是原始数组数据（4字节长度+数据），按顺序排列
	如是是多维数组**(上面的示例按这个解析)**
		第六个整数表示第二个维度大小,示例中元素数量为4
		第七个整数（永远为1），下边界
	之后是原始数组数据（4字节长度+数据），按顺序排列
	*****************************************************************************/
	//数组维度,数组是否包含有NULL值.bool类型,数组元素数量,数组下边界
	int32_t ndim, /*dataoffset,lower_bnds,*/ array_size;
	//数组中元素的类型
	Oid elemtype;
	const char * ptrtmp;
	ptrtmp = data;
	ndim = pg_hton32(*((const uint32_t *)ptrtmp));
	ptrtmp += sizeof(int32_t);
	if (1 != ndim) //是否为1维数组,暂时不支持多维数组
		SE_send_not_support_type(arg, SE_2STR(SEPQ_parse_one_array));

	//dataoffset = pg_hton32(*((const uint32_t *)ptrtmp));
	ptrtmp += sizeof(int32_t);

	elemtype = pg_hton32(*((const uint32_t *)ptrtmp));
	ptrtmp += sizeof(int32_t);

	array_size = pg_hton32(*((const uint32_t *)ptrtmp));
	ptrtmp += sizeof(int32_t);

	//lower_bnds = pg_hton32(*((const uint32_t *)ptrtmp));
	ptrtmp += sizeof(int32_t);
	//创建列.此列是数组
	if (']' == arg->sys->data[arg->sys->len - 1]) { //数组已经存在
		appendBinaryStringBuilderNT(arg->sys, ",[", 2);
	} else {
		appendStringBuilderChar(arg->sys, '[');
	}
	SEPQ_parse_one_array_elem(arg, array_size, elemtype, ptrtmp);
	appendStringBuilderChar(arg->sys, ']');
	return true;
SE_ERROR_CLEAR:
	return false;
}

static bool SE_reader(struct SE_WORKINFO *arg, const PGresult *result) {
	int32_t i = 0, j = 0;
	int32_t rows_count = 0, cells_count = 0;

	Oid *cells_type;
	char *bin_result;
	int32_t bin_mem_len;

	int16_t tmp16_val;
	int32_t tmp32_val;
	int64_t tmp64_val;
	int8_t	jsonb_version = 0;
	const struct NumericVar *num_val;
	const char *tmpstr_val;

	//获取列数和列类型
	cells_count = PQnfields(result);
	if (cells_count < 1) {
		//如果没有列直接生成一个空的数据集并返回	
		appendBinaryStringBuilderNT(arg->sys, "[null]", 6);
		return true;
	}

	SE_send_malloc_fail((cells_type = (Oid *)soap_malloc(arg->tsoap, cells_count * sizeof(Oid))), arg, SE_2STR(SE_soap_generate_recordset));
	for (j = 0; j < cells_count; ++j)
		cells_type[j] = PQftype(result, j);
	//获取行数
	rows_count = PQntuples(result);
	for (i = 0; i < rows_count; ++i) {
		appendStringBuilderChar(arg->sys, '[');
#ifdef SE_COLLAPSED		
		for (j = 0; j < cells_count; ++j) {
#ifdef SE_COLLAPSED
			bin_result = PQgetvalue(result, i, j);
			bin_mem_len = PQgetlength(result, i, j);
			if (bin_mem_len > 0) {	//非NULL数据
				switch (cells_type[j]) {
				case PQ_BOOLOID:
					if (0 == bin_result[0])
						appendBinaryStringBuilderNT(arg->sys, "false", 5);
					else
						appendBinaryStringBuilderNT(arg->sys, "true", 4);
					break;
				case PQ_BYTEAOID:
					SE_throw(SE_parse_base64(arg, bin_result, bin_mem_len));
					break;
				case PQ_INT2OID:
					tmp16_val = (int16_t)pg_hton16(*((const uint16_t *)bin_result));
					appendStringBuilder(arg->sys, "%d", tmp16_val);
					break;
				case PQ_INT4OID:
					tmp32_val = (int32_t)pg_hton32(*((const uint32_t *)bin_result));
					appendStringBuilder(arg->sys, "%d", tmp32_val);
					break;
				case PQ_INT8OID:
					tmp64_val = (int64_t)pg_hton64(*((const uint64_t *)bin_result));
#if defined(_MSC_VER)		
					appendStringBuilder(arg->sys, "%I64d", tmp64_val);
#else
#	ifdef __x86_64__
					appendStringBuilder(arg->sys, "%ld", tmp64_val);
#	elif __i386__
					appendStringBuilder(arg->sys, "%lld", tmp64_val);
#	endif
#endif
					break;
				case PQ_FLOAT4OID:
					tmp32_val = (int32_t)pg_hton32(*((const uint32_t *)bin_result));
					appendStringBuilder(arg->sys, global_shared->conf->double_format, *((float *)(&tmp32_val)));
					break;
				case PQ_FLOAT8OID:
					tmp64_val = (int64_t)pg_hton64(*((const uint64_t *)bin_result));
					appendStringBuilder(arg->sys, global_shared->conf->double_format, *((double *)(&tmp64_val)));
					break;
				case PQ_MONEYOID:
					tmp64_val = (int64_t)pg_hton64(*((const uint64_t *)bin_result));
					appendStringBuilder(arg->sys, "%.2Lf", ((double)tmp64_val) / 100.0L);
					break;
				case PQ_NUMERICOID:
					num_val = bytes2numeric(arg, (const uint8_t *)bin_result);
					if (NULL == num_val)
						goto SE_ERROR_CLEAR;
					tmpstr_val = get_str_from_var(arg, num_val);
					appendStringBuilderChar(arg->sys, '\"');
					appendBinaryStringBuilderNT(arg->sys, tmpstr_val, strlen(tmpstr_val));
					appendStringBuilderChar(arg->sys, '\"');
					break;
				case PQ_BPCHAROID:
				case PQ_CHAROID:
				case PQ_VARCHAROID:
				case PQ_TEXTOID:
					appendStringBuilderChar(arg->sys, '\"');
					appendBinaryStringBuilderJsonStr(arg->sys, bin_result, bin_mem_len);
					appendStringBuilderChar(arg->sys, '\"');
					break;
				case PQ_UUIDOID:
					SE_parse_uuid(arg, (const uint8_t *)bin_result);
					break;
				case PQ_TIMESTAMPOID:
					SE_parse_timestamp(arg, bin_result, false);
					break;
				case PQ_TIMESTAMPTZOID:
					SE_parse_timestamp(arg, bin_result, true);
					break;
				case PQ_DATEOID:
					SE_parse_date(arg, bin_result);
					break;
				case PQ_TIMEOID:
					SE_parse_time(arg, bin_result, false);
					break;
				case PQ_TIMETZOID:
					SE_parse_time(arg, bin_result, true);
					break;
				case PQ_INTERVAL:
					SE_parse_interval(arg, bin_result);
					break;
				case PQ_JSON:
					appendStringBuilderChar(arg->sys, '\"');
					appendBinaryStringBuilderJsonStr(arg->sys, bin_result, bin_mem_len);
					appendStringBuilderChar(arg->sys, '\"');
					break;
				case PQ_JSONB:
					tmpstr_val = bin_result;
					jsonb_version = tmpstr_val[0];
					tmpstr_val += sizeof(int8_t);
					appendStringBuilderChar(arg->sys, '\"');
					appendBinaryStringBuilderJsonStr(arg->sys, tmpstr_val, bin_mem_len);
					appendStringBuilderChar(arg->sys, '\"');
					break;
				case PQ_INT2ARRAYOID:
				case PQ_INT4ARRAYOID:
				case PQ_INT8ARRAYOID:
				case PQ_FLOAT4ARRAYOID:
				case PQ_FLOAT8ARRAYOID:
				case PQ_BPCHARARRAYOID:
				case PQ_CHARARRAYOID:
				case PQ_VARCHARARRAYOID:
				case PQ_TEXTARRAYOID:
				case PQ_UUIDARRAYOID:
					SEPQ_parse_one_array(arg, bin_result);
					break;
				default:
					//不支持的数据类型输出为null
					appendBinaryStringBuilderNT(arg->sys, "null", 4);
					break;
				}
			} else {
				//列值为null
				appendBinaryStringBuilderNT(arg->sys, "null", 4);
			}
#endif
			appendStringBuilderChar(arg->sys, ',');
		}
#endif
		--arg->sys->len;
		appendBinaryStringBuilderNT(arg->sys, "],", 2);
	}
	--arg->sys->len;
	return true;
SE_ERROR_CLEAR:
	return false;
}

bool  SEAPI SE_soap_generate_recordset(struct SE_WORKINFO *arg, const PGresult *result) {
	ExecStatusType status;
	//检查输入参数
	if (SE_PTR_ISNULL(arg))
		goto SE_ERROR_CLEAR;
	SE_send_arg_validate(result, arg, SE_2STR(SE_soap_generate_recordset));
	if (']' == arg->sys->data[arg->sys->len - 1]) { //记录集已经存在,添加至记录中
		arg->sys->data[arg->sys->len - 1] = ',';
		appendStringBuilderChar(arg->sys, '[');
	} else {
		appendBinaryStringBuilderNT(arg->sys, "\"recordset\":[[", 14);
	}
	//检查执行结果是否正确
	status = PQresultStatus(result);
	switch (status) {
	case PGRES_EMPTY_QUERY:
	case PGRES_COMMAND_OK:
		//行没有null,列才有null
		appendBinaryStringBuilderNT(arg->sys, "]]", 2); //下一记录集时会将最后一个]替换为,所以要]]
		return true;
	case PGRES_TUPLES_OK:
		SE_throw(SE_reader(arg, result));
		appendBinaryStringBuilderNT(arg->sys, "]]", 2); //下一记录集时会将最后一个]替换为,所以要]]
		break;
	default:
		SE_soap_generate_error(arg, PQerrorMessage(arg->pg->conn));
		goto SE_ERROR_CLEAR;
	}
	return true;
SE_ERROR_CLEAR:
	return false;
}

