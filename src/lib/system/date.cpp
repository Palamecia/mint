#include <memory/functiontool.h>
#include <memory/casttool.h>
#include <chrono>
#include <cmath>

#ifdef OS_UNIX
#include "tzfile.h"
static constexpr const char *UtcName = "Etc/GMT";
#else
#include "wintz.h"
static constexpr const char *UtcName = "UTC";
#endif

using namespace std;
using namespace mint;

#define isleap(y) (((y) % 4) == 0 && (((y) % 100) != 0 || ((y) % 400) == 0))

static constexpr const int MONTH_PER_YEAR = 12;

static constexpr const int mon_lengths[2][MONTH_PER_YEAR] = {
	{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
	{ 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
};

static int week_number_to_year_day(int year, int week) {
	return (week * 7) - int(365.25 * year) % 7;
}

static bool year_day_to_month_day(int year, int yday, int *mon, int *mday) {

	for (int i = 0; i < MONTH_PER_YEAR; ++i) {
		if (yday < mon_lengths[isleap(year)][i]) {
			*mon = i;
			*mday = yday + 1;
			return true;
		}
		else {
			yday -= mon_lengths[isleap(year)][i];
		}
	}

	return false;
}

static string offset_to_timezone(int offset) {
#ifdef OS_UNIX
	char buffer[14];
	sprintf(buffer, "Etc/GMT%c%02d:%02d", offset < 0 ? '-' : '+', abs(offset) / 60, abs(offset) % 60);
#else
	char buffer[10];
	sprintf(buffer, "UTC%c%02d:%02d", offset < 0 ? '-' : '+', abs(offset) / 60, abs(offset) % 60);
#endif
	return buffer;
}

struct TimeZoneDeleter {
	void operator ()(TimeZone *tz) {
#ifdef OS_UNIX
		tzfile_free(tz);
#else
		wintz_free(tz);
#endif
	}
};

static chrono::milliseconds parse_iso_date(const std::string &date, std::string *tz, bool *ok) {

	enum State {
		ReadStart,
		ReadYearFraction,
		ReadMonthDay,
		ReadWeek,
		ReadWeekDay,
		ReadTime,
		ReadMinutes,
		ReadSeconds,
		ReadSecondsFraction,
		ReadPositiveOffset,
		ReadPositiveOffsetMinutes,
		ReadNegativeOffset,
		ReadNegativeOffsetMinutes,
		ReadEnd
	};

	if (ok) {
		*ok = false;
	}

#ifdef OS_UNIX
	std::unique_ptr<TimeZone, TimeZoneDeleter> utc(tzfile_find(UtcName));
#else
	std::unique_ptr<TimeZone, TimeZoneDeleter> utc(wintz_find(UtcName));
#endif

	if (utc == nullptr) {
		return chrono::milliseconds();
	}

	time_t now = chrono::duration_cast<chrono::seconds>(chrono::system_clock::now().time_since_epoch()).count();
#ifdef OS_UNIX
	struct tm tm = tzfile_localtime(utc.get(), now);
#else
	struct tm tm = wintz_localtime(utc.get(), now);
#endif
	State state = ReadStart;
	std::string token;
	int milliseconds = 0;
	int offset = 0;

	tm.tm_sec = tm.tm_min = tm.tm_hour = tm.tm_isdst = 0;

	for(const char *c = date.c_str(); *c; ++c) {
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
			case ReadStart:
			case ReadTime:
				switch (token.length()) {
				case 2:
					tm.tm_hour = atoi(token.c_str());
					break;
				default:
					return chrono::milliseconds();
				}
				state = ReadMinutes;
				token.clear();
				break;
			case ReadMinutes:
				switch (token.length()) {
				case 2:
					tm.tm_min = atoi(token.c_str());
					break;
				default:
					return chrono::milliseconds();
				}
				state = ReadSeconds;
				token.clear();
				break;
			case ReadNegativeOffset:
				switch (token.length()) {
				case 2:
					offset -= atoi(token.c_str()) * 60;
					break;
				default:
					return chrono::milliseconds();
				}
				state = ReadNegativeOffsetMinutes;
				token.clear();
				break;
			case ReadPositiveOffset:
				switch (token.length()) {
				case 2:
					offset += atoi(token.c_str()) * 60;
					break;
				default:
					return chrono::milliseconds();
				}
				state = ReadPositiveOffsetMinutes;
				token.clear();
				break;
			default:
				return chrono::milliseconds();
			}
			break;
		case '-':
			switch (state) {
			case ReadStart:
				switch (token.length()) {
				case 4:
					tm.tm_year = atoi(token.c_str()) - TM_YEAR_BASE;
					break;
				default:
					return chrono::milliseconds();
				}
				state = ReadYearFraction;
				token.clear();
				break;
			case ReadYearFraction:
				switch (token.length()) {
				case 2:
					tm.tm_mon = atoi(token.c_str()) - 1;
					break;
				default:
					return chrono::milliseconds();
				}
				state = ReadMonthDay;
				token.clear();
				break;
			case ReadWeek:
				switch (token.length()) {
				case 2:
					tm.tm_yday = week_number_to_year_day(tm.tm_year, atoi(token.c_str()));
					break;
				default:
					return chrono::milliseconds();
				}
				state = ReadWeekDay;
				token.clear();
				break;
			case ReadTime:
				switch (token.length()) {
				case 2:
					tm.tm_hour = atoi(token.c_str());
					break;
				case 4:
					tm.tm_hour = atoi(token.substr(0, 2).c_str());
					tm.tm_min = atoi(token.substr(2, 2).c_str());
					break;
				case 6:
					tm.tm_hour = atoi(token.substr(0, 2).c_str());
					tm.tm_min = atoi(token.substr(2, 2).c_str());
					tm.tm_sec = atoi(token.substr(4, 2).c_str());
					break;
				default:
					return chrono::milliseconds();
				}
				state = ReadNegativeOffset;
				token.clear();
				break;
			case ReadMinutes:
				switch (token.length()) {
				case 2:
					tm.tm_min = atoi(token.c_str());
					break;
				default:
					return chrono::milliseconds();
				}
				state = ReadNegativeOffset;
				token.clear();
				break;
			case ReadSeconds:
				switch (token.length()) {
				case 2:
					tm.tm_sec = atoi(token.c_str());
					break;
				default:
					return chrono::milliseconds();
				}
				state = ReadNegativeOffset;
				token.clear();
				break;
			case ReadSecondsFraction:
				while (token.length() < 3) {
					token += "0";
				}
				milliseconds = atoi(token.substr(0, 3).c_str());
				state = ReadNegativeOffset;
				token.clear();
				break;
			default:
				return chrono::milliseconds();
			}
			break;
		case '+':
			switch (state) {
			case ReadTime:
				switch (token.length()) {
				case 2:
					tm.tm_hour = atoi(token.c_str());
					break;
				case 4:
					tm.tm_hour = atoi(token.substr(0, 2).c_str());
					tm.tm_min = atoi(token.substr(2, 2).c_str());
					break;
				case 6:
					tm.tm_hour = atoi(token.substr(0, 2).c_str());
					tm.tm_min = atoi(token.substr(2, 2).c_str());
					tm.tm_sec = atoi(token.substr(4, 2).c_str());
					break;
				default:
					return chrono::milliseconds();
				}
				state = ReadPositiveOffset;
				token.clear();
				break;
			case ReadMinutes:
				switch (token.length()) {
				case 2:
					tm.tm_min = atoi(token.c_str());
					break;
				default:
					return chrono::milliseconds();
				}
				state = ReadPositiveOffset;
				token.clear();
				break;
			case ReadSeconds:
				switch (token.length()) {
				case 2:
					tm.tm_sec = atoi(token.c_str());
					break;
				default:
					return chrono::milliseconds();
				}
				state = ReadPositiveOffset;
				token.clear();
				break;
			case ReadSecondsFraction:
				while (token.length() < 3) {
					token += "0";
				}
				milliseconds = atoi(token.substr(0, 3).c_str());
				state = ReadPositiveOffset;
				token.clear();
				break;
			default:
				return chrono::milliseconds();
			}
			break;
		case 'T':
			switch (state) {
			case ReadStart:
				switch (token.length()) {
				case 0:
					break;
				case 4:
					tm.tm_year = atoi(token.c_str()) - TM_YEAR_BASE;
					break;
				case 6:
					tm.tm_year = atoi(token.substr(0, 4).c_str()) - TM_YEAR_BASE;
					tm.tm_mon = atoi(token.substr(4, 2).c_str()) - 1;
					break;
				case 7:
					tm.tm_year = atoi(token.substr(0, 4).c_str()) - TM_YEAR_BASE;
					tm.tm_yday = atoi(token.substr(4, 3).c_str());
					break;
				case 8:
					tm.tm_year = atoi(token.substr(0, 4).c_str()) - TM_YEAR_BASE;
					tm.tm_mon = atoi(token.substr(4, 2).c_str()) - 1;
					tm.tm_mday = atoi(token.substr(6, 2).c_str());
					break;
				default:
					return chrono::milliseconds();
				}
				state = ReadTime;
				token.clear();
				break;
			case ReadYearFraction:
				switch (token.length()) {
				case 2:
					tm.tm_mon = atoi(token.c_str()) - 1;
					break;
				case 3:
					tm.tm_yday = atoi(token.c_str());
					break;
				default:
					return chrono::milliseconds();
				}
				state = ReadTime;
				token.clear();
				break;
			case ReadMonthDay:
				switch (token.length()) {
				case 2:
					tm.tm_mday = atoi(token.c_str());
					break;
				default:
					return chrono::milliseconds();
				}
				state = ReadTime;
				token.clear();
				break;
			case ReadWeek:
				switch (token.length()) {
				case 3:
					tm.tm_yday = week_number_to_year_day(tm.tm_year, atoi(token.substr(0, 2).c_str()));
					tm.tm_wday = atoi(token.substr(2, 1).c_str());
					year_day_to_month_day(tm.tm_year, tm.tm_yday + tm.tm_wday - 1, &tm.tm_mon, &tm.tm_mday);
					break;
				default:
					return chrono::milliseconds();
				}
				state = ReadTime;
				token.clear();
				break;
			case ReadWeekDay:
				switch (token.length()) {
				case 1:
					tm.tm_wday = atoi(token.c_str());
					year_day_to_month_day(tm.tm_year, tm.tm_yday + tm.tm_wday - 1, &tm.tm_mon, &tm.tm_mday);
					break;
				default:
					return chrono::milliseconds();
				}
				state = ReadTime;
				token.clear();
				break;
			default:
				return chrono::milliseconds();
			}
			break;
		case 'W':
			switch (state) {
			case ReadStart:
				switch (token.length()) {
				case 4:
					tm.tm_year = atoi(token.c_str()) - TM_YEAR_BASE;
					break;
				default:
					return chrono::milliseconds();
				}
				break;
			case ReadYearFraction:
				break;
			default:
				return chrono::milliseconds();
			}
			state = ReadWeek;
			token.clear();
			break;
		case 'Z':
			switch (state) {
			case ReadTime:
				switch (token.length()) {
				case 2:
					tm.tm_hour = atoi(token.c_str());
					break;
				case 4:
					tm.tm_hour = atoi(token.substr(0, 2).c_str());
					tm.tm_min = atoi(token.substr(2, 2).c_str());
					break;
				case 6:
					tm.tm_hour = atoi(token.substr(0, 2).c_str());
					tm.tm_min = atoi(token.substr(2, 2).c_str());
					tm.tm_sec = atoi(token.substr(4, 2).c_str());
					break;
				default:
					return chrono::milliseconds();
				}
				state = ReadEnd;
				token.clear();
				break;
			case ReadMinutes:
				switch (token.length()) {
				case 2:
					tm.tm_min = atoi(token.c_str());
					break;
				default:
					return chrono::milliseconds();
				}
				state = ReadEnd;
				token.clear();
				break;
			case ReadSeconds:
				switch (token.length()) {
				case 2:
					tm.tm_sec = atoi(token.c_str());
					break;
				default:
					return chrono::milliseconds();
				}
				state = ReadEnd;
				token.clear();
				break;
			case ReadSecondsFraction:
				while (token.length() < 3) {
					token += "0";
				}
				milliseconds = atoi(token.substr(0, 3).c_str());
				state = ReadEnd;
				token.clear();
				break;
			default:
				return chrono::milliseconds();
			}
			break;
		case '.':
		case ',':
			switch (state) {
			case ReadTime:
				switch (token.length()) {
				case 2:
					tm.tm_hour = atoi(token.c_str());
					break;
				case 4:
					tm.tm_hour = atoi(token.substr(0, 2).c_str());
					tm.tm_min = atoi(token.substr(2, 2).c_str());
					break;
				case 6:
					tm.tm_hour = atoi(token.substr(0, 2).c_str());
					tm.tm_min = atoi(token.substr(2, 2).c_str());
					tm.tm_sec = atoi(token.substr(4, 2).c_str());
					break;
				default:
					return chrono::milliseconds();
				}
				break;
			case ReadSeconds:
				switch (token.length()) {
				case 2:
					tm.tm_sec = atoi(token.c_str());
					break;
				default:
					return chrono::milliseconds();
				}
				break;
			default:
				return chrono::milliseconds();
			}
			state = ReadSecondsFraction;
			token.clear();
			break;
		default:
			return chrono::milliseconds();
		}
	}

	if (!token.empty()) {
		switch (state) {
		case ReadStart:
			switch (token.length()) {
			case 4:
				tm.tm_year = atoi(token.c_str()) - TM_YEAR_BASE;
				tm.tm_mon = 0;
				tm.tm_mday = 1;
				break;
			case 6:
				tm.tm_year = atoi(token.substr(0, 4).c_str()) - TM_YEAR_BASE;
				tm.tm_mon = atoi(token.substr(4, 2).c_str()) - 1;
				tm.tm_mday = 1;
				break;
			case 7:
				tm.tm_year = atoi(token.substr(0, 4).c_str()) - TM_YEAR_BASE;
				tm.tm_yday = atoi(token.substr(4, 3).c_str());
				break;
			case 8:
				tm.tm_year = atoi(token.substr(0, 4).c_str()) - TM_YEAR_BASE;
				tm.tm_mon = atoi(token.substr(4, 2).c_str()) - 1;
				tm.tm_mday = atoi(token.substr(6, 2).c_str());
				break;
			default:
				return chrono::milliseconds();
			}
			break;
		case ReadYearFraction:
			switch (token.length()) {
			case 2:
				tm.tm_mon = atoi(token.c_str()) - 1;
				tm.tm_mday = 1;
				break;
			case 3:
				tm.tm_yday = atoi(token.c_str());
				break;
			default:
				return chrono::milliseconds();
			}
			break;
		case ReadMonthDay:
			switch (token.length()) {
			case 2:
				tm.tm_mday = atoi(token.c_str());
				break;
			default:
				return chrono::milliseconds();
			}
			break;
		case ReadWeek:
			switch (token.length()) {
			case 3:
				tm.tm_yday = week_number_to_year_day(tm.tm_year, atoi(token.substr(0, 2).c_str()));
				tm.tm_wday = atoi(token.substr(2, 1).c_str());
				year_day_to_month_day(tm.tm_year, tm.tm_yday + tm.tm_wday - 1, &tm.tm_mon, &tm.tm_mday);
				break;
			default:
				return chrono::milliseconds();
			}
			break;
		case ReadWeekDay:
			switch (token.length()) {
			case 1:
				tm.tm_wday = atoi(token.c_str());
				year_day_to_month_day(tm.tm_year, tm.tm_yday + tm.tm_wday - 1, &tm.tm_mon, &tm.tm_mday);
				break;
			default:
				return chrono::milliseconds();
			}
			break;
		case ReadTime:
			switch (token.length()) {
			case 2:
				tm.tm_hour = atoi(token.c_str());
				break;
			case 4:
				tm.tm_hour = atoi(token.substr(0, 2).c_str());
				tm.tm_min = atoi(token.substr(2, 2).c_str());
				break;
			case 6:
				tm.tm_hour = atoi(token.substr(0, 2).c_str());
				tm.tm_min = atoi(token.substr(2, 2).c_str());
				tm.tm_sec = atoi(token.substr(4, 2).c_str());
				break;
			default:
				return chrono::milliseconds();
			}
			break;
		case ReadMinutes:
			switch (token.length()) {
			case 2:
				tm.tm_min = atoi(token.c_str());
				break;
			default:
				return chrono::milliseconds();
			}
			break;
		case ReadSeconds:
			switch (token.length()) {
			case 2:
				tm.tm_sec = atoi(token.c_str());
				break;
			default:
				return chrono::milliseconds();
			}
			break;
		case ReadSecondsFraction:
			while (token.length() < 3) {
				token += "0";
			}
			milliseconds = atoi(token.substr(0, 3).c_str());
			break;
		case ReadNegativeOffset:
			switch (token.length()) {
			case 4:
				offset -= atoi(token.substr(0, 2).c_str()) * 60;
				offset -= atoi(token.substr(2, 2).c_str());
				break;
			default:
				return chrono::milliseconds();
			}
			break;
		case ReadNegativeOffsetMinutes:
			switch (token.length()) {
			case 2:
				offset -= atoi(token.c_str());
				break;
			default:
				return chrono::milliseconds();
			}
			break;
		case ReadPositiveOffset:
			switch (token.length()) {
			case 4:
				offset += atoi(token.substr(0, 2).c_str()) * 60;
				offset += atoi(token.substr(2, 2).c_str());
				break;
			default:
				return chrono::milliseconds();
			}
			break;
		case ReadPositiveOffsetMinutes:
			switch (token.length()) {
			case 2:
				offset += atoi(token.c_str());
				break;
			default:
				return chrono::milliseconds();
			}
			break;
		default:
			return chrono::milliseconds();
		}
	}

	bool valid = false;
#ifdef OS_UNIX
	time_t timestamp = tzfile_mktime(utc.get(), tm, &valid);
#else
	time_t timestamp = wintz_mktime(utc.get(), tm, &valid);
#endif

	if (valid) {
		if (ok) {
			*ok = true;
		}
		if (tz) {
			if (offset) {
				*tz = offset_to_timezone(offset);
			}
			else {
				*tz = UtcName;
			}
		}
		timestamp -= offset * 60;
		return chrono::milliseconds(timestamp * 1000 + milliseconds);
	}

	return chrono::milliseconds();
}

MINT_FUNCTION(mint_date_current_timepoint, 0, cursor) {
	FunctionHelper helper(cursor, 0);
	helper.returnValue(create_object(new chrono::milliseconds(chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()))));
}

MINT_FUNCTION(mint_date_delete, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &milliseconds = helper.popParameter();

	delete milliseconds.data<LibObject<chrono::milliseconds>>()->impl;
}

MINT_FUNCTION(mint_date_set_seconds, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &value = helper.popParameter();
	Reference &milliseconds = helper.popParameter();

	(*milliseconds.data<LibObject<chrono::milliseconds>>()->impl) = chrono::seconds(to_integer(cursor, value));
}

MINT_FUNCTION(mint_date_timepoint_to_seconds, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &milliseconds = helper.popParameter();

	helper.returnValue(create_number(static_cast<double>(chrono::duration_cast<chrono::seconds>(*milliseconds.data<LibObject<chrono::milliseconds>>()->impl).count())));
}

MINT_FUNCTION(mint_date_seconds_to_timepoint, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &number = helper.popParameter();

	helper.returnValue(create_object(new chrono::milliseconds(chrono::duration_cast<chrono::milliseconds>(chrono::seconds(to_integer(cursor, number))))));
}

MINT_FUNCTION(mint_date_set_milliseconds, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &value = helper.popParameter();
	Reference &milliseconds = helper.popParameter();

	(*milliseconds.data<LibObject<chrono::milliseconds>>()->impl) = chrono::milliseconds(to_integer(cursor, value));
}

MINT_FUNCTION(mint_date_timepoint_to_milliseconds, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &milliseconds = helper.popParameter();

	helper.returnValue(create_number(static_cast<double>(milliseconds.data<LibObject<chrono::milliseconds>>()->impl->count())));
}

MINT_FUNCTION(mint_date_milliseconds_to_timepoint, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &number = helper.popParameter();

	helper.returnValue(create_object(new chrono::milliseconds(to_integer(cursor, number))));
}

MINT_FUNCTION(mint_date_equals, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &other = helper.popParameter();
	Reference &self = helper.popParameter();

	helper.returnValue(create_boolean((*self.data<LibObject<chrono::milliseconds>>()->impl) == (*other.data<LibObject<chrono::milliseconds>>()->impl)));
}

MINT_FUNCTION(mint_parse_iso_date, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	string date = to_string(helper.popParameter());
	string time_zone;
	bool ok = false;

	chrono::milliseconds timepoint = parse_iso_date(date, &time_zone, &ok);

	if (ok) {
		WeakReference result = create_iterator();
		iterator_insert(result.data<Iterator>(), create_object(new chrono::milliseconds(timepoint)));
		iterator_insert(result.data<Iterator>(), create_string(time_zone));
		helper.returnValue(move(result));
	}
}
