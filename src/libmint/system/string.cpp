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

#include <algorithm>
#include <cinttypes>
#include <cstdarg>
#include <cstdint>

using namespace mint;

enum StringFormatLength : std::uint8_t {
	STRING_DEFAULT_LENGTH,
	STRING_BYTE_LENGTH,
	STRING_HALF_LENGTH,
	STRING_LONG_LENGTH,
	STRING_LONG_LONG_LENGTH,
	STRING_MAX_LENGTH,
	STRING_SIZE_LENGTH,
	STRING_PTRDIFF_LENGTH,
	STRING_LONG_DOUBLE_LENGTH
};

std::string mint::format(const char *format, ...) {
	va_list args;
	va_start(args, format);
	std::string str = mint::vformat(format, args);
	va_end(args);
	return str;
}

std::string mint::vformat(const char *format, va_list args) {

	std::string result;

	for (const char *cptr = format; *cptr != '\0'; ++cptr) {

		if ((*cptr == '%')) {

			if (*(cptr + 1) == '%') {
				result += '%';
				++cptr;
				continue;
			}

			StringFormatLength length = STRING_DEFAULT_LENGTH;
			StringFormatFlags flags = 0;
			bool handled = false;

			while (!handled && *cptr != '\0') {
				if (UNLIKELY(*++cptr == '\0')) {
					return result;
				}
				switch (*cptr) {
				case '-':
					flags |= STRING_LEFT;
					continue;
				case '+':
					flags |= STRING_PLUS;
					continue;
				case ' ':
					flags |= STRING_SPACE;
					continue;
				case '#':
					flags |= STRING_SPECIAL;
					continue;
				case '0':
					flags |= STRING_ZEROPAD;
					continue;
				case 'h':
					switch (length) {
					case STRING_DEFAULT_LENGTH:
						length = STRING_HALF_LENGTH;
						break;
					case STRING_HALF_LENGTH:
						length = STRING_BYTE_LENGTH;
						break;
					default:
						return {};
					}
					continue;
				case 'l':
					switch (length) {
					case STRING_DEFAULT_LENGTH:
						length = STRING_LONG_LENGTH;
						break;
					case STRING_LONG_LENGTH:
						length = STRING_LONG_DOUBLE_LENGTH;
						break;
					default:
						return {};
					}
					continue;
				case 'j':
					switch (length) {
					case STRING_DEFAULT_LENGTH:
						length = STRING_MAX_LENGTH;
						break;
					default:
						return {};
					}
					continue;
				case 'z':
					switch (length) {
					case STRING_DEFAULT_LENGTH:
						length = STRING_SIZE_LENGTH;
						break;
					default:
						return {};
					}
					continue;
				case 't':
					switch (length) {
					case STRING_DEFAULT_LENGTH:
						length = STRING_PTRDIFF_LENGTH;
						break;
					default:
						return {};
					}
					continue;
				case 'L':
					switch (length) {
					case STRING_DEFAULT_LENGTH:
						length = STRING_LONG_DOUBLE_LENGTH;
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
					std::string num;
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
						flags |= STRING_LEFT;
					}
				}

				int precision = -1;
				if (*cptr == '.') {
					if (UNLIKELY(*++cptr == '\0')) {
						return result;
					}
					if (isdigit(*cptr)) {
						std::string num;
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
					precision = std::max(precision, 0);
				}

				std::string s;
				int len;
				int base = 10;

				switch (*cptr) {
				case 'c':
					if (!(flags & STRING_LEFT)) {
						while (--field_width > 0) {
							result += ' ';
						}
					}
					switch (length) {
					case STRING_DEFAULT_LENGTH:
						result += va_arg(args, int);
						break;
					case STRING_LONG_LENGTH:
						// TODO: result += va_arg(args, wint_t);
						break;
					default:
						return {};
					}
					while (--field_width > 0) {
						result += ' ';
					}
					continue;
				case 's':
					switch (length) {
					case STRING_DEFAULT_LENGTH:
						s = va_arg(args, const char *);
						break;
					case STRING_LONG_LENGTH:
						// TODO: s = va_arg(args, const wchar_t *);
						break;
					default:
						return {};
					}
					len = (precision < 0) ? static_cast<int>(s.size())
										  : std::min(precision, static_cast<int>(s.size()));
					if (!(flags & STRING_LEFT)) {
						while (len < field_width--) {
							result += ' ';
						}
					}
					result += s.substr(0, static_cast<size_t>(len));
					while (len < field_width--) {
						result += ' ';
					}
					continue;
				case 'P':
					flags |= STRING_LARGE;
					[[fallthrough]];
				case 'p':
					if (field_width == -1) {
						field_width = 2 * sizeof(void *);
						flags |= STRING_ZEROPAD;
					}
					switch (length) {
					case STRING_DEFAULT_LENGTH:
						result += format_integer(reinterpret_cast<uintptr_t>(va_arg(args, void *)), 16, field_width,
												 precision, flags);
						break;
					default:
						return {};
					}
					continue;
				case 'A':
					flags |= STRING_LARGE;
					[[fallthrough]];
				case 'a':
					switch (length) {
					case STRING_DEFAULT_LENGTH:
						result += format_float(va_arg(args, double), 16, DECIMAL_FORMAT, field_width, precision, flags);
						break;
					case STRING_LONG_DOUBLE_LENGTH:
						result += format_float(va_arg(args, long double), 16, DECIMAL_FORMAT, field_width, precision,
											   flags);
						break;
					default:
						return {};
					}
					continue;
				case 'B':
					flags |= STRING_LARGE;
					[[fallthrough]];
				case 'b':
					base = 2;
					break;
				case 'O':
					flags |= STRING_LARGE;
					[[fallthrough]];
				case 'o':
					base = 8;
					break;
				case 'X':
					flags |= STRING_LARGE;
					[[fallthrough]];
				case 'x':
					base = 16;
					break;
				case 'd':
				case 'i':
					flags |= STRING_SIGN;
					break;
				case 'u':
					break;
				case 'E':
					flags |= STRING_LARGE;
					[[fallthrough]];
				case 'e':
					switch (length) {
					case STRING_DEFAULT_LENGTH:
						result += format_float(va_arg(args, double), 10, SCIENTIFIC_FORMAT, field_width, precision,
											   flags | STRING_SIGN);
						break;
					case STRING_LONG_DOUBLE_LENGTH:
						result += format_float(va_arg(args, long double), 10, SCIENTIFIC_FORMAT, field_width, precision,
											   flags | STRING_SIGN);
						break;
					default:
						return {};
					}
					continue;
				case 'F':
					flags |= STRING_LARGE;
					[[fallthrough]];
				case 'f':
					switch (length) {
					case STRING_DEFAULT_LENGTH:
						result += format_float(va_arg(args, double), 10, DECIMAL_FORMAT, field_width, precision,
											   flags | STRING_SIGN);
						break;
					case STRING_LONG_DOUBLE_LENGTH:
						result += format_float(va_arg(args, long double), 10, DECIMAL_FORMAT, field_width, precision,
											   flags | STRING_SIGN);
						break;
					default:
						return {};
					}
					continue;
				case 'G':
					flags |= STRING_LARGE;
					[[fallthrough]];
				case 'g':
					switch (length) {
					case STRING_DEFAULT_LENGTH:
						result += format_float(va_arg(args, double), 10, SHORTEST_FORMAT, field_width, precision,
											   flags | STRING_SIGN);
						break;
					case STRING_LONG_DOUBLE_LENGTH:
						result += format_float(va_arg(args, long double), 10, SHORTEST_FORMAT, field_width, precision,
											   flags | STRING_SIGN);
						break;
					default:
						return {};
					}
					continue;
				default:
					result += *cptr;
					continue;
				}

				if (flags & STRING_SIGN) {
					switch (length) {
					case STRING_DEFAULT_LENGTH:
						result += format_integer(va_arg(args, int), base, field_width, precision, flags);
						break;
					case STRING_BYTE_LENGTH:
						result += format_integer(va_arg(args, signed char), base, field_width, precision, flags);
						break;
					case STRING_HALF_LENGTH:
						result += format_integer(va_arg(args, short int), base, field_width, precision, flags);
						break;
					case STRING_LONG_LENGTH:
						result += format_integer(va_arg(args, long int), base, field_width, precision, flags);
						break;
					case STRING_LONG_LONG_LENGTH:
						result += format_integer(va_arg(args, long long int), base, field_width, precision, flags);
						break;
					case STRING_MAX_LENGTH:
						result += format_integer(va_arg(args, std::intmax_t), base, field_width, precision, flags);
						break;
					case STRING_SIZE_LENGTH:
						result += format_integer(va_arg(args, std::size_t), base, field_width, precision, flags);
						break;
					case STRING_PTRDIFF_LENGTH:
						result += format_integer(va_arg(args, std::ptrdiff_t), base, field_width, precision, flags);
						break;
					default:
						return {};
					}
				}
				else {
					switch (length) {
					case STRING_DEFAULT_LENGTH:
						result += format_integer(va_arg(args, unsigned int), base, field_width, precision, flags);
						break;
					case STRING_BYTE_LENGTH:
						result += format_integer(va_arg(args, unsigned char), base, field_width, precision, flags);
						break;
					case STRING_HALF_LENGTH:
						result += format_integer(va_arg(args, unsigned short int), base, field_width, precision, flags);
						break;
					case STRING_LONG_LENGTH:
						result += format_integer(va_arg(args, unsigned long int), base, field_width, precision, flags);
						break;
					case STRING_LONG_LONG_LENGTH:
						result += format_integer(va_arg(args, unsigned long long int), base, field_width, precision,
												 flags);
						break;
					case STRING_MAX_LENGTH:
						result += format_integer(va_arg(args, std::uintmax_t), base, field_width, precision, flags);
						break;
					case STRING_SIZE_LENGTH:
						result += format_integer(va_arg(args, std::size_t), base, field_width, precision, flags);
						break;
					case STRING_PTRDIFF_LENGTH:
						result += format_integer(va_arg(args, std::ptrdiff_t), base, field_width, precision, flags);
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

std::string mint::to_string(intmax_t value) {
	return format_integer(value, 10, -1, -1, STRING_SIGN);
}

std::string mint::to_string(double value, DigitsFormat format) {
	return format_float(value, 10, format, -1, -1, STRING_SIGN);
}

std::string mint::to_string(const void *value) {
	char buffer[(sizeof(void *) * 2) + 3];
	sprintf(buffer, "0x%0*" PRIXPTR, static_cast<int>(sizeof(decltype(value)) * 2), reinterpret_cast<uintptr_t>(value));
	return buffer;
}

bool mint::starts_with(const std::string &str, const std::string &pattern) {
	const auto pattern_size = pattern.size();
	if (str.size() < pattern_size) {
		return false;
	}
	return std::string::traits_type::compare(str.data(), pattern.data(), pattern_size) == 0;
}

bool mint::ends_with(const std::string &str, const std::string &pattern) {
	const auto pattern_size = pattern.size();
	if (str.size() < pattern_size) {
		return false;
	}
	return std::string::traits_type::compare(str.data() + (str.size() - pattern_size), pattern.data(), pattern_size)
		   == 0;
}

void mint::force_decimal_point(std::string &buffer) {

	std::string::iterator cptr = buffer.begin();

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

void mint::crop_zeros(std::string &buffer) {

	std::string::iterator stop = buffer.end();
	std::string::iterator start = buffer.begin();

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
