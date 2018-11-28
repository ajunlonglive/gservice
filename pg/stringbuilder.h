
/*-------------------------------------------------------------------------
 *
 *	stringbuilder.h
 *	StringBuilder����PostgreSQLԴ���е�StringBuilder�޸Ķ���(src/include/lib/StringBuilder.h	/src/backend/lib/StringBuilder.c)
 *	PostgreSQL��̫��ֵ�������������յĶ���.ѧC�����ѿ���ѧϰһ��,��������������C��ص�֪ʶ
 *	StringBuilder�������1GB��С���ַ���,��ʼʱ�ڴ��СΪ1024,���·���ʱ�Ե�ǰ�ڴ��С*2�Ĵ�С�����ڴ�
 *	StringBuilderֻ��vsnprintf����ʧ�ܡ��ַ�����С����MaxAllocSize
 *	�����������쳣���ʱ,�Զ��˳���ǰ����(����),���ܻᷢ���ڴ�й¶.�˳�ʱ���쳣��Ϣд�뵽��ǰ����Ŀ¼��log��
 *	�����ִ��ļ������˵�ڴ�ϴ�,���mallocʧ�ܼ��ʱȽϵ�,���Ϊ����ʹ��StringBuilder�е����к������޷��ص��쳣����
 *-------------------------------------------------------------------------*/

#ifndef SE_F59E2B51_AB5B_4414_972B_3AE4E0FEC526
#define SE_F59E2B51_AB5B_4414_972B_3AE4E0FEC526
#include <stdarg.h>
#include "../core/se_typedef.h"

 /*************************************************************************************
 *	GCC and XLC support format attributes
*************************************************************************************/
#if defined(__GNUC__) || defined(__IBMC__)
#define SE_ATTRIBUTE_FORMAT_ARG(a) __attribute__((format_arg(a)))
#define SE_ATTRIBUTE_PRINTF(f,a) __attribute__((format(SE_PRINTF_ATTRIBUTE, f, a)))
#define SE_PRINTF_ATTRIBUTE gnu_printf
#else
#define SE_ATTRIBUTE_FORMAT_ARG(a)
#define SE_ATTRIBUTE_PRINTF(f,a)
#define SE_PRINTF_ATTRIBUTE printf
#endif

/*************************************************************************************
* StringBuilder�������1GB��С���ַ���
*************************************************************************************/
#define MaxAllocSize	((int32_t) 0x3fffffff) /* 1 gigabyte - 1 */

/*************************************************************************************
*	StringBuilder���ݽṹ��ָ��
*************************************************************************************/
typedef struct StringBuilderData {
	char *data;
	int32_t	len;
	int32_t	maxlen;
	int32_t	cursor;
} StringBuilderData;
typedef StringBuilderData *StringBuilder;

#ifdef __cplusplus
extern "C" {
#endif	/*__cplusplus 1*/

	/*************************************************************************************
	*	����StringBuilder����
	*	ʹ����ɺ����ʹ��freeStringBuilder�ͷ�
	*************************************************************************************/
	extern StringBuilder SEAPI makeStringBuilder(int32_t mem_size);

	/*************************************************************************************
	*	�ͷ�StringBuilder����
	*************************************************************************************/
	extern void SEAPI freeStringBuilder(StringBuilder *str);

	/*************************************************************************************
	*	���StringBuilder�е�����
	*************************************************************************************/
	extern void SEAPI resetStringBuilder(StringBuilder str);

	/*************************************************************************************
	*	�Ը�ʽ���ķ�ʽ����ַ���
	*************************************************************************************/
	extern void SEAPI appendStringBuilder(StringBuilder str, const char *fmt, ...) SE_ATTRIBUTE_PRINTF(2, 3);

	/*************************************************************************************
	*	�����ʽ������ַ�����С
	*************************************************************************************/
	extern int32_t	SEAPI appendStringBuilderVA(StringBuilder str, const char *fmt, va_list args) SE_ATTRIBUTE_PRINTF(2, 0);

	/*************************************************************************************
	*	����ַ���
	*************************************************************************************/
	extern void SEAPI appendStringBuilderString(StringBuilder str, const char *s);

	/*************************************************************************************
	*	����ַ�
	*************************************************************************************/
	extern void SEAPI appendStringBuilderChar(StringBuilder str, char ch);
	extern void SEAPI appendStringBuilderCharNT(StringBuilder str, char ch);

	/*************************************************************************************
	*	����ַ�
	*************************************************************************************/
#define appendStringBuilderCharMacro(str,ch) \
	(((str)->len + 1 >= (str)->maxlen) ? \
	 appendStringBuilderChar(str, ch) : \
	 (void)((str)->data[(str)->len] = (ch), (str)->data[++(str)->len] = '\0'))

	/*************************************************************************************
	*	��ӿո�,count����ָ���ո������
	*************************************************************************************/
	extern void SEAPI appendStringBuilderSpaces(StringBuilder str, int32_t count);

	/*************************************************************************************
	*	����ַ�����������ַ����������\0
	*************************************************************************************/
	extern void SEAPI appendBinaryStringBuilder(StringBuilder str, const char *data, int32_t datalen);

	/*************************************************************************************
	*	����ַ���,���ǲ�����ַ����������\0
	*************************************************************************************/
	extern void SEAPI appendBinaryStringBuilderNT(StringBuilder str, const char *data, int32_t datalen);

	extern void SEAPI appendBinaryStringBuilderJsonStr(StringBuilder sb, const char * data, int32_t datalen);
	
#ifdef __cplusplus
};
#endif  /*__cplusplus 2*/

#endif							/* SE_F59E2B51_AB5B_4414_972B_3AE4E0FEC526 */
