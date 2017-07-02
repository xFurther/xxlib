#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#include "lexical_cast.hpp"
#include <stdint.h>


namespace detail {
	const std::string bool_helper::__true("true");
	const std::string bool_helper::__false("false");
}

namespace detail {
	/**
	* Powers of 10
	* 10^0 to 10^9
	*/
	static const double powers_of_10[] = { 1, 10, 100, 1000, 10000, 100000, 1000000,
		10000000, 100000000, 1000000000 };
	static void strreverse(char* begin, char* end)
	{
		char aux;
		while (end > begin)
			aux = *end, *end-- = *begin, *begin++ = aux;
	}
	size_t modp_dtoa(double value, char* str, int prec)
	{
		/* Hacky test for NaN
		* under -fast-math this won't work, but then you also won't
		* have correct nan values anyways.  The alternative is
		* to link with libmath (bad) or hack IEEE double bits (bad)
		*/
		if (!(value == value)) {
			str[0] = 'n'; str[1] = 'a'; str[2] = 'n'; str[3] = '\0';
			return (size_t)3;
		}
		/* if input is larger than thres_max, revert to exponential */
		const double thres_max = (double)(0x7FFFFFFF);

		double diff = 0.0;
		char* wstr = str;

		if (prec < 0) {
			prec = 0;
		}
		else if (prec > 9) {
			/* precision of >= 10 can lead to overflow errors */
			prec = 9;
		}


		/* we'll work in positive values and deal with the
		negative sign issue later */
		int neg = 0;
		if (value < 0) {
			neg = 1;
			value = -value;
		}


		int whole = (int)value;
		double tmp = (value - whole) * powers_of_10[prec];
		uint32_t frac = (uint32_t)(tmp);
		diff = tmp - frac;

		if (diff > 0.5) {
			++frac;
			/* handle rollover, e.g.  case 0.99 with prec 1 is 1.0  */
			if (frac >= powers_of_10[prec]) {
				frac = 0;
				++whole;
			}
		}
		else if (diff == 0.5 && ((frac == 0) || (frac & 1))) {
			/* if halfway, round up if odd, OR
			if last digit is 0.  That last part is strange */
			++frac;
		}

		/* for very large numbers switch back to native sprintf for exponentials.
		anyone want to write code to replace this? */
		/*
		normal printf behavior is to print EVERY whole number digit
		which can be 100s of characters overflowing your buffers == bad
		*/
		if (value > thres_max) {
			sprintf(str, "%e", neg ? -value : value);
			return strlen(str);
		}

		if (prec == 0) {
			diff = value - whole;
			if (diff > 0.5) {
				/* greater than 0.5, round up, e.g. 1.6 -> 2 */
				++whole;
			}
			else if (diff == 0.5 && (whole & 1)) {
				/* exactly 0.5 and ODD, then round up */
				/* 1.5 -> 2, but 2.5 -> 2 */
				++whole;
			}
		}
		else {
			int count = prec;
			/* now do fractional part, as an unsigned number */
			do {
				--count;
				*wstr++ = (char)(48 + (frac % 10));
			} while (frac /= 10);
			/* add extra 0s */
			while (count-- > 0) *wstr++ = '0';
			/* add decimal */
			*wstr++ = '.';
		}

		/* do whole part
		* Take care of sign
		* Conversion. Number is reversed.
		*/
		do *wstr++ = (char)(48 + (whole % 10)); while (whole /= 10);
		if (neg) {
			*wstr++ = '-';
		}
		*wstr = '\0';
		strreverse(str, wstr - 1);
		return (size_t)(wstr - str);
	}
}

namespace detail {
	unsigned digits10(uint64_t v) {
		unsigned result = 1;
		for (;;) {
			if (v < 10) return result;
			if (v < 100) return result + 1;
			if (v < 1000) return result + 2;
			if (v < 10000) return result + 3;
			// Skip ahead by 4 orders of magnitude
			v /= 10000U;
			result += 4;
		}
	}

	unsigned facebook_uint64_to_str(uint64_t value, char* dst) {
		static const char digits[201] =
			"0001020304050607080910111213141516171819"
			"2021222324252627282930313233343536373839"
			"4041424344454647484950515253545556575859"
			"6061626364656667686970717273747576777879"
			"8081828384858687888990919293949596979899";
		uint32_t const length = digits10(value);
		uint32_t next = length - 1;
		while (value >= 100) {
			auto const i = (value % 100) * 2;
			value /= 100;
			dst[next] = digits[i + 1];
			dst[next - 1] = digits[i];
			next -= 2;
		}
		// Handle last 1-2 digits
		if (value < 10) {
			dst[next] = '0' + uint32_t(value);
		}
		else {
			auto i = uint32_t(value) * 2;
			dst[next] = digits[i + 1];
			dst[next - 1] = digits[i];
		}
		return length;
	}
}
