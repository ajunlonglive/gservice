#include "../pg/pg_bswap.h"
#include "se_numeric.h"



const char *get_str_from_var(struct SE_WORKINFO *arg, const struct NumericVar * const var) {
	int			dscale;
	char	   *str;
	char	   *cp;
	char	   *endcp;
	int			i;
	int			d;
	NumericDigit dig;

#if DEC_DIGITS > 1
	NumericDigit d1;
#endif

	dscale = var->dscale;

	/*
	* Allocate space for the result.
	*
	* i is set to the # of decimal digits before decimal point. dscale is the
	* # of decimal digits we will print after decimal point. We may generate
	* as many as DEC_DIGITS-1 excess digits at the end, and in addition we
	* need room for sign, decimal point, null terminator.
	*/
	i = (var->weight + 1) * DEC_DIGITS;
	if (i <= 0)
		i = 1;

	SE_send_malloc_fail((str = (char *)soap_malloc(arg->tsoap, i + dscale + DEC_DIGITS + 2)), arg, SE_2STR(get_str_from_var));
	cp = str;

	/*
	* Output a dash for negative values
	*/
	if (var->sign == NUMERIC_NEG)
		*cp++ = '-';

	/*
	* Output all digits before the decimal point
	*/
	if (var->weight < 0) {
		d = var->weight + 1;
		*cp++ = '0';
	} else {
		for (d = 0; d <= var->weight; d++) {
			dig = (d < var->ndigits) ? var->digits[d] : 0;
			/* In the first digit, suppress extra leading decimal zeroes */
#if DEC_DIGITS == 4
			{
				bool		putit = (d > 0);

				d1 = dig / 1000;
				dig -= d1 * 1000;
				putit |= (d1 > 0);
				if (putit)
					*cp++ = d1 + '0';
				d1 = dig / 100;
				dig -= d1 * 100;
				putit |= (d1 > 0);
				if (putit)
					*cp++ = d1 + '0';
				d1 = dig / 10;
				dig -= d1 * 10;
				putit |= (d1 > 0);
				if (putit)
					*cp++ = d1 + '0';
				*cp++ = dig + '0';
			}
#elif DEC_DIGITS == 2
			d1 = dig / 10;
			dig -= d1 * 10;
			if (d1 > 0 || d > 0)
				*cp++ = d1 + '0';
			*cp++ = dig + '0';
#elif DEC_DIGITS == 1
			* cp++ = dig + '0';
#else
#error unsupported NBASE
#endif
		}
	}

	/*
	* If requested, output a decimal point and all the digits that follow it.
	* We initially put out a multiple of DEC_DIGITS digits, then truncate if
	* needed.
	*/
	if (dscale > 0) {
		*cp++ = '.';
		endcp = cp + dscale;
		for (i = 0; i < dscale; d++, i += DEC_DIGITS) {
			dig = (d >= 0 && d < var->ndigits) ? var->digits[d] : 0;
#if DEC_DIGITS == 4
			d1 = dig / 1000;
			dig -= d1 * 1000;
			*cp++ = d1 + '0';
			d1 = dig / 100;
			dig -= d1 * 100;
			*cp++ = d1 + '0';
			d1 = dig / 10;
			dig -= d1 * 10;
			*cp++ = d1 + '0';
			*cp++ = dig + '0';
#elif DEC_DIGITS == 2
			d1 = dig / 10;
			dig -= d1 * 10;
			*cp++ = d1 + '0';
			*cp++ = dig + '0';
#elif DEC_DIGITS == 1
			* cp++ = dig + '0';
#else
#error unsupported NBASE
#endif
		}
		cp = endcp;
	}

	/*
	* terminate the string and return it
	*/
	*cp = '\0';
	return str;
SE_ERROR_CLEAR:
	return NULL;
}

const struct NumericVar *bytes2numeric(struct SE_WORKINFO *arg, const uint8_t *const data) {
	struct NumericVar *num;
	const uint8_t *tmp_ptr;
	int16_t i;

	SE_send_malloc_fail((num = (struct NumericVar *)soap_malloc(arg->tsoap, sizeof(struct NumericVar))), arg, SE_2STR(bytes2numeric));
	memset(num, 0, sizeof(struct NumericVar));

	tmp_ptr = data;

	num->ndigits = pg_hton16(*((const uint16_t *)tmp_ptr));
	tmp_ptr += sizeof(int16_t);

	num->weight = pg_hton16(*((const uint16_t *)tmp_ptr));
	tmp_ptr += sizeof(int16_t);

	num->sign = pg_hton16(*((const uint16_t *)tmp_ptr));
	tmp_ptr += sizeof(int16_t);

	num->dscale = pg_hton16(*((const uint16_t *)tmp_ptr));
	tmp_ptr += sizeof(int16_t);

	SE_send_malloc_fail((num->digits = (int16_t *)soap_malloc(arg->tsoap, num->ndigits * sizeof(int16_t))), arg, SE_2STR(bytes2numeric));	
	for (i = 0; i < num->ndigits; i++) {
		num->digits[i] = pg_hton16(*((const uint16_t *)tmp_ptr));
		tmp_ptr += sizeof(int16_t);
	}
	return num;
SE_ERROR_CLEAR:
	return NULL;
}
