#include "memory/functiontool.h"
#include "memory/casttool.h"
#include "system/assert.h"
#include "system/errno.h"

#include <cstdlib>

#ifdef OS_WINDOWS
#include <Windows.h>
#endif

using namespace std;
using namespace mint;

#ifdef OS_WINDOWS
static wstring utf8_to_windows(const string &str) {

	int length = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
	wchar_t *buffer = static_cast<wchar_t *>(alloca(length * sizeof(wchar_t)));

	if (MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, buffer, length)) {
		return buffer;
	}

	return wstring(str.begin(), str.end());
}

static string windows_to_utf8(const wstring &str) {

	int length = WideCharToMultiByte(CP_UTF8, 0, str.c_str(), -1, nullptr, 0, nullptr, nullptr);
	char *buffer = static_cast<char *>(alloca(length * sizeof(char)));

	if (WideCharToMultiByte(CP_UTF8, 0, str.c_str(), -1, buffer, length, nullptr, nullptr)) {
		return buffer;
	}

	return string(str.begin(), str.end());
}
#endif

namespace symbols {
static const Symbol System("System");
static const Symbol OSType("OSType");
static const Symbol Linux("Linux");
static const Symbol Windows("Windows");
static const Symbol MacOS("MacOs");
}

MINT_FUNCTION(mint_os_get_type, 0, cursor) {

	FunctionHelper helper(cursor, 0);
	ReferenceHelper OSType = helper.reference(symbols::System).member(symbols::OSType);

#if defined (OS_UNIX)
	helper.returnValue(OSType.member(symbols::Linux));
#elif defined (OS_WINDOWS)
	helper.returnValue(OSType.member(symbols::Windows));
#elif defined (OS_MAC)
	helper.returnValue(OSType.member(symbols::MacOS));
#else
	assert_x(false, "mint_os_get_type", "unsuported operating system");
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
	WeakReference default_value = move(helper.popParameter());
	Reference &name = helper.popParameter();

#ifdef OS_WINDOWS
	wchar_t buffer[32767];

	wstring name_str = utf8_to_windows(to_string(name));
	if (GetEnvironmentVariableW(name_str.c_str(), buffer, sizeof(buffer))) {
		helper.returnValue(create_string(windows_to_utf8(buffer)));
	}
#else
	string name_str = to_string(name);
	if (const char *value = getenv(name_str.c_str())) {
		helper.returnValue(create_string(value));
	}
#endif
	else {
		helper.returnValue(move(default_value));
	}
}

MINT_FUNCTION(mint_os_set_environment, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &value = helper.popParameter();
	Reference &name = helper.popParameter();

#ifdef OS_WINDOWS
	wstring name_str = utf8_to_windows(to_string(name));
	wstring value_str = utf8_to_windows(to_string(value));
	if (!SetEnvironmentVariableW(name_str.c_str(), value_str.c_str())) {
		helper.returnValue(create_number(errno_from_windows_last_error()));
	}
#else
	string name_str = to_string(name);
	string value_str = to_string(value);
	if (setenv(name_str.c_str(), value_str.c_str(), true)) {
		helper.returnValue(create_number(errno));
	}
#endif
}

MINT_FUNCTION(mint_os_unset_environment, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &name = helper.popParameter();

#ifdef OS_WINDOWS
	wstring name_str = utf8_to_windows(to_string(name));
	if (!SetEnvironmentVariableW(name_str.c_str(), nullptr)) {
		helper.returnValue(create_number(errno_from_windows_last_error()));
	}
#else
	string name_str = to_string(name);
	if (unsetenv(name_str.c_str())) {
		helper.returnValue(create_number(errno));
	}
#endif
}
