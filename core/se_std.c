#include <math.h>
#if defined(_MSC_VER)
#include <windows.h>
#include <bcrypt.h>
#endif
#include "../github/aes.h"
#include "se_std.h"
#include "se_errno.h"

/*UTF-8字符串长度对照表*/
const unsigned char utf8_look_for_table[] = {
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 1, 1
};

/*获取UTF8字符占用的内存大小（单位为字节）*/
#define UTF8LEN(x)  utf8_look_for_table[(x)]

///*获取ANIS字符占用的内存大小（单位为字节）*/
//#define ANISLEN(x)  x < 0 ? 2 : 1
/*
*	AES算法向量和密钥
*/
static const uint8_t KEY[16] = { (uint8_t)0x00,(uint8_t)0x08, (uint8_t)0x07, (uint8_t)0x01, (uint8_t)0x06, (uint8_t)0x08, (uint8_t)0x03, (uint8_t)0x03,
(uint8_t)0x07, (uint8_t)0x08, (uint8_t)0x02, (uint8_t)0x02, (uint8_t)0x00, (uint8_t)0x00, (uint8_t)0x00, (uint8_t)0x00 };
static const uint8_t IV[16] = { (uint8_t)0x00, (uint8_t)0x00, (uint8_t)0x00, (uint8_t)0x00 ,(uint8_t)0x00,(uint8_t)0x08, (uint8_t)0x07,(uint8_t)0x01,
(uint8_t)0x06, (uint8_t)0x08, (uint8_t)0x03, (uint8_t)0x03,(uint8_t)0x07, (uint8_t)0x08, (uint8_t)0x02, (uint8_t)0x02 };

void SEAPI SE_free(char **ptr_) {
	void *p = *ptr_;
	if (!SE_PTR_ISNULL(p))
		free(p);
	ptr_ = NULL;
}

void SEAPI SE_usleep(long microsec) {
	if (microsec > 0) {
#ifndef WIN32
		struct timeval delay;
		delay.tv_sec = microsec / 1000000L;
		delay.tv_usec = microsec % 1000000L;
		(void)select(0, NULL, NULL, NULL, &delay);
#else
		SleepEx((microsec < 500 ? 1 : (microsec + 500) / 1000), FALSE);
#endif
	}
}

void SEAPI SE_elapsed2unit(double elapsed_msec, StringBuilder str) {
	double seconds;
	double minutes;
	double hours;
	double days;


	resetStringBuilder(str);
	if (elapsed_msec < 1000.0) {
		/* This is the traditional (pre-v10) output format */
		appendStringBuilder(str, "%.3f ms", elapsed_msec);
		return;
	}

	/*
	* Note: we could print just seconds, in a format like %06.3f, when the
	* total is less than 1min.  But that's hard to interpret unless we tack
	* on "s" or otherwise annotate it.  Forcing the display to include
	* minutes seems like a better solution.
	*/
	seconds = elapsed_msec / 1000.0;
	minutes = floor(seconds / 60.0);
	seconds -= 60.0 * minutes;
	if (minutes < 60.0) {
		appendStringBuilder(str, "%.3f ms (%02d:%06.3f)", elapsed_msec, (int32_t)minutes, seconds);
		return;
	}

	hours = floor(minutes / 60.0);
	minutes -= 60.0 * hours;
	if (hours < 24.0) {
		appendStringBuilder(str, "%.3f ms (%02d:%02d:%06.3f)", elapsed_msec, (int32_t)hours, (int32_t)minutes, seconds);
		return;
	}

	days = floor(hours / 24.0);
	hours -= 24.0 * days;
	appendStringBuilder(str, "%.3f ms (%.0f d %02d:%02d:%06.3f)", elapsed_msec, days, (int32_t)hours, (int32_t)minutes, seconds);
}

#if defined(WIN32)
static BCRYPT_ALG_HANDLE g_hAlgorithm = NULL;

bool SEAPI SE_random_init() {
	if (!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&g_hAlgorithm, BCRYPT_RNG_ALGORITHM, NULL, 0))) {
		fprintf(stderr, "BCryptOpenAlgorithmProvider fail.\n");
		return false;
	}
	return true;
}

void SEAPI SE_random_free() {
	if (!SE_PTR_ISNULL(g_hAlgorithm)) {
		BCryptCloseAlgorithmProvider(g_hAlgorithm, 0);
		g_hAlgorithm = NULL;
	}
}

bool SEAPI SE_random(int64_t *rand_num) {
	//BCRYPT_ALG_HANDLE hAlgorithm = NULL;
	int64_t buffer;

	if (SE_PTR_ISNULL(g_hAlgorithm)) {
		fprintf(stderr, "random not initialized.\n");
		return false;
	}

	/*if (!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&hAlgorithm, BCRYPT_RNG_ALGORITHM, NULL, 0))) {
		fprintf(stderr, "BCryptOpenAlgorithmProvider fail.\n");
		goto SE_ERROR_CLEAR;
	}*/
	if (!BCRYPT_SUCCESS(BCryptGenRandom(g_hAlgorithm, (PUCHAR)(&buffer), sizeof(uint64_t), 0))) {
		fprintf(stderr, "BCryptGenRandom fail.\n");
		goto SE_ERROR_CLEAR;
	}
	//BCryptCloseAlgorithmProvider(hAlgorithm, 0);
	*rand_num = buffer;
	return true;
SE_ERROR_CLEAR:
	/*if (!SE_PTR_ISNULL(hAlgorithm)) {
		BCryptCloseAlgorithmProvider(hAlgorithm, 0);
		hAlgorithm = NULL;
	}*/
	return false;
}
#else
static struct timespec *g_timespec = NULL;

bool SEAPI SE_random_init() {
	g_timespec = (struct timespec *)malloc(sizeof(struct timespec));
	if (SE_PTR_ISNULL(g_timespec)) {
		fprintf(stderr, "Out of memory.\n");
		return false;
	}
	return true;
}

void SEAPI SE_random_free() {
	if (!SE_PTR_ISNULL(g_timespec)) {
		free(g_timespec);
		g_timespec = NULL;
	}
}

bool SEAPI SE_random(int64_t *rand_num) {
	//struct timespec ts;
	if (SE_PTR_ISNULL(g_timespec)) {
		fprintf(stderr, "random not initialized.\n");
		return false;
	}
	if (0 == timespec_get(g_timespec, TIME_UTC)) {
		fprintf(stderr, "timespec_get fail.\n");
		goto SE_ERROR_CLEAR;
	}
	srandom(g_timespec->tv_nsec ^ g_timespec->tv_sec); /* Seed the PRNG */
	*rand_num = random(); /* Generate a random integer */
	return true;
SE_ERROR_CLEAR:
	return false;
}
#endif

//src/backend/utils/mb/wchar.c
/*
* Return the byte length of a UTF8 character pointed to by s
*
* Note: in the current implementation we do not support UTF8 sequences
* of more than 4 bytes; hence do NOT return a value larger than 4.
* We return "1" for any leading byte that is either flat-out illegal or
* indicates a length larger than we support.
*
* pg_utf2wchar_with_len(), utf8_to_unicode(), pg_utf8_islegal(), and perhaps
* other places would need to be fixed to change this.
*/
static int32_t pg_utf_mblen(const unsigned char *s) {
	int32_t			len;

	if ((*s & 0x80) == 0)
		len = 1;
	else if ((*s & 0xe0) == 0xc0)
		len = 2;
	else if ((*s & 0xf0) == 0xe0)
		len = 3;
	else if ((*s & 0xf8) == 0xf0)
		len = 4;
#ifdef NOT_USED
	else if ((*s & 0xfc) == 0xf8)
		len = 5;
	else if ((*s & 0xfe) == 0xfc)
		len = 6;
#endif
	else
		len = 1;
	return len;
}

/*
* Check for validity of a single UTF-8 encoded character
*
* This directly implements the rules in RFC3629.  The bizarre-looking
* restrictions on the second byte are meant to ensure that there isn't
* more than one encoding of a given Unicode character point; that is,
* you may not use a longer-than-necessary byte sequence with high order
* zero bits to represent a character that would fit in fewer bytes.
* To do otherwise is to create security hazards (eg, create an apparent
* non-ASCII character that decodes to plain ASCII).
*
* length is assumed to have been obtained by pg_utf_mblen(), and the
* caller must have checked that that many bytes are present in the buffer.
*/
static bool pg_utf8_islegal(const unsigned char *source, int32_t length) {
	unsigned char a;

	switch (length) {
	default:
		/* reject lengths 5 and 6 for now */
		return false;
	case 4:
		a = source[3];
		if (a < 0x80 || a > 0xBF)
			return false;
		/* FALL THRU */
	case 3:
		a = source[2];
		if (a < 0x80 || a > 0xBF)
			return false;
		/* FALL THRU */
	case 2:
		a = source[1];
		switch (*source) {
		case 0xE0:
			if (a < 0xA0 || a > 0xBF)
				return false;
			break;
		case 0xED:
			if (a < 0x80 || a > 0x9F)
				return false;
			break;
		case 0xF0:
			if (a < 0x90 || a > 0xBF)
				return false;
			break;
		case 0xF4:
			if (a < 0x80 || a > 0x8F)
				return false;
			break;
		default:
			if (a < 0x80 || a > 0xBF)
				return false;
			break;
		}
		/* FALL THRU */
	case 1:
		a = *source;
		if (a >= 0x80 && a < 0xC2)
			return false;
		if (a > 0xF4)
			return false;
		break;
	}
	return true;
}

static int32_t SE_utf8_verifier(const unsigned char *s, int32_t len) {
	int32_t			l = pg_utf_mblen(s);
	if (len < l)
		return -1;
	if (!pg_utf8_islegal(s, l))
		return -1;
	return l;
}

int32_t SEAPI SE_utf8_character_count(const uint8_t *bin_data, int32_t bin_len) {
	int32_t character_count = 0, character_bin_len = 0;
	int32_t i, verifier;
	if (SE_PTR_ISNULL(bin_data) || bin_len < 1)
		return 0;
	for (i = 0; i < bin_len;) {
		character_bin_len = UTF8LEN(bin_data[i]);
		verifier = SE_utf8_verifier(&(bin_data[i]), character_bin_len);
		if (-1 == verifier)
			return  -1;
		i += character_bin_len;
		++character_count;
	}
	return character_count;
}


int32_t SEAPI SE_anis_character_count(const char *bin_data, int32_t bin_len) {
	int32_t count = 0, i;
	if (SE_PTR_ISNULL(bin_data) || bin_len < 1)
		return 0;
	for (i = 0; i < bin_len; ++i) {
		if (bin_data[i] < 0)
			++i;
		++count;
	}
	return count;
}

bool SEAPI SE_string_copy(const char *source, char **ptr) {
	char *str = NULL;
	int32_t src_len;
	SE_validate_para_ptr(source, SE_2STR(SE_string_copy), __LINE__);
	src_len = strlen(source);
	SE_validate_para_zero(src_len, SE_2STR(SE_string_copy), __LINE__);
	++src_len;

	SE_check_nullptr((str = (char *)malloc(src_len)), "%s(%d),%s", SE_2STR(calloc), __LINE__, strerror(ENOMEM));
	SE_memcpy(str, src_len, source, src_len);
	(*ptr) = str;
	return true;
SE_ERROR_CLEAR:
	SE_free(&str);
	return false;
}

bool SEAPI SE_mkdirex(const char *path, int32_t mode) {
	int32_t rc = 0, errcode = 0;

	SE_validate_para_ptr(path, SE_2STR(SE_mkdirex), __LINE__);

	rc = se_access(path, 0);
	errcode = errno;
	if (0 != rc) {  //访问目录发生错误	
		if (ENOENT == errcode) { //没有那个目录
			rc = se_mkdir(path, mode);
			if (0 != rc) {//创建目录发生错误				
				errcode = errno;
				fprintf(stderr, "%d:%s\n", errcode, strerror(errcode));
				return false;
			} else {
				return true;
			}
		} else {
			fprintf(stderr, "%d:%s\n", errcode, strerror(errcode));
			return false;
		}
	} else {
		return true;
	}
SE_ERROR_CLEAR:
	return false;
}

bool SEAPI SE_base64_encrypt(const char *data, int32_t data_len, char **ptr, int32_t *base64_len) {
	char *encrypt = NULL;
	int32_t rc = 0, b64_len = 0;

	SE_validate_para_ptr(data, SE_2STR(SE_base64_encrypt), __LINE__);
	SE_validate_para_zero(data_len, SE_2STR(SE_base64_encrypt), __LINE__);

	b64_len = pg_b64_enc_len(data_len);
	SE_check_nullptr((encrypt = (char *)malloc(b64_len + 1)), "%s(%d),%s", SE_2STR(calloc), __LINE__, strerror(ENOMEM));
	rc = pg_b64_encode(data, data_len, encrypt);
	if (-1 == rc)
		goto SE_ERROR_CLEAR;
	encrypt[rc] = 0;
	*base64_len = rc;
	*ptr = encrypt;
	return true;
SE_ERROR_CLEAR:
	SE_free(&encrypt);
	(*ptr) = NULL;
	return false;
}

bool SEAPI SE_base64_decrypt(const char *base64, int32_t b64_len, char **ptr, int32_t *data_len) {
	char *str = NULL;
	int32_t rc = 0, src_len = 0;

	SE_validate_para_ptr(base64, SE_2STR(SE_base64_decrypt), __LINE__);
	SE_validate_para_zero(b64_len, SE_2STR(SE_base64_decrypt), __LINE__);

	src_len = pg_b64_dec_len(b64_len);
	SE_check_nullptr((str = (char *)malloc(src_len + 1)), "%s(%d),%s", SE_2STR(malloc), __LINE__, strerror(ENOMEM));
	rc = pg_b64_decode(base64, b64_len, str);
	if (-1 == rc)
		goto SE_ERROR_CLEAR;
	str[rc] = 0;
	*data_len = rc;
	(*ptr) = str;
	return true;
SE_ERROR_CLEAR:
	SE_free(&str);
	(*ptr) = NULL;
	return false;
}

bool SEAPI SE_aes_encrypt(const uint8_t  *aes_key, const uint8_t  *aes_iv, const char *source, char **ptr) {
	char *base64 = NULL, *bin_encrypt = NULL;
	int32_t src_len, encrypt_len = 0;
	struct AES_ctx ctx;
	char *ptrtmp;

	SE_validate_para_ptr(source, SE_2STR(SE_aes_encrypt), __LINE__);
	src_len = strlen(source);
	SE_validate_para_zero(src_len, SE_2STR(SE_aes_encrypt), __LINE__);
	++src_len;
	//计算补位后的字节长度,asc数据大小要求16的倍数
	encrypt_len = (src_len / AES_BLOCKLEN * AES_BLOCKLEN) + (0 == (src_len % AES_BLOCKLEN) ? 0 : AES_BLOCKLEN);
	SE_check_nullptr((bin_encrypt = (char *)malloc(encrypt_len)), "%s(%d),%s", SE_2STR(malloc), __LINE__, strerror(ENOMEM));
	SE_memcpy(bin_encrypt, encrypt_len, source, src_len);
	//补位的字节设置为0	
	if ((encrypt_len - src_len) > 0) {
		ptrtmp = bin_encrypt + src_len;
		memset(ptrtmp, 0, encrypt_len - src_len);
	}
	//初始化密钥和向量后开始加密
	if (NULL != aes_key && NULL != aes_iv)
		AES_init_ctx_iv(&ctx, aes_key, aes_iv);
	else
		AES_init_ctx_iv(&ctx, KEY, IV);
	AES_CBC_encrypt_buffer(&ctx, (uint8_t *)bin_encrypt, encrypt_len);
	//转换为base64编码
	SE_throw(SE_base64_encrypt(bin_encrypt, encrypt_len, &base64, &encrypt_len));
	SE_free(&bin_encrypt);
	*ptr = base64;
	return true;
SE_ERROR_CLEAR:
	SE_free(&base64);
	SE_free(&bin_encrypt);
	*ptr = NULL;
	return false;
}

bool SEAPI SE_aes_decrypt(const uint8_t  *aes_key, const uint8_t  *aes_iv, const char *b64_encrypt, char **ptr) {
	char *bin_encrypt = NULL;
	int32_t bin_len;
	struct AES_ctx ctx;

	SE_validate_para_ptr(b64_encrypt, SE_2STR(SE_aes_decrypt), __LINE__);
	SE_throw(SE_base64_decrypt(b64_encrypt, strlen(b64_encrypt), &bin_encrypt, &bin_len));
	if (0 != (bin_len % AES_BLOCKLEN))
		goto SE_ERROR_CLEAR;
	//初始化密钥和向量后开始加密
	if (NULL != aes_key && NULL != aes_iv)
		AES_init_ctx_iv(&ctx, aes_key, aes_iv);
	else
		AES_init_ctx_iv(&ctx, KEY, IV);
	AES_CBC_decrypt_buffer(&ctx, (uint8_t *)bin_encrypt, bin_len);
	*ptr = bin_encrypt;
	return true;
SE_ERROR_CLEAR:
	SE_free(&bin_encrypt);
	*ptr = NULL;
	return false;
}