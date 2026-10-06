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

#include "../locale_data.h"

#include <cerrno>
#include <langinfo.h>
#include <locale.h>
#include <string>
#include <system_error>
#include <utility>

namespace {

std::string query(_localet locale, nl_item item) {
	errno = 0;
	const char* value = nl_langinfo_l(item, locale);

	if (!value) {
		const int error = errno ? errno : EINVAL;
		throw std::system_error(error, std::generic_category(), "nl_langinfo_l");
	}

	return value;
}

}

mint_system::LocaleData::LocaleData() :
    LocaleData(newlocale(LC_ALL_MASK, "", nullptr)) {}

mint_system::LocaleData::LocaleData(const std::string& name) :
    LocaleData(newlocale(LC_ALL_MASK, name.c_str(), nullptr)) {}

mint_system::LocaleData::LocaleData(locale_t locale) :
    _locale(locale) {
	if (!_locale) {
		throw std::system_error(errno, std::generic_category(), "newlocale");
	}
}

mint_system::LocaleData::~LocaleData() {
	if (_locale) {
		freelocale(_locale);
	}
}

std::string mint_system::LocaleData::abbreviated_day_name(int day) const {
	static constexpr nl_item items[] = {ABDAY_1, ABDAY_2, ABDAY_3, ABDAY_4, ABDAY_5, ABDAY_6, ABDAY_7};

	if (day < 0 || day >= 7) {
		throw std::out_of_range("Day index must be in [0, 6]");
	}

	return query(_locale, items[day]);
}

std::string mint_system::LocaleData::day_name(int day) const {
	static constexpr nl_item items[] = {DAY_1, DAY_2, DAY_3, DAY_4, DAY_5, DAY_6, DAY_7};

	if (day < 0 || day >= 7) {
		throw std::out_of_range("Day index must be in [0, 6]");
	}

	return query(_locale, items[day]);
}

std::string mint_system::LocaleData::abbreviated_month_name(int month) const {
	static constexpr nl_item items[] = {ABMON_1, ABMON_2, ABMON_3, ABMON_4, ABMON_5, ABMON_6, ABMON_7, ABMON_8, ABMON_9,
	    ABMON_10, ABMON_11, ABMON_12};

	if (month < 1 || month > 12) {
		throw std::out_of_range("Month must be in [1, 12]");
	}

	return query(_locale, items[month - 1]);
}

std::string mint_system::LocaleData::month_name(int month) const {
	static constexpr nl_item items[] = {MON_1, MON_2, MON_3, MON_4, MON_5, MON_6, MON_7, MON_8, MON_9, MON_10, MON_11,
	    MON_12};

	if (month < 1 || month > 12) {
		throw std::out_of_range("Month must be in [1, 12]");
	}

	return query(_locale, items[month - 1]);
}

std::string mint_system::LocaleData::am_name() const {
	return query(_locale, AM_STR);
}

std::string mint_system::LocaleData::pm_name() const {
	return query(_locale, PM_STR);
}

std::string mint_system::LocaleData::date_format() const {
	return query(_locale, D_FMT);
}

std::string mint_system::LocaleData::time_format() const {
	return query(_locale, T_FMT);
}

std::string mint_system::LocaleData::date_time_format() const {
	return query(_locale, D_T_FMT);
}

std::string mint_system::LocaleData::decimal_point() const {
	return query(_locale, RADIXCHAR);
}

std::string mint_system::LocaleData::thousands_separator() const {
	return query(_locale, THOUSEP);
}
