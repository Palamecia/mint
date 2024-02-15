/**
 * Copyright (c) 2024 Gauvain CHERY.
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

#include "mint/system/string.h"

#include <cinttypes>
#include <cstdarg>

using namespace mint;
using namespace std;

enum StringFormatLength {
	string_default_length,
	string_byte_length,
	string_half_length,
	string_long_length,
	string_long_long_length,
	string_max_length,
	string_size_length,
	string_ptrdiff_length,
	string_long_double_length
};

string mint::format(const char *format, ...) {
	va_list args;
	va_start(args, format);
	string str = mint::vformat(format, args);
	va_end(args);
	return str;
}

string mint::vformat(const char *format, va_list args) {

	string result;

	for (const char *cptr = format; *cptr != '\0'; ++cptr) {

		if ((*cptr == '%')) {

			if (*(cptr + 1) == '%') {
				result += '%';
				++cptr;
				continue;
			}

			StringFormatLength length = string_default_length;
			StringFormatFlags flags = 0;
			bool handled = false;

			while (!handled && *cptr != '\0') {
				if (UNLIKELY(*++cptr == '\0')) {
					return result;
				}
				switch (*cptr) {
				case '-':
					flags |= string_left;
					continue;
				case '+':
					flags |= string_plus;
					continue;
				case ' ':
					flags |= string_space;
					continue;
				case '#':
					flags |= string_special;
					continue;
				case '0':
					flags |= string_zeropad;
					continue;
				case 'h':
					switch (length) {
					case string_default_length:
						length = string_half_length;
						break;
					case string_half_length:
						length = string_byte_length;
						break;
					default:
						return {};
					}
					continue;
				case 'l':
					switch (length) {
					case string_default_length:
						length = string_long_length;
						break;
					case string_long_length:
						length = string_long_double_length;
						break;
					default:
						return {};
					}
					continue;
				case 'j':
					switch (length) {
					case string_default_length:
						length = string_max_length;
						break;
					default:
						return {};
					}
					continue;
				case 'z':
					switch (length) {
					case string_default_length:
						length = string_size_length;
						break;
					default:
						return {};
					}
					continue;
				case 't':
					switch (length) {
					case string_default_length:
						length = string_ptrdiff_length;
						break;
					default:
						return {};
					}
					continue;
				case 'L':
					switch (length) {
					case string_default_length:
						length = string_long_double_length;
						break;
					default:
						return {};
					}
					continue;
				default:
					handled = true;
					break;
				}

				int field_width = -1;
				if (isdigit(*cptr)) {
					string num;
					while (isdigit(*cptr)) {
						num += *cptr;
						if (UNLIKELY(*++cptr == '\0')) {
							return result;
						}
					}
					field_width = atoi(num.c_str());
				}
				else if (*cptr == '*') {
					if (UNLIKELY(*++cptr == '\0')) {
						return result;
					}
					field_width = va_arg(args, int);
					if (field_width < 0) {
						field_width = -field_width;
						flags |= string_left;
					}
				}

				int precision = -1;
				if (*cptr == '.') {
					if (UNLIKELY(*++cptr == '\0')) {
						return result;
					}
					if (isdigit(*cptr)) {
						string num;
						while (isdigit(*cptr)) {
							num += *cptr;
							if (UNLIKELY(*++cptr == '\0')) {
								return result;
							}
						}
						precision = atoi(num.c_str());
					}
					else if (*cptr == '*') {
						if (UNLIKELY(*++cptr == '\0')) {
							return result;
						}
						precision = va_arg(args, int);
					}
					if (precision < 0) {
						precision = 0;
					}
				}

				string s;
				int len;
				int base = 10;

				switch (*cptr) {
				case 'c':
					if (!(flags & string_left)) while (--field_width > 0) result += ' ';
					switch (length) {
					case string_default_length:
						result += va_arg(args, int);
						break;
					case string_long_length:
						// TODO: result += va_arg(args, wint_t);
						break;
					default:
						return {};
					}
					while (--field_width > 0) result += ' ';
					continue;
				case 's':
					switch (length) {
					case string_default_length:
						s = va_arg(args, const char *);
						break;
					case string_long_length:
						// TODO: s = va_arg(args, const wchar_t *);
						break;
					default:
						return {};
					}
					len = (precision < 0) ? static_cast<int>(s.size()) : min(precision, static_cast<int>(s.size()));
					if (!(flags & string_left)) while (len < field_width--) result += ' ';
					result += s.substr(0, static_cast<size_t>(len));
					while (len < field_width--) result += ' ';
					continue;
				case 'P':
					flags |= string_large;
					[[fallthrough]];
				case 'p':
					if (field_width == -1) {
						field_width = 2 * sizeof(void *);
						flags |= string_zeropad;
					}
					switch (length) {
					case string_default_length:
						result += format_integer(reinterpret_cast<uintptr_t>(va_arg(args, void *)), 16, field_width, precision, flags);
						break;
					default:
						return {};
					}
					continue;
				case 'A':
					flags |= string_large;
					[[fallthrough]];
				case 'a':
					switch (length) {
					case string_default_length:
						result += format_float(va_arg(args, double), 16, decimal_format, field_width, precision, flags);
						break;
					case string_long_double_length:
						result += format_float(va_arg(args, long double), 16, decimal_format, field_width, precision, flags);
						break;
					default:
						return {};
					}
					continue;
				case 'B':
					flags |= string_large;
					[[fallthrough]];
				case 'b':
					base = 2;
					break;
				case 'O':
					flags |= string_large;
					[[fallthrough]];
				case 'o':
					base = 8;
					break;
				case 'X':
					flags |= string_large;
					[[fallthrough]];
				case 'x':
					base = 16;
					break;
				case 'd':
				case 'i':
					flags |= string_sign;
					break;
				case 'u':
					break;
				case 'E':
					flags |= string_large;
					[[fallthrough]];
				case 'e':
					switch (length) {
					case string_default_length:
						result += format_float(va_arg(args, double), 10, scientific_format, field_width, precision, flags | string_sign);
						break;
					case string_long_double_length:
						result += format_float(va_arg(args, long double), 10, scientific_format, field_width, precision, flags | string_sign);
						break;
					default:
						return {};
					}
					continue;
				case 'F':
					flags |= string_large;
					[[fallthrough]];
				case 'f':
					switch (length) {
					case string_default_length:
						result += format_float(va_arg(args, double), 10, decimal_format, field_width, precision, flags | string_sign);
						break;
					case string_long_double_length:
						result += format_float(va_arg(args, long double), 10, decimal_format, field_width, precision, flags | string_sign);
						break;
					default:
						return {};
					}
					continue;
				case 'G':
					flags |= string_large;
					[[fallthrough]];
				case 'g':
					switch (length) {
					case string_default_length:
						result += format_float(va_arg(args, double), 10, shortest_format, field_width, precision, flags | string_sign);
						break;
					case string_long_double_length:
						result += format_float(va_arg(args, long double), 10, shortest_format, field_width, precision, flags | string_sign);
						break;
					default:
						return {};
					}
					continue;
				default:
					result += *cptr;
					continue;
				}

				if (flags & string_sign) {
					switch (length) {
					case string_default_length:
						result += format_integer(va_arg(args, int), base, field_width, precision, flags);
						break;
					case string_byte_length:
						result += format_integer(va_arg(args, signed char), base, field_width, precision, flags);
						break;
					case string_half_length:
						result += format_integer(va_arg(args, short int), base, field_width, precision, flags);
						break;
					case string_long_length:
						result += format_integer(va_arg(args, long int), base, field_width, precision, flags);
						break;
					case string_long_long_length:
						result += format_integer(va_arg(args, long long int), base, field_width, precision, flags);
						break;
					case string_max_length:
						result += format_integer(va_arg(args, intmax_t), base, field_width, precision, flags);
						break;
					case string_size_length:
						result += format_integer(va_arg(args, size_t), base, field_width, precision, flags);
						break;
					case string_ptrdiff_length:
						result += format_integer(va_arg(args, ptrdiff_t), base, field_width, precision, flags);
						break;
					default:
						return {};
					}
				}
				else {
					switch (length) {
					case string_default_length:
						result += format_integer(va_arg(args, unsigned int), base, field_width, precision, flags);
						break;
					case string_byte_length:
						result += format_integer(va_arg(args, unsigned char), base, field_width, precision, flags);
						break;
					case string_half_length:
						result += format_integer(va_arg(args, unsigned short int), base, field_width, precision, flags);
						break;
					case string_long_length:
						result += format_integer(va_arg(args, unsigned long int), base, field_width, precision, flags);
						break;
					case string_long_long_length:
						result += format_integer(va_arg(args, unsigned long long int), base, field_width, precision, flags);
						break;
					case string_max_length:
						result += format_integer(va_arg(args, uintmax_t), base, field_width, precision, flags);
						break;
					case string_size_length:
						result += format_integer(va_arg(args, size_t), base, field_width, precision, flags);
						break;
					case string_ptrdiff_length:
						result += format_integer(va_arg(args, ptrdiff_t), base, field_width, precision, flags);
						break;
					default:
						return {};
					}
				}
			}
		}
		else {
			result += *cptr;
		}
	}

	return result;
}

string mint::to_string(intmax_t value) {
	return format_integer(value, 10, -1, -1, string_sign);
}

string mint::to_string(double value) {
	return format_float(value, 10, shortest_format, -1, -1, string_sign);
}

string mint::to_string(const void *value) {
	char buffer[(sizeof(void *) * 2) + 3];
	sprintf(buffer, "0x%0*" PRIXPTR,
			static_cast<int>(sizeof(decltype(value)) * 2),
			reinterpret_cast<uintptr_t>(value));
	return buffer;
}

bool mint::starts_with(const string_view &str, const string_view &pattern) {
	const auto pattern_size = pattern.size();
	if (str.size() < pattern_size) {
		return false;
	}
	return string_view::traits_type::compare(str.data(), pattern.data(), pattern_size) == 0;
}

bool mint::ends_with(const string_view &str, const string_view &pattern) {
	const auto pattern_size = pattern.size();
	if (str.size() < pattern_size) {
		return false;
	}
	return string_view::traits_type::compare(str.data() + (str.size() - pattern_size), pattern.data(), pattern_size) == 0;
}

void mint::force_decimal_point(string &buffer) {

	string::iterator cptr = buffer.begin();

	while (cptr != buffer.end()) {
		if (*cptr == '.') {
			return;
		}
		if ((*cptr == 'e') || (*cptr == 'E')) {
			break;
		}
		++cptr;
	}

	if (cptr != buffer.end()) {
		buffer.insert(cptr, '.');
	}
	else {
		buffer += '.';
	}
}

void mint::crop_zeros(string &buffer) {

	string::iterator stop = buffer.end();
	string::iterator start = buffer.begin();

	while ((start != buffer.end()) && (*start != '.')) {
		++start;
	}
	if (start++ != buffer.end()) {
		while ((start != buffer.end()) && (*start != 'e') && (*start != 'E')) {
			++start;
		}
		stop = start--;
		while (*start == '0') {
			--start;
		}
		if (*start == '.') {
			--start;
		}
		buffer.erase(start + 1, stop);
	}
}
