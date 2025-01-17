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

#include "mint/memory/functiontool.h"
#include "mint/memory/casttool.h"
#include "mint/system/assert.h"
#include "mint/system/errno.h"

#include <cstdlib>

#ifdef OS_WINDOWS
#include <Windows.h>
#endif

using namespace mint;

namespace {

#ifdef OS_WINDOWS
std::wstring utf8_to_windows(const std::string &str) {

	std::wstring buffer(MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0), L'\0');

	if (MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, buffer.data(), buffer.length())) {
		return buffer;
	}

	return {};
}

std::string windows_to_utf8(const std::wstring &str) {

	std::string buffer(WideCharToMultiByte(CP_UTF8, 0, str.c_str(), -1, nullptr, 0, nullptr, nullptr), '\0');

	if (WideCharToMultiByte(CP_UTF8, 0, str.c_str(), -1, buffer.data(), buffer.length(), nullptr, nullptr)) {
		return buffer;
	}

	return {};
}
#endif

}

namespace symbols {
static const Symbol System("System");
static const Symbol OSType("OSType");
static const Symbol Linux("Linux");
static const Symbol Windows("Windows");
static const Symbol MacOS("MacOs");
}

MINT_FUNCTION(mint_os_get_type, 0, cursor) {

	FunctionHelper helper(cursor, 0);
	const ReferenceHelper OSType = helper.reference(symbols::System).member(symbols::OSType);

#if defined(OS_UNIX)
	helper.return_value(OSType.member(symbols::Linux));
#elif defined(OS_WINDOWS)
	helper.return_value(OSType.member(symbols::Windows));
#elif defined(OS_MAC)
	helper.return_value(OSType.member(symbols::MacOS));
#else
	assert_x(false, "mint_os_get_type", "unsupported operating system");
#endif
}

MINT_FUNCTION(mint_os_get_name, 0, cursor) {

	FunctionHelper helper(cursor, 0);
}

MINT_FUNCTION(mint_os_get_version, 0, cursor) {

	FunctionHelper helper(cursor, 0);
}

MINT_FUNCTION(mint_os_get_environment, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference default_value = std::move(helper.pop_parameter());
	const Reference &name = helper.pop_parameter();

#ifdef OS_WINDOWS
	wchar_t buffer[32767];

	std::wstring name_str = utf8_to_windows(to_string(name));
	if (GetEnvironmentVariableW(name_str.c_str(), buffer, sizeof(buffer))) {
		helper.return_value(create_string(windows_to_utf8(buffer)));
	}
#else
	std::string name_str = to_string(name);
	if (const char *value = getenv(name_str.c_str())) {
		helper.return_value(create_string(value));
	}
#endif
	else {
		helper.return_value(std::move(default_value));
	}
}

MINT_FUNCTION(mint_os_set_environment, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	const Reference &value = helper.pop_parameter();
	const Reference &name = helper.pop_parameter();

#ifdef OS_WINDOWS
	std::wstring name_str = utf8_to_windows(to_string(name));
	std::wstring value_str = utf8_to_windows(to_string(value));
	if (!SetEnvironmentVariableW(name_str.c_str(), value_str.c_str())) {
		helper.return_value(create_number(errno_from_windows_last_error()));
	}
#else
	std::string name_str = to_string(name);
	std::string value_str = to_string(value);
	if (setenv(name_str.c_str(), value_str.c_str(), true)) {
		helper.return_value(create_number(errno));
	}
#endif
}

MINT_FUNCTION(mint_os_unset_environment, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &name = helper.pop_parameter();

#ifdef OS_WINDOWS
	std::wstring name_str = utf8_to_windows(to_string(name));
	if (!SetEnvironmentVariableW(name_str.c_str(), nullptr)) {
		helper.return_value(create_number(errno_from_windows_last_error()));
	}
#else
	std::string name_str = to_string(name);
	if (unsetenv(name_str.c_str())) {
		helper.return_value(create_number(errno));
	}
#endif
}
