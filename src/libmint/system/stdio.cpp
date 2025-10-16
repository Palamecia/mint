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

#include "mint/system/stdio.h"
#include "mint/system/errno.h"
#include "mint/system/pipe.h"
#include "mint/system/terminal.h"

#include <cstdio>
#include <string>
#include <system_error>

std::string mint::get_line(FILE* stream) {
	return get_delim('\n', stream);
}

std::string mint::get_delim(int delim, FILE* stream) {

	std::string buffer = {};

	for (int c = fgetc(stream); c != EOF; c = fgetc(stream)) {
		buffer += static_cast<char>(c);
		if (c == delim) {
			break;
		}
	}

	return buffer;
}

void mint::print(FILE* stream, const std::string& str) {
	if (is_term(stream)) {
		Terminal::print(stream, str);
	}
	else if (is_pipe(stream)) {
		Pipe::print(stream, str);
	}
	else if (std::fputs(str.data(), stream) == EOF) {
		throw std::system_error(last_error_code());
	}
}
