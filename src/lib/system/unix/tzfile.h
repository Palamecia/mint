#ifndef TZFILE_H
#define TZFILE_H

#include <mint/system/stdio.h>
#include <string>
#include <vector>

struct tzfile;

namespace mint {

using TimeZone = tzfile;
static constexpr const int TM_YEAR_BASE = 1900;

TimeZone *timezone_read(FILE *file, size_t size, char **extra = nullptr, size_t extra_size = 0);
void timezone_free(tzfile *tz);

tm timezone_localtime(TimeZone *tz, time_t timer, bool *ok = nullptr);
time_t timezone_mktime(TimeZone *tz, const tm &time, bool *ok = nullptr);
bool timezone_match(TimeZone *tz1, TimeZone *tz2);

std::string timezone_default_name();
std::vector<std::string> timezone_list_names();
TimeZone *timezone_find(const char *time_zone);
int timezone_set_default(const char *time_zone);

}

#endif // TZFILE_H
