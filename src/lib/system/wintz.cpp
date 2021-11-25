#include "wintz.h"

#ifdef MINT_TIMEZONE_WITH_ICU
#include <icu.h>
#endif

#include <cstring>
#include <memory>
#include <ctime>
#include <map>

#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 16383

static const std::wstring TIME_ZONE_KEY_PATH = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones";
static constexpr const time_t DAYS_PER_YEAR = 365;
static constexpr const time_t DAYS_PER_4_YEAR = (4 * DAYS_PER_YEAR + 1);
static constexpr const time_t DAYS_PER_100_YEAR = (25 * DAYS_PER_4_YEAR - 1);
static constexpr const time_t DAYS_PER_400_YEAR = (4 * DAYS_PER_100_YEAR + 1);
static constexpr const time_t DIFF_DAYS = (3 * DAYS_PER_100_YEAR + 17 * DAYS_PER_4_YEAR + 1 * DAYS_PER_YEAR);
static constexpr const time_t SECS_PER_HOUR = (60 * 60);
static constexpr const time_t SECS_PER_DAY = (SECS_PER_HOUR * 24);
static constexpr const time_t LEAP_DAY = 59;

unsigned int g_monthdays[13] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};
unsigned int g_lpmonthdays[13] = {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366};

static const std::map<std::string, std::wstring> g_timezones = [] {

	std::map<std::string, std::wstring> timezones;

	HKEY hKey;
	LONG lResult = RegOpenKeyExW(HKEY_LOCAL_MACHINE, TIME_ZONE_KEY_PATH.c_str(), 0, KEY_READ, &hKey);

	if (lResult != ERROR_SUCCESS) {
		return timezones;
	}

	DWORD dwCount = 0;

	lResult = RegQueryInfoKeyW(hKey, nullptr, nullptr, nullptr, &dwCount, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);

	if (lResult != ERROR_SUCCESS) {
		return timezones;
	}

	for (DWORD dwIndex = 0; dwIndex < dwCount; ++dwIndex) {

		wchar_t szKeyName[MAX_KEY_LENGTH];
		DWORD dwKeyLength = MAX_KEY_LENGTH;

		lResult = RegEnumKeyExW(hKey, dwIndex, szKeyName, &dwKeyLength, nullptr, nullptr, nullptr, nullptr);

		if (lResult != ERROR_SUCCESS) {
			continue;
		}

		char szName[MAX_KEY_LENGTH * 4];

		if (WideCharToMultiByte(CP_UTF8, 0, szKeyName, -1, szName, static_cast<int>(sizeof szName), nullptr, nullptr)) {
			timezones.emplace(szName, TIME_ZONE_KEY_PATH + L"\\" + szKeyName);
		}
	}

	return timezones;
} ();

static std::map<std::wstring, std::string> g_displayToWindowsId = [] {

	std::map<std::wstring, std::string> timezones;

	for (const auto &time_zone : g_timezones) {

		HKEY hKey;
		LONG lResult = RegOpenKeyExW(HKEY_LOCAL_MACHINE, time_zone.second.c_str(), 0, KEY_READ, &hKey);

		if (lResult != ERROR_SUCCESS) {
			continue;
		}

		wchar_t szBuffer[512];
		DWORD dwBufferSize = sizeof(szBuffer);

		lResult = RegQueryValueExW(hKey, L"Std", nullptr, nullptr, reinterpret_cast<LPBYTE>(szBuffer), &dwBufferSize);

		if (lResult == ERROR_SUCCESS) {
			timezones.emplace(szBuffer, time_zone.first);
		}

		dwBufferSize = sizeof(szBuffer);
		lResult = RegQueryValueExW(hKey, L"Dlt", nullptr, nullptr, reinterpret_cast<LPBYTE>(szBuffer), &dwBufferSize);

		if (lResult == ERROR_SUCCESS) {
			timezones.emplace(szBuffer, time_zone.first);
		}
	}

	return timezones;
} ();

TimeZone *wintz_read(HKEY hKey) {

	std::unique_ptr<TimeZone> tz(new TimeZone);

	DWORD dwBufferSize = sizeof(TimeZone);
	ULONG lResult = RegQueryValueExW(hKey, L"TZI", 0, NULL, reinterpret_cast<LPBYTE>(tz.get()), &dwBufferSize);

	if (lResult != ERROR_SUCCESS) {
		return nullptr;
	}

	return tz.release();
}

void wintz_free(TimeZone *tz) {
	delete tz;
}

static __inline long leapyears_passed(long days) {
	long quadcenturies, centuries, quadyears;
	quadcenturies = days / DAYS_PER_400_YEAR;
	days -= quadcenturies;
	centuries = days / DAYS_PER_100_YEAR;
	days += centuries;
	quadyears = days / DAYS_PER_YEAR;
	return quadyears - centuries + quadcenturies;
}

static __inline long leapdays_passed(long days) {
	return leapyears_passed(days + DAYS_PER_YEAR - LEAP_DAY + 1);
}

tm wintz_localtime(TimeZone *tz, time_t timer, bool *ok) {

	unsigned int days, daystoyear, dayinyear, leapdays, leapyears, years, month;
	unsigned int secondinday, secondinhour;
	unsigned int *padays;
	__time64_t time = timer;
	tm ptm;

	if (ok) {
		*ok = false;
	}

	if (time < 0) {
		return {};
	}

	/* Divide into date and time */
	days = (unsigned int)(time / SECS_PER_DAY);
	secondinday = time % SECS_PER_DAY;

	/* Shift to days from 1.1.1601 */
	days += DIFF_DAYS;

	/* Calculate leap days passed till today */
	leapdays = leapdays_passed(days);

	/* Calculate number of full leap years passed */
	leapyears = leapyears_passed(days);

	/* Are more leap days passed than leap years? */
	if (leapdays > leapyears) {
		/* Yes, we're in a leap year */
		padays = g_lpmonthdays;
	}
	else {
		/* No, normal year */
		padays = g_monthdays;
	}

	/* Calculate year */
	years = (days - leapdays) / 365;
	ptm.tm_year = years - 299;

	/* Calculate number of days till 1.1. of this year */
	daystoyear = years * 365 + leapyears;

	/* Calculate the day in this year */
	dayinyear = days - daystoyear;

	/* Shall we do DST corrections? */
	ptm.tm_isdst = 0;
	/*
	int yeartime = dayinyear * SECS_PER_DAY + secondinday;
	if (yeartime >= dst_begin && yeartime <= dst_end) { // FIXME! DST in winter
		time -= tz->DaylightBias;
		days = (unsigned int)(time / SECS_PER_DAY + DIFF_DAYS);
		dayinyear = days - daystoyear;
		ptm.tm_isdst = 1;
	}
	*/

	ptm.tm_yday = dayinyear;

	/* dayinyear < 366 => terminates with i <= 11 */
	for (month = 0; dayinyear >= padays[month+1]; month++)
		;

	/* Set month and day in month */
	ptm.tm_mon = month;
	ptm.tm_mday = 1 + dayinyear - padays[month];

	/* Get weekday */
	ptm.tm_wday = (days + 1) % 7;

	/* Calculate hour and second in hour */
	ptm.tm_hour = secondinday / SECS_PER_DAY;
	secondinhour = secondinday % SECS_PER_DAY;

	/* Calculate minute and second */
	ptm.tm_min = secondinhour / 60;
	ptm.tm_sec = secondinhour % 60;

	if (ok) {
		*ok = true;
	}

	return ptm;
}

time_t wintz_mktime(TimeZone *tzi, const tm &tm, bool *ok) {

	struct tm ptm = tm;
	int mons, years, leapyears;

	if (ok) {
		*ok = false;
	}

	/* Normalize year and month */
	if (ptm.tm_mon < 0) {
		mons = -ptm.tm_mon - 1;
		ptm.tm_year -= 1 + mons / 12;
		ptm.tm_mon = 11 - (mons % 12);
	}
	else if (ptm.tm_mon > 11) {
		mons = ptm.tm_mon;
		ptm.tm_year += (mons / 12);
		ptm.tm_mon = mons % 12;
	}

	/* Is it inside margins */
	if (ptm.tm_year < 70 || ptm.tm_year > 139) {// FIXME: max year for 64 bits
		return -1;
	}

	years = ptm.tm_year - 70;

	/* Number of leapyears passed since 1970 */
	leapyears = (years + 1) / 4;

	/* Calculate days up to 1st of Jan */
	__time64_t time = years * 365 + leapyears;

	/* Calculate days up to 1st of month */
	time += g_monthdays[ptm.tm_mon];

	/* Check if we need to add a leap day */
	if (((years + 2) % 4) == 0) {
		if (ptm.tm_mon > 2) {
			time++;
		}
	}

	time += ptm.tm_mday - 1;

	time *= 24;
	time += ptm.tm_hour;

	time *= 60;
	time += ptm.tm_min;

	time *= 60;
	time += ptm.tm_sec;

	if (time < 0) {
		return -1;
	}

	/* Finally adjust by the difference to GMT in seconds */
	time += tzi->Bias * 60;

	if (ok) {
		*ok = true;
	}

	return time;
}

bool wintz_match(TimeZone *tz1, TimeZone *tz2) {
	return !wcscmp(tz1->StandardName, tz2->StandardName) && !wcscmp(tz1->DaylightName, tz2->DaylightName);
}

const char *wintz_default_name() {

	TIME_ZONE_INFORMATION timeZoneInformation;
	auto it = g_displayToWindowsId.end();

	switch (GetTimeZoneInformation(&timeZoneInformation)) {
	case TIME_ZONE_ID_STANDARD:
		it = g_displayToWindowsId.find(timeZoneInformation.StandardName);
		break;
	case TIME_ZONE_ID_DAYLIGHT:
		it = g_displayToWindowsId.find(timeZoneInformation.DaylightName);
		break;
	default:
		break;
	}

	if (it != g_displayToWindowsId.end()) {

#ifdef MINT_TIMEZONE_WITH_ICU
		UErrorCode status;
		UChar ucWinId[128];
		UChar ucIanaId[128];

		u_strFromUTF8(ucWinId, ARRAYSIZE(ucWinId), nullptr, it->second.c_str(), static_cast<int32_t>(it->second.length()), &status);

		if (U_SUCCESS(status)) {

			ucal_getTimeZoneIDForWindowsID(ucWinId, u_strlen(ucWinId), nullptr, ucIanaId, ARRAYSIZE(ucIanaId), &status);

			if (U_SUCCESS(status)) {

				static char g_default_name[255];

				u_strToUTF8(g_default_name, ARRAYSIZE(g_default_name), nullptr, ucIanaId, u_strlen(ucIanaId), &status);

				if (U_SUCCESS(status)) {
					return g_default_name;
				}
			}
		}
#endif

		return it->second.c_str();
	}

	return "";
}

std::vector<std::string> wintz_list_names() {

#ifdef MINT_TIMEZONE_WITH_ICU
	UErrorCode status;
#endif
	std::vector<std::string> names;

#ifdef MINT_TIMEZONE_WITH_ICU
	UEnumeration* tz_list = ucal_openTimeZones(&status);

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
			names.emplace_back(timezone.first);
		}
#ifdef MINT_TIMEZONE_WITH_ICU
	}
#endif

	return names;
}

TimeZone *wintz_find(const char *time_zone) {

#ifdef MINT_TIMEZONE_WITH_ICU
	UErrorCode status;
	UChar ucWinId[128];
	UChar ucIanaId[128];

	u_strFromUTF8(ucIanaId, ARRAYSIZE(ucIanaId), nullptr, time_zone, static_cast<int32_t>(strlen(time_zone)), &status);

	if (U_SUCCESS(status)) {

		ucal_getWindowsTimeZoneID(ucIanaId, -1, ucWinId, ARRAYSIZE(ucWinId), &status);

		if (U_SUCCESS(status)) {

			char windows_id[255];

			u_strToUTF8(windows_id, ARRAYSIZE(windows_id), nullptr, ucWinId, u_strlen(ucWinId), &status);

			if (U_SUCCESS(status)) {

				auto it = g_timezones.find(windows_id);

				if (it != g_timezones.end()) {

					HKEY hKey;
					LONG lResult = RegOpenKeyExW(HKEY_LOCAL_MACHINE, it->second.c_str(), 0, KEY_READ, &hKey);

					if (lResult != ERROR_SUCCESS) {
						return nullptr;
					}

					return wintz_read(hKey);
				}
			}
		}
	}
#endif

	auto it = g_timezones.find(time_zone);

	if (it != g_timezones.end()) {

		HKEY hKey;
		LONG lResult = RegOpenKeyExW(HKEY_LOCAL_MACHINE, it->second.c_str(), 0, KEY_READ, &hKey);

		if (lResult != ERROR_SUCCESS) {
			return nullptr;
		}

		return wintz_read(hKey);
	}

	char sign = '+';
	int hours = 0, minutes = 0;

	if (!strcmp(time_zone, "UTC") || sscanf(time_zone, "UTC%c%02d:%02d", &sign, &hours, &minutes) > 0) {

		std::unique_ptr<TimeZone> tz(new TimeZone);
		ZeroMemory(tz.get(), sizeof (TimeZone));

		MultiByteToWideChar(CP_UTF8, 0, time_zone, -1, tz->StandardName, static_cast<int>(std::extent<decltype(TimeZone::StandardName)>::value));

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
