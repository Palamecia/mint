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

#ifndef MINT_STDIO_H
#define MINT_STDIO_H

#include "mint/config.h"

#include <cstdio>

#ifdef OS_WINDOWS
#include <Windows.h>

using ssize_t = SSIZE_T;

MINT_EXPORT ssize_t getline(char **lineptr, size_t *n, FILE *stream);
MINT_EXPORT ssize_t getdelim(char **lineptr, size_t *n, int delim, FILE *stream);
#endif

namespace mint {

MINT_EXPORT int printf(FILE *stream, const char *format, ...) __attribute__((format(printf, 2, 3)));
MINT_EXPORT int vprintf(FILE *stream, const char *format, va_list args);
MINT_EXPORT int print(FILE *stream, const char *str);

}

#endif // MINT_STDIO_H
