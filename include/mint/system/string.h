/**
 * Copyright (c) 2025 Gauvain CHERY.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef MINT_STRING_H
#define MINT_STRING_H

#include <cstdint>
#include <mint/config.h>
#include <string_view>
#include <cinttypes>
#include <string>
#include <cmath>

namespace mint {

static constexpr const char *LOWER_DIGITS = "0123456789abcdefghijklmnopqrstuvwxyz";
static constexpr const char *UPPER_DIGITS = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static constexpr const char *INF_STRING = "inf";
static constexpr const char *NAN_STRING = "nan";

enum StringFormatFlag : std::uint8_t {
	STRING_LEFT = 0x01,
	STRING_PLUS = 0x02,
	STRING_SPACE = 0x04,
	STRING_SPECIAL = 0x08,
	STRING_ZEROPAD = 0x10,
	STRING_LARGE = 0x20,
	STRING_SIGN = 0x40
};

using StringFormatFlags = std::underlying_type_t<StringFormatFlag>;

enum DigitsFormat : std::uint8_t {
	SCIENTIFIC_FORMAT,
	DECIMAL_FORMAT,
	SHORTEST_FORMAT
};

MINT_EXPORT std::string format(const char *format, ...) __attribute__((format(printf, 1, 2)));
MINT_EXPORT std::string vformat(const char *format, va_list args);

MINT_EXPORT std::string to_string(intmax_t value);
MINT_EXPORT std::string to_string(double value, DigitsFormat format = SHORTEST_FORMAT);
MINT_EXPORT std::string to_string(const void *value);

MINT_EXPORT bool starts_with(const std::string &str, const std::string &pattern);
MINT_EXPORT bool ends_with(const std::string &str, const std::string &pattern);

MINT_EXPORT void force_decimal_point(std::string &buffer);
MINT_EXPORT void crop_zeros(std::string &buffer);

template<class StringList, class Adapter>
std::string join(
	StringList &list, std::string_view separator, const Adapter &adapter = [](auto it) {
		return *it;
	}) {
	std::string str;
	for (auto it = list.begin(); it != list.end(); ++it) {
		if (it != list.begin()) {
			str += separator;
		}
		str += adapter(it);
	}
	return str;
}

template<typename number_t>
static std::string digits_to_string(number_t number, int base, DigitsFormat format, int precision, bool capexp,
									int *decpt, bool *sign) {

	std::string result;
	number_t fi, fj;
	const char *digits = (capexp) ? UPPER_DIGITS : LOWER_DIGITS;
	static auto mod_function = [](number_t x, number_t *intptr) -> number_t {
		if constexpr (std::is_same_v<number_t, double>) {
			return modf(x, intptr);
		}
		else if constexpr (std::is_same_v<number_t, float>) {
			return modff(x, intptr);
		}
		else if constexpr (std::is_same_v<number_t, long double>) {
			return modfl(x, intptr);
		}
	};

	int r2 = 0;
	*sign = false;
	if (number < 0) {
		*sign = true;
		number = -number;
	}
	number = mod_function(number, &fi);

	if (fi != 0.) {
		std::string buffer;
		while (fi != 0.) {
			fj = mod_function(fi / base, &fi);
			buffer += digits[static_cast<int>((fj + .03) * base)];
			r2++;
		}
		for (auto i = buffer.rbegin(); i != buffer.rend(); ++i) {
			result += *i;
		}
	}
	else if (number > 0) {
		while ((fj = number * base) < 1) {
			number = fj;
			r2--;
		}
	}
	int pos = precision;
	if (format == DECIMAL_FORMAT) {
		pos += r2;
	}
	*decpt = r2;
	if (pos < 0) {
		return result;
	}
	while (result.size() <= static_cast<size_t>(pos)) {
		number *= base;
		number = mod_function(number, &fj);
		result += digits[static_cast<int>(fj)];
	}
	int last = pos;
	result[static_cast<size_t>(pos)] += static_cast<char>(base >> 1);
	while (result[static_cast<size_t>(pos)] > digits[base - 1]) {
		result[static_cast<size_t>(pos)] = '0';
		if (pos > 0) {
			++result[static_cast<size_t>(--pos)];
		}
		else {
			result[static_cast<size_t>(pos)] = '1';
			(*decpt)++;
			if (format == DECIMAL_FORMAT) {
				if (last > 0) {
					result[static_cast<size_t>(last)] = '0';
				}
				result.push_back('0');
				last++;
			}
		}
	}
	while (last < static_cast<int>(result.size())) {
		result.pop_back();
	}
	return result;
}

template<typename number_t>
static std::string float_to_string(number_t number, int base, DigitsFormat format, int precision, bool capexp) {

	std::string result;
	int decpt = 0;
	bool sign = false;
	const char *digits = (capexp) ? UPPER_DIGITS : LOWER_DIGITS;

	if (std::isinf(number)) {
		return INF_STRING;
	}

	if (std::isnan(number)) {
		return NAN_STRING;
	}

	if (format == SHORTEST_FORMAT) {
		digits_to_string(number, base, SCIENTIFIC_FORMAT, precision, capexp, &decpt, &sign);
		int magnitude = decpt - 1;
		if ((magnitude < -4) || (magnitude > precision - 1)) {
			format = SCIENTIFIC_FORMAT;
			precision -= 1;
		}
		else {
			format = DECIMAL_FORMAT;
			precision -= decpt;
		}
	}

	if (format == SCIENTIFIC_FORMAT) {
		std::string num_digits = digits_to_string(number, base, format, precision + 1, capexp, &decpt, &sign);

		if (sign) {
			result += '-';
		}
		result += num_digits.front();
		if (precision > 0) {
			result += '.';
		}
		result += std::string(num_digits.data() + 1, static_cast<size_t>(precision)) + (capexp ? 'E' : 'e');

		int exp = 0;

		if (decpt == 0) {
			if (number == 0.0) {
				exp = 0;
			}
			else {
				exp = -1;
			}
		}
		else {
			exp = decpt - 1;
		}

		if (exp < 0) {
			result += '-';
			exp = -exp;
		}
		else {
			result += '+';
		}

		char buffer[4];
		char *cptr = &buffer[4];
		*(--cptr) = '\0';

		while (exp && buffer < cptr) {
			*(--cptr) = digits[(exp % base)];
			exp = exp / base;
		}

		result += cptr;
	}
	else if (format == DECIMAL_FORMAT) {
		std::string num_digits = digits_to_string(number, base, format, precision, capexp, &decpt, &sign);
		if (sign) {
			result += '-';
		}
		if (!num_digits.empty()) {
			if (decpt <= 0) {
				result += '0';
				result += '.';
				for (int pos = 0; pos < -decpt; pos++) {
					result += '0';
				}
				result += num_digits;
			}
			else {
				for (size_t pos = 0; pos < num_digits.size(); ++pos) {
					if (static_cast<int>(pos) == decpt) {
						result += '.';
					}
					result += num_digits[pos];
				}
			}
		}
		else {
			result += '0';
			if (precision > 0) {
				result += '.';
				for (int pos = 0; pos < precision; pos++) {
					result += '0';
				}
			}
		}
	}

	return result;
}

template<typename number_t>
static std::string format_float(number_t number, int base, DigitsFormat format, int size, int precision,
								StringFormatFlags flags) {

	std::string result;

	if (flags & STRING_LEFT) {
		flags &= ~STRING_ZEROPAD;
	}

	char c = (flags & STRING_ZEROPAD) ? '0' : ' ';
	char sign = 0;
	if (flags & STRING_SIGN) {
		if (number < 0.0) {
			sign = '-';
			number = -number;
			size--;
		}
		else if (flags & STRING_PLUS) {
			sign = '+';
			size--;
		}
		else if (flags & STRING_SPACE) {
			sign = ' ';
			size--;
		}
	}

	if (precision < 0) {
		precision = 6;
	}
	else if ((precision == 0) && (format == SHORTEST_FORMAT)) {
		precision = 1;
	}

	std::string buffer = float_to_string(number, base, format, precision, flags & STRING_LARGE);

	if ((flags & STRING_SPECIAL) && (precision == 0)) {
		force_decimal_point(buffer);
	}

	if ((format == SHORTEST_FORMAT) && !(flags & STRING_SPECIAL)) {
		crop_zeros(buffer);
	}

	size -= static_cast<int>(buffer.size());
	if (!(flags & (STRING_ZEROPAD | STRING_LEFT))) {
		while (size-- > 0) {
			result += ' ';
		}
	}
	if (sign) {
		result += sign;
	}
	if (!(flags & STRING_LEFT)) {
		while (size-- > 0) {
			result += c;
		}
	}
	result += buffer;
	while (size-- > 0) {
		result += ' ';
	}

	return result;
}

template<typename number_t>
static std::string format_integer(number_t number, int base, int size, int precision, StringFormatFlags flags) {

	std::string tmp;
	std::string result;
	const char *digits = (flags & STRING_LARGE) ? UPPER_DIGITS : LOWER_DIGITS;

	if (flags & STRING_LEFT) {
		flags &= ~STRING_ZEROPAD;
	}
	if (base < 2 || base > 36) {
		return result;
	}

	char c = (flags & STRING_ZEROPAD) ? '0' : ' ';
	char sign = 0;
	if (flags & STRING_SIGN) {
		if constexpr (std::is_signed_v<number_t>) {
			if (number < 0) {
				sign = '-';
				number = -number;
				size--;
			}
			else if (flags & STRING_PLUS) {
				sign = '+';
				size--;
			}
			else if (flags & STRING_SPACE) {
				sign = ' ';
				size--;
			}
		}
		else {
			if (flags & STRING_PLUS) {
				sign = '+';
				size--;
			}
			else if (flags & STRING_SPACE) {
				sign = ' ';
				size--;
			}
		}
	}

	if (flags & STRING_SPECIAL) {
		if ((base == 16) || (base == 8) || (base == 2)) {
			size -= 2;
		}
	}

	if (number == 0) {
		tmp = "0";
	}
	else {
		while (number != 0) {
			tmp += digits[number % static_cast<number_t>(base)];
			number = number / static_cast<number_t>(base);
		}
	}

	if (static_cast<int>(tmp.size()) > precision) {
		precision = static_cast<int>(tmp.size());
	}
	size -= precision;
	if (!(flags & (STRING_ZEROPAD + STRING_LEFT))) {
		while (size-- > 0) {
			result += ' ';
		}
	}
	if (sign) {
		result += sign;
	}

	if (flags & STRING_SPECIAL) {
		if (base == 16) {
			result += "0";
			result += digits[33];
		}
		else if (base == 8) {
			result += "0";
			result += digits[24];
		}
		else if (base == 2) {
			result += "0";
			result += digits[11];
		}
	}

	if (!(flags & STRING_LEFT)) {
		while (size-- > 0) {
			result += c;
		}
	}
	while (static_cast<int>(tmp.size()) < precision--) {
		result += '0';
	}
	for (auto cptr = tmp.rbegin(); cptr != tmp.rend(); ++cptr) {
		result += *cptr;
	}
	while (size-- > 0) {
		result += ' ';
	}

	return result;
}

}

#endif // MINT_STRING_H
