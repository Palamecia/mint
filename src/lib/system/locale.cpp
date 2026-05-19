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

#include "mint/memory/builtin/array.h"
#include "mint/memory/builtin/libobject.h"
#include "mint/memory/functiontool.h"
#include "mint/memory/casttool.h"
#include "mint/memory/reference.h"
#include <array>
#include <cerrno>
#include <clocale>
#include <cstddef>
#include <cstdint>
#include <locale>
#include <span>
#include <type_traits>
#include <unicode/locid.h>

#ifdef MINT_OS_WINDOWS
#include "win32/winlocale.h"
#else
#include <bits/types/locale_t.h>
#include <langinfo.h>
#include <locale.h>
#include <nl_types.h>
#endif

/**
@see https://man7.org/linux/man-pages/man5/locale.5.html
@see https://docs.microsoft.com/en-us/windows/win32/intl/national-language-support
*/

namespace {

#ifdef MINT_OS_WINDOWS
using Locale = std::remove_pointer_t<MSVCRT__locale_t>;
#else
using Locale = std::remove_pointer_t<locale_t>;
#endif

mint::Reference mint_locale_current_name(mint::Cursor& cursor) {
	return mint::create_string(cursor.ast(), std::locale().name());
}

mint::Reference mint_locale_set_current_name(mint::Cursor& /*cursor*/, const mint::Reference& name) {
	if (std::setlocale(LC_ALL, to_string(name).c_str()) == nullptr) {
		return mint::create_number(errno);
	}
	return {};
}

mint::Reference mint_locale_list(mint::Cursor& cursor) {
	if (std::int32_t count = 0; const auto* locales = icu::Locale::getAvailableLocales(count)) {
		mint::Reference result = mint::create_array(cursor.ast());
		for (const auto& locale : std::span(locales, count)) {
			array_append(result.data<mint::Array>(), mint::create_string(cursor.ast(), locale.getName()));
		}
		return result;
	}
	return {};
}

mint::Reference mint_locale_create(mint::Cursor& cursor, const mint::Reference& name) {
#ifdef MINT_OS_WINDOWS
	if (MSVCRT__locale_t locale = MSVCRT__create_locale(MSVCRT_LC_ALL, to_string(name).c_str())) {
		return mint::create_c_object(cursor.ast(), locale);
	}
#else
	if (locale_t locale = newlocale(LC_ALL_MASK, to_string(name).c_str(), nullptr)) {
		return mint::create_c_object(cursor.ast(), locale);
	}
#endif
	return {};
}

mint::Reference mint_locale_delete(mint::Cursor& /*cursor*/, const mint::Reference& locale) {
#ifdef MINT_OS_WINDOWS
	MSVCRT__free_locale(locale.data<mint::LibObject<std::remove_pointer_t<MSVCRT__locale_t>>>().ptr);
#else
	freelocale(locale.data<mint::LibObject<std::remove_pointer_t<locale_t>>>().ptr);
#endif
	return {};
}

mint::Reference mint_locale_day_name(mint::Cursor& cursor, const mint::Reference& locale,
    const mint::Reference& day, mint::Reference& format) {

	static constexpr std::size_t day_count = 7;
	static const std::array<std::array<nl_item, day_count>, 2> day_item {{
	    {ABDAY_1, ABDAY_2, ABDAY_3, ABDAY_4, ABDAY_5, ABDAY_6, ABDAY_7},
	    {DAY_1, DAY_2, DAY_3, DAY_4, DAY_5, DAY_6, DAY_7},
	}};

	const auto format_index = mint::to_integer<int>(cursor, format);
	const auto day_index = mint::to_integer<int>(cursor, day);

	if ((day_index >= 0) && (day_index < day_count) && (format_index >= 0) && (format_index <= 1)) {
		return mint::create_string(cursor.ast(),
		    nl_langinfo_l(day_item[format_index][day_index], locale.data<mint::LibObject<Locale>>().ptr));
	}

	return {};
}

mint::Reference mint_locale_month_name(mint::Cursor& cursor, const mint::Reference& locale,
    const mint::Reference& month, mint::Reference& format) {

	static constexpr std::size_t month_count = 12;
	static const std::array<std::array<nl_item, month_count>, 2> month_item {{
	    {ABMON_1, ABMON_2, ABMON_3, ABMON_4, ABMON_5, ABMON_6, ABMON_7, ABMON_8, ABMON_9, ABMON_10, ABMON_11, ABMON_12},
	    {MON_1, MON_2, MON_3, MON_4, MON_5, MON_6, MON_7, MON_8, MON_9, MON_10, MON_11, MON_12},
	}};

	const auto format_index = mint::to_integer<int>(cursor, format);
	const auto month_index = mint::to_integer<int>(cursor, month);

	if ((month_index >= 1) && (month_index <= month_count) && (format_index >= 0) && (format_index <= 1)) {
		return mint::create_string(cursor.ast(),
		    nl_langinfo_l(month_item[format_index][month_index - 1], locale.data<mint::LibObject<Locale>>().ptr));
	}

	return {};
}

mint::Reference mint_locale_am_name(mint::Cursor& cursor, const mint::Reference& locale) {
	return mint::create_string(cursor.ast(), nl_langinfo_l(AM_STR, locale.data<mint::LibObject<Locale>>().ptr));
}

mint::Reference mint_locale_pm_name(mint::Cursor& cursor, const mint::Reference& locale) {
	return mint::create_string(cursor.ast(), nl_langinfo_l(PM_STR, locale.data<mint::LibObject<Locale>>().ptr));
}

mint::Reference mint_locale_date_format(mint::Cursor& cursor, const mint::Reference& locale,
    const mint::Reference& format) {

	static constexpr std::size_t format_count = 4;
	static const std::array<nl_item, format_count> format_item = {D_T_FMT, D_FMT, T_FMT, T_FMT_AMPM};

	const auto format_index = mint::to_integer<int>(cursor, format);

	if ((format_index >= 0) && (format_index < format_count)) {
		return mint::create_string(cursor.ast(),
		    nl_langinfo_l(format_item[format_index], locale.data<mint::LibObject<Locale>>().ptr));
	}

	return {};
}

}

MINT_EXPORT_FUNCTION(mint_locale_current_name, 0);
MINT_EXPORT_FUNCTION(mint_locale_set_current_name, 1);
MINT_EXPORT_FUNCTION(mint_locale_list, 0);
MINT_EXPORT_FUNCTION(mint_locale_create, 1);
MINT_EXPORT_FUNCTION(mint_locale_delete, 1);
MINT_EXPORT_FUNCTION(mint_locale_day_name, 3);
MINT_EXPORT_FUNCTION(mint_locale_month_name, 3);
MINT_EXPORT_FUNCTION(mint_locale_am_name, 1);
MINT_EXPORT_FUNCTION(mint_locale_pm_name, 1);
MINT_EXPORT_FUNCTION(mint_locale_date_format, 2);
