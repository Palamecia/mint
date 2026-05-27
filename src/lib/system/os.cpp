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

#include "mint/ast/symbol.h"
#include "mint/memory/function_tools.h"
#include "mint/memory/cast_tools.h"
#include "mint/memory/reference.h"
#include "mint/system/errno.h"
#include <string>
#include <utility>

#ifdef MINT_OS_WINDOWS
#include <array>
#include <Windows.h>
#include <processenv.h>
#include <stringapiset.h>
#include <winnls.h>
#else
#undef linux
#endif

namespace symbols {

static const mint::Symbol system("System");
static const mint::Symbol os_type("OSType");
static const mint::Symbol linux("Linux");
static const mint::Symbol windows("Windows");
static const mint::Symbol mac_os("MacOs");

}

namespace {

#ifdef MINT_OS_WINDOWS
std::wstring utf8_to_windows(const std::string& str) {
	const int length = MultiByteToWideChar(CP_UTF8, 0, str.data(), -1, nullptr, 0);
	if (std::wstring buffer(length, L'\0'); MultiByteToWideChar(CP_UTF8, 0, str.data(), -1, buffer.data(), length)) {
		buffer.resize(length - 1);
		return buffer;
	}
	return {};
}

std::string windows_to_utf8(const std::wstring& str) {
	const int length = WideCharToMultiByte(CP_UTF8, 0, str.data(), -1, nullptr, 0, nullptr, nullptr);
	if (std::string buffer(length, '\0');
	    WideCharToMultiByte(CP_UTF8, 0, str.data(), -1, buffer.data(), length, nullptr, nullptr)) {
		buffer.resize(length - 1);
		return buffer;
	}
	return {};
}
#endif

mint::Reference mint_os_get_type(mint::FunctionHelper& helper) {

	const mint::ReferenceHelper os_type = helper.reference(symbols::system).member(symbols::os_type);

#ifdef MINT_OS_UNIX
	return os_type.member(symbols::linux).share();
#elifdef MINT_OS_WINDOWS
	return os_type.member(symbols::windows).share();
#elifdef MINT_OS_MAC
	return os_type.member(symbols::mac_os).share();
#else
	assert_x(false, __func__, "unsupported operating system");
#endif
}

mint::Reference mint_os_get_name(mint::Cursor& /*cursor*/) {
	return {};
}

mint::Reference mint_os_get_version(mint::Cursor& /*cursor*/) {
	return {};
}

mint::Reference mint_os_get_environment(mint::Cursor& cursor, const mint::Reference& name,
    mint::Reference& default_value) {
#ifdef MINT_OS_WINDOWS
	std::array<wchar_t, 32767> buffer;
	std::wstring name_str = utf8_to_windows(to_string(name));
	if (GetEnvironmentVariableW(name_str.data(), buffer.data(), buffer.size())) {
		return mint::create_string(cursor.ast(), windows_to_utf8(buffer.data()));
	}
#else
	const auto name_str = to_string(name);
	if (const char* value = std::getenv(name_str.c_str())) {
		return mint::create_string(cursor.ast(), value);
	}
#endif
	return std::move(default_value);
}

mint::Reference mint_os_set_environment(mint::Cursor& /*cursor*/, const mint::Reference& name,
    const mint::Reference& value) {
#ifdef MINT_OS_WINDOWS
	std::wstring name_str = utf8_to_windows(to_string(name));
	std::wstring value_str = utf8_to_windows(to_string(value));
	if (!SetEnvironmentVariableW(name_str.c_str(), value_str.c_str())) {
		return mint::create_number(mint::errno_from_error_code(mint::last_error_code()));
	}
#else
	const auto name_str = to_string(name);
	const auto value_str = to_string(value);
	if (setenv(name_str.c_str(), value_str.c_str(), true)) {
		return mint::create_number(errno);
	}
#endif
	return {};
}

mint::Reference mint_os_unset_environment(mint::Cursor& /*cursor*/, const mint::Reference& name) {
#ifdef MINT_OS_WINDOWS
	std::wstring name_str = utf8_to_windows(to_string(name));
	if (!SetEnvironmentVariableW(name_str.c_str(), nullptr)) {
		return mint::create_number(mint::errno_from_error_code(mint::last_error_code()));
	}
#else
	const auto name_str = to_string(name);
	if (unsetenv(name_str.c_str())) {
		return mint::create_number(errno);
	}
#endif
	return {};
}

}

MINT_EXPORT_FUNCTION(mint_os_get_type, 0)
MINT_EXPORT_FUNCTION(mint_os_get_name, 0)
MINT_EXPORT_FUNCTION(mint_os_get_version, 0)
MINT_EXPORT_FUNCTION(mint_os_get_environment, 2)
MINT_EXPORT_FUNCTION(mint_os_set_environment, 2)
MINT_EXPORT_FUNCTION(mint_os_unset_environment, 1)
