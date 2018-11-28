#ifndef SE_7AD3F818_BF2A_4797_978C_A37314F589BD
#define SE_7AD3F818_BF2A_4797_978C_A37314F589BD
#include "se_utils.h"

/*****************************************************************************
*	pg numeric 相关定义
*****************************************************************************/
#define NUMERIC_NEG			0x4000
#if 1
#define NBASE		10000
#define HALF_NBASE	5000
#define DEC_DIGITS	4			/* decimal digits per NBASE digit */
#define MUL_GUARD_DIGITS	2	/* these are measured in NBASE digits */
#define DIV_GUARD_DIGITS	4

typedef int16_t NumericDigit;
#endif

struct NumericVar {
	int16_t			ndigits;		/* # of digits in digits[] - can be 0! */
	int16_t			weight;			/* weight of first digit */
	int16_t			sign;			/* NUMERIC_POS, NUMERIC_NEG, or NUMERIC_NAN */
	int16_t			dscale;			/* display scale */
	NumericDigit *digits;		/* base-NBASE digits */
};

const char * get_str_from_var(struct SE_WORKINFO *arg, const struct NumericVar * const var);

const struct NumericVar *bytes2numeric(struct SE_WORKINFO *arg, const uint8_t *const data);

#endif /*SE_7AD3F818_BF2A_4797_978C_A37314F589BD*/