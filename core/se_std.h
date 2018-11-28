/*************************************************************************************
*	自定义标准头文件
*	整数使用C99中的stdint.h
*************************************************************************************/

#ifndef SE_FCF028B1_44FB_4B58_B6F6_C412E06A64C2
#define SE_FCF028B1_44FB_4B58_B6F6_C412E06A64C2


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#if defined(WIN32)
#include  <windows.h>
#include <direct.h>
#include <corecrt_io.h>
#else
#include<sys/time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include<unistd.h>
#endif
#include "se_typedef.h"
#include "../pg/base64.h"
#include "../pg/instr_time.h"
#include "../pg/stringbuilder.h"


/* copy string (truncating the result) */
#if _MSC_VER >= 1400
# define SE_strcpy(buf, len, src) (void)strncpy_s((buf), (len), (src), _TRUNCATE)
#elif defined(HAVE_STRLCPY)
# define SE_strcpy(buf, len, src) (void)strlcpy((buf), (src), (len))
#else
# define SE_strcpy(buf, len, src) (void)((buf) == NULL || (len) <= 0 || (strncpy((buf), (src), (len) - 1), (buf)[(len) - 1] = '\0') || 1)
#endif

/* copy string up to n chars (sets string to empty on overrun and returns nonzero, zero if OK) */
#if _MSC_VER >= 1400
# define SE_strncpy(buf, len, src, num) ((buf) == NULL || ((size_t)(len) > (size_t)(num) ? strncpy_s((buf), (len), (src), (num)) : ((buf)[0] = '\0', 1)))
#else
# define SE_strncpy(buf, len, src, num) ((buf) == NULL || ((size_t)(len) > (size_t)(num) ? (strncpy((buf), (src), (num)), (buf)[(size_t)(num)] = '\0') : ((buf)[0] = '\0', 1)))
#endif

/* concat string up to n chars (truncates on overrun and returns nonzero, zero if OK) */
#if _MSC_VER >= 1400
# define SE_strncat(buf, len, src, num) ((buf) == NULL || ((size_t)(len) > strlen((buf)) + (size_t)(num) ? strncat_s((buf), (len), (src), (num)) : 1))
#else
# define SE_strncat(buf, len, src, num) ((buf) == NULL || ((size_t)(len) > strlen((buf)) + (size_t)(num) ? (strncat((buf), (src), (num)), (buf)[(size_t)(len) - 1] = '\0') : 1))
#endif

/* copy memory (returns SOAP_ERANGE on overrun, zero if OK) */
#if _MSC_VER >= 1400
# define SE_memcpy(buf, len, src, num) ((buf) && (size_t)(len) >= (size_t)(num) ? memcpy_s((buf), (len), (src), (num)) : ERANGE)
#else
# define SE_memcpy(buf, len, src, num) ((buf) && (size_t)(len) >= (size_t)(num) ? !memcpy((buf), (src), (num)) : ERANGE)
#endif

/* move memory (returns SOAP_ERANGE on overrun, zero if OK) */
#if _MSC_VER >= 1400
# define SE_memmove(buf, len, src, num) ((buf) && (size_t)(len) >= (size_t)(num) ? memmove_s((buf), (len), (src), (num)) : ERANGE)
#else
# define SE_memmove(buf, len, src, num) ((buf) && (size_t)(len) >= (size_t)(num) ? !memmove((buf), (src), (num)) : ERANGE)
#endif


/*****************************************************************************
*	文件夹和文件访问权限及创建目录API
*****************************************************************************/
#if defined(WIN32)
#define se_access(_name,_mode) _access(_name,_mode)
#define se_mkdir(_name,_mode) _mkdir(_name)
#else
#define se_access(_name,_mode) access(_name,_mode)
#define se_mkdir(_name,_mode) mkdir(_name,_mode)
#define MAX_PATH	(256)
#endif



/*****************************************************************************
*	文件打开标志,保证可移植性
*	fopen时mod不要直接使用字符,它可能会在跨平台发生一些问题
*****************************************************************************/
#if defined(WIN32) || defined(__CYGWIN__)
#define PG_BINARY     O_BINARY
#define PG_BINARY_A "ab"			//追加,二进制
#define PG_BINARY_R "rb"			//只读,二进制
#define PG_BINARY_W "wb"		//只写,二进制
#else
#define PG_BINARY	 0
#define PG_BINARY_A "a"
#define PG_BINARY_R "r"
#define PG_BINARY_W "w"
#endif

/*UTF-8字符长度查找表*/
extern const unsigned char utf8_look_for_table[];

/*获取UTF8字符占用的内存大小（单位为字节）*/
#define UTF8LEN(x)  utf8_look_for_table[(x)]

/*获取ANIS字符占用的内存大小（单位为字节）*/
#define ANISLEN(x)  x < 0 ? 2 : 1

#ifdef __cplusplus
extern "C" {
#endif	/*__cplusplus 1*/

	extern void SEAPI SE_free(char **ptr_);

	/*************************************************************************************
	*	休眠指定的微秒.windows最多精确到毫秒
	*	Parameter:
	*		[in] long microsec - 微秒
	*************************************************************************************/
	extern void SEAPI SE_usleep(long microsec);
#define SLEEP_SEC (sec) (sec*1000000L)					//秒转换为微秒
#define SLEEP_MILLISEC(millisec) (millisec*1000L)		//毫秒转换为微秒


	/*************************************************************************************
	*	将运行时间转换为合适的时间单位
	*	Parameter:
	*		[in] double elapsed_msec - 运行时间毫秒
	*		[out] StringBuilder str - 合适的时间单位字符串
	*************************************************************************************/
	extern void SEAPI SE_elapsed2unit(double elapsed_msec, StringBuilder str);

	extern bool SEAPI SE_random_init();
	extern bool SEAPI SE_random(int64_t *rand_num);
	extern void SEAPI SE_random_free();
	

	/*************************************************************************************
	*	计算utf8字符数量.strlen返回的是内存字节数
	*		str:						[in]		UTF-8字符串
	*	返回值:
	*		-1表示无效的UTF-8字符串,否则返回UTF-8字符串的字符个数,英文和中文都算为1个字符
	*************************************************************************************/
	extern int32_t SEAPI SE_utf8_character_count(const uint8_t *bin_data, int32_t bin_len);


	/*************************************************************************************
	*	计算ANIS字符数量.strlen返回的是内存字节数
	*		str:						[in]		ANIS字符串
	*	返回值:
	*		ANIS字符串的字符个数
	*		ASCII值0-127的算1个字符,其它都算为2个字符
	*************************************************************************************/
	extern int32_t SEAPI SE_anis_character_count(const char *bin_data, int32_t bin_len);

	/*************************************************************************************
	*	复制字符串
	*	Parameter:
	*		[in] const char * source - 要复制的字符串
	*		[out] string * * target - 复制后的字符串,无需分配内存,使用完成后用SE_free释放
	*	Returns:true表示成功,false表示失败
	*************************************************************************************/
	extern bool SEAPI SE_string_copy(const char *source, char **target);


	/*************************************************************************************
	*	创建目录
	*	目录存在则跳过,目录不存在才创建
	*	windows和linux均可使用
	*	Parameter:
	*		[in] const char * path	完整目录名称
	*		[in] int mode -				访问权限(0777),这个参数只有linux才有效,默认使用0744
	*	Returns:true表示成功,false表示失败
	*************************************************************************************/
	extern bool SEAPI SE_mkdirex(const char *path, int32_t mode);

	/*************************************************************************************
	*	base64 编码
	*	Parameter:
	*		[in] const char **data - 要编码的字符串
	*		[out] char **ptr - base64编码后的字符串.使用完成后调用SE_free释放.
	*	Returns:true表示成功,false表示失败
	*************************************************************************************/
	extern bool SEAPI SE_base64_encrypt(const char *data, int32_t data_len,char **base64,int32_t *b64_len);

	/*************************************************************************************
	*	base64 解码.使用完成后调用SE_free释放data.
	*	Parameter:
	*		[in] const char *base64 - base64编码字符串
	*		[out] char **data - 解码后的字符串.使用完成后调用SE_free释放.
	*	Returns:true表示成功,false表示失败
	*************************************************************************************/
	extern bool SEAPI SE_base64_decrypt(const char *base64, int32_t b64_len, char **data, int32_t *data_len);


	/*************************************************************************************
	*	AES加密字符串
	*	Parameter:
	*		[in] const uint8_t  *aes_key - AES密钥,设置为NULL时使用系统默认值
	*		[in] const uint8_t  *aes_iv - AES向量,设置为NULL时使用系统默认值
	*		[in] const char * source - 待加密的字符串,注意source必须包含字符结束标志\0
	*		[out] char **b64_encrypt - 加密后的字符串(base64编码),使用完成后用SE_free释放资源
	*	Returns:true表示成功,false表示失败
	*	Remake:aes_key和aes_iv必须同时设置才有效,否则使用系统默认值
	*************************************************************************************/
	extern bool SEAPI SE_aes_encrypt(const uint8_t  *aes_key, const uint8_t  *aes_iv, const char *source, char **b64_encrypt);

	/*************************************************************************************
	*	AES解密字符串
	*	Parameter:
	*		[in] const uint8_t  *aes_key - AES密钥,设置为NULL时使用系统默认值
	*		[in] const uint8_t  *aes_iv - AES向量,设置为NULL时使用系统默认值
	*		[in] const char *b64_encrypt - 待解密的字符串(base64编码)
	*		[in] char **data - 解密后的字符串),使用完成后用SE_free释放资源
	*	Returns:true表示成功,false表示失败
	*	Remake:aes_key和aes_iv必须同时设置才有效,否则使用系统默认值
	*************************************************************************************/
	extern bool SEAPI SE_aes_decrypt(const uint8_t  *aes_key, const uint8_t  *aes_iv, const char *b64_encrypt, char **data);

#ifdef __cplusplus
};
#endif  /*__cplusplus 2*/

#endif	/* SE_FCF028B1_44FB_4B58_B6F6_C412E06A64C2 */