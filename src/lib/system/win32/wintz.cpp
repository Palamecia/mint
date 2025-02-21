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

#include "wintz.h"

#ifdef MINT_WITH_ICU
#include <icu.h>
#endif

#include <mint/system/errno.h>
#include <mint/system/utf8.h>
#include <unordered_map>
#include <cstring>
#include <memory>
#include <ctime>

#ifdef MINT_WITH_ICU
#define MAX_TZ_NAME_LENGTH ULOC_FULLNAME_CAPACITY
#else
#define MAX_TZ_NAME_LENGTH 160
#endif

static constexpr const int SECS_PER_DAY = 86400;
static constexpr const int SECS_PER_HOUR = 3600;
static constexpr const int SECS_PER_MIN = 60;
static constexpr const int MINS_PER_HOUR = 60;
static constexpr const int HOURS_PER_DAY = 24;
static constexpr const int EPOCH_WEEK_DAY = 1;
static constexpr const int DAYS_PER_WEEK = 7;
static constexpr const int EPOCH_YEAR = 1601;
static constexpr const int DAYS_PER_NORMAL_YEAR = 365;
static constexpr const int DAYS_PER_LEAP_YEAR = 366;
static constexpr const int MONS_PER_YEAR = 12;

static constexpr const ULONGLONG SECS_TO_UNIX = 11644473600ull;

static const unsigned int YearLengths[2] = {DAYS_PER_NORMAL_YEAR, DAYS_PER_LEAP_YEAR};

static const UCHAR MonthLengths[2][MONS_PER_YEAR] = {
	{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
	{31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
};

static __inline int IsLeapYear(int Year) {
	return Year % 4 == 0 && (Year % 100 != 0 || Year % 400 == 0) ? 1 : 0;
}

static int DaysSinceEpoch(int Year) {
	int Days;
	Year--; /* Don't include a leap day from the current year */
	Days = Year * DAYS_PER_NORMAL_YEAR + Year / 4 - Year / 100 + Year / 400;
	Days -= (EPOCH_YEAR - 1) * DAYS_PER_NORMAL_YEAR + (EPOCH_YEAR - 1) / 4 - (EPOCH_YEAR - 1) / 100
			+ (EPOCH_YEAR - 1) / 400;
	return Days;
}

static int TIME_DayLightCompareDate(const SYSTEMTIME *date, const SYSTEMTIME *compareDate) {

	int limit_day, dayinsecs;

	if (date->wMonth < compareDate->wMonth) {
		return -1; /* We are in a month before the date limit. */
	}

	if (date->wMonth > compareDate->wMonth) {
		return 1; /* We are in a month after the date limit. */
	}

	/* if year is 0 then date is in day-of-week format, otherwise
	 * it's absolute date.
	 */
	if (compareDate->wYear == 0) {
		WORD First;
		/* compareDate->wDay is interpreted as number of the week in the month
		 * 5 means: the last week in the month */
		int weekofmonth = compareDate->wDay;
		/* calculate the day of the first DayOfWeek in the month */
		First = (6 + compareDate->wDayOfWeek - date->wDayOfWeek + date->wDay) % 7 + 1;
		limit_day = First + 7 * (weekofmonth - 1);
		/* check needed for the 5th weekday of the month */
		if (limit_day > MonthLengths[date->wMonth == 2 && IsLeapYear(date->wYear)][date->wMonth - 1]) {
			limit_day -= 7;
		}
	}
	else {
		limit_day = compareDate->wDay;
	}

	/* convert to seconds */
	limit_day = ((limit_day * 24 + compareDate->wHour) * 60 + compareDate->wMinute) * 60;
	dayinsecs = ((date->wDay * 24 + date->wHour) * 60 + date->wMinute) * 60 + date->wSecond;

	/* and compare */
	return dayinsecs < limit_day ? -1 : dayinsecs > limit_day ? 1 : 0; /* date is equal to the date limit. */
}

static DWORD TIME_CompTimeZoneID(const DYNAMIC_TIME_ZONE_INFORMATION *pTZinfo, FILETIME *lpFileTime, BOOL islocal) {

#define TICKSPERMIN 600000000

#define LL2FILETIME(ll, pft) \
	(pft)->dwLowDateTime = (UINT)(ll); \
	(pft)->dwHighDateTime = (UINT)((ll) >> 32);
#define FILETIME2LL(pft, ll) ll = (((LONGLONG)((pft)->dwHighDateTime)) << 32) + (pft)->dwLowDateTime;

	int ret, year;
	BOOL beforeStandardDate, afterDaylightDate;
	DWORD retval = TIME_ZONE_ID_INVALID;
	LONGLONG llTime = 0; /* initialized to prevent gcc complaining */
	SYSTEMTIME SysTime;
	FILETIME ftTemp;

	if (pTZinfo->DaylightDate.wMonth != 0) {
		/* if year is 0 then date is in day-of-week format, otherwise
		 * it's absolute date.
		 */
		if (pTZinfo->StandardDate.wMonth == 0
			|| (pTZinfo->StandardDate.wYear == 0
				&& (pTZinfo->StandardDate.wDay < 1 || pTZinfo->StandardDate.wDay > 5 || pTZinfo->DaylightDate.wDay < 1
					|| pTZinfo->DaylightDate.wDay > 5))) {
			SetLastError(ERROR_INVALID_PARAMETER);
			return TIME_ZONE_ID_INVALID;
		}

		if (!islocal) {
			FILETIME2LL(lpFileTime, llTime);
			llTime -= pTZinfo->Bias * (LONGLONG)TICKSPERMIN;
			LL2FILETIME(llTime, &ftTemp)
			lpFileTime = &ftTemp;
		}

		FileTimeToSystemTime(lpFileTime, &SysTime);
		year = SysTime.wYear;

		if (!islocal) {
			llTime -= pTZinfo->DaylightBias * (LONGLONG)TICKSPERMIN;
			LL2FILETIME(llTime, &ftTemp)
			FileTimeToSystemTime(lpFileTime, &SysTime);
		}

		/* check for daylight savings */
		if (year == SysTime.wYear) {
			ret = TIME_DayLightCompareDate(&SysTime, &pTZinfo->StandardDate);
			if (ret == -2) {
				return TIME_ZONE_ID_INVALID;
			}

			beforeStandardDate = ret < 0;
		}
		else {
			beforeStandardDate = SysTime.wYear < year;
		}

		if (!islocal) {
			llTime -= (pTZinfo->StandardBias - pTZinfo->DaylightBias) * (LONGLONG)TICKSPERMIN;
			LL2FILETIME(llTime, &ftTemp)
			FileTimeToSystemTime(lpFileTime, &SysTime);
		}

		if (year == SysTime.wYear) {

			ret = TIME_DayLightCompareDate(&SysTime, &pTZinfo->DaylightDate);

			if (ret == -2) {
				return TIME_ZONE_ID_INVALID;
			}

			afterDaylightDate = ret >= 0;
		}
		else {
			afterDaylightDate = SysTime.wYear > year;
		}

		retval = TIME_ZONE_ID_STANDARD;

		if (pTZinfo->DaylightDate.wMonth < pTZinfo->StandardDate.wMonth) {
			/* Northern hemisphere */
			if (beforeStandardDate && afterDaylightDate) {
				retval = TIME_ZONE_ID_DAYLIGHT;
			}
		}
		else {
			/* Down south */
			if (beforeStandardDate || afterDaylightDate) {
				retval = TIME_ZONE_ID_DAYLIGHT;
			}
		}
	}
	else {
		/* No transition date */
		retval = TIME_ZONE_ID_UNKNOWN;
	}

	return retval;
}

static DWORD TIME_ZoneID(const DYNAMIC_TIME_ZONE_INFORMATION *pTzi, SYSTEMTIME *lpSystemTime) {
	FILETIME ftTime;
	SystemTimeToFileTime(lpSystemTime, &ftTime);
	return TIME_CompTimeZoneID(pTzi, &ftTime, FALSE);
}

static const std::unordered_map<std::wstring, mint::TimeZone> g_timezones = [] {
	std::unordered_map<std::wstring, mint::TimeZone> timezones;
	DYNAMIC_TIME_ZONE_INFORMATION dynamicTimezone = {};
	DWORD dwResult = 0;
	DWORD i = 0;

	do {
		dwResult = EnumDynamicTimeZoneInformation(i++, &dynamicTimezone);
		if (dwResult == ERROR_SUCCESS) {
			timezones.emplace(dynamicTimezone.TimeZoneKeyName, dynamicTimezone);
		}
	}
	while (dwResult != ERROR_NO_MORE_ITEMS);

	return timezones;
}();

void mint::timezone_free(TimeZone *tz) {
	delete tz;
}

tm mint::timezone_localtime(TimeZone *tz, time_t timer, bool *ok) {

	SYSTEMTIME localTime, universalTime;

	const UCHAR *Months;
	ULONG LeapYear, CurMonth;
	ULONGLONG IntTime = SECS_TO_UNIX + timer;

	/* Extract millisecond from time and convert time into seconds */
	universalTime.wMilliseconds = 0;

	/* Split the time into days and seconds within the day */
	ULONG Days = (ULONG)(IntTime / SECS_PER_DAY);
	ULONG SecondsInDay = IntTime % SECS_PER_DAY;

	/* Compute time of day */
	universalTime.wHour = (WORD)(SecondsInDay / SECS_PER_HOUR);
	universalTime.wMinute = (WORD)((SecondsInDay % SECS_PER_HOUR) / SECS_PER_MIN);
	universalTime.wSecond = (WORD)(SecondsInDay % SECS_PER_MIN);

	/* Compute day of week */
	universalTime.wDayOfWeek = (WORD)((EPOCH_WEEK_DAY + Days) % DAYS_PER_WEEK);

	/* Compute year */
	ULONG CurYear = EPOCH_YEAR;
	CurYear += Days / DAYS_PER_LEAP_YEAR;
	Days -= DaysSinceEpoch(CurYear);
	while (TRUE) {
		LeapYear = IsLeapYear(CurYear);
		if (Days < YearLengths[LeapYear]) {
			break;
		}
		CurYear++;
		Days = Days - YearLengths[LeapYear];
	}
	universalTime.wYear = (WORD)CurYear;

	/* Compute month of year */
	LeapYear = IsLeapYear(CurYear);
	Months = MonthLengths[LeapYear];
	for (CurMonth = 0; Days >= Months[CurMonth]; ++CurMonth) {
		Days = Days - Months[CurMonth];
	}
	universalTime.wMonth = (WORD)(CurMonth + 1);
	universalTime.wDay = (WORD)(Days + 1);

	if (SystemTimeToTzSpecificLocalTimeEx(tz, &universalTime, &localTime)) {

		if (ok) {
			*ok = true;
		}

		DWORD wYearDay = localTime.wDay;

		for (CurMonth = 1; CurMonth < universalTime.wMonth; CurMonth++) {
			wYearDay += MonthLengths[IsLeapYear(localTime.wYear)][CurMonth - 1];
		}

		tm ptm;

		ptm.tm_year = localTime.wYear - TM_YEAR_BASE;
		ptm.tm_mon = localTime.wMonth - 1;
		ptm.tm_yday = wYearDay;
		ptm.tm_wday = localTime.wDayOfWeek;
		ptm.tm_mday = localTime.wDay;
		ptm.tm_hour = localTime.wHour;
		ptm.tm_min = localTime.wMinute;
		ptm.tm_sec = localTime.wSecond;
		ptm.tm_isdst = TIME_ZoneID(tz, &localTime) == TIME_ZONE_ID_DAYLIGHT;

		return ptm;
	}

	if (ok) {
		*ok = false;
	}

	return {};
}

time_t mint::timezone_mktime(TimeZone *tzi, const tm &tm, bool *ok) {

	SYSTEMTIME localTime, universalTime;

	localTime.wYear = static_cast<WORD>(tm.tm_year + TM_YEAR_BASE);
	localTime.wMonth = static_cast<WORD>(tm.tm_mon + 1);
	localTime.wDayOfWeek = static_cast<WORD>(tm.tm_wday);
	localTime.wDay = static_cast<WORD>(tm.tm_mday);
	localTime.wHour = static_cast<WORD>(tm.tm_hour);
	localTime.wMinute = static_cast<WORD>(tm.tm_min);
	localTime.wSecond = static_cast<WORD>(tm.tm_sec);
	localTime.wMilliseconds = 0;

	if (TzSpecificLocalTimeToSystemTimeEx(tzi, &localTime, &universalTime)) {

		if (ok) {
			*ok = true;
		}

		ULONG CurMonth;

		/* Compute the time */
		ULONGLONG Time = DaysSinceEpoch(universalTime.wYear);

		for (CurMonth = 1; CurMonth < universalTime.wMonth; CurMonth++) {
			Time += MonthLengths[IsLeapYear(universalTime.wYear)][CurMonth - 1];
		}

		Time += universalTime.wDay - 1;
		Time *= SECS_PER_DAY;
		Time += universalTime.wHour * SECS_PER_HOUR + universalTime.wMinute * SECS_PER_MIN + universalTime.wSecond;

		return Time - SECS_TO_UNIX;
	}

	if (ok) {
		*ok = false;
	}

	return -1;
}

bool mint::timezone_match(TimeZone *tz1, TimeZone *tz2) {
	return !wcscmp(tz1->StandardName, tz2->StandardName) && !wcscmp(tz1->DaylightName, tz2->DaylightName);
}

std::string mint::timezone_default_name() {

#ifdef MINT_WITH_ICU
	UErrorCode status = U_ZERO_ERROR;
	UChar ucIanaId[MAX_TZ_NAME_LENGTH];

	ucal_getDefaultTimeZone(ucIanaId, ARRAYSIZE(ucIanaId), &status);

	if (U_SUCCESS(status)) {

		char default_name[MAX_TZ_NAME_LENGTH * sizeof(UChar)];

		u_strToUTF8(default_name, ARRAYSIZE(default_name), nullptr, ucIanaId, u_strlen(ucIanaId), &status);

		if (U_SUCCESS(status)) {
			return default_name;
		}
	}
#endif

	TimeZone timeZoneInformation;

	if (GetDynamicTimeZoneInformation(&timeZoneInformation) != TIME_ZONE_ID_INVALID) {

#ifdef MINT_WITH_ICU
		UChar ucWinId[MAX_TZ_NAME_LENGTH];

		u_strFromWCS(ucWinId, ARRAYSIZE(ucWinId), nullptr, timeZoneInformation.TimeZoneKeyName,
					 ARRAYSIZE(timeZoneInformation.TimeZoneKeyName), &status);

		if (U_SUCCESS(status)) {

			ucal_getTimeZoneIDForWindowsID(ucWinId, u_strlen(ucWinId), nullptr, ucIanaId, ARRAYSIZE(ucIanaId), &status);

			if (U_SUCCESS(status)) {

				char default_name[MAX_TZ_NAME_LENGTH * sizeof(UChar)];

				u_strToUTF8(default_name, ARRAYSIZE(default_name), nullptr, ucIanaId, u_strlen(ucIanaId), &status);

				if (U_SUCCESS(status)) {
					return default_name;
				}
			}
		}
#endif

		char default_name[MAX_TZ_NAME_LENGTH * sizeof(wchar_t)];
		WideCharToMultiByte(CP_UTF8, 0, timeZoneInformation.TimeZoneKeyName, -1, default_name,
							static_cast<int>(std::extent<decltype(default_name)>::value), nullptr, nullptr);
		return default_name;
	}

	return "";
}

std::vector<std::string> mint::timezone_list_names() {

#ifdef MINT_WITH_ICU
	UErrorCode status = U_ZERO_ERROR;
#endif
	std::vector<std::string> names;

#ifdef MINT_WITH_ICU
	UEnumeration *tz_list = ucal_openTimeZones(&status);

	if (U_SUCCESS(status)) {
		while (const char *name = uenum_next(tz_list, nullptr, &status)) {
			if (U_SUCCESS(status)) {
				names.emplace_back(name);
			}
		}
		uenum_close(tz_list);
	}
	else {
#endif
		for (const auto &timezone : g_timezones) {
			char name[MAX_TZ_NAME_LENGTH * sizeof(wchar_t)];
			WideCharToMultiByte(CP_UTF8, 0, timezone.first.c_str(), -1, name,
								static_cast<int>(std::extent<decltype(name)>::value), nullptr, nullptr);
			names.emplace_back(name);
		}
#ifdef MINT_WITH_ICU
	}
#endif

	return names;
}

mint::TimeZone *mint::timezone_find(const char *time_zone) {

#ifdef MINT_WITH_ICU
	UErrorCode status = U_ZERO_ERROR;
	UChar ucWinId[MAX_TZ_NAME_LENGTH];
	UChar ucIanaId[MAX_TZ_NAME_LENGTH];

	u_strFromUTF8(ucIanaId, ARRAYSIZE(ucIanaId), nullptr, time_zone, static_cast<int32_t>(strlen(time_zone)), &status);

	if (U_SUCCESS(status)) {

		ucal_getWindowsTimeZoneID(ucIanaId, -1, ucWinId, ARRAYSIZE(ucWinId), &status);

		if (U_SUCCESS(status)) {

			wchar_t windows_id[MAX_TZ_NAME_LENGTH];

			u_strToWCS(windows_id, ARRAYSIZE(windows_id), nullptr, ucWinId, u_strlen(ucWinId), &status);

			if (U_SUCCESS(status)) {

				auto it = g_timezones.find(windows_id);

				if (it != g_timezones.end()) {
					return new TimeZone(it->second);
				}
			}
		}
	}
#endif

	wchar_t windows_id[MAX_TZ_NAME_LENGTH];
	MultiByteToWideChar(CP_UTF8, 0, time_zone, -1, windows_id,
						static_cast<int>(std::extent<decltype(windows_id)>::value));
	auto it = g_timezones.find(windows_id);

	if (it != g_timezones.end()) {
		return new TimeZone(it->second);
	}

	char sign = '+';
	int hours = 0, minutes = 0;

	if (!utf8_compare(time_zone, "UTC") || sscanf(time_zone, "UTC%c%02d:%02d", &sign, &hours, &minutes) > 0) {

		std::unique_ptr<TimeZone> tz(new TimeZone);
		ZeroMemory(tz.get(), sizeof(TimeZone));

		MultiByteToWideChar(CP_UTF8, 0, time_zone, -1, tz->StandardName,
							static_cast<int>(std::extent<decltype(TimeZone::StandardName)>::value));

		switch (sign) {
		case '-':
			tz->Bias -= hours * 60 + minutes;
			return tz.release();
		case '+':
			tz->Bias += hours * 60 + minutes;
			return tz.release();
		default:
			break;
		}
	}

	return nullptr;
}

static USHORT get_current_year(mint::TimeZone *tz) {
	tm tm = mint::timezone_localtime(tz, ::time(nullptr));
	return static_cast<USHORT>(tm.tm_year + mint::TM_YEAR_BASE);
}

int mint::timezone_set_default(const char *time_zone) {

#ifdef MINT_WITH_ICU
	UErrorCode status = U_ZERO_ERROR;
	UChar ucWinId[MAX_TZ_NAME_LENGTH];
	UChar ucIanaId[MAX_TZ_NAME_LENGTH];

	u_strFromUTF8(ucIanaId, ARRAYSIZE(ucIanaId), nullptr, time_zone, static_cast<int32_t>(strlen(time_zone)), &status);

	if (U_SUCCESS(status)) {

		ucal_getWindowsTimeZoneID(ucIanaId, -1, ucWinId, ARRAYSIZE(ucWinId), &status);

		if (U_SUCCESS(status)) {

			wchar_t windows_id[MAX_TZ_NAME_LENGTH];

			u_strToWCS(windows_id, ARRAYSIZE(windows_id), nullptr, ucWinId, u_strlen(ucWinId), &status);

			if (U_SUCCESS(status)) {

				auto it = g_timezones.find(windows_id);

				if (it != g_timezones.end()) {

					std::unique_ptr<DYNAMIC_TIME_ZONE_INFORMATION> pdtzi(new DYNAMIC_TIME_ZONE_INFORMATION(it->second));
					USHORT wYear = get_current_year(pdtzi.get());
					TIME_ZONE_INFORMATION tzi;

					if (!GetTimeZoneInformationForYear(wYear, pdtzi.get(), &tzi)) {
						return errno_from_error_code(last_error_code());
					}

					if (!SetTimeZoneInformation(&tzi)) {
						return errno_from_error_code(last_error_code());
					}

					return 0;
				}
			}
		}
	}
#endif

	wchar_t windows_id[MAX_TZ_NAME_LENGTH];
	MultiByteToWideChar(CP_UTF8, 0, time_zone, -1, windows_id,
						static_cast<int>(std::extent<decltype(windows_id)>::value));
	auto it = g_timezones.find(windows_id);

	if (it != g_timezones.end()) {

		std::unique_ptr<DYNAMIC_TIME_ZONE_INFORMATION> pdtzi(new DYNAMIC_TIME_ZONE_INFORMATION(it->second));
		USHORT wYear = get_current_year(pdtzi.get());
		TIME_ZONE_INFORMATION tzi;

		if (!GetTimeZoneInformationForYear(wYear, pdtzi.get(), &tzi)) {
			return errno_from_error_code(last_error_code());
		}

		if (!SetTimeZoneInformation(&tzi)) {
			return errno_from_error_code(last_error_code());
		}

		return 0;
	}

	return EINVAL;
}
