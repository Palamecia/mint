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

#ifndef LIB_SYSTEM_LOCALE_DATA_H
#define LIB_SYSTEM_LOCALE_DATA_H

#include "mint/config.h"
#include <string>

#ifdef MINT_OS_WINDOWS
#include <Windows.h>
#include <corecrt.h>
#endif

namespace mint_system {

class LocaleData {
public:
	LocaleData();
	explicit LocaleData(const std::string& name);
	LocaleData(const LocaleData&) = delete;
	LocaleData(LocaleData&&) = delete;
	~LocaleData();

	LocaleData& operator=(const LocaleData&) = delete;
	LocaleData& operator=(LocaleData&&) = delete;

	[[nodiscard]] std::string abbreviated_day_name(int day) const;
	[[nodiscard]] std::string day_name(int day) const;

	[[nodiscard]] std::string abbreviated_month_name(int month) const;
	[[nodiscard]] std::string month_name(int month) const;

	[[nodiscard]] std::string am_name() const;
	[[nodiscard]] std::string pm_name() const;

	[[nodiscard]] std::string date_format() const;
	[[nodiscard]] std::string time_format() const;
	[[nodiscard]] std::string date_time_format() const;

	[[nodiscard]] std::string decimal_point() const;
	[[nodiscard]] std::string thousands_separator() const;

private:
#ifdef MINT_OS_WINDOWS
	explicit LocaleData(std::wstring name);

	_locale_t _crt_locale;
	std::wstring _name;
#elifdef MINT_OS_UNIX
	explicit LocaleData(locale_t locale);

	locale_t _locale;
#endif
};

}

#endif // LIB_SYSTEM_LOCALE_DATA_H
