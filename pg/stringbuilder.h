
/*-------------------------------------------------------------------------
 *
 *	stringbuilder.h
 *	StringBuilder根据PostgreSQL源码中的StringBuilder修改而来(src/include/lib/StringBuilder.h	/src/backend/lib/StringBuilder.c)
 *	PostgreSQL有太多值的我们深挖掌握的东西.学C的朋友可以学习一下,基本涵盖了所有C相关的知识
 *	StringBuilder最多允许1GB大小的字符串,初始时内存大小为1024,重新分配时以当前内存大小*2的大小分配内存
 *	StringBuilder只有vsnprintf运行失败、字符串大小超过MaxAllocSize
 *	当发生上述异常情况时,自动退出当前进程(闪退),可能会发生内存泄露.退出时将异常信息写入到当前程序目录的log中
 *	对于现代的计算机来说内存较大,因此malloc失败几率比较低,因此为方便使用StringBuilder中的所有函数均无返回的异常代码
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
* StringBuilder最多允许1GB大小的字符串
*************************************************************************************/
#define MaxAllocSize	((int32_t) 0x3fffffff) /* 1 gigabyte - 1 */

/*************************************************************************************
*	StringBuilder数据结构和指针
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
	*	创建StringBuilder对象
	*	使用完成后必须使用freeStringBuilder释放
	*************************************************************************************/
	extern StringBuilder SEAPI makeStringBuilder(int32_t mem_size);

	/*************************************************************************************
	*	释放StringBuilder对象
	*************************************************************************************/
	extern void SEAPI freeStringBuilder(StringBuilder *str);

	/*************************************************************************************
	*	清除StringBuilder中的内容
	*************************************************************************************/
	extern void SEAPI resetStringBuilder(StringBuilder str);

	/*************************************************************************************
	*	以格式化的方式添加字符串
	*************************************************************************************/
	extern void SEAPI appendStringBuilder(StringBuilder str, const char *fmt, ...) SE_ATTRIBUTE_PRINTF(2, 3);

	/*************************************************************************************
	*	计算格式化后的字符串大小
	*************************************************************************************/
	extern int32_t	SEAPI appendStringBuilderVA(StringBuilder str, const char *fmt, va_list args) SE_ATTRIBUTE_PRINTF(2, 0);

	/*************************************************************************************
	*	添加字符串
	*************************************************************************************/
	extern void SEAPI appendStringBuilderString(StringBuilder str, const char *s);

	/*************************************************************************************
	*	添加字符
	*************************************************************************************/
	extern void SEAPI appendStringBuilderChar(StringBuilder str, char ch);
	extern void SEAPI appendStringBuilderCharNT(StringBuilder str, char ch);

	/*************************************************************************************
	*	添加字符
	*************************************************************************************/
#define appendStringBuilderCharMacro(str,ch) \
	(((str)->len + 1 >= (str)->maxlen) ? \
	 appendStringBuilderChar(str, ch) : \
	 (void)((str)->data[(str)->len] = (ch), (str)->data[++(str)->len] = '\0'))

	/*************************************************************************************
	*	添加空格,count参数指明空格的数量
	*************************************************************************************/
	extern void SEAPI appendStringBuilderSpaces(StringBuilder str, int32_t count);

	/*************************************************************************************
	*	添加字符串并且添加字符串结束标记\0
	*************************************************************************************/
	extern void SEAPI appendBinaryStringBuilder(StringBuilder str, const char *data, int32_t datalen);

	/*************************************************************************************
	*	添加字符串,但是不添加字符串结束标记\0
	*************************************************************************************/
	extern void SEAPI appendBinaryStringBuilderNT(StringBuilder str, const char *data, int32_t datalen);

	extern void SEAPI appendBinaryStringBuilderJsonStr(StringBuilder sb, const char * data, int32_t datalen);
	
#ifdef __cplusplus
};
#endif  /*__cplusplus 2*/

#endif							/* SE_F59E2B51_AB5B_4414_972B_3AE4E0FEC526 */
