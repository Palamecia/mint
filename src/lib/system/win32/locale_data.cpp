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
#include <array>
#include <string_view>
#include <utility>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <cstddef>
#include <errhandlingapi.h>
#include <locale.h>
#include <string>
#include <stringapiset.h>
#include <system_error>
#include <vector>
#include <winnls.h>
#include <winnt.h>

namespace {

std::wstring utf8_to_windows(const std::string& str) {
	const int length = MultiByteToWideChar(CP_UTF8, 0, str.data(), -1, nullptr, 0);
	if (auto buffer = std::vector<wchar_t>(static_cast<size_t>(length));
	    MultiByteToWideChar(CP_UTF8, 0, str.data(), -1, buffer.data(), length)) {
		buffer.resize(length - 1);
		return {buffer.data()};
	}
	throw std::system_error(static_cast<int>(GetLastError()), std::system_category());
}

std::string windows_to_utf8(const std::wstring& str) {
	const int length = WideCharToMultiByte(CP_UTF8, 0, str.data(), -1, nullptr, 0, nullptr, nullptr);
	if (auto buffer = std::vector<char>(static_cast<size_t>(length));
	    WideCharToMultiByte(CP_UTF8, 0, str.data(), -1, buffer.data(), length, nullptr, nullptr)) {
		buffer.resize(length - 1);
		return {buffer.data()};
	}
	throw std::system_error(static_cast<int>(GetLastError()), std::system_category());
}

std::wstring query(const std::wstring& locale, LCTYPE type) {
	const int length = GetLocaleInfoEx(locale.c_str(), type, nullptr, 0);
	if (auto buffer = std::vector<wchar_t>(static_cast<size_t>(length));
	    GetLocaleInfoEx(locale.c_str(), type, buffer.data(), length)) {
		return {buffer.data()};
	}
	throw std::system_error(static_cast<int>(GetLastError()), std::system_category());
}

std::string query_utf8(const std::wstring& locale, LCTYPE type) {
	return windows_to_utf8(query(locale, type));
}

std::wstring current_locale_name() {
	auto name = std::array<wchar_t, LOCALE_NAME_MAX_LENGTH>();
	if (!GetUserDefaultLocaleName(name.data(), static_cast<int>(name.size()))) {
		throw std::system_error(static_cast<int>(GetLastError()), std::system_category(), "GetUserDefaultLocaleName");
	}
	return {name.data()};
}

[[nodiscard]] std::string windows_to_strftime(const std::string_view windows_format) {

	auto result = std::string();
	result.reserve(windows_format.size() * 2);

	const auto append_literal = [&result](const char c) {
		if (c == '%') {
			result += "%%";
		}
		else {
			result += c;
		}
	};

	const auto is_ascii_letter = [](const char c) {
		return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
	};

	for (auto i = std::size_t {}; i < windows_format.size();) {
		const auto c = windows_format.at(i);

		if (c == '\'') {
			++i;
			auto closed = false;

			while (i < windows_format.size()) {
				if (windows_format.at(i) == '\'') {
					if (i + 1 < windows_format.size() && windows_format.at(i + 1) == '\'') {
						append_literal('\'');
						i += 2;
						continue;
					}

					++i;
					closed = true;
					break;
				}

				append_literal(windows_format.at(i++));
			}

			if (!closed) {
				throw std::system_error(std::make_error_code(std::errc::invalid_argument));
			}

			continue;
		}

		if (!is_ascii_letter(c)) {
			append_literal(c);
			++i;
			continue;
		}

		auto count = std::size_t {1};

		while (i + count < windows_format.size() && windows_format.at(i + count) == c) {
			++count;
		}

		switch (c) {
		case 'd':
			switch (count) {
			case 1:
				result += "%-d";
				break;
			case 2:
				result += "%d";
				break;
			case 3:
				result += "%a";
				break;
			case 4:
				result += "%A";
				break;
			default:
				throw std::system_error(std::make_error_code(std::errc::invalid_argument));
			}
			break;

		case 'M':
			switch (count) {
			case 1:
				result += "%-m";
				break;
			case 2:
				result += "%m";
				break;
			case 3:
				result += "%b";
				break;
			case 4:
				result += "%B";
				break;
			default:
				throw std::system_error(std::make_error_code(std::errc::invalid_argument));
			}
			break;

		case 'y':
			if (count == 1 || count == 2) {
				result += "%y";
			}
			else if (count == 4) {
				result += "%Y";
			}
			else {
				throw std::system_error(std::make_error_code(std::errc::invalid_argument));
			}
			break;

		case 'h':
			if (count == 1) {
				result += "%-I";
			}
			else if (count == 2) {
				result += "%I";
			}
			else {
				throw std::system_error(std::make_error_code(std::errc::invalid_argument));
			}
			break;

		case 'H':
			if (count == 1) {
				result += "%-H";
			}
			else if (count == 2) {
				result += "%H";
			}
			else {
				throw std::system_error(std::make_error_code(std::errc::invalid_argument));
			}
			break;

		case 'm':
			if (count == 1) {
				result += "%-M";
			}
			else if (count == 2) {
				result += "%M";
			}
			else {
				throw std::system_error(std::make_error_code(std::errc::invalid_argument));
			}
			break;

		case 's':
			if (count == 1) {
				result += "%-S";
			}
			else if (count == 2) {
				result += "%S";
			}
			else {
				throw std::system_error(std::make_error_code(std::errc::invalid_argument));
			}
			break;

		case 't':
			if (count == 1 || count == 2) {
				result += "%p";
			}
			else {
				throw std::system_error(std::make_error_code(std::errc::invalid_argument));
			}
			break;

		default:
			throw std::system_error(std::make_error_code(std::errc::invalid_argument));
		}

		i += count;
	}

	return result;
}

}

mint_system::LocaleData::LocaleData() :
    LocaleData(current_locale_name()) {}

mint_system::LocaleData::LocaleData(const std::string& name) :
    LocaleData(utf8_to_windows(name)) {}

mint_system::LocaleData::LocaleData(std::wstring name) :
    _crt_locale(_wcreate_locale(LC_ALL, name.c_str())),
    _name(std::move(name)) {
	if (!_crt_locale) {
		throw std::system_error(std::make_error_code(std::errc::invalid_argument));
	}
}

mint_system::LocaleData::~LocaleData() {
	if (_crt_locale) {
		_free_locale(_crt_locale);
	}
}

std::string mint_system::LocaleData::abbreviated_day_name(int day) const {
	static constexpr auto items = std::to_array<LCTYPE>({
	    LOCALE_SABBREVDAYNAME7, // Sunday
	    LOCALE_SABBREVDAYNAME1,
	    LOCALE_SABBREVDAYNAME2,
	    LOCALE_SABBREVDAYNAME3,
	    LOCALE_SABBREVDAYNAME4,
	    LOCALE_SABBREVDAYNAME5,
	    LOCALE_SABBREVDAYNAME6,
	});
	const auto index = static_cast<std::size_t>(day);
	if (index < items.size()) {
		return query_utf8(_name, items.at(index));
	}
	throw std::system_error(std::make_error_code(std::errc::invalid_argument));
}

std::string mint_system::LocaleData::day_name(int day) const {
	static constexpr auto items = std::to_array<LCTYPE>({
	    LOCALE_SDAYNAME7, // Sunday
	    LOCALE_SDAYNAME1,
	    LOCALE_SDAYNAME2,
	    LOCALE_SDAYNAME3,
	    LOCALE_SDAYNAME4,
	    LOCALE_SDAYNAME5,
	    LOCALE_SDAYNAME6,
	});
	const auto index = static_cast<std::size_t>(day);
	if (index < items.size()) {
		return query_utf8(_name, items.at(index));
	}
	throw std::system_error(std::make_error_code(std::errc::invalid_argument));
}

std::string mint_system::LocaleData::abbreviated_month_name(int month) const {
	static constexpr auto items = std::to_array<LCTYPE>({
	    LOCALE_SABBREVMONTHNAME1,
	    LOCALE_SABBREVMONTHNAME2,
	    LOCALE_SABBREVMONTHNAME3,
	    LOCALE_SABBREVMONTHNAME4,
	    LOCALE_SABBREVMONTHNAME5,
	    LOCALE_SABBREVMONTHNAME6,
	    LOCALE_SABBREVMONTHNAME7,
	    LOCALE_SABBREVMONTHNAME8,
	    LOCALE_SABBREVMONTHNAME9,
	    LOCALE_SABBREVMONTHNAME10,
	    LOCALE_SABBREVMONTHNAME11,
	    LOCALE_SABBREVMONTHNAME12,
	});
	const auto index = static_cast<std::size_t>(month - 1);
	if (index < items.size()) {
		return query_utf8(_name, items.at(index));
	}
	throw std::system_error(std::make_error_code(std::errc::invalid_argument));
}

std::string mint_system::LocaleData::month_name(int month) const {
	static constexpr auto items = std::to_array<LCTYPE>({
	    LOCALE_SMONTHNAME1,
	    LOCALE_SMONTHNAME2,
	    LOCALE_SMONTHNAME3,
	    LOCALE_SMONTHNAME4,
	    LOCALE_SMONTHNAME5,
	    LOCALE_SMONTHNAME6,
	    LOCALE_SMONTHNAME7,
	    LOCALE_SMONTHNAME8,
	    LOCALE_SMONTHNAME9,
	    LOCALE_SMONTHNAME10,
	    LOCALE_SMONTHNAME11,
	    LOCALE_SMONTHNAME12,
	});
	const auto index = static_cast<std::size_t>(month - 1);
	if (index < items.size()) {
		return query_utf8(_name, items.at(index));
	}
	throw std::system_error(std::make_error_code(std::errc::invalid_argument));
}

std::string mint_system::LocaleData::am_name() const {
	return query_utf8(_name, LOCALE_S1159);
}

std::string mint_system::LocaleData::pm_name() const {
	return query_utf8(_name, LOCALE_S2359);
}

std::string mint_system::LocaleData::date_format() const {
	return query_utf8(_name, LOCALE_SSHORTDATE);
}

std::string mint_system::LocaleData::time_format() const {
	return query_utf8(_name, LOCALE_STIMEFORMAT);
}

std::string mint_system::LocaleData::date_time_format() const {
	const auto date = query_utf8(_name, LOCALE_SLONGDATE);
	const auto time = query_utf8(_name, LOCALE_STIMEFORMAT);
	return windows_to_strftime(date + " " + time);
}

std::string mint_system::LocaleData::decimal_point() const {
	return windows_to_strftime(query_utf8(_name, LOCALE_SDECIMAL));
}

std::string mint_system::LocaleData::thousands_separator() const {
	return windows_to_strftime(query_utf8(_name, LOCALE_STHOUSAND));
}
