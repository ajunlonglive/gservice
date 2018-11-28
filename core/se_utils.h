#ifndef SE_4549801D_ACD3_440B_9FD6_4CE6EC8C8975
#define SE_4549801D_ACD3_440B_9FD6_4CE6EC8C8975
#include <pthread.h>
#include <libpq-fe.h>
#include "../pg/stringbuilder.h"
#include "../pg/pg_bswap.h"
#include "../gsoap/stdsoap2.h"
#include "../gsoap/json.h"
#include "se_std.h"
#include "se_conf.h"
#include "se_service.h"


/*****************************************************************************
*	PG����ȼ�����
*****************************************************************************/
typedef enum {
	////SQL ��׼������һ�ֶ���ļ���READ UNCOMMITTED���� PostgreSQL��READ UNCOMMITTED������ READ COMMITTED
	//READ_UNCOMMITTED,
	//READ_COMMITTED,
	//REPEATABLE_READ,
	//SERIALIZABLE,	
	//
	READ_UNCOMMITTED_READ_WRITE,
	READ_UNCOMMITTED_READ_ONLY,

	READ_COMMITTED_READ_WRITE,
	READ_COMMITTED_READ_ONLY,

	REPEATABLE_READ_READ_WRITE,
	REPEATABLE_READ_READ_ONLY,

	SERIALIZABLE_READ_WRITE,
	SERIALIZABLE_READ_ONLY,
	//ֻ������Ҳ��SERIALIZABLE�Լ� READ ONLYʱ��DEFERRABLE �������ԲŻ���Ч��
	//��һ��������������������Զ���ѡ��ʱ���������� ��һ�λ�ȡ�����ʱ���ܻ�����������֮��������ʱ�Ͳ����� SERIALIZABLE����Ŀ������Ҳ������κ��������� ��һ�����л�ʧ��ȡ���ķ��ա�
	//����ģʽ���ʺ��ڳ�ʱ�����еı������ ���ݡ� 
	SERIALIZABLE_READ_ONLY_DEFERRABLE
} SEPQ_ISOLATION_LEVEL;

/*****************************************************************************
*	PG OID���Ͷ���
*	select oid,typname,typarray from pg_type order by oid;
*	typarray>0 �ı�ʾ���������͵���������
*****************************************************************************/
#ifdef SE_COLLAPSED

#define PQ_BOOLOID						16
#define PQ_BYTEAOID						17

#define PQ_CHAROID						18
#define PQ_INT8OID							20
#define PQ_INT2OID							21
#define PQ_INT4OID							23
#define PQ_TEXTOID						25

#define PQ_NUMERICOID					1700
#define PQ_FLOAT4OID					700
#define PQ_FLOAT8OID					701
#define PQ_MONEYOID					790

#define PQ_BPCHAROID					1042
#define PQ_VARCHAROID					1043
#define PQ_JSONOID						114
#define PQ_JSONBOID						3802

#define PQ_DATEOID						1082
#define PQ_TIMEOID						1083
#define PQ_TIMETZOID					1266
#define PQ_TIMESTAMPOID				1114
#define PQ_TIMESTAMPTZOID			1184
#define PQ_UUIDOID						2950
#define PQ_INTERVAL						1186

#define PQ_JSON								114
#define PQ_JSONB							3802

#define PQ_INT2ARRAYOID				1005
#define PQ_INT4ARRAYOID				1007
#define PQ_INT8ARRAYOID				1016

#define PQ_FLOAT4ARRAYOID			1021
#define PQ_FLOAT8ARRAYOID			1022

#define PQ_CHARARRAYOID				1002
#define PQ_TEXTARRAYOID				1009
#define PQ_BPCHARARRAYOID			1014
#define PQ_VARCHARARRAYOID		1015
#define PQ_UUIDARRAYOID				2951
#endif

/*****************************************************************************
*	������Ч���������
*****************************************************************************/
#define SE_function_arg_invalide(arg_)\
do{\
	if (SE_PTR_ISNULL(arg_))\
		goto SE_ERROR_CLEAR;\
}while (0)

/*****************************************************************************
*	��ͻ��˷�����Ч���������-���ں��������ļ��
*****************************************************************************/
#define SE_send_arg_validate(ptr_,arg_,funname_)\
do{\
	if(SE_PTR_ISNULL(ptr_) ){\
		if (!SE_PTR_ISNULL(arg_)){\
			resetStringBuilder(arg_->buf);\
			appendStringBuilder(arg_->buf, "Invalid input parameters(%s).", SE_PTR_ISNULL(funname_)?"":funname_);\
			SE_soap_generate_error(arg_, arg->buf->data);\
			goto SE_ERROR_CLEAR;\
		}else{\
			goto SE_ERROR_CLEAR;\
		}\
	}\
}while (0)

/*****************************************************************************
*	��ͻ��˷������Ͳ���Ϊ0�쳣
*****************************************************************************/
#define SE_send_arg_zero(param_,arg_,name_) do{\
	if( 0==param_){\
		if ( !SE_PTR_ISNULL(arg_)){\
			resetStringBuilder(arg_->buf);\
			appendStringBuilder(arg_->buf, "Input parameters is zero(%s).", SE_PTR_ISNULL(name_)?"":name_);\
			SE_soap_generate_error(arg_, arg_->buf->data);\
			goto SE_ERROR_CLEAR;\
		}else{\
			goto SE_ERROR_CLEAR;\
		}\
	}\
}while (0)

/*****************************************************************************
*	��ͻ��˷������Ͳ���С��1�쳣
*****************************************************************************/
#define SE_send_arg_less1(intval_,arg_,funname_)\
do{\
	if(intval_< 1){\
		if (!SE_PTR_ISNULL(arg_)){\
			resetStringBuilder(arg_->buf);\
			appendStringBuilder(arg_->buf, "Input parameters less than 1(%s).", SE_PTR_ISNULL(funname_)?"":funname_);\
			SE_soap_generate_error(arg_, arg->buf->data);\
			goto SE_ERROR_CLEAR;\
		}else{\
			goto SE_ERROR_CLEAR;\
		}\
	}\
}while (0)


/*****************************************************************************
*	��ͻ��˷���ָ�����������δ����-���ڼ��ͻ��˵ı������
*****************************************************************************/
#define SE_request_arg_isnotset(param_,arg_,name_) do{\
	if( SE_PTR_ISNULL(param_)){\
		if (!SE_PTR_ISNULL(arg_)){\
			resetStringBuilder(arg_->buf);\
			appendStringBuilder(arg_->buf, "Request parameter ""%s""  is not set.",  SE_PTR_ISNULL(name_)?"":name_);\
			SE_soap_generate_error(arg_, arg->buf->data);\
			goto SE_ERROR_CLEAR;\
		}else{\
			goto SE_ERROR_CLEAR;\
		}\
	}\
}while (0)



/*****************************************************************************
*	��ͻ��˷���δ֪����
*****************************************************************************/
#define SE_send_unknown_err(arg_) do{\
	if ( !SE_PTR_ISNULL(arg_)){\
		resetStringBuilder(arg_->buf);\
		appendStringBuilder(arg_->buf, "Unknown error.");\
		SE_soap_generate_error(arg_, arg->buf->data);\
		goto SE_ERROR_CLEAR;\
	}else{\
		goto SE_ERROR_CLEAR;\
	}\
}while (0)

/*****************************************************************************
*	��ͻ��˷���û�п��õ������쳣
*****************************************************************************/
#define SE_send_conn_null(conn_,arg_) do{\
	if( SE_PTR_ISNULL(conn_)){\
		if ( !SE_PTR_ISNULL(arg_)){\
			resetStringBuilder(arg_->buf);\
			appendStringBuilder(arg_->buf, "No connections are available.");\
			SE_soap_generate_error(arg_, arg->buf->data);\
			goto SE_ERROR_CLEAR;\
		}else{\
			goto SE_ERROR_CLEAR;\
		}\
	}\
}while (0)

/*****************************************************************************
*	��ͻ��˺�������ʧ����Ϣ
*****************************************************************************/
#define SE_send_check_rc(rc_,arg_,funname_) do{\
	if(rc_){\
		if (!SE_PTR_ISNULL(arg_)){\
			resetStringBuilder(arg_->buf);\
			appendStringBuilder(arg_->buf, "call \"%s\" fail.", SE_PTR_ISNULL(funname_)?"":funname_);\
			SE_soap_generate_error(arg_, arg_->buf->data);\
			goto SE_ERROR_CLEAR;\
		}else{\
			goto SE_ERROR_CLEAR; \
		}\
	}\
}while (0)
/*****************************************************************************
*	��ͻ��˷��ͷ����ڴ�ʧ����Ϣ
*****************************************************************************/
#define SE_send_malloc_fail(ptr_,arg_,funname_) do{\
	if(SE_PTR_ISNULL(ptr_) ){\
		if (!SE_PTR_ISNULL(arg_)){\
			resetStringBuilder(arg_->buf);\
			appendStringBuilder(arg_->buf, "%s:%s.", SE_PTR_ISNULL(funname_)?"":funname_,strerror(ENOMEM));\
			SE_soap_generate_error(arg_, arg_->buf->data);\
			goto SE_ERROR_CLEAR;\
		}else{\
			goto SE_ERROR_CLEAR; \
		}\
	}\
}while (0)

/*****************************************************************************
*	��ͻ��˷���һ�����������������ݷ�Χ�쳣��Ϣ`
*****************************************************************************/
#define SE_send_check_array_index(array_index_,array_size_,arg_,funname_) do{\
	if (array_index_<0 || array_index_>=array_size_){\
		if (!SE_PTR_ISNULL(arg_)){\
			resetStringBuilder(arg_->buf); \
			appendStringBuilder(arg_->buf, "Array subscript exceeds the range of arrays(%s).", SE_PTR_ISNULL(funname_) ? "" : funname_); \
			SE_soap_generate_error(arg_, arg_->buf->data); \
			goto SE_ERROR_CLEAR; \
		} else {\
			goto SE_ERROR_CLEAR; \
		}\
	}\
}while (0)

/*****************************************************************************
*	��ͻ��˷���һ������Ϊindex�Ĳ����Ѿ����õ��쳣
*****************************************************************************/
#define SE_send_param_has_set(params,index,arg_,funname_) do{\
	if (!SE_PTR_ISNULL(params->values[index])){\
		if (!SE_PTR_ISNULL(arg_)){\
			resetStringBuilder(arg_->buf); \
			appendStringBuilder(arg_->buf, "Parameters with index \"%d\" have been set(%s).",index, SE_PTR_ISNULL(funname_) ? "" : funname_); \
			SE_soap_generate_error(arg_, arg_->buf->data); \
			goto SE_ERROR_CLEAR; \
		} else {\
			goto SE_ERROR_CLEAR; \
		}\
	}\
}while (0)


/*****************************************************************************
*	��ͻ��˷���һ����֧�ֵ������쳣��Ϣ
*****************************************************************************/
#define SE_send_not_support_type(arg_,funname_) do{\
	if (!SE_PTR_ISNULL(arg_)){\
		resetStringBuilder(arg_->buf);\
		appendStringBuilder(arg_->buf, "Types that are not supported(%s).", SE_PTR_ISNULL(funname_)?"":funname_);\
		SE_soap_generate_error(arg_, arg->buf->data);\
		goto SE_ERROR_CLEAR;\
	}else{\
		goto SE_ERROR_CLEAR; \
	}\
}while (0)

/*****************************************************************************
*	���pgִ�������Ƿ�ɹ�,�粻�ɹ�����ͻ��˷���ʧ��ԭ��
*****************************************************************************/
#define SE_check_pqexec_fail_send(success_,result_,arg_) do{\
	ExecStatusType status_=PQresultStatus(result_);\
	if(success_ != status_){\
		if (!SE_PTR_ISNULL(arg_)){\
			resetStringBuilder(arg_->buf); \
			appendStringBuilder(arg_->buf, "(%d)%s", status_,PQerrorMessage(arg_->pg->conn)); \
			SE_soap_generate_error(arg_, arg_->buf->data); \
			goto SE_ERROR_CLEAR;\
		}else{\
			goto SE_ERROR_CLEAR; \
		}\
	}\
}while (0)


/*****************************************************************************
*	����tokenʧ��
*****************************************************************************/
#define SE_get_jwtval_check(function_,arg_) do{\
	function_;\
	if( errno ){\
		if (!SE_PTR_ISNULL(arg_)){\
			resetStringBuilder(arg_->buf);\
			appendStringBuilder(arg_->buf, "get token value fail(%d).", errno);\
			SE_soap_generate_error(arg_, arg_->buf->data);\
			goto SE_ERROR_CLEAR;\
		}else{\
			goto SE_ERROR_CLEAR; \
		}\
	}\
}while (0)

/*****************************************************************************
*	PQexecParamsʹ�õĲ����ṹ����
*****************************************************************************/
struct SEPQ_EXECPARAMS {
	//��������
	int32_t count;
	//������������
	Oid *types;
	//����ֵ����
	char **values;
	//ÿ������ֵ�Ĵ�С����(�����ƴ�С)
	int32_t *lengths;
	//����ֵ�Ƿ�Ϊ����������
	int32_t *formats;
};

#ifdef __cplusplus
extern "C" {
#endif	/*__cplusplus 1*/


	/*************************************************************************************
	*	���������Ϣ���ͻ���
	*	Parameter:
	*		[in] sstruct SE_WORKINFO * arg -  ���������
	*		[in] const char * err - ������Ϣ
	*	Remarks:	����ʹ��generate_error��,�Ա��ʽ��������Ϣ
	*************************************************************************************/
	extern void  SEAPI SE_soap_generate_error(struct SE_WORKINFO *arg, const char *err);

	/*************************************************************************************
	*	�����Ϣ���ͻ���.status:200,msg:msg
	*	Parameter:
	*		[in] struct SE_WORKINFO * arg -  ���������
	*		[in] const char * msg - ��Ϣ
	*************************************************************************************/
	extern void  SEAPI SE_soap_generate_message(struct SE_WORKINFO *arg, const char *msg);

	/*************************************************************************************
	*	Ҫ��ͻ����ض���.status:302,msg:msg,url:url
	*	Parameter:
	*		[in] struct SE_WORKINFO * arg -  ���������
	*		[in] const char * msg - ��Ϣ
	*		[in] const char * url - �ض���url
	*************************************************************************************/
	extern void  SEAPI SE_soap_generate_redirect(struct SE_WORKINFO *arg, const char *msg, const char *url);

	/*************************************************************************************
	*	���pg��ѯ���
	*	Parameter:
	*		[in] struct SE_WORKINFO * arg -  ���������
	*		[in] const PGresult *result - pg��ѯ���
	*	Remarks:
	*	֧�������PG��������
	*		��ͨ����
	*			BOOL
	*			BYTEA																			ת��Ϊbase64�ַ������
	*			INT2/INT4/INT8
	*			FLOAT4/FLOAT8/MONEY
	*			NUMERIC/DECIMAL														�������ݾ�������,��������ַ���
	*			CHAR/VARCHAR/TEXT
	*			UUID
	*			TIMESTAMP/TIMESTAMPTZ/DATE/TIME/TIMETZ			����ʱ�����UNIXʱ���
	*		��������,ֻ֧��һά����,��ά���齫��֧��
	*			INT2/INT4/INT8
	*			FLOAT4/FLOAT8
	*			CHAR/VARCHAR/TEXT
	*			UUID
	*	ֻ֧�ֶ����������ʽ,��֧���ַ������ʽ
	*	ʹ��gsoap�е�struct value *v��Ȼ����json�ĵ��ǳ�����,���������ѯ�����Ҫ�ǳ���εķ�����ͷ��ڴ�
	*	�ھ������ԱȽϺ�,û��ʹ��struct value *v,����ʹ��StringBuilder��ƴ���ַ����ķ�ʽ����json
	*	��Ϊ��ƴ���ַ����ķ�ʽ���ɵ�json,��Ȼ���߷�������,������Ӧ�����ݲ���ȷ�ĵط�
	*	�緢���в���ȷ�ĵط�,��д������ϵ�޸�81855841@qq.com
	*************************************************************************************/
	extern bool  SEAPI SE_soap_generate_recordset(struct SE_WORKINFO *arg, const PGresult *result);

	/*************************************************************************************
	*	�����ַ���crcУ����
	*	Parameter:
	*		[in] const void * buf - �ַ���
	*		[in] size_t bufLen - �ַ�������
	*************************************************************************************/
	extern uint16_t SEAPI crc16(const char *buf, uint16_t bufLen);
	extern uint32_t SEAPI crc32(const char *buf, uint32_t bufLen);
	extern uint64_t SEAPI crc64(const char *buf, uint64_t bufLen);


	/*************************************************************************************
	*	������Ĳ����б��л�ȡָ���Ĳ���
	*	Parameter:
	*		[in] struct value * request - �������
	*		[in] const char * const name - ������
	*		[in] uint32_t desired_val_type -	�����Ĳ�������,ֵΪ����֮һ
	*					��json��׼ԭ��,û��int16_t��int32_t����,SOAP_TYPE__i4��SOAP_TYPE__int��ȫһ��,���ص���int64_t����ָ��
	SOAP_TYPE__boolean:						��������Ϊconst char *ָ��
	SOAP_TYPE__double:							��������Ϊconst double *ָ��
	SOAP_TYPE__i4:									��������Ϊconst int64_t *ָ��
	SOAP_TYPE__int:								��������Ϊconst int64_t *ָ��
	SOAP_TYPE__string:							��������Ϊconst char *ָ��
	SOAP_TYPE__dateTime_DOTiso8601:	��������Ϊconst char *ָ��
	SOAP_TYPE__array:							��������Ϊconst struct value *ָ��,��Ҫ�ٴν���
	SOAP_TYPE__struct:							��������Ϊconst struct value *ָ��,��Ҫ�ٴν���
	SOAP_TYPE__base64:							��������Ϊconst struct _base64 *ָ��
	*	Remarks:	������Ĳ�����Ч��ָ�����ƵĲ��������ڡ�ָ���Ĳ��������������Ĳ������Ͳ�һ�����ַ����Ĳ���ֵΪ��("") ������NULL	*
	*************************************************************************************/
	extern const void *SEAPI get_req_param_value(struct SE_WORKINFO *arg, const char * const name, uint32_t desired_val_type);

	/*************************************************************************************
	*	��ʼһ������
	*	Parameter:
	*		[in] struct SE_WORKINFO * arg - ���������
	*		[in] enum SEPQ_ISOLATION_LEVEL level - ����ȼ�
	*************************************************************************************/
	extern bool SEAPI SEPQ_begin(struct SE_WORKINFO *arg, SEPQ_ISOLATION_LEVEL level);

	/*************************************************************************************
	*	�ύһ������
	*	Parameter:
	*		[in] struct SE_WORKINFO * arg - ���������
	*************************************************************************************/
	extern bool SEAPI SEPQ_commit(struct SE_WORKINFO *arg);

	/*************************************************************************************
	*	�ع�һ������
	*	Parameter:
	*		[in] struct SE_WORKINFO * arg -���������
	*************************************************************************************/
	extern void SEAPI SEPQ_rollback(struct SE_WORKINFO *arg);

	/*************************************************************************************
	*	����token���������ͻ���
	*	Parameter:
	*		[in] struct SE_WORKINFO * arg - ���������
	*		[in] int64_t userid - �û����
	*		[in] time_t current_tm - ��ǰʱ��,unixʱ��
	*		[in] const int64_t * last - token����������,unixʱ��,������ΪNULL��current_tm+8Сʱ
	*	Remarks:expired��ÿ��������ڸ��µ�,last�򴴽����ڸ���
	*************************************************************************************/
	extern bool SEAPI SE_make_token(struct SE_WORKINFO *arg, int64_t userid, time_t current_tm, const int64_t *last);

	/*************************************************************************************
	*	��������֤token
	*	Parameter:
	*		[in] struct SE_WORKINFO * arg - ���������
	*		[in] const char * token_base64 - token base64����
	*		[out] int64_t * userid - �û����
	*		[out] int64_t * expired - ��ǰtoken����Ч��
	*		[out] int64_t * last - token����������,unixʱ��
	*	Remarks:expired��ÿ��������ڸ��µ�,last�򴴽��Ժ󲻸���
	*************************************************************************************/
	extern bool SEAPI SE_parse_token(struct SE_WORKINFO *arg, const char *token_base64, int64_t *userid, int64_t *expired, int64_t *last);

	/*************************************************************************************
	*	base64����
	*	Parameter:
	*		[in] struct SE_WORKINFO * arg - ���������
	*		[in] const char * bin_data - Ҫ���������
	*		[in] int32_t bin_memlen - Ҫ����������ڴ��С
	*		[in] char * * ptr - ����������
	*		[in] int32_t * base64_len - �����������ڴ��С
	*	Remarks:�����ڴ����soap_malloc,��Ϊ�����ͷ��ڴ�.��SE_base64_encryptֻ���ڴ���䷽ʽ��ͬ
	*************************************************************************************/
	extern bool SEAPI SE_b64soap_encrypt(struct SE_WORKINFO *arg, const char *bin_data, int32_t bin_memlen, char **ptr, int32_t *base64_len);
	/*************************************************************************************
	*	base64����
	*	Parameter:
	*		[in] struct SE_WORKINFO * arg - ���������
	*		[in] const char * base64 -  Ҫ���������
	*		[in] int32_t b64_len -  Ҫ����������ڴ��С
	*		[in] char * * ptr - ����������
	*		[in] int32_t * bin_memlen - �����������ڴ��С
	*	Remarks:�����ڴ����soap_malloc,��Ϊ�����ͷ��ڴ�.��SE_base64_decryptֻ���ڴ���䷽ʽ��ͬ
	*************************************************************************************/
	extern bool SEAPI SE_b64soap_decrypt(struct SE_WORKINFO *arg, const char *base64, int32_t b64_len, char **ptr, int32_t *bin_memlen);
	/*************************************************************************************
	*	����PQexecParams��������
	*	Parameter:
	*		[in] struct SE_WORKINFO * arg - ���������
	*		[in] int32_t param_count - ���������С
	*		[out] struct SEPQ_EXECPARAMS * * ptr - SEPQ_EXECPARAMS����
	*	Remarks:	�����ɹ���һ���յ�����,����Ҫ����SEPQ_param_add���ø�������
	*					���������޸Ĳ���������,����޸������´���
	*					ptr����������soap_malloc�����ڴ�,��������ͷ�
	*************************************************************************************/
	extern bool SEAPI SEPQ_params_create(struct SE_WORKINFO *arg, int32_t params_count, struct SEPQ_EXECPARAMS **ptr);

	/*************************************************************************************
	*	�ͷ���SEPQ_EXECPARAMSʹ�õ��ڴ�[��ѡ��]
	*	Parameter:
	*		[in] struct SE_WORKINFO * arg - ���������
	*		[in] struct SEPQ_EXECPARAMS * * ptr - SEPQ_EXECPARAMS����
	*	Remarks:SEPQ_EXECPARAMS��soap_malloc�����ڴ�,��˿��������ͷ�,�ڱ�����������ɺ���ϵͳ�Զ��ͷ�
	*************************************************************************************/
	extern void SEAPI SEPQ_params_free(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS **ptr);

	/*************************************************************************************
	*	ΪPQexecParams��������ָ����������ֵ
	*	Parameter:
	*		[in] struct SE_WORKINFO * arg - ���������
	*		[in] struct SEPQ_EXECPARAM * params - PQexecParams��������
	*		[in] int32_t index - Ҫ����ֵ��PQexecParams������������,��0��ʼ
	*		[in] bin_data - ��ǰ�������ֵ,������
	*		[in] ibin_memlen- ��ǰ�������ֵ�ڴ��С
	*************************************************************************************/
	extern bool SEAPI SEPQ_params_add_null(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index);
	extern bool SEAPI SEPQ_params_add_bool(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, bool bin_data);
	extern bool SEAPI SEPQ_params_add_bytea(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, const char *bin_data);
	extern bool SEAPI SEPQ_params_add_int16(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, int16_t bin_data);
	extern bool SEAPI SEPQ_params_add_int32(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, int32_t bin_data);
	extern bool SEAPI SEPQ_params_add_int64(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, int64_t bin_data);
	extern bool SEAPI SEPQ_params_add_money(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, int64_t bin_data);
	extern bool SEAPI SEPQ_params_add_float4(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, float bin_data);
	extern bool SEAPI SEPQ_params_add_float8(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, double bin_data);
	extern bool SEAPI SEPQ_params_add_numeric(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, const char *bin_data);
	extern bool SEAPI SEPQ_params_add_decimal(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, const char *bin_data);
	extern bool SEAPI SEPQ_params_add_char(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, const char *bin_data);
	extern bool SEAPI SEPQ_params_add_varchar(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, const char *bin_data);
	extern bool SEAPI SEPQ_params_add_text(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, const char *bin_data);
	extern bool SEAPI SEPQ_params_add_uuid(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, const char *uuid_str);
	extern bool SEAPI SEPQ_params_add_json(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, const char *bin_data);
	extern bool SEAPI SEPQ_params_add_jsonb(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, const char *bin_data);
	//����ʱ��ͼ��ֻ����ISO 8601��ʽ���ַ���
	extern bool SEAPI SEPQ_params_add_timestamp(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, const char *bin_data);
	extern bool SEAPI SEPQ_params_add_timestamptz(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, const char *bin_data);
	extern bool SEAPI SEPQ_params_add_date(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, const char *bin_data);
	extern bool SEAPI SEPQ_params_add_time(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, const char *bin_data);
	extern bool SEAPI SEPQ_params_add_timetz(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, const char *bin_data);
	extern bool SEAPI SEPQ_params_add_interval(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, const char *bin_data);



	/*************************************************************************************
	*	ΪPQexecParams��������ָ����������ֵ-��������
	*	Parameter:
	*		[in] struct SE_WORKINFO * arg - ���������
	*		[in] struct SEPQ_EXECPARAMS * params - PQexecParams��������
	*		[in] int32_t index - Ҫ����ֵ��PQexecParams������������,��0��ʼ
	*		[in] const int16_t * values - ��ǰ��������
	*		[in] int32_t array_size -  ��ǰ���������С
	*************************************************************************************/
	extern bool SEAPI SEPQ_params_add_int16_array(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, const int16_t *values, int32_t array_size);
	extern bool SEAPI SEPQ_params_add_int32_array(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, const int32_t *array_vals, int32_t array_size);
	extern bool SEAPI SEPQ_params_add_int64_array(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, const int64_t *array_vals, int32_t array_size);
	extern bool SEAPI SEPQ_params_add_float4_array(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, const float *array_vals, int32_t array_size);
	extern bool SEAPI SEPQ_params_add_float8_array(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, const double *array_vals, int32_t array_size);
	extern bool SEAPI SEPQ_params_add_char_one_array(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, const char *array_vals, int32_t array_size);
	extern bool SEAPI SEPQ_params_add_char_more_array(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, const char * const *array_vals, int32_t array_size);
	extern bool SEAPI SEPQ_params_add_varchar_array(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, const char * const *array_vals, int32_t array_size);
	extern bool SEAPI SEPQ_params_add_text_array(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, const char * const *array_vals, int32_t array_size);
	extern bool SEAPI SEPQ_params_add_uuid_array(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, const char * const *array_vals, int32_t array_size);

#ifdef __cplusplus
};
#endif  /*__cplusplus 2*/


#endif /*SE_4549801D_ACD3_440B_9FD6_4CE6EC8C8975*/