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
*	PG事务等级定义
*****************************************************************************/
typedef enum {
	////SQL 标准定义了一种额外的级别：READ UNCOMMITTED。在 PostgreSQL中READ UNCOMMITTED被视作 READ COMMITTED
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
	//只有事务也是SERIALIZABLE以及 READ ONLY时，DEFERRABLE 事务属性才会有效。
	//当一个事务的所有这三个属性都被选择时，该事务在 第一次获取其快照时可能会阻塞，在那之后它运行时就不会有 SERIALIZABLE事务的开销并且不会有任何牺牲或者 被一次序列化失败取消的风险。
	//这种模式很适合于长时间运行的报表或者 备份。 
	SERIALIZABLE_READ_ONLY_DEFERRABLE
} SEPQ_ISOLATION_LEVEL;

/*****************************************************************************
*	PG OID类型定义
*	select oid,typname,typarray from pg_type order by oid;
*	typarray>0 的表示此数据类型的数组类型
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
*	函数无效的输入参数
*****************************************************************************/
#define SE_function_arg_invalide(arg_)\
do{\
	if (SE_PTR_ISNULL(arg_))\
		goto SE_ERROR_CLEAR;\
}while (0)

/*****************************************************************************
*	向客户端发送无效的输入参数-用于函数参数的检查
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
*	向客户端发送整型参数为0异常
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
*	向客户端发送整型参数小于1异常
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
*	向客户端发送指定的请求参数未设置-用于检查客户端的必填参数
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
*	向客户端发送未知错误
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
*	向客户端发送没有可用的连接异常
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
*	向客户端函数运行失败消息
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
*	向客户端发送分配内存失败消息
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
*	向客户端发送一个数组索引超出数据范围异常消息`
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
*	向客户端发送一个索引为index的参数已经设置的异常
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
*	向客户端发送一个不支持的类型异常消息
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
*	检查pg执行命令是否成功,如不成功将向客户端发送失败原因
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
*	解析token失败
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
*	PQexecParams使用的参数结构定义
*****************************************************************************/
struct SEPQ_EXECPARAMS {
	//参数数量
	int32_t count;
	//参数类型数组
	Oid *types;
	//参数值数组
	char **values;
	//每个参数值的大小数组(二进制大小)
	int32_t *lengths;
	//参数值是否为二进制数组
	int32_t *formats;
};

#ifdef __cplusplus
extern "C" {
#endif	/*__cplusplus 1*/


	/*************************************************************************************
	*	输出错误信息到客户端
	*	Parameter:
	*		[in] sstruct SE_WORKINFO * arg -  请求处理参数
	*		[in] const char * err - 错误信息
	*	Remarks:	建议使用generate_error宏,以便格式化错误信息
	*************************************************************************************/
	extern void  SEAPI SE_soap_generate_error(struct SE_WORKINFO *arg, const char *err);

	/*************************************************************************************
	*	输出消息到客户端.status:200,msg:msg
	*	Parameter:
	*		[in] struct SE_WORKINFO * arg -  请求处理参数
	*		[in] const char * msg - 信息
	*************************************************************************************/
	extern void  SEAPI SE_soap_generate_message(struct SE_WORKINFO *arg, const char *msg);

	/*************************************************************************************
	*	要求客户端重定向.status:302,msg:msg,url:url
	*	Parameter:
	*		[in] struct SE_WORKINFO * arg -  请求处理参数
	*		[in] const char * msg - 信息
	*		[in] const char * url - 重定向url
	*************************************************************************************/
	extern void  SEAPI SE_soap_generate_redirect(struct SE_WORKINFO *arg, const char *msg, const char *url);

	/*************************************************************************************
	*	输出pg查询结果
	*	Parameter:
	*		[in] struct SE_WORKINFO * arg -  请求处理参数
	*		[in] const PGresult *result - pg查询结果
	*	Remarks:
	*	支持输出的PG数据类型
	*		普通类型
	*			BOOL
	*			BYTEA																			转换为base64字符串输出
	*			INT2/INT4/INT8
	*			FLOAT4/FLOAT8/MONEY
	*			NUMERIC/DECIMAL														由于数据精度问题,输出的是字符串
	*			CHAR/VARCHAR/TEXT
	*			UUID
	*			TIMESTAMP/TIMESTAMPTZ/DATE/TIME/TIMETZ			日期时间输出UNIX时间戳
	*		数组类型,只支持一维数组,多维数组将来支持
	*			INT2/INT4/INT8
	*			FLOAT4/FLOAT8
	*			CHAR/VARCHAR/TEXT
	*			UUID
	*	只支持二进制输出格式,不支持字符输出格式
	*	使用gsoap中的struct value *v虽然生成json文档非常方便,但是输出查询结果需要非常多次的分配和释放内存
	*	在经过测试比较后,没有使用struct value *v,而是使用StringBuilder以拼接字符串的方式生成json
	*	因为是拼接字符串的方式生成的json,虽然作者反复测试,难免有应答数据不正确的地方
	*	如发现有不正确的地方,请写作者联系修改81855841@qq.com
	*************************************************************************************/
	extern bool  SEAPI SE_soap_generate_recordset(struct SE_WORKINFO *arg, const PGresult *result);

	/*************************************************************************************
	*	生成字符串crc校验码
	*	Parameter:
	*		[in] const void * buf - 字符串
	*		[in] size_t bufLen - 字符串长度
	*************************************************************************************/
	extern uint16_t SEAPI crc16(const char *buf, uint16_t bufLen);
	extern uint32_t SEAPI crc32(const char *buf, uint32_t bufLen);
	extern uint64_t SEAPI crc64(const char *buf, uint64_t bufLen);


	/*************************************************************************************
	*	从请求的参数列表中获取指定的参数
	*	Parameter:
	*		[in] struct value * request - 请求对象
	*		[in] const char * const name - 参数名
	*		[in] uint32_t desired_val_type -	期望的参数类型,值为下列之一
	*					因json标准原因,没有int16_t和int32_t类型,SOAP_TYPE__i4和SOAP_TYPE__int完全一样,返回的是int64_t类型指针
	SOAP_TYPE__boolean:						返回类型为const char *指针
	SOAP_TYPE__double:							返回类型为const double *指针
	SOAP_TYPE__i4:									返回类型为const int64_t *指针
	SOAP_TYPE__int:								返回类型为const int64_t *指针
	SOAP_TYPE__string:							返回类型为const char *指针
	SOAP_TYPE__dateTime_DOTiso8601:	返回类型为const char *指针
	SOAP_TYPE__array:							返回类型为const struct value *指针,需要再次解析
	SOAP_TYPE__struct:							返回类型为const struct value *指针,需要再次解析
	SOAP_TYPE__base64:							返回类型为const struct _base64 *指针
	*	Remarks:	如输入的参数无效、指定名称的参数不存在、指定的参数类型与期望的参数类型不一至、字符串的参数值为空("") 均返回NULL	*
	*************************************************************************************/
	extern const void *SEAPI get_req_param_value(struct SE_WORKINFO *arg, const char * const name, uint32_t desired_val_type);

	/*************************************************************************************
	*	开始一个事务
	*	Parameter:
	*		[in] struct SE_WORKINFO * arg - 请求处理参数
	*		[in] enum SEPQ_ISOLATION_LEVEL level - 事务等级
	*************************************************************************************/
	extern bool SEAPI SEPQ_begin(struct SE_WORKINFO *arg, SEPQ_ISOLATION_LEVEL level);

	/*************************************************************************************
	*	提交一个事务
	*	Parameter:
	*		[in] struct SE_WORKINFO * arg - 请求处理参数
	*************************************************************************************/
	extern bool SEAPI SEPQ_commit(struct SE_WORKINFO *arg);

	/*************************************************************************************
	*	回滚一个事务
	*	Parameter:
	*		[in] struct SE_WORKINFO * arg -请求处理参数
	*************************************************************************************/
	extern void SEAPI SEPQ_rollback(struct SE_WORKINFO *arg);

	/*************************************************************************************
	*	生成token并发送至客户端
	*	Parameter:
	*		[in] struct SE_WORKINFO * arg - 请求处理参数
	*		[in] int64_t userid - 用户编号
	*		[in] time_t current_tm - 当前时间,unix时间
	*		[in] const int64_t * last - token的生命周期,unix时间,如设置为NULL则current_tm+8小时
	*	Remarks:expired是每次请求后都在更新的,last则创建后不在更新
	*************************************************************************************/
	extern bool SEAPI SE_make_token(struct SE_WORKINFO *arg, int64_t userid, time_t current_tm, const int64_t *last);

	/*************************************************************************************
	*	解析并验证token
	*	Parameter:
	*		[in] struct SE_WORKINFO * arg - 请求处理参数
	*		[in] const char * token_base64 - token base64编码
	*		[out] int64_t * userid - 用户编号
	*		[out] int64_t * expired - 当前token的有效期
	*		[out] int64_t * last - token的生命周期,unix时间
	*	Remarks:expired是每次请求后都在更新的,last则创建以后不更新
	*************************************************************************************/
	extern bool SEAPI SE_parse_token(struct SE_WORKINFO *arg, const char *token_base64, int64_t *userid, int64_t *expired, int64_t *last);

	/*************************************************************************************
	*	base64编码
	*	Parameter:
	*		[in] struct SE_WORKINFO * arg - 请求处理参数
	*		[in] const char * bin_data - 要编码的数据
	*		[in] int32_t bin_memlen - 要编码的数据内存大小
	*		[in] char * * ptr - 编码后的数据
	*		[in] int32_t * base64_len - 编码后的数据内存大小
	*	Remarks:分配内存采用soap_malloc,因为无需释放内存.较SE_base64_encrypt只有内存分配方式不同
	*************************************************************************************/
	extern bool SEAPI SE_b64soap_encrypt(struct SE_WORKINFO *arg, const char *bin_data, int32_t bin_memlen, char **ptr, int32_t *base64_len);
	/*************************************************************************************
	*	base64解码
	*	Parameter:
	*		[in] struct SE_WORKINFO * arg - 请求处理参数
	*		[in] const char * base64 -  要解码的数据
	*		[in] int32_t b64_len -  要解码的数据内存大小
	*		[in] char * * ptr - 解码后的数据
	*		[in] int32_t * bin_memlen - 解码后的数据内存大小
	*	Remarks:分配内存采用soap_malloc,因为无需释放内存.较SE_base64_decrypt只有内存分配方式不同
	*************************************************************************************/
	extern bool SEAPI SE_b64soap_decrypt(struct SE_WORKINFO *arg, const char *base64, int32_t b64_len, char **ptr, int32_t *bin_memlen);
	/*************************************************************************************
	*	创建PQexecParams参数数组
	*	Parameter:
	*		[in] struct SE_WORKINFO * arg - 请求处理参数
	*		[in] int32_t param_count - 参数数组大小
	*		[out] struct SEPQ_EXECPARAMS * * ptr - SEPQ_EXECPARAMS对象
	*	Remarks:	创建成功是一个空的数组,还需要调用SEPQ_param_add设置各个参数
	*					创建后不能修改参数的数量,如果修改请重新创建
	*					ptr及子项是由soap_malloc分配内存,因此无需释放
	*************************************************************************************/
	extern bool SEAPI SEPQ_params_create(struct SE_WORKINFO *arg, int32_t params_count, struct SEPQ_EXECPARAMS **ptr);

	/*************************************************************************************
	*	释放由SEPQ_EXECPARAMS使用的内存[可选择]
	*	Parameter:
	*		[in] struct SE_WORKINFO * arg - 请求处理参数
	*		[in] struct SEPQ_EXECPARAMS * * ptr - SEPQ_EXECPARAMS对象
	*	Remarks:SEPQ_EXECPARAMS由soap_malloc分配内存,因此可以无需释放,在本次请求处理完成后由系统自动释放
	*************************************************************************************/
	extern void SEAPI SEPQ_params_free(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS **ptr);

	/*************************************************************************************
	*	为PQexecParams参数数组指定的项设置值
	*	Parameter:
	*		[in] struct SE_WORKINFO * arg - 请求处理参数
	*		[in] struct SEPQ_EXECPARAM * params - PQexecParams参数数组
	*		[in] int32_t index - 要设置值的PQexecParams参数数组索引,从0开始
	*		[in] bin_data - 当前参数项的值,二进制
	*		[in] ibin_memlen- 当前参数项的值内存大小
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
	//日期时间和间隔只接受ISO 8601格式的字符串
	extern bool SEAPI SEPQ_params_add_timestamp(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, const char *bin_data);
	extern bool SEAPI SEPQ_params_add_timestamptz(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, const char *bin_data);
	extern bool SEAPI SEPQ_params_add_date(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, const char *bin_data);
	extern bool SEAPI SEPQ_params_add_time(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, const char *bin_data);
	extern bool SEAPI SEPQ_params_add_timetz(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, const char *bin_data);
	extern bool SEAPI SEPQ_params_add_interval(struct SE_WORKINFO *arg, struct SEPQ_EXECPARAMS *params, int32_t index, const char *bin_data);



	/*************************************************************************************
	*	为PQexecParams参数数组指定的项设置值-数组类型
	*	Parameter:
	*		[in] struct SE_WORKINFO * arg - 请求处理参数
	*		[in] struct SEPQ_EXECPARAMS * params - PQexecParams参数数组
	*		[in] int32_t index - 要设置值的PQexecParams参数数组索引,从0开始
	*		[in] const int16_t * values - 当前参数数组
	*		[in] int32_t array_size -  当前参数数组大小
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