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

#include "mint/system/stdio.h"

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
