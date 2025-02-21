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

#include "mint/system/stdio.h"
#include "mint/system/pipe.h"
#include "mint/system/terminal.h"

#include <cstdarg>

#ifdef OS_WINDOWS
ssize_t getline(char **lineptr, size_t *n, FILE *stream) {
	return getdelim(lineptr, n, '\n', stream);
}

ssize_t getdelim(char **lineptr, size_t *n, int delim, FILE *stream) {

	if (lineptr == nullptr) {
		return -1;
	}

	if (stream == nullptr) {
		return -1;
	}

	if (n == nullptr) {
		return -1;
	}

	char *bufptr = *lineptr;
	size_t size = *n;

	int c = fgetc(stream);

	if (c == EOF) {
		return -1;
	}
	if (bufptr == nullptr) {

		bufptr = static_cast<char *>(malloc(128));

		if (bufptr == nullptr) {
			return -1;
		}

		size = 128;
	}

	char *cptr = bufptr;

	while (c != EOF) {
		const size_t pos = size_t(cptr - bufptr);
		if (pos > (size - 1)) {
			size = size + 128;
			bufptr = static_cast<char *>(realloc(bufptr, size));
			if (bufptr == nullptr) {
				return -1;
			}
			cptr = bufptr + pos;
		}
		*cptr++ = c;
		if (c == delim) {
			break;
		}
		c = fgetc(stream);
	}

	*cptr++ = '\0';
	*lineptr = bufptr;
	*n = size;

	return cptr - bufptr - 1;
}
#endif

int mint::printf(FILE *stream, const char *format, ...) {
	va_list args;
	va_start(args, format);
	int written = mint::vprintf(stream, format, args);
	va_end(args);
	return written;
}

int mint::vprintf(FILE *stream, const char *format, va_list args) {
	if (is_term(stream)) {
		return Terminal::vprintf(stream, format, args);
	}
	if (is_pipe(stream)) {
		return Pipe::vprintf(stream, format, args);
	}
	return vfprintf(stream, format, args);
}

int mint::print(FILE *stream, const char *str) {
	if (is_term(stream)) {
		return Terminal::print(stream, str);
	}
	if (is_pipe(stream)) {
		return Pipe::print(stream, str);
	}
	return fputs(str, stream);
}
