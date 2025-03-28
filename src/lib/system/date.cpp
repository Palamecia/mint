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
#include <mint/system/errno.h>
#include <cstdint>
#include <chrono>
#include <array>
#include <cmath>

#ifdef OS_UNIX
#include <sys/time.h>
#include "unix/tzfile.h"
static constexpr const char *UTC_NAME = "Etc/GMT";
#else
#include "win32/wintz.h"
static constexpr const char *UTC_NAME = "UTC";
#endif

using namespace mint;

#define IS_LEAP(y) (((y) % 4) == 0 && (((y) % 100) != 0 || ((y) % 400) == 0))

namespace {

static constexpr const int MONTH_PER_YEAR = 12;

static constexpr const int MON_LENGTHS[2][MONTH_PER_YEAR] = {
	{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
	{31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
};

int week_number_to_year_day(int year, int week) {
	return (week * 7) - int(365.25 * year) % 7;
}

bool year_day_to_month_day(int year, int yday, int *mon, int *mday) {

	for (int i = 0; i < MONTH_PER_YEAR; ++i) {
		if (yday < MON_LENGTHS[IS_LEAP(year)][i]) {
			*mon = i;
			*mday = yday + 1;
			return true;
		}
		else {
			yday -= MON_LENGTHS[IS_LEAP(year)][i];
		}
	}

	return false;
}

std::string offset_to_timezone(int offset) {
#ifdef OS_UNIX
	std::array<char, 14> buffer {};
	sprintf(buffer.data(), "Etc/GMT%c%02d:%02d", offset < 0 ? '-' : '+', abs(offset) / 60, abs(offset) % 60);
#else
	std::array<char, 10> buffer {};
	sprintf(buffer.data(), "UTC%c%02d:%02d", offset < 0 ? '-' : '+', abs(offset) / 60, abs(offset) % 60);
#endif
	return buffer.data();
}

struct TimeZoneDeleter {
	void operator()(TimeZone *tz) {
		mint::timezone_free(tz);
	}
};

std::chrono::milliseconds parse_iso_date(const std::string &date, std::string *tz, bool *ok) {

	enum State : std::uint8_t {
		READ_START,
		READ_YEAR_FRACTION,
		READ_MONTH_DAY,
		READ_WEEK,
		READ_WEEK_DAY,
		READ_TIME,
		READ_MINUTES,
		READ_SECONDS,
		READ_SECONDS_FRACTION,
		READ_POSITIVE_OFFSET,
		READ_POSITIVE_OFFSET_MINUTES,
		READ_NEGATIVE_OFFSET,
		READ_NEGATIVE_OFFSET_MINUTES,
		READ_END
	};

	if (ok) {
		*ok = false;
	}

	std::unique_ptr<TimeZone, TimeZoneDeleter> utc(mint::timezone_find(UTC_NAME));

	if (utc == nullptr) {
		return {};
	}

	time_t now = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
					 .count();
	struct tm tm = mint::timezone_localtime(utc.get(), now);
	State state = READ_START;
	std::string token;
	int milliseconds = 0;
	int offset = 0;

	tm.tm_sec = tm.tm_min = tm.tm_hour = tm.tm_isdst = 0;

	for (const char *c = date.c_str(); *c; ++c) {
		switch (*c) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			token += *c;
			break;
		case ':':
			switch (state) {
			case READ_START:
			case READ_TIME:
				switch (token.length()) {
				case 2:
					tm.tm_hour = stoi(token);
					break;
				default:
					return {};
				}
				state = READ_MINUTES;
				token.clear();
				break;
			case READ_MINUTES:
				switch (token.length()) {
				case 2:
					tm.tm_min = stoi(token);
					break;
				default:
					return {};
				}
				state = READ_SECONDS;
				token.clear();
				break;
			case READ_NEGATIVE_OFFSET:
				switch (token.length()) {
				case 2:
					offset -= stoi(token) * 60;
					break;
				default:
					return {};
				}
				state = READ_NEGATIVE_OFFSET_MINUTES;
				token.clear();
				break;
			case READ_POSITIVE_OFFSET:
				switch (token.length()) {
				case 2:
					offset += stoi(token) * 60;
					break;
				default:
					return {};
				}
				state = READ_POSITIVE_OFFSET_MINUTES;
				token.clear();
				break;
			default:
				return {};
			}
			break;
		case '-':
			switch (state) {
			case READ_START:
				switch (token.length()) {
				case 4:
					tm.tm_year = stoi(token) - TM_YEAR_BASE;
					break;
				default:
					return {};
				}
				state = READ_YEAR_FRACTION;
				token.clear();
				break;
			case READ_YEAR_FRACTION:
				switch (token.length()) {
				case 2:
					tm.tm_mon = stoi(token) - 1;
					break;
				default:
					return {};
				}
				state = READ_MONTH_DAY;
				token.clear();
				break;
			case READ_WEEK:
				switch (token.length()) {
				case 2:
					tm.tm_yday = week_number_to_year_day(tm.tm_year, stoi(token));
					break;
				default:
					return {};
				}
				state = READ_WEEK_DAY;
				token.clear();
				break;
			case READ_TIME:
				switch (token.length()) {
				case 2:
					tm.tm_hour = stoi(token);
					break;
				case 4:
					tm.tm_hour = stoi(token.substr(0, 2));
					tm.tm_min = stoi(token.substr(2, 2));
					break;
				case 6:
					tm.tm_hour = stoi(token.substr(0, 2));
					tm.tm_min = stoi(token.substr(2, 2));
					tm.tm_sec = stoi(token.substr(4, 2));
					break;
				default:
					return {};
				}
				state = READ_NEGATIVE_OFFSET;
				token.clear();
				break;
			case READ_MINUTES:
				switch (token.length()) {
				case 2:
					tm.tm_min = stoi(token);
					break;
				default:
					return {};
				}
				state = READ_NEGATIVE_OFFSET;
				token.clear();
				break;
			case READ_SECONDS:
				switch (token.length()) {
				case 2:
					tm.tm_sec = stoi(token);
					break;
				default:
					return {};
				}
				state = READ_NEGATIVE_OFFSET;
				token.clear();
				break;
			case READ_SECONDS_FRACTION:
				while (token.length() < 3) {
					token += "0";
				}
				milliseconds = stoi(token.substr(0, 3));
				state = READ_NEGATIVE_OFFSET;
				token.clear();
				break;
			default:
				return {};
			}
			break;
		case '+':
			switch (state) {
			case READ_TIME:
				switch (token.length()) {
				case 2:
					tm.tm_hour = stoi(token);
					break;
				case 4:
					tm.tm_hour = stoi(token.substr(0, 2));
					tm.tm_min = stoi(token.substr(2, 2));
					break;
				case 6:
					tm.tm_hour = stoi(token.substr(0, 2));
					tm.tm_min = stoi(token.substr(2, 2));
					tm.tm_sec = stoi(token.substr(4, 2));
					break;
				default:
					return {};
				}
				state = READ_POSITIVE_OFFSET;
				token.clear();
				break;
			case READ_MINUTES:
				switch (token.length()) {
				case 2:
					tm.tm_min = stoi(token);
					break;
				default:
					return {};
				}
				state = READ_POSITIVE_OFFSET;
				token.clear();
				break;
			case READ_SECONDS:
				switch (token.length()) {
				case 2:
					tm.tm_sec = stoi(token);
					break;
				default:
					return {};
				}
				state = READ_POSITIVE_OFFSET;
				token.clear();
				break;
			case READ_SECONDS_FRACTION:
				while (token.length() < 3) {
					token += "0";
				}
				milliseconds = stoi(token.substr(0, 3));
				state = READ_POSITIVE_OFFSET;
				token.clear();
				break;
			default:
				return {};
			}
			break;
		case 'T':
			switch (state) {
			case READ_START:
				switch (token.length()) {
				case 0:
					break;
				case 4:
					tm.tm_year = stoi(token) - TM_YEAR_BASE;
					break;
				case 6:
					tm.tm_year = stoi(token.substr(0, 4)) - TM_YEAR_BASE;
					tm.tm_mon = stoi(token.substr(4, 2)) - 1;
					break;
				case 7:
					tm.tm_year = stoi(token.substr(0, 4)) - TM_YEAR_BASE;
					tm.tm_yday = stoi(token.substr(4, 3));
					break;
				case 8:
					tm.tm_year = stoi(token.substr(0, 4)) - TM_YEAR_BASE;
					tm.tm_mon = stoi(token.substr(4, 2)) - 1;
					tm.tm_mday = stoi(token.substr(6, 2));
					break;
				default:
					return {};
				}
				state = READ_TIME;
				token.clear();
				break;
			case READ_YEAR_FRACTION:
				switch (token.length()) {
				case 2:
					tm.tm_mon = stoi(token) - 1;
					break;
				case 3:
					tm.tm_yday = stoi(token);
					break;
				default:
					return {};
				}
				state = READ_TIME;
				token.clear();
				break;
			case READ_MONTH_DAY:
				switch (token.length()) {
				case 2:
					tm.tm_mday = stoi(token);
					break;
				default:
					return {};
				}
				state = READ_TIME;
				token.clear();
				break;
			case READ_WEEK:
				switch (token.length()) {
				case 3:
					tm.tm_yday = week_number_to_year_day(tm.tm_year, stoi(token.substr(0, 2)));
					tm.tm_wday = stoi(token.substr(2, 1));
					year_day_to_month_day(tm.tm_year, tm.tm_yday + tm.tm_wday - 1, &tm.tm_mon, &tm.tm_mday);
					break;
				default:
					return {};
				}
				state = READ_TIME;
				token.clear();
				break;
			case READ_WEEK_DAY:
				switch (token.length()) {
				case 1:
					tm.tm_wday = stoi(token);
					year_day_to_month_day(tm.tm_year, tm.tm_yday + tm.tm_wday - 1, &tm.tm_mon, &tm.tm_mday);
					break;
				default:
					return {};
				}
				state = READ_TIME;
				token.clear();
				break;
			default:
				return {};
			}
			break;
		case 'W':
			switch (state) {
			case READ_START:
				switch (token.length()) {
				case 4:
					tm.tm_year = stoi(token) - TM_YEAR_BASE;
					break;
				default:
					return {};
				}
				break;
			case READ_YEAR_FRACTION:
				break;
			default:
				return {};
			}
			state = READ_WEEK;
			token.clear();
			break;
		case 'Z':
			switch (state) {
			case READ_TIME:
				switch (token.length()) {
				case 2:
					tm.tm_hour = stoi(token);
					break;
				case 4:
					tm.tm_hour = stoi(token.substr(0, 2));
					tm.tm_min = stoi(token.substr(2, 2));
					break;
				case 6:
					tm.tm_hour = stoi(token.substr(0, 2));
					tm.tm_min = stoi(token.substr(2, 2));
					tm.tm_sec = stoi(token.substr(4, 2));
					break;
				default:
					return {};
				}
				state = READ_END;
				token.clear();
				break;
			case READ_MINUTES:
				switch (token.length()) {
				case 2:
					tm.tm_min = stoi(token);
					break;
				default:
					return {};
				}
				state = READ_END;
				token.clear();
				break;
			case READ_SECONDS:
				switch (token.length()) {
				case 2:
					tm.tm_sec = stoi(token);
					break;
				default:
					return {};
				}
				state = READ_END;
				token.clear();
				break;
			case READ_SECONDS_FRACTION:
				while (token.length() < 3) {
					token += "0";
				}
				milliseconds = stoi(token.substr(0, 3));
				state = READ_END;
				token.clear();
				break;
			default:
				return {};
			}
			break;
		case '.':
		case ',':
			switch (state) {
			case READ_TIME:
				switch (token.length()) {
				case 2:
					tm.tm_hour = stoi(token);
					break;
				case 4:
					tm.tm_hour = stoi(token.substr(0, 2));
					tm.tm_min = stoi(token.substr(2, 2));
					break;
				case 6:
					tm.tm_hour = stoi(token.substr(0, 2));
					tm.tm_min = stoi(token.substr(2, 2));
					tm.tm_sec = stoi(token.substr(4, 2));
					break;
				default:
					return {};
				}
				break;
			case READ_SECONDS:
				switch (token.length()) {
				case 2:
					tm.tm_sec = stoi(token);
					break;
				default:
					return {};
				}
				break;
			default:
				return {};
			}
			state = READ_SECONDS_FRACTION;
			token.clear();
			break;
		default:
			return {};
		}
	}

	if (!token.empty()) {
		switch (state) {
		case READ_START:
			switch (token.length()) {
			case 4:
				tm.tm_year = stoi(token) - TM_YEAR_BASE;
				tm.tm_mon = 0;
				tm.tm_mday = 1;
				break;
			case 6:
				tm.tm_year = stoi(token.substr(0, 4)) - TM_YEAR_BASE;
				tm.tm_mon = stoi(token.substr(4, 2)) - 1;
				tm.tm_mday = 1;
				break;
			case 7:
				tm.tm_year = stoi(token.substr(0, 4)) - TM_YEAR_BASE;
				tm.tm_yday = stoi(token.substr(4, 3));
				break;
			case 8:
				tm.tm_year = stoi(token.substr(0, 4)) - TM_YEAR_BASE;
				tm.tm_mon = stoi(token.substr(4, 2)) - 1;
				tm.tm_mday = stoi(token.substr(6, 2));
				break;
			default:
				return {};
			}
			break;
		case READ_YEAR_FRACTION:
			switch (token.length()) {
			case 2:
				tm.tm_mon = stoi(token) - 1;
				tm.tm_mday = 1;
				break;
			case 3:
				tm.tm_yday = stoi(token);
				break;
			default:
				return {};
			}
			break;
		case READ_MONTH_DAY:
			switch (token.length()) {
			case 2:
				tm.tm_mday = stoi(token);
				break;
			default:
				return {};
			}
			break;
		case READ_WEEK:
			switch (token.length()) {
			case 3:
				tm.tm_yday = week_number_to_year_day(tm.tm_year, stoi(token.substr(0, 2)));
				tm.tm_wday = stoi(token.substr(2, 1));
				year_day_to_month_day(tm.tm_year, tm.tm_yday + tm.tm_wday - 1, &tm.tm_mon, &tm.tm_mday);
				break;
			default:
				return {};
			}
			break;
		case READ_WEEK_DAY:
			switch (token.length()) {
			case 1:
				tm.tm_wday = stoi(token);
				year_day_to_month_day(tm.tm_year, tm.tm_yday + tm.tm_wday - 1, &tm.tm_mon, &tm.tm_mday);
				break;
			default:
				return {};
			}
			break;
		case READ_TIME:
			switch (token.length()) {
			case 2:
				tm.tm_hour = stoi(token);
				break;
			case 4:
				tm.tm_hour = stoi(token.substr(0, 2));
				tm.tm_min = stoi(token.substr(2, 2));
				break;
			case 6:
				tm.tm_hour = stoi(token.substr(0, 2));
				tm.tm_min = stoi(token.substr(2, 2));
				tm.tm_sec = stoi(token.substr(4, 2));
				break;
			default:
				return {};
			}
			break;
		case READ_MINUTES:
			switch (token.length()) {
			case 2:
				tm.tm_min = stoi(token);
				break;
			default:
				return {};
			}
			break;
		case READ_SECONDS:
			switch (token.length()) {
			case 2:
				tm.tm_sec = stoi(token);
				break;
			default:
				return {};
			}
			break;
		case READ_SECONDS_FRACTION:
			while (token.length() < 3) {
				token += "0";
			}
			milliseconds = stoi(token.substr(0, 3));
			break;
		case READ_NEGATIVE_OFFSET:
			switch (token.length()) {
			case 4:
				offset -= stoi(token.substr(0, 2)) * 60;
				offset -= stoi(token.substr(2, 2));
				break;
			default:
				return {};
			}
			break;
		case READ_NEGATIVE_OFFSET_MINUTES:
			switch (token.length()) {
			case 2:
				offset -= stoi(token);
				break;
			default:
				return {};
			}
			break;
		case READ_POSITIVE_OFFSET:
			switch (token.length()) {
			case 4:
				offset += stoi(token.substr(0, 2)) * 60;
				offset += stoi(token.substr(2, 2));
				break;
			default:
				return {};
			}
			break;
		case READ_POSITIVE_OFFSET_MINUTES:
			switch (token.length()) {
			case 2:
				offset += stoi(token);
				break;
			default:
				return {};
			}
			break;
		default:
			return {};
		}
	}

	bool valid = false;
	time_t timestamp = mint::timezone_mktime(utc.get(), tm, &valid);

	if (valid) {
		if (ok) {
			*ok = true;
		}
		if (tz) {
			if (offset) {
				*tz = offset_to_timezone(offset);
			}
			else {
				*tz = UTC_NAME;
			}
		}
		timestamp -= offset * 60;
		return std::chrono::milliseconds(timestamp * 1000 + milliseconds);
	}

	return {};
}

}

MINT_FUNCTION(mint_date_current_timepoint, 0, cursor) {
	FunctionHelper helper(cursor, 0);
	helper.return_value(create_object(new std::chrono::milliseconds(
		std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()))));
}

MINT_FUNCTION(mint_date_set_current, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &milliseconds = helper.pop_parameter();

#ifdef OS_WINDOWS
	bool ok = false;
	SYSTEMTIME systemTime;
	std::unique_ptr<TimeZone, TimeZoneDeleter> utc(mint::timezone_find(UTC_NAME));
	tm &&time = mint::timezone_localtime(utc.get(),
										 std::chrono::duration_cast<std::chrono::seconds>(
											 *milliseconds.data<LibObject<std::chrono::milliseconds>>()->impl)
											 .count(),
										 &ok);

	systemTime.wYear = static_cast<WORD>(time.tm_year + TM_YEAR_BASE);
	systemTime.wMonth = static_cast<WORD>(time.tm_mon + 1);
	systemTime.wDayOfWeek = static_cast<WORD>(time.tm_wday);
	systemTime.wDay = static_cast<WORD>(time.tm_mday);
	systemTime.wHour = static_cast<WORD>(time.tm_hour);
	systemTime.wMinute = static_cast<WORD>(time.tm_min);
	systemTime.wSecond = static_cast<WORD>(time.tm_sec);
	systemTime.wMilliseconds = static_cast<WORD>(
		milliseconds.data<LibObject<std::chrono::milliseconds>>()->impl->count() % 1000);

	if (!ok) {
		helper.return_value(create_number(EINVAL));
	}
	else if (!SetSystemTime(&systemTime)) {
		helper.return_value(create_number(mint::errno_from_error_code(mint::last_error_code())));
	}
#else
	timeval tv;

	tv.tv_sec = static_cast<time_t>(std::chrono::duration_cast<std::chrono::seconds>(
										*milliseconds.data<LibObject<std::chrono::milliseconds>>()->impl)
										.count());
	tv.tv_usec = static_cast<suseconds_t>(
		(milliseconds.data<LibObject<std::chrono::milliseconds>>()->impl->count() % 1000) * 1000);

	if (settimeofday(&tv, nullptr)) {
		helper.return_value(create_number(errno));
	}
#endif
	else {
		helper.return_value(WeakReference::create<None>());
	}
}

MINT_FUNCTION(mint_date_delete, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &milliseconds = helper.pop_parameter();

	delete milliseconds.data<LibObject<std::chrono::milliseconds>>()->impl;
}

MINT_FUNCTION(mint_date_set_seconds, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &value = helper.pop_parameter();
	Reference &milliseconds = helper.pop_parameter();

	(*milliseconds.data<LibObject<std::chrono::milliseconds>>()->impl) = std::chrono::seconds(to_integer(cursor, value));
}

MINT_FUNCTION(mint_date_timepoint_to_seconds, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &milliseconds = helper.pop_parameter();

	helper.return_value(
		create_number(static_cast<double>(std::chrono::duration_cast<std::chrono::seconds>(
											  *milliseconds.data<LibObject<std::chrono::milliseconds>>()->impl)
											  .count())));
}

MINT_FUNCTION(mint_date_seconds_to_timepoint, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &number = helper.pop_parameter();

	helper.return_value(create_object(new std::chrono::milliseconds(
		std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::seconds(to_integer(cursor, number))))));
}

MINT_FUNCTION(mint_date_set_milliseconds, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &value = helper.pop_parameter();
	Reference &milliseconds = helper.pop_parameter();

	(*milliseconds.data<LibObject<std::chrono::milliseconds>>()->impl) = std::chrono::milliseconds(
		to_integer(cursor, value));
}

MINT_FUNCTION(mint_date_timepoint_to_milliseconds, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &milliseconds = helper.pop_parameter();

	helper.return_value(
		create_number(static_cast<double>(milliseconds.data<LibObject<std::chrono::milliseconds>>()->impl->count())));
}

MINT_FUNCTION(mint_date_milliseconds_to_timepoint, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &number = helper.pop_parameter();

	helper.return_value(create_object(new std::chrono::milliseconds(to_integer(cursor, number))));
}

MINT_FUNCTION(mint_date_equals, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	const Reference &other = helper.pop_parameter();
	const Reference &self = helper.pop_parameter();

	helper.return_value(create_boolean((*self.data<LibObject<std::chrono::milliseconds>>()->impl)
									   == (*other.data<LibObject<std::chrono::milliseconds>>()->impl)));
}

MINT_FUNCTION(mint_parse_iso_date, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	std::string date = to_string(helper.pop_parameter());
	std::string time_zone;
	bool ok = false;

	std::chrono::milliseconds timepoint = parse_iso_date(date, &time_zone, &ok);

	if (ok) {
		helper.return_value(
			create_iterator(create_object(new std::chrono::milliseconds(timepoint)), create_string(time_zone)));
	}
}

MINT_FUNCTION(mint_date_is_leap, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &year = helper.pop_parameter();

	helper.return_value(create_boolean(IS_LEAP(to_integer(cursor, year))));
}

MINT_FUNCTION(mint_date_days_in_month, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &month = helper.pop_parameter();
	Reference &year = helper.pop_parameter();

	helper.return_value(create_number(MON_LENGTHS[IS_LEAP(to_integer(cursor, year))][to_integer(cursor, month)]));
}
