#include <time.h>
#include "../pg/pg_bswap.h"
#include "se_utils.h"
#include "se_numeric.h"
/*************************************************************************************
*	����PQexecParams�����Ʋ�ѯ���
*************************************************************************************/

#define USECS_PER_SEC   INT64CONST(1000000)/*
*	pgʱ���UNIXʱ���ֵ,�ֱ�Ϊ��\����\��
*	postgresql ���ں�ʱ�����2000-01-01 00:00:00��ʼ����
*/
#define  SE_PQUNIX_DAY_DIFF	 (-10957)
#define  SE_PQUNIX_SEC_DIFF	 (-946684800)
/*pgʱ��ת��ΪUNIXʱ��*/
#define  SE_PQDATE_UNIXTM(x)	((x-SE_PQUNIX_DAY_DIFF)*24*3600)
#define  SE_PQTM_UNIXTM(x)	((x/USECS_PER_SEC) - SE_PQUNIX_SEC_DIFF)


//base64����
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
/* UUID�ֽڴ�С */
#define UUID_LEN 16
//UUID����
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

//ISO 8601 DateTime 8�ֽ�
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
			tzhour = arg->pg->time_zone / 3600;	//���ʱ��
			tzminute = arg->pg->time_zone % 3600;//���ʱ���Ƿ��������3600,�粻����������Ҫ������Ӳ���
			if (0 == tzminute)//ֻ���Сʱ
				appendStringBuilder(arg->sys, "%04d-%02d-%02dT%02d:%02d:%02d%s%02d", pt->tm_year + 1900, pt->tm_mon + 1, pt->tm_mday,
					pt->tm_hour, pt->tm_min, pt->tm_sec, (arg->pg->time_zone > 0 ? "+" : "-"), abs(tzhour));
			else//������Ӳ���
				appendStringBuilder(arg->sys, "%04d-%02d-%02dT%02d:%02d:%02d%s%02d:%02d", pt->tm_year + 1900, pt->tm_mon + 1, pt->tm_mday,
					pt->tm_hour, pt->tm_min, pt->tm_sec, (arg->pg->time_zone > 0 ? "+" : "-"), abs(tzhour), abs(tzminute / 60));
		} else {
			appendStringBuilder(arg->sys, "%04d-%02d-%02dT%02d:%02d:%02dZ", pt->tm_year + 1900, pt->tm_mon + 1, pt->tm_mday, pt->tm_hour, pt->tm_min, pt->tm_sec);
		}
	}
	appendStringBuilderCharNT(arg->sys, '\"');
}

//ISO 8601 Date 4�ֽ�
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

//ISO 8601  time 8�ֽ� timetz 12�ֽ�
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

	tzhour = arg->pg->time_zone / 3600;	//���ʱ��
	tzminute = arg->pg->time_zone % 3600;//���ʱ���Ƿ��������3600,�粻����������Ҫ������Ӳ���

	appendStringBuilderCharNT(arg->sys, '\"');
	if (time_zone) {
		appendStringBuilder(arg->sys, "%02d:%02d:%02d%s%02d:%02d",
			hour, minute, second, (arg->pg->time_zone > 0 ? "+" : "-"), abs(tzhour), abs(tzminute / 60));
	} else {
		appendStringBuilder(arg->sys, "%02d:%02d:%02dZ", hour, minute, second);
	}
	appendStringBuilderCharNT(arg->sys, '\"');
}

//ISO 8601  interval 16�ֽ�
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
*	POSTGRESQLһά����ͷ��Ϣ
*	Ŀǰֻ֧��һά����
*****************************************************************************/
//struct SEPQ_ONE_ARRTYPE {
//	//����ά��
//	int32_t ndim;
//	//�����Ƿ������NULLֵ.bool����
//	int32_t dataoffset;
//	//������Ԫ�ص�����
//	Oid elemtype;
//	//����Ԫ������
//	int32_t array_size;
//	//�����±߽�.��ԶΪ1,û���õ��������
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
		if (elem_memlen > 0) { //�����е�Ԫ�ز�ΪNULL
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
			default: /*��֧�ֵ�����*/
				appendBinaryStringBuilderNT(arg->sys, "null", 4);
			}
			appendStringBuilderChar(arg->sys, ',');
			ptrtmp += elem_memlen;	//ǰ������һԪ��
		} else {
			appendBinaryStringBuilderNT(arg->sys, "null,", 5);
		}
		//ptrtmp += (elem_memlen < 1 ? 0 : elem_memlen);
	}
	--arg->sys->len;
}
static bool SEPQ_parse_one_array(struct SE_WORKINFO *arg, const char *const data) {
	/*****************************************************************************
	��һ������4�ֽڣ�����ά��
		00 00 00 02������ʾ��ά����
	�ڶ���������4�ֽڣ���ʾ�������Ƿ���ΪNULL��Ԫ��,bool�����ж�
		00 00 00 00������ʾ�����е�Ԫ���Ƿ�û��ΪNULL��Ԫ��,����Ϊ 00 00 00 01
	�������������������͵�OID
		00 00 00 17����ת����ʮ���ƺ�ֵΪ23��23��INT4OID����ʾ����������INT4����
	���ĸ������ǵ�һ��ά����Ԫ������
		00 00 00 02����ת����ʮ���ƺ�ֵΪ2����ʾ��һά��������Ԫ��
	�������������ԶΪ1����ÿ��ά�ȵ��±߽�
		00 00 00 01
	������һά����
		֮����ԭʼ�������ݣ�4�ֽڳ���+���ݣ�����˳������
	�����Ƕ�ά����**(�����ʾ�����������)**
		������������ʾ�ڶ���ά�ȴ�С,ʾ����Ԫ������Ϊ4
		���߸���������ԶΪ1�����±߽�
	֮����ԭʼ�������ݣ�4�ֽڳ���+���ݣ�����˳������
	*****************************************************************************/
	//����ά��,�����Ƿ������NULLֵ.bool����,����Ԫ������,�����±߽�
	int32_t ndim, /*dataoffset,lower_bnds,*/ array_size;
	//������Ԫ�ص�����
	Oid elemtype;
	const char * ptrtmp;
	ptrtmp = data;
	ndim = pg_hton32(*((const uint32_t *)ptrtmp));
	ptrtmp += sizeof(int32_t);
	if (1 != ndim) //�Ƿ�Ϊ1ά����,��ʱ��֧�ֶ�ά����
		SE_send_not_support_type(arg, SE_2STR(SEPQ_parse_one_array));

	//dataoffset = pg_hton32(*((const uint32_t *)ptrtmp));
	ptrtmp += sizeof(int32_t);

	elemtype = pg_hton32(*((const uint32_t *)ptrtmp));
	ptrtmp += sizeof(int32_t);

	array_size = pg_hton32(*((const uint32_t *)ptrtmp));
	ptrtmp += sizeof(int32_t);

	//lower_bnds = pg_hton32(*((const uint32_t *)ptrtmp));
	ptrtmp += sizeof(int32_t);
	//������.����������
	if (']' == arg->sys->data[arg->sys->len - 1]) { //�����Ѿ�����
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

	//��ȡ������������
	cells_count = PQnfields(result);
	if (cells_count < 1) {
		//���û����ֱ������һ���յ����ݼ�������	
		appendBinaryStringBuilderNT(arg->sys, "[null]", 6);
		return true;
	}

	SE_send_malloc_fail((cells_type = (Oid *)soap_malloc(arg->tsoap, cells_count * sizeof(Oid))), arg, SE_2STR(SE_soap_generate_recordset));
	for (j = 0; j < cells_count; ++j)
		cells_type[j] = PQftype(result, j);
	//��ȡ����
	rows_count = PQntuples(result);
	for (i = 0; i < rows_count; ++i) {
		appendStringBuilderChar(arg->sys, '[');
#ifdef SE_COLLAPSED		
		for (j = 0; j < cells_count; ++j) {
#ifdef SE_COLLAPSED
			bin_result = PQgetvalue(result, i, j);
			bin_mem_len = PQgetlength(result, i, j);
			if (bin_mem_len > 0) {	//��NULL����
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
					//��֧�ֵ������������Ϊnull
					appendBinaryStringBuilderNT(arg->sys, "null", 4);
					break;
				}
			} else {
				//��ֵΪnull
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
	//����������
	if (SE_PTR_ISNULL(arg))
		goto SE_ERROR_CLEAR;
	SE_send_arg_validate(result, arg, SE_2STR(SE_soap_generate_recordset));
	if (']' == arg->sys->data[arg->sys->len - 1]) { //��¼���Ѿ�����,�������¼��
		arg->sys->data[arg->sys->len - 1] = ',';
		appendStringBuilderChar(arg->sys, '[');
	} else {
		appendBinaryStringBuilderNT(arg->sys, "\"recordset\":[[", 14);
	}
	//���ִ�н���Ƿ���ȷ
	status = PQresultStatus(result);
	switch (status) {
	case PGRES_EMPTY_QUERY:
	case PGRES_COMMAND_OK:
		//��û��null,�в���null
		appendBinaryStringBuilderNT(arg->sys, "]]", 2); //��һ��¼��ʱ�Ὣ���һ��]�滻Ϊ,����Ҫ]]
		return true;
	case PGRES_TUPLES_OK:
		SE_throw(SE_reader(arg, result));
		appendBinaryStringBuilderNT(arg->sys, "]]", 2); //��һ��¼��ʱ�Ὣ���һ��]�滻Ϊ,����Ҫ]]
		break;
	default:
		SE_soap_generate_error(arg, PQerrorMessage(arg->pg->conn));
		goto SE_ERROR_CLEAR;
	}
	return true;
SE_ERROR_CLEAR:
	return false;
}

