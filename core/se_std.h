/*************************************************************************************
*	�Զ����׼ͷ�ļ�
*	����ʹ��C99�е�stdint.h
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
*	�ļ��к��ļ�����Ȩ�޼�����Ŀ¼API
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
*	�ļ��򿪱�־,��֤����ֲ��
*	fopenʱmod��Ҫֱ��ʹ���ַ�,�����ܻ��ڿ�ƽ̨����һЩ����
*****************************************************************************/
#if defined(WIN32) || defined(__CYGWIN__)
#define PG_BINARY     O_BINARY
#define PG_BINARY_A "ab"			//׷��,������
#define PG_BINARY_R "rb"			//ֻ��,������
#define PG_BINARY_W "wb"		//ֻд,������
#else
#define PG_BINARY	 0
#define PG_BINARY_A "a"
#define PG_BINARY_R "r"
#define PG_BINARY_W "w"
#endif

/*UTF-8�ַ����Ȳ��ұ�*/
extern const unsigned char utf8_look_for_table[];

/*��ȡUTF8�ַ�ռ�õ��ڴ��С����λΪ�ֽڣ�*/
#define UTF8LEN(x)  utf8_look_for_table[(x)]

/*��ȡANIS�ַ�ռ�õ��ڴ��С����λΪ�ֽڣ�*/
#define ANISLEN(x)  x < 0 ? 2 : 1

#ifdef __cplusplus
extern "C" {
#endif	/*__cplusplus 1*/

	extern void SEAPI SE_free(char **ptr_);

	/*************************************************************************************
	*	����ָ����΢��.windows��ྫȷ������
	*	Parameter:
	*		[in] long microsec - ΢��
	*************************************************************************************/
	extern void SEAPI SE_usleep(long microsec);
#define SLEEP_SEC (sec) (sec*1000000L)					//��ת��Ϊ΢��
#define SLEEP_MILLISEC(millisec) (millisec*1000L)		//����ת��Ϊ΢��


	/*************************************************************************************
	*	������ʱ��ת��Ϊ���ʵ�ʱ�䵥λ
	*	Parameter:
	*		[in] double elapsed_msec - ����ʱ�����
	*		[out] StringBuilder str - ���ʵ�ʱ�䵥λ�ַ���
	*************************************************************************************/
	extern void SEAPI SE_elapsed2unit(double elapsed_msec, StringBuilder str);

	extern bool SEAPI SE_random_init();
	extern bool SEAPI SE_random(int64_t *rand_num);
	extern void SEAPI SE_random_free();
	

	/*************************************************************************************
	*	����utf8�ַ�����.strlen���ص����ڴ��ֽ���
	*		str:						[in]		UTF-8�ַ���
	*	����ֵ:
	*		-1��ʾ��Ч��UTF-8�ַ���,���򷵻�UTF-8�ַ������ַ�����,Ӣ�ĺ����Ķ���Ϊ1���ַ�
	*************************************************************************************/
	extern int32_t SEAPI SE_utf8_character_count(const uint8_t *bin_data, int32_t bin_len);


	/*************************************************************************************
	*	����ANIS�ַ�����.strlen���ص����ڴ��ֽ���
	*		str:						[in]		ANIS�ַ���
	*	����ֵ:
	*		ANIS�ַ������ַ�����
	*		ASCIIֵ0-127����1���ַ�,��������Ϊ2���ַ�
	*************************************************************************************/
	extern int32_t SEAPI SE_anis_character_count(const char *bin_data, int32_t bin_len);

	/*************************************************************************************
	*	�����ַ���
	*	Parameter:
	*		[in] const char * source - Ҫ���Ƶ��ַ���
	*		[out] string * * target - ���ƺ���ַ���,��������ڴ�,ʹ����ɺ���SE_free�ͷ�
	*	Returns:true��ʾ�ɹ�,false��ʾʧ��
	*************************************************************************************/
	extern bool SEAPI SE_string_copy(const char *source, char **target);


	/*************************************************************************************
	*	����Ŀ¼
	*	Ŀ¼����������,Ŀ¼�����ڲŴ���
	*	windows��linux����ʹ��
	*	Parameter:
	*		[in] const char * path	����Ŀ¼����
	*		[in] int mode -				����Ȩ��(0777),�������ֻ��linux����Ч,Ĭ��ʹ��0744
	*	Returns:true��ʾ�ɹ�,false��ʾʧ��
	*************************************************************************************/
	extern bool SEAPI SE_mkdirex(const char *path, int32_t mode);

	/*************************************************************************************
	*	base64 ����
	*	Parameter:
	*		[in] const char **data - Ҫ������ַ���
	*		[out] char **ptr - base64�������ַ���.ʹ����ɺ����SE_free�ͷ�.
	*	Returns:true��ʾ�ɹ�,false��ʾʧ��
	*************************************************************************************/
	extern bool SEAPI SE_base64_encrypt(const char *data, int32_t data_len,char **base64,int32_t *b64_len);

	/*************************************************************************************
	*	base64 ����.ʹ����ɺ����SE_free�ͷ�data.
	*	Parameter:
	*		[in] const char *base64 - base64�����ַ���
	*		[out] char **data - �������ַ���.ʹ����ɺ����SE_free�ͷ�.
	*	Returns:true��ʾ�ɹ�,false��ʾʧ��
	*************************************************************************************/
	extern bool SEAPI SE_base64_decrypt(const char *base64, int32_t b64_len, char **data, int32_t *data_len);


	/*************************************************************************************
	*	AES�����ַ���
	*	Parameter:
	*		[in] const uint8_t  *aes_key - AES��Կ,����ΪNULLʱʹ��ϵͳĬ��ֵ
	*		[in] const uint8_t  *aes_iv - AES����,����ΪNULLʱʹ��ϵͳĬ��ֵ
	*		[in] const char * source - �����ܵ��ַ���,ע��source��������ַ�������־\0
	*		[out] char **b64_encrypt - ���ܺ���ַ���(base64����),ʹ����ɺ���SE_free�ͷ���Դ
	*	Returns:true��ʾ�ɹ�,false��ʾʧ��
	*	Remake:aes_key��aes_iv����ͬʱ���ò���Ч,����ʹ��ϵͳĬ��ֵ
	*************************************************************************************/
	extern bool SEAPI SE_aes_encrypt(const uint8_t  *aes_key, const uint8_t  *aes_iv, const char *source, char **b64_encrypt);

	/*************************************************************************************
	*	AES�����ַ���
	*	Parameter:
	*		[in] const uint8_t  *aes_key - AES��Կ,����ΪNULLʱʹ��ϵͳĬ��ֵ
	*		[in] const uint8_t  *aes_iv - AES����,����ΪNULLʱʹ��ϵͳĬ��ֵ
	*		[in] const char *b64_encrypt - �����ܵ��ַ���(base64����)
	*		[in] char **data - ���ܺ���ַ���),ʹ����ɺ���SE_free�ͷ���Դ
	*	Returns:true��ʾ�ɹ�,false��ʾʧ��
	*	Remake:aes_key��aes_iv����ͬʱ���ò���Ч,����ʹ��ϵͳĬ��ֵ
	*************************************************************************************/
	extern bool SEAPI SE_aes_decrypt(const uint8_t  *aes_key, const uint8_t  *aes_iv, const char *b64_encrypt, char **data);

#ifdef __cplusplus
};
#endif  /*__cplusplus 2*/

#endif	/* SE_FCF028B1_44FB_4B58_B6F6_C412E06A64C2 */