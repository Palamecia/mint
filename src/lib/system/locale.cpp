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

#include <mint/memory/functiontool.h>
#include <mint/memory/casttool.h>
#include <locale>
#include <array>

#ifdef OS_WINDOWS
#include "win32/winlocale.h"
#else
#include <langinfo.h>
#endif

using namespace mint;

/**
@see https://man7.org/linux/man-pages/man5/locale.5.html
@see https://docs.microsoft.com/en-us/windows/win32/intl/national-language-support
*/

#ifdef OS_WINDOWS
using Locale = std::remove_pointer<MSVCRT__locale_t>::type;
#else
using Locale = std::remove_pointer<locale_t>::type;
#endif

MINT_FUNCTION(mint_locale_current_name, 0, cursor) {
	FunctionHelper helper(cursor, 0);
	helper.return_value(create_string(std::locale().name()));
}

MINT_FUNCTION(mint_locale_set_current_name, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &name = helper.pop_parameter();

	std::string name_str = to_string(name);
	if (setlocale(LC_ALL, name_str.c_str()) == nullptr) {
		helper.return_value(create_number(errno));
	}
}

MINT_FUNCTION(mint_locale_list, 0, cursor) {

	FunctionHelper helper(cursor, 0);
	WeakReference result = create_array();

#ifdef OS_WINDOWS
	if (EnumSystemLocalesEx(
			[](LPWSTR name, DWORD flags, LPARAM result) -> BOOL {
				std::array<char, 255> locale_name {};
				WideCharToMultiByte(CP_UTF8, 0, name, -1, locale_name.data(), locale_name.size(), nullptr, nullptr);
				array_append(((WeakReference *)result)->data<Array>(), create_string(locale_name.data()));
				return TRUE;
			},
			LOCALE_ALL, LPARAM(&result), NULL)) {
		helper.return_value(std::move(result));
	}
#else

	helper.return_value(std::move(result));
#endif
}

MINT_FUNCTION(mint_locale_create, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &name = helper.pop_parameter();

	std::string name_str = to_string(name);
#ifdef OS_WINDOWS
	if (MSVCRT__locale_t locale = MSVCRT__create_locale(MSVCRT_LC_ALL, name_str.c_str())) {
		helper.return_value(create_object(locale));
	}
#else
	if (locale_t locale = newlocale(LC_ALL_MASK, name_str.c_str(), nullptr)) {
		helper.return_value(create_object(locale));
	}
#endif
}

MINT_FUNCTION(mint_locale_delete, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &locale = helper.pop_parameter();

#ifdef OS_WINDOWS
	MSVCRT__free_locale(locale.data<LibObject<std::remove_pointer<MSVCRT__locale_t>::type>>()->impl);
#else
	freelocale(locale.data<LibObject<std::remove_pointer<locale_t>::type>>()->impl);
#endif
}

MINT_FUNCTION(mint_locale_day_name, 3, cursor) {

	FunctionHelper helper(cursor, 3);
	Reference &format = helper.pop_parameter();
	Reference &day = helper.pop_parameter();
	Reference &locale = helper.pop_parameter();

	static const nl_item DAY_ITEM[2][7] = {
		{ABDAY_1, ABDAY_2, ABDAY_3, ABDAY_4, ABDAY_5, ABDAY_6, ABDAY_7},
		{DAY_1, DAY_2, DAY_3, DAY_4, DAY_5, DAY_6, DAY_7},
	};

	auto format_index = to_integer(cursor, format);
	auto day_index = to_integer(cursor, day);

	if ((day_index >= 0) && (day_index <= 6) && (format_index >= 0) && (format_index <= 1)) {
		helper.return_value(
			create_string(nl_langinfo_l(DAY_ITEM[format_index][day_index], locale.data<LibObject<Locale>>()->impl)));
	}
}

MINT_FUNCTION(mint_locale_month_name, 3, cursor) {

	FunctionHelper helper(cursor, 3);
	Reference &format = helper.pop_parameter();
	Reference &month = helper.pop_parameter();
	Reference &locale = helper.pop_parameter();

	static const nl_item MONTH_ITEM[2][12] = {
		{ABMON_1, ABMON_2, ABMON_3, ABMON_4, ABMON_5, ABMON_6, ABMON_7, ABMON_8, ABMON_9, ABMON_10, ABMON_11, ABMON_12},
		{MON_1, MON_2, MON_3, MON_4, MON_5, MON_6, MON_7, MON_8, MON_9, MON_10, MON_11, MON_12},
	};

	auto format_index = to_integer(cursor, format);
	auto month_index = to_integer(cursor, month);

	if ((month_index >= 1) && (month_index <= 12) && (format_index >= 0) && (format_index <= 1)) {
		helper.return_value(create_string(
			nl_langinfo_l(MONTH_ITEM[format_index][month_index - 1], locale.data<LibObject<Locale>>()->impl)));
	}
}

MINT_FUNCTION(mint_locale_am_name, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &locale = helper.pop_parameter();

	helper.return_value(create_string(nl_langinfo_l(AM_STR, locale.data<LibObject<Locale>>()->impl)));
}

MINT_FUNCTION(mint_locale_pm_name, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &locale = helper.pop_parameter();

	helper.return_value(create_string(nl_langinfo_l(PM_STR, locale.data<LibObject<Locale>>()->impl)));
}

MINT_FUNCTION(mint_locale_date_format, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &format = helper.pop_parameter();
	Reference &locale = helper.pop_parameter();

	static const nl_item FORMAT_ITEM[4] = {D_T_FMT, D_FMT, T_FMT, T_FMT_AMPM};

	auto format_index = to_integer(cursor, format);

	if ((format_index >= 0) && (format_index < 4)) {
		helper.return_value(
			create_string(nl_langinfo_l(FORMAT_ITEM[format_index], locale.data<LibObject<Locale>>()->impl)));
	}
}
