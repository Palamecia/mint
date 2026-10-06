/**
 * Copyright (c) 2026 Gauvain CHÉRY.
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
#include <utility>

namespace mint {

static constexpr const std::string_view lower_digits = "0123456789abcdefghijklmnopqrstuvwxyz";
static constexpr const std::string_view upper_digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static constexpr const std::string_view inf_string = "inf";
static constexpr const std::string_view nan_string = "nan";

using StringFormatFlags = unsigned;
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
std::string digits_to_string(number_t number, NumberBase base, DigitsFormat format, std::size_t precision, bool capexp,
    int& decpt, bool& sign) {

	std::string result;
	number_t fi;
	number_t fj;
	const auto digits = (capexp) ? upper_digits : lower_digits;
	static const auto mod_function = [](number_t x, number_t* intptr) -> number_t {
		if constexpr (std::is_same_v<number_t, double>) {
			return std::modf(x, intptr);
		}
		else if constexpr (std::is_same_v<number_t, float>) {
			return std::modf(x, intptr);
		}
		else if constexpr (std::is_same_v<number_t, long double>) {
			return std::modf(x, intptr);
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
			buffer += digits.at(static_cast<std::size_t>((fj + .03) * static_cast<number_t>(base)));
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
	// Signed on purpose: the number of digits to produce is negative for a small number
	// whose first significant digit is beyond the requested precision.
	const auto signed_pos = static_cast<std::ptrdiff_t>(precision) + (format == DigitsFormat::decimal ? r2 : 0);
	decpt = r2;
	if (signed_pos < 0) {
		return result;
	}
	auto pos = static_cast<std::size_t>(signed_pos);
	while (pos >= result.size()) {
		number *= static_cast<number_t>(base);
		number = mod_function(number, &fj);
		result += digits.at(static_cast<std::size_t>(fj));
	}
	auto last = pos;
	result[pos] += static_cast<char>(base >> 1);
	while (result[pos] > digits.at(base - 1)) {
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
std::string float_to_string(number_t number, NumberBase base, DigitsFormat format, std::size_t precision, bool capexp) {

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

		do {
			buffer += digits.at(exp % base);
			exp = exp / base;
		} while (exp && buffer.size() < 3);

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
					if (std::cmp_equal(pos, decpt)) {
						result += '.';
					}
					result += num_digits.at(pos);
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
std::string format_float(number_t number, NumberBase base = decimal_base, DigitsFormat format = DigitsFormat::shortest,
    std::size_t size = unknown_size, std::size_t precision = unknown_precision,
    StringFormatFlags flags = std::is_signed_v<number_t> ? string_sign : 0) {

	if (flags & string_left) {
		flags &= ~string_zeropad;
	}

	char sign = 0;
	if (flags & string_sign) {
		if (number < 0.0) {
			sign = '-';
			number = -number;
		}
		else if (flags & string_plus) {
			sign = '+';
		}
		else if (flags & string_space) {
			sign = ' ';
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

	// The width is optional: `size` is never modified, so it can not underflow and the
	// "unknown" marker can not be lost. A width smaller than the content gives no padding.
	const std::size_t used = (sign ? 1 : 0) + buffer.size();
	const std::size_t padding = (size != unknown_size && size > used) ? size - used : 0;

	auto result = std::string();
	if (!(flags & (string_zeropad | string_left))) {
		result.append(padding, ' ');
	}
	if (sign) {
		result += sign;
	}
	if (flags & string_zeropad) {
		result.append(padding, '0');
	}
	result += buffer;
	if (flags & string_left) {
		result.append(padding, ' ');
	}

	return result;
}

template<typename number_t>
    requires std::is_integral_v<number_t> && (!std::is_same_v<number_t, bool>)
std::string format_integer(number_t number, NumberBase base = decimal_base, std::size_t size = unknown_size,
    std::size_t precision = unknown_precision, StringFormatFlags flags = std::is_signed_v<number_t> ? string_sign : 0) {

	using unsigned_t = std::make_unsigned_t<number_t>;

	const auto digits = (flags & string_large) ? upper_digits : lower_digits;

	if (flags & string_left) {
		flags &= ~string_zeropad;
	}

	// The magnitude is computed on the unsigned type. This is well defined for the minimum
	// value of a signed type, and gives the two's complement representation of a negative
	// number when the sign flag is not set (as printf does).
	auto value = static_cast<unsigned_t>(number);
	char sign = 0;
	if (flags & string_sign) {
		if constexpr (std::is_signed_v<number_t>) {
			if (number < 0) {
				sign = '-';
				value = static_cast<unsigned_t>(0 - value);
			}
		}
		if (!sign) {
			if (flags & string_plus) {
				sign = '+';
			}
			else if (flags & string_space) {
				sign = ' ';
			}
		}
	}

	const bool special = (flags & string_special)
	                     && (base == hexadecimal_base || base == octal_base || base == binary_base);

	auto tmp = std::string();
	const auto radix = static_cast<unsigned_t>(base);
	if (value == 0) {
		tmp = '0';
	}
	else {
		for (; value != 0; value /= radix) {
			tmp += digits.at(value % radix);
		}
	}

	precision = (precision == unknown_precision) ? tmp.size() : std::max(precision, tmp.size());

	// The width is optional: `size` is never modified, so it can not underflow and the
	// "unknown" marker can not be lost. A width smaller than the content gives no padding.
	const std::size_t used = (sign ? 1 : 0) + (special ? 2 : 0) + precision;
	const std::size_t padding = (size != unknown_size && size > used) ? size - used : 0;

	auto result = std::string();
	if (!(flags & (string_zeropad | string_left))) {
		result.append(padding, ' ');
	}
	if (sign) {
		result += sign;
	}
	if (special) {
		result += '0';
		switch (base) {
		case hexadecimal_base:
			result += digits.at(33);
			break;
		case octal_base:
			result += digits.at(24);
			break;
		case binary_base:
			result += digits.at(11);
			break;
		default:
			break;
		}
	}
	if (flags & string_zeropad) {
		result.append(padding, '0');
	}
	result.append(precision - tmp.size(), '0');
	result.append(tmp.rbegin(), tmp.rend());
	if (flags & string_left) {
		result.append(padding, ' ');
	}

	return result;
}

}

#endif // MINT_SYSTEM_STRING_H
