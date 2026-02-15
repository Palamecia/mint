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

#include "mint/config.h"
#include "mint/system/assert.h"
#include <format>
#include <string>

#if defined(MINT_BUILD_TYPE_DEBUG)
#if defined(MINT_OS_WINDOWS)

#include <Windows.h>
#include <crtdbg.h>
#include <stringapiset.h>
#include <winnls.h>

std::wstring wchar_from_multi_byte(const std::string& str) {
	const int length = MultiByteToWideChar(CP_UTF8, 0, str.data(), -1, nullptr, 0);
	if (std::wstring buffer(length, L'\0'); MultiByteToWideChar(CP_UTF8, 0, str.data(), -1, buffer.data(), length)) {
		buffer.resize(length - 1);
		return buffer;
	}
	return {};
}

void __assert_x_fail(const char* __file, unsigned int __line, const char* __where, const char* __what) {
	(1
	    != _CrtDbgReportW(_CRT_ASSERT, wchar_from_multi_byte(__file).data(), __line, nullptr, L"%ls",
	        wchar_from_multi_byte(std::format("{}: {}", __where, __what)).data()))
	    || (_CrtDbgBreak(), 0);
}
#else

#include "mint/system/terminal.h"

void __assert_x_fail(const char* __assertion, const char* __file, unsigned int __line, const char* __function,
    const char* __where, const char* __what) __THROW {
	mint::Terminal::print(stderr, std::format("{}: {}\n", __where, __what));
	__assert_fail(__assertion, __file, __line, __function);
}
#endif
#endif
