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

#ifndef MINT_ASSERT_H
#define MINT_ASSERT_H

#include "mint/config.h"

#include <cassert>
#include <utility>

#ifdef BUILD_TYPE_DEBUG
#ifdef OS_WINDOWS
#define assert_x(expr, where, what) _ASSERT_EXPR(expr, L#where L":" L#what)
#else
extern void __assert_x_fail(const char *__assertion, const char *__file, unsigned int __line, const char *__function,
							const char *__where, const char *__what) __THROW __attribute__((__noreturn__));

#define assert_x(expr, where, what) \
	(static_cast<bool>(expr) ? void(0) : __assert_x_fail(#expr, __FILE__, __LINE__, __ASSERT_FUNCTION, where, what))
#endif
#else
#define assert_x(expr, where, what)
#endif

namespace mint {

template<typename Exception, typename Type, typename... Args>
Type *assert_not_null(Type *value, Args &&...args) {
	if (UNLIKELY(value == nullptr)) {
		throw Exception(std::forward<Args>(args)...);
	}
	return value;
}

}

#endif // MINT_ASSERT_H
