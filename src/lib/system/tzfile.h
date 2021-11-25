#ifndef TZFILE_H
#define TZFILE_H

#include <system/stdio.h>
#include <string>
#include <vector>

struct tzfile;
using TimeZone = tzfile;
static constexpr const int TM_YEAR_BASE = 1900;

tzfile *tzfile_read(FILE *file, size_t size, char **extra = nullptr, size_t extra_size = 0);
void tzfile_free(tzfile *tz);

tm tzfile_localtime(tzfile *tz, time_t timer, bool *ok = nullptr);
time_t tzfile_mktime(tzfile *tz, const tm &time, bool *ok = nullptr);
bool tzfile_match(tzfile *tz1, tzfile *tz2);

const char *tzfile_default_name();
std::vector<std::string> tzfile_list_names();
tzfile *tzfile_find(const char *time_zone);

#endif // TZFILE_H
