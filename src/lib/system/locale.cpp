/**
 * Copyright (c) 2026 Gauvain CHÉRY.
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

#include "locale_data.h"
#include "mint/memory/builtin/array.h"
#include "mint/memory/builtin/libobject.h"
#include "mint/memory/function_tools.h"
#include "mint/memory/cast_tools.h"
#include "mint/memory/reference.h"
#include <cerrno>
#include <clocale>
#include <corecrt.h>
#include <cstdint>
#include <memory>
#include <span>
#include <system_error>
#include <unicode/locid.h>

/**
@see https://man7.org/linux/man-pages/man5/locale.5.html
@see https://docs.microsoft.com/en-us/windows/win32/intl/national-language-support
*/

namespace {

mint::Reference mint_locale_set_current_name(mint::Cursor& /*cursor*/, const mint::Reference& name) {
	if (std::setlocale(LC_ALL, to_string(name).c_str()) == nullptr) {
		return mint::create_number(errno);
	}
	return {};
}

mint::Reference mint_locale_list(mint::Cursor& cursor) {
	if (std::int32_t count = 0; const auto* locales = icu::Locale::getAvailableLocales(count)) {
		mint::Reference result = mint::create_array(cursor.program());
		for (const auto& locale : std::span(locales, count)) {
			array_append(result.data<mint::Array>(), mint::create_string(cursor.program(), locale.getName()));
		}
		return result;
	}
	return {};
}

mint::Reference mint_locale_current(mint::Cursor& cursor) {
	try {
		auto locale = std::make_unique<mint_system::LocaleData>();
		return mint::create_iterator_from(cursor, mint::create_number(0),
		    mint::create_c_object(cursor.program(), locale.release()));
	}
	catch (const std::system_error& error) {
		return mint::create_iterator_from(cursor, mint::create_number(error.code().value()));
	}
}

mint::Reference mint_locale_create(mint::Cursor& cursor, const mint::Reference& name) {
	try {
		auto locale = std::make_unique<mint_system::LocaleData>(mint::to_string(name));
		return mint::create_iterator_from(cursor, mint::create_number(0),
		    mint::create_c_object(cursor.program(), locale.release()));
	}
	catch (const std::system_error& error) {
		return mint::create_iterator_from(cursor, mint::create_number(error.code().value()));
	}
}

mint::Reference mint_locale_delete(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr) {
	delete d_ptr.data<mint::LibObject<mint_system::LocaleData>>().ptr;
	return {};
}

mint::Reference mint_locale_abbreviated_day_name(mint::Cursor& cursor, const mint::Reference& d_ptr,
    const mint::Reference& day) {
	try {
		return mint::create_iterator_from(cursor, mint::create_number(0),
		    mint::create_string(cursor.program(),
		        d_ptr.data<mint::LibObject<mint_system::LocaleData>>().ptr->abbreviated_day_name(
		            mint::to_integer<int>(cursor, day))));
	}
	catch (const std::system_error& error) {
		return mint::create_iterator_from(cursor, mint::create_number(error.code().value()));
	}
}

mint::Reference mint_locale_day_name(mint::Cursor& cursor, const mint::Reference& d_ptr, const mint::Reference& day) {
	try {
		return mint::create_iterator_from(cursor, mint::create_number(0),
		    mint::create_string(cursor.program(), d_ptr.data<mint::LibObject<mint_system::LocaleData>>().ptr->day_name(
		                                              mint::to_integer<int>(cursor, day))));
	}
	catch (const std::system_error& error) {
		return mint::create_iterator_from(cursor, mint::create_number(error.code().value()));
	}
}

mint::Reference mint_locale_abbreviated_month_name(mint::Cursor& cursor, const mint::Reference& d_ptr,
    const mint::Reference& month) {
	try {
		return mint::create_iterator_from(cursor, mint::create_number(0),
		    mint::create_string(cursor.program(),
		        d_ptr.data<mint::LibObject<mint_system::LocaleData>>().ptr->abbreviated_month_name(
		            mint::to_integer<int>(cursor, month))));
	}
	catch (const std::system_error& error) {
		return mint::create_iterator_from(cursor, mint::create_number(error.code().value()));
	}
}

mint::Reference mint_locale_month_name(mint::Cursor& cursor, const mint::Reference& d_ptr,
    const mint::Reference& month) {
	try {
		return mint::create_iterator_from(cursor, mint::create_number(0),
		    mint::create_string(cursor.program(), d_ptr.data<mint::LibObject<mint_system::LocaleData>>().ptr->month_name(
		                                              mint::to_integer<int>(cursor, month))));
	}
	catch (const std::system_error& error) {
		return mint::create_iterator_from(cursor, mint::create_number(error.code().value()));
	}
}

mint::Reference mint_locale_am_name(mint::Cursor& cursor, const mint::Reference& d_ptr) {
	try {
		return mint::create_iterator_from(cursor, mint::create_number(0),
		    mint::create_string(cursor.program(),
		        d_ptr.data<mint::LibObject<mint_system::LocaleData>>().ptr->am_name()));
	}
	catch (const std::system_error& error) {
		return mint::create_iterator_from(cursor, mint::create_number(error.code().value()));
	}
}

mint::Reference mint_locale_pm_name(mint::Cursor& cursor, const mint::Reference& d_ptr) {
	try {
		return mint::create_iterator_from(cursor, mint::create_number(0),
		    mint::create_string(cursor.program(),
		        d_ptr.data<mint::LibObject<mint_system::LocaleData>>().ptr->pm_name()));
	}
	catch (const std::system_error& error) {
		return mint::create_iterator_from(cursor, mint::create_number(error.code().value()));
	}
}

mint::Reference mint_locale_date_format(mint::Cursor& cursor, const mint::Reference& d_ptr) {
	try {
		return mint::create_iterator_from(cursor, mint::create_number(0),
		    mint::create_string(cursor.program(),
		        d_ptr.data<mint::LibObject<mint_system::LocaleData>>().ptr->date_format()));
	}
	catch (const std::system_error& error) {
		return mint::create_iterator_from(cursor, mint::create_number(error.code().value()));
	}
}

mint::Reference mint_locale_time_format(mint::Cursor& cursor, const mint::Reference& d_ptr) {
	try {
		return mint::create_iterator_from(cursor, mint::create_number(0),
		    mint::create_string(cursor.program(),
		        d_ptr.data<mint::LibObject<mint_system::LocaleData>>().ptr->time_format()));
	}
	catch (const std::system_error& error) {
		return mint::create_iterator_from(cursor, mint::create_number(error.code().value()));
	}
}

mint::Reference mint_locale_date_time_format(mint::Cursor& cursor, const mint::Reference& d_ptr) {
	try {
		return mint::create_iterator_from(cursor, mint::create_number(0),
		    mint::create_string(cursor.program(),
		        d_ptr.data<mint::LibObject<mint_system::LocaleData>>().ptr->date_time_format()));
	}
	catch (const std::system_error& error) {
		return mint::create_iterator_from(cursor, mint::create_number(error.code().value()));
	}
}

mint::Reference mint_locale_decimal_point(mint::Cursor& cursor, const mint::Reference& d_ptr) {
	try {
		return mint::create_iterator_from(cursor, mint::create_number(0),
		    mint::create_iterator_from(cursor, mint::create_number(0),
		        mint::create_string(cursor.program(),
		            d_ptr.data<mint::LibObject<mint_system::LocaleData>>().ptr->decimal_point())));
	}
	catch (const std::system_error& error) {
		return mint::create_iterator_from(cursor, mint::create_number(error.code().value()));
	}
}

mint::Reference mint_locale_thousands_separator(mint::Cursor& cursor, const mint::Reference& d_ptr) {
	try {
		return mint::create_iterator_from(cursor, mint::create_number(0),
		    mint::create_string(cursor.program(),
		        d_ptr.data<mint::LibObject<mint_system::LocaleData>>().ptr->thousands_separator()));
	}
	catch (const std::system_error& error) {
		return mint::create_iterator_from(cursor, mint::create_number(error.code().value()));
	}
}

}

MINT_EXPORT_FUNCTION(mint_locale_set_current_name, 1);
MINT_EXPORT_FUNCTION(mint_locale_list, 0);
MINT_EXPORT_FUNCTION(mint_locale_current, 0);
MINT_EXPORT_FUNCTION(mint_locale_create, 1);
MINT_EXPORT_FUNCTION(mint_locale_delete, 1);
MINT_EXPORT_FUNCTION(mint_locale_abbreviated_day_name, 2);
MINT_EXPORT_FUNCTION(mint_locale_day_name, 2);
MINT_EXPORT_FUNCTION(mint_locale_abbreviated_month_name, 2);
MINT_EXPORT_FUNCTION(mint_locale_month_name, 2);
MINT_EXPORT_FUNCTION(mint_locale_am_name, 1);
MINT_EXPORT_FUNCTION(mint_locale_pm_name, 1);
MINT_EXPORT_FUNCTION(mint_locale_date_format, 1);
MINT_EXPORT_FUNCTION(mint_locale_time_format, 1);
MINT_EXPORT_FUNCTION(mint_locale_date_time_format, 1);
MINT_EXPORT_FUNCTION(mint_locale_decimal_point, 1);
MINT_EXPORT_FUNCTION(mint_locale_thousands_separator, 1);
