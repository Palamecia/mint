#ifndef WINTZ_H
#define WINTZ_H

#include <system/stdio.h>
#include <timezoneapi.h>
#include <string>
#include <vector>

#ifdef MINT_TIMEZONE_WITH_ICU
#include <icu.h>
#endif

namespace mint {

using TimeZone = DYNAMIC_TIME_ZONE_INFORMATION;
static constexpr const int TM_YEAR_BASE = 1900;

void timezone_free(TimeZone *tz);

tm timezone_localtime(TimeZone *tz, time_t timer, bool *ok = nullptr);
time_t timezone_mktime(TimeZone *tz, const tm &time, bool *ok = nullptr);
bool timezone_match(TimeZone *tz1, TimeZone *tz2);

std::string timezone_default_name();
std::vector<std::string> timezone_list_names();
TimeZone *timezone_find(const char *time_zone);
int timezone_set_default(const char *time_zone);

}

#endif // WINTZ_H
