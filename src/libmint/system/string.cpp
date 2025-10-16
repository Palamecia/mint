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

#include "mint/system/string.h"

#include <bit>
#include <cstdint>
#include <format>
#include <string>

using namespace mint;

std::string mint::to_string(std::intmax_t value) {
	return format_integer(value, decimal_base, unknown_size, unknown_precision, string_sign);
}

std::string mint::to_string(std::uintmax_t value) {
	return format_integer(value, decimal_base, unknown_size, unknown_precision);
}

std::string mint::to_string(double value, DigitsFormat format) {
	return format_float(value, decimal_base, format, unknown_size, unknown_precision, string_sign);
}

std::string mint::to_string(const void* value) {
	return std::format("0x{:0{}X}", std::bit_cast<uintptr_t>(value), sizeof(decltype(value)) * 2);
}

void mint::force_decimal_point(std::string& buffer) {

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

void mint::crop_zeros(std::string& buffer) {

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
