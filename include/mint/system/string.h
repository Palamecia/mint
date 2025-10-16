/**
 * Copyright (c) 2026 Gauvain CHERY.
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

#ifndef MINT_SYSTEM_STRING_H
#define MINT_SYSTEM_STRING_H

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <mint/config.h>
#include <string_view>
#include <ranges>
#include <string>
#include <cmath>
#include <type_traits>

namespace mint {

static constexpr const std::string_view lower_digits = "0123456789abcdefghijklmnopqrstuvwxyz";
static constexpr const std::string_view upper_digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static constexpr const std::string_view inf_string = "inf";
static constexpr const std::string_view nan_string = "nan";

using StringFormatFlags = std::uint8_t;
constexpr StringFormatFlags string_left = 0x01;
constexpr StringFormatFlags string_plus = 0x02;
constexpr StringFormatFlags string_space = 0x04;
constexpr StringFormatFlags string_special = 0x08;
constexpr StringFormatFlags string_zeropad = 0x10;
constexpr StringFormatFlags string_large = 0x20;
constexpr StringFormatFlags string_sign = 0x40;

using NumberBase = std::uint8_t;
constexpr NumberBase binary_base = 2;
constexpr NumberBase octal_base = 8;
constexpr NumberBase decimal_base = 10;
constexpr NumberBase hexadecimal_base = 16;

enum class DigitsFormat : std::uint8_t {
	scientific,
	decimal,
	shortest,
};

constexpr std::size_t digits_default_precision = 6;
constexpr std::size_t unknown_size = std::numeric_limits<std::size_t>::max();
constexpr std::size_t unknown_precision = std::numeric_limits<std::size_t>::max();

MINT_EXPORT std::string to_string(std::intmax_t value);
MINT_EXPORT std::string to_string(std::uintmax_t value);
MINT_EXPORT std::string to_string(double value, DigitsFormat format = DigitsFormat::shortest);
MINT_EXPORT std::string to_string(const void* value);

MINT_EXPORT void force_decimal_point(std::string& buffer);
MINT_EXPORT void crop_zeros(std::string& buffer);

template<typename number_t>
    requires std::is_arithmetic_v<number_t>
static std::string digits_to_string(number_t number, NumberBase base, DigitsFormat format, std::size_t precision,
    bool capexp, int& decpt, bool& sign) {

	std::string result;
	number_t fi;
	number_t fj;
	const auto digits = (capexp) ? upper_digits : lower_digits;
	static const auto mod_function = [](number_t x, number_t* intptr) -> number_t {
		if constexpr (std::is_same_v<number_t, double>) {
			return std::modf(x, intptr);
		}
		else if constexpr (std::is_same_v<number_t, float>) {
			return std::modff(x, intptr);
		}
		else if constexpr (std::is_same_v<number_t, long double>) {
			return std::modfl(x, intptr);
		}
	};

	int r2 = 0;
	sign = false;
	if (number < 0) {
		sign = true;
		number = -number;
	}
	number = mod_function(number, &fi);

	if (fi != 0.) {
		std::string buffer;
		while (fi != 0.) {
			fj = mod_function(fi / static_cast<number_t>(base), &fi);
			buffer += digits[static_cast<std::size_t>((fj + .03) * static_cast<number_t>(base))];
			r2++;
		}
		result.append_range(std::views::reverse(buffer));
	}
	else if (number > 0) {
		while ((fj = number * static_cast<number_t>(base)) < 1) {
			number = fj;
			r2--;
		}
	}
	auto pos = precision;
	if (format == DigitsFormat::decimal) {
		pos += r2;
	}
	decpt = r2;
	if (pos < 0) {
		return result;
	}
	while (pos >= result.size()) {
		number *= static_cast<number_t>(base);
		number = mod_function(number, &fj);
		result += digits[static_cast<std::size_t>(fj)];
	}
	auto last = pos;
	result[pos] += static_cast<char>(base >> 1);
	while (result[pos] > digits[base - 1]) {
		result[pos] = '0';
		if (pos > 0) {
			++result[--pos];
		}
		else {
			result[pos] = '1';
			decpt++;
			if (format == DigitsFormat::decimal) {
				if (last > 0) {
					result[last] = '0';
				}
				result.push_back('0');
				last++;
			}
		}
	}
	while (last < result.size()) {
		result.pop_back();
	}
	return result;
}

template<typename number_t>
    requires std::is_floating_point_v<number_t>
static std::string float_to_string(number_t number, NumberBase base, DigitsFormat format, std::size_t precision,
    bool capexp) {

	std::string result;
	int decpt = 0;
	bool sign = false;
	const auto digits = (capexp) ? upper_digits : lower_digits;

	if (std::isinf(number)) {
		return std::string(inf_string);
	}

	if (std::isnan(number)) {
		return std::string(nan_string);
	}

	if (format == DigitsFormat::shortest) {
		digits_to_string(number, base, DigitsFormat::scientific, precision, capexp, decpt, sign);
		const int magnitude = decpt - 1;
		if ((magnitude < -4) || (magnitude > static_cast<int>(precision) - 1)) {
			format = DigitsFormat::scientific;
			precision -= 1;
		}
		else {
			format = DigitsFormat::decimal;
			precision -= decpt;
		}
	}

	if (format == DigitsFormat::scientific) {
		std::string num_digits = digits_to_string(number, base, format, precision + 1, capexp, decpt, sign);

		if (sign) {
			result += '-';
		}
		result += num_digits.front();
		if (precision > 0) {
			result += '.';
		}
		result += std::string(std::next(num_digits.data(), 1), precision) + (capexp ? 'E' : 'e');

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

		auto buffer = std::string();

		while (exp && buffer.size() < 3) {
			buffer += digits[(exp % base)];
			exp = exp / base;
		}

		result.append_range(std::views::reverse(buffer));
	}
	else if (format == DigitsFormat::decimal) {
		std::string num_digits = digits_to_string(number, base, format, precision, capexp, decpt, sign);
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
				for (std::size_t pos = 0; pos < num_digits.size(); ++pos) {
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
    requires std::is_floating_point_v<number_t>
static std::string format_float(number_t number, NumberBase base = decimal_base,
    DigitsFormat format = DigitsFormat::shortest, std::size_t size = unknown_size,
    std::size_t precision = unknown_precision, StringFormatFlags flags = std::is_signed_v<number_t> ? string_sign : 0) {

	std::string result;

	if (flags & string_left) {
		flags &= ~string_zeropad;
	}

	const char c = (flags & string_zeropad) ? '0' : ' ';
	char sign = 0;
	if (flags & string_sign) {
		if (number < 0.0) {
			sign = '-';
			number = -number;
			size--;
		}
		else if (flags & string_plus) {
			sign = '+';
			size--;
		}
		else if (flags & string_space) {
			sign = ' ';
			size--;
		}
	}

	if (precision == unknown_precision) {
		precision = digits_default_precision;
	}
	else if ((precision == 0) && (format == DigitsFormat::shortest)) {
		precision = 1;
	}

	std::string buffer = float_to_string(number, base, format, precision, flags & string_large);

	if ((flags & string_special) && (precision == 0)) {
		force_decimal_point(buffer);
	}

	if ((format == DigitsFormat::shortest) && !(flags & string_special)) {
		crop_zeros(buffer);
	}
	if (size == unknown_size) {
		size = 0;
	}
	else {
		size -= buffer.size();
	}
	if (!(flags & (string_zeropad | string_left))) {
		for (; size > 0; --size) {
			result += ' ';
		}
	}
	if (sign) {
		result += sign;
	}
	if (!(flags & string_left)) {
		for (; size > 0; --size) {
			result += c;
		}
	}
	result += buffer;
	for (; size > 0; --size) {
		result += ' ';
	}

	return result;
}

template<typename number_t>
    requires std::is_integral_v<number_t>
static std::string format_integer(number_t number, NumberBase base = decimal_base, std::size_t size = unknown_size,
    std::size_t precision = unknown_precision, StringFormatFlags flags = std::is_signed_v<number_t> ? string_sign : 0) {

	std::string tmp;
	std::string result;
	const auto digits = (flags & string_large) ? upper_digits : lower_digits;

	if (flags & string_left) {
		flags &= ~string_zeropad;
	}

	const char c = (flags & string_zeropad) ? '0' : ' ';
	char sign = 0;
	if (flags & string_sign) {
		if constexpr (std::is_signed_v<number_t>) {
			if (number < 0) {
				sign = '-';
				number = -number;
				size--;
			}
			else if (flags & string_plus) {
				sign = '+';
				size--;
			}
			else if (flags & string_space) {
				sign = ' ';
				size--;
			}
		}
		else {
			if (flags & string_plus) {
				sign = '+';
				size--;
			}
			else if (flags & string_space) {
				sign = ' ';
				size--;
			}
		}
	}

	if (flags & string_special) {
		if ((base == hexadecimal_base) || (base == octal_base) || (base == binary_base)) {
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
	if (precision == unknown_precision) {
		precision = tmp.size();
	}
	else {
		precision = std::max(precision, tmp.size());
	}
	if (size == unknown_size) {
		size = 0;
	}
	else {
		size -= precision;
	}
	if (!(flags & (string_zeropad + string_left))) {
		for (; size > 0; --size) {
			result += ' ';
		}
	}
	if (sign) {
		result += sign;
	}

	if (flags & string_special) {
		switch (base) {
		case hexadecimal_base:
			result += "0";
			result += digits[33];
			break;
		case octal_base:
			result += "0";
			result += digits[24];
			break;
		case binary_base:
			result += "0";
			result += digits[11];
			break;
		case decimal_base:
			break;
		}
	}

	if (!(flags & string_left)) {
		for (; size > 0; --size) {
			result += c;
		}
	}
	while (precision-- > tmp.size()) {
		result += '0';
	}
	std::ranges::copy(std::views::reverse(tmp), std::back_inserter(result));
	for (; size > 0; --size) {
		result += ' ';
	}

	return result;
}

}

#endif // MINT_SYSTEM_STRING_H
