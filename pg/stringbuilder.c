
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "stringbuilder.h"


#define Assert(condition)	((void)true)
#define AssertMacro(condition)	((void)true)

#define  USE_ASSERT_CHECKING

static int32_t pvsnprintf(char *buf, int32_t len, const char *fmt, va_list args) {
	int32_t nprinted;

	Assert(len > 0);
	errno = 0;
#ifdef USE_ASSERT_CHECKING
	buf[len - 1] = '\0';
#endif
	nprinted = vsnprintf(buf, len, fmt, args);
	Assert(buf[len - 1] == '\0');
	if (nprinted < 0 && errno != 0 && errno != ENOMEM) {
		fprintf(stderr, "vsnprintf failed: %s(%d)\n", strerror(errno), __LINE__);
		abort();
	}
	if (nprinted >= 0 && nprinted < len - 1) {
		return nprinted;
	}
	if (nprinted >= 0 && nprinted > len) {
		if (nprinted <= MaxAllocSize - 2)
			return nprinted + 2;
	}

	if (len >= MaxAllocSize) {
		fprintf(stderr, "out of memory(%d)", __LINE__);
		abort();
	}
	if (len >= MaxAllocSize / 2)
		return MaxAllocSize;
	return len * 2;
}

/*
* 重新调整内存大小
*/
static void enlargeStringBuilder(StringBuilder str, int32_t needed) {
	int32_t newlen;

	if (needed < 1 || needed >= (MaxAllocSize - str->len)) {		/*申请的内存大小超过了最大内容大小*/
		freeStringBuilder(&str);
		fprintf(stderr, "Cannot enlarge string buffer containing %d bytes by %d more bytes.", str->len, needed);
		abort();
	}
	needed += str->len + 1;
	if (needed <= str->maxlen)
		return;
	newlen = 2 * str->maxlen;
	while (needed > newlen)
		newlen = 2 * newlen;
	if (newlen > MaxAllocSize)
		newlen = MaxAllocSize;

	str->data = (char *)realloc(str->data, newlen);
	if (NULL == str->data) {
		fprintf(stderr, "out of memory(%d)", __LINE__);
		abort();
	}
	str->maxlen = newlen;
}


StringBuilder SEAPI makeStringBuilder(int32_t mem_size) {
	StringBuilder	str = NULL;
	int32_t init_size;
	str = (StringBuilder)calloc(1, sizeof(StringBuilderData));
	if (NULL == str) {
		fprintf(stderr, "out of memory(%d)", __LINE__);
		abort();
	}
	init_size = mem_size < 1024 || mem_size > MaxAllocSize ? 1024 : mem_size;	/* initial default buffer size */
	str->data = (char *)malloc(init_size);
	if (NULL == str->data) {
		fprintf(stderr, "out of memory(%d)", __LINE__);
		abort();
	}
	str->maxlen = init_size;
	resetStringBuilder(str);
	return str;
}

void SEAPI freeStringBuilder(StringBuilder *ptr) {
	StringBuilder str = *ptr;
	if (NULL != str) {
		if (NULL != str->data)
			free(str->data);
		free(str);
	}
	*ptr = NULL;
}

void SEAPI resetStringBuilder(StringBuilder str) {
	str->data[0] = '\0';
	str->len = 0;
	str->cursor = 0;
}

void SEAPI appendStringBuilder(StringBuilder str, const char *fmt, ...) {
	for (;;) {
		va_list		args;
		int32_t			needed;
		va_start(args, fmt);
		needed = appendStringBuilderVA(str, fmt, args);
		va_end(args);
		if (needed == 0)
			break;
		enlargeStringBuilder(str, needed);
	}
}


int32_t SEAPI appendStringBuilderVA(StringBuilder str, const char *fmt, va_list args) {
	int32_t avail, nprinted;

	Assert(str != NULL);
	avail = str->maxlen - str->len;
	if (avail < 16)
		return 32;
	nprinted = pvsnprintf(str->data + str->len, avail, fmt, args);
	if (nprinted < avail) {
		str->len += nprinted;
		return 0;
	}
	str->data[str->len] = '\0';
	return nprinted;
}


void SEAPI appendStringBuilderString(StringBuilder str, const char *s) {
	appendBinaryStringBuilder(str, s, strlen(s));
}



void SEAPI appendStringBuilderChar(StringBuilder str, char ch) {
	if (str->len + 1 >= str->maxlen)
		enlargeStringBuilder(str, 1);
	str->data[str->len] = ch;
	str->len++;
	str->data[str->len] = '\0';
}


void SEAPI appendStringBuilderCharNT(StringBuilder str, char ch) {
	if (str->len + 1 >= str->maxlen)
		enlargeStringBuilder(str, 1);
	str->data[str->len] = ch;
	str->len++;
}

void SEAPI appendStringBuilderSpaces(StringBuilder str, int32_t count) {
	if (count > 0) {
		enlargeStringBuilder(str, count);
		while (--count >= 0)
			str->data[str->len++] = ' ';
		str->data[str->len] = '\0';
	}
}


void SEAPI appendBinaryStringBuilder(StringBuilder str, const char *data, int32_t datalen) {
	Assert(str != NULL);
	enlargeStringBuilder(str, datalen);
	memcpy(str->data + str->len, data, datalen);
	str->len += datalen;
	str->data[str->len] = '\0';
}


void SEAPI appendBinaryStringBuilderNT(StringBuilder str, const char *data, int32_t datalen) {
	Assert(str != NULL);
	enlargeStringBuilder(str, datalen);
	memcpy(str->data + str->len, data, datalen);
	str->len += datalen;
}

void SEAPI appendBinaryStringBuilderJsonStr(StringBuilder str, const char *bin_data, int32_t bin_memlen) {
	int32_t index = 0;
	if (NULL == str || NULL == bin_data || 0 == bin_memlen)
		return;
	//一个"要变成\",多了一个字符,所以*2
	enlargeStringBuilder(str, bin_memlen * 2);

	while (index < bin_memlen) {
		if ('"' == bin_data[index]) {
			str->data[str->len++] = '\\';
			str->data[str->len++] = '"';			
		} else {
			str->data[str->len++] = bin_data[index];
		}
		++index;
	}
}