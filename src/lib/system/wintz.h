#ifndef WINTZ_H
#define WINTZ_H

#include <system/stdio.h>
#include <timezoneapi.h>
#include <string>
#include <vector>

using TimeZone = TIME_ZONE_INFORMATION;
static constexpr const int TM_YEAR_BASE = 1900;

TimeZone *wintz_read(HKEY hKey);
void wintz_free(TimeZone *tz);

tm wintz_localtime(TimeZone *tz, time_t timer, bool *ok = nullptr);
time_t wintz_mktime(TimeZone *tz, const tm &time, bool *ok = nullptr);
bool wintz_match(TimeZone *tz1, TimeZone *tz2);

const char *wintz_default_name();
std::vector<std::string> wintz_list_names();
TimeZone *wintz_find(const char *time_zone);

#endif // WINTZ_H
