#include "tzfile.h"

#include <mint/system/filesystem.h>
#include <mint/system/assert.h>

#include <byteswap.h>
#include <algorithm>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <string>
#include <sys/param.h>
#include <unordered_map>
#include <vector>

#if BYTE_ORDER != BIG_ENDIAN
#define decode(_x) bswap_32(_x)
#define decode64(_x) bswap_64(_x)
#else
#define decode(_x) (_x)
#define decode64(_x) (_x)
#endif

using namespace mint;

static constexpr const char *TZ_MAGIC = "TZif";
static constexpr const size_t TZ_NAME_MAX = NAME_MAX * 2 + 2;
static constexpr const time_t SECS_PER_HOUR = (60 * 60);
static constexpr const time_t SECS_PER_DAY = (SECS_PER_HOUR * 24);
static constexpr const int EPOCH_YEAR = 1970;

struct tzinfo {
	std::string code;
	std::string coordinates;
	std::string comment;
};

struct ttinfo {
	std::int32_t tt_utoff;
	std::uint8_t tt_isdst;
	std::uint8_t tt_desigidx;
	std::uint8_t tt_isstd;
	std::uint8_t tt_isgmt;
};

struct leap {
	std::time_t transition;
	std::int32_t change;
};

struct tzhead {
	char tzh_magic[4];
	char tzh_version;
	std::uint8_t tzh_reserved[15];
	std::uint32_t tzh_ttisgmtcnt;
	std::uint32_t tzh_ttisstdcnt;
	std::uint32_t tzh_leapcnt;
	std::uint32_t tzh_timecnt;
	std::uint32_t tzh_typecnt;
	std::uint32_t tzh_charcnt;
};

struct tzfile {
	char *tz_name[2];
	tzhead tz_head;
	int tz_daylight;
	long int tz_timezone;
	std::int32_t tz_stdoff;
	std::int32_t tz_dstoff;
	std::time_t *tz_transitions;
	leap *tz_leaps;
	ttinfo *tz_types;
	std::uint8_t *tz_typeidxs;
	char *tz_zonenames;
	char *tz_specs;
};

struct tzrule {
	const char *name;

	enum {
		J0,
		J1,
		M
	} type;

	int secs;
	int offset;
	int computed_for;
	std::time_t change;
	unsigned short int m;
	unsigned short int n;
	unsigned short int d;
};

static bool leap_year(long int year) {
	return ((year & 3) == 0 && (year % 100 != 0 || ((year / 100) & 3) == (-(TM_YEAR_BASE / 100) & 3)));
}

static constexpr const time_t MONTH_YEAR_DAY[2][13] = {
	// Normal years
	{0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365},
	// Leap years
	{0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366}};

static std::unordered_map<std::string, tzinfo> setup_tz_files() {

	std::unordered_map<std::string, tzinfo> tz_files;
	std::filesystem::path path = "/usr/share/zoneinfo/zone.tab";

	if (!std::filesystem::exists(path)) {
		path = "/usr/lib/zoneinfo/zone.tab";
	}

	std::ifstream stream(path.c_str());

	if (stream.good()) {
		while (!stream.eof()) {

			std::string line;
			std::string name;
			tzinfo info;

			getline(stream, line);

			if (line.empty() || line[0] == '#') {
				continue;
			}

			size_t from = 0;
			size_t pos = line.find('\t');

			if (pos != std::string::npos) {
				info.code = line.substr(from, pos - from);
				pos = line.find('\t', from = pos + 1);
			}

			if (pos != std::string::npos) {
				info.coordinates = line.substr(from, pos - from);
				pos = line.find('\t', from = pos + 1);
			}

			if ((pos != std::string::npos) || (from < line.size())) {
				name = line.substr(from, pos - from);
				if (pos != std::string::npos) {
					pos = line.find('\t', from = pos + 1);
				}
				else {
					from = line.size();
				}
			}

			if (from < line.size()) {
				info.comment = line.substr(from, pos - from);
			}

			tz_files.emplace(name, info);
		}
	}

	return tz_files;
};

static std::unordered_map<std::string, tzinfo> g_tz_files = setup_tz_files();

static bool tzhead_read(tzhead *tz_head, FILE *file) {

	if (UNLIKELY(fread(tz_head, sizeof(tzhead), 1, file) != 1)
		|| UNLIKELY(memcmp(&tz_head->tzh_magic, TZ_MAGIC, sizeof(tz_head->tzh_magic)) != 0)) {
		return false;
	}

#if BYTE_ORDER != BIG_ENDIAN
	tz_head->tzh_ttisgmtcnt = decode(tz_head->tzh_ttisgmtcnt);
	tz_head->tzh_ttisstdcnt = decode(tz_head->tzh_ttisstdcnt);
	tz_head->tzh_leapcnt = decode(tz_head->tzh_leapcnt);
	tz_head->tzh_timecnt = decode(tz_head->tzh_timecnt);
	tz_head->tzh_typecnt = decode(tz_head->tzh_typecnt);
	tz_head->tzh_charcnt = decode(tz_head->tzh_charcnt);
#endif

	if (UNLIKELY(tz_head->tzh_ttisstdcnt > tz_head->tzh_typecnt || tz_head->tzh_ttisgmtcnt > tz_head->tzh_typecnt)) {
		return false;
	}

	return true;
}

TimeZone *mint::timezone_read(FILE *file, size_t size, char **extra, size_t extra_size) {

	auto tz = std::make_unique<tzfile>();
	tzhead *tz_head = &tz->tz_head;
	size_t trans_width = 4;

	static_assert(sizeof(time_t) == 8, "time_t must be eight bytes");

	if (!tzhead_read(tz_head, file)) {
		return nullptr;
	}

	if (tz_head->tzh_version != '\0') {

		trans_width = 8;

		long int to_skip = (tz_head->tzh_timecnt * (4 + 1) + tz_head->tzh_typecnt * 6 + tz_head->tzh_charcnt
							+ tz_head->tzh_leapcnt * 8 + tz_head->tzh_ttisstdcnt + tz_head->tzh_ttisgmtcnt);

		if (fseek(file, to_skip, SEEK_CUR) != 0) {
			return nullptr;
		}

		if (!tzhead_read(tz_head, file)) {
			return nullptr;
		}
	}

	size_t tzspec_len = 0;

	if (trans_width == 8) {

		off_t rem = off_t(size) - ftello(file);

		if (UNLIKELY(rem < 0
					 || size_t(rem) < size_t(tz_head->tzh_timecnt * (8 + 1) + tz_head->tzh_typecnt * 6
											 + tz_head->tzh_charcnt))) {
			return nullptr;
		}

		tzspec_len = size_t(rem) - (tz_head->tzh_timecnt * (8 + 1) + tz_head->tzh_typecnt * 6 + tz_head->tzh_charcnt);

		if (UNLIKELY(size_t(tz_head->tzh_leapcnt) > SIZE_MAX / 12 || tzspec_len < tz_head->tzh_leapcnt * 12)) {
			return nullptr;
		}

		tzspec_len -= tz_head->tzh_leapcnt * 12;

		if (UNLIKELY(tzspec_len < tz_head->tzh_ttisstdcnt)) {
			return nullptr;
		}

		tzspec_len -= tz_head->tzh_ttisstdcnt;

		if (UNLIKELY(tzspec_len == 0 || tzspec_len - 1 < tz_head->tzh_ttisgmtcnt)) {
			return nullptr;
		}

		tzspec_len -= tz_head->tzh_ttisgmtcnt + 1;

		if (tzspec_len == 0) {
			return nullptr;
		}
	}

	auto tz_transitions = std::make_unique<time_t[]>(tz_head->tzh_timecnt);
	auto tz_leaps = std::make_unique<leap[]>(tz_head->tzh_leapcnt);
	auto tz_types = std::make_unique<ttinfo[]>(tz_head->tzh_typecnt);
	auto tz_typeidxs = std::make_unique<uint8_t[]>(tz_head->tzh_timecnt);
	auto tz_zonenames = std::make_unique<char[]>(tz_head->tzh_charcnt);
	std::unique_ptr<char[]> tz_specs;
	std::unique_ptr<char[]> tz_extra;

	if (trans_width == 8) {
		tz_specs = std::make_unique<char[]>(tzspec_len);
	}

	tz->tz_transitions = tz_transitions.get();
	tz->tz_leaps = tz_leaps.get();
	tz->tz_types = tz_types.get();
	tz->tz_typeidxs = tz_typeidxs.get();
	tz->tz_zonenames = tz_zonenames.get();
	tz->tz_specs = tz_specs.get();

	if (extra && extra_size) {
		tz_extra = std::make_unique<char[]>(extra_size);
		*extra = tz_extra.get();
	}

	if (UNLIKELY(fread(tz->tz_transitions, trans_width, tz_head->tzh_timecnt, file) != tz_head->tzh_timecnt)
		|| UNLIKELY(fread(tz->tz_typeidxs, 1, tz_head->tzh_timecnt, file) != tz_head->tzh_timecnt)) {
		return nullptr;
	}

	for (size_t i = 0; i < tz_head->tzh_timecnt; ++i) {
		if (UNLIKELY(tz->tz_typeidxs[i] >= tz_head->tzh_typecnt)) {
			return nullptr;
		}
	}

	if (trans_width == 4) {
		size_t i = tz_head->tzh_timecnt;
		while (i-- > 0) {
			tz->tz_transitions[i] = static_cast<time_t>(decode(reinterpret_cast<uint32_t *>(tz->tz_transitions)[i]));
		}
	}
#if BYTE_ORDER != BIG_ENDIAN
	else {
		for (size_t i = 0; i < tz_head->tzh_timecnt; ++i) {
			tz->tz_transitions[i] = static_cast<time_t>(decode64(reinterpret_cast<uint64_t *>(tz->tz_transitions)[i]));
		}
	}
#endif

	for (size_t i = 0; i < tz_head->tzh_typecnt; ++i) {

		int32_t x;
		uint8_t c;

		if (UNLIKELY(fread(&x, 1, sizeof(x), file) != sizeof(x))) {
			return nullptr;
		}

		c = static_cast<uint8_t>(getc(file));

		if (UNLIKELY(c > 1u)) {
			return nullptr;
		}

		tz->tz_types[i].tt_isdst = c;

		c = static_cast<uint8_t>(getc(file));

		if (UNLIKELY(c > tz_head->tzh_charcnt)) {
			return nullptr;
		}

		tz->tz_types[i].tt_desigidx = c;
		tz->tz_types[i].tt_utoff = static_cast<int32_t>(decode(x));
	}

	if (UNLIKELY(fread(tz->tz_zonenames, 1, tz_head->tzh_charcnt, file) != tz_head->tzh_charcnt)) {
		return nullptr;
	}

	for (size_t i = 0; i < tz_head->tzh_leapcnt; ++i) {

		int32_t x;

		if (UNLIKELY(fread(&x, 1, trans_width, file) != trans_width)) {
			return nullptr;
		}

		if (trans_width == 4) {
			tz->tz_leaps[i].transition = static_cast<time_t>(decode(x));
		}
		else {
			tz->tz_leaps[i].transition = static_cast<time_t>(decode64(x));
		}

		if (UNLIKELY(fread(&x, 1, 4, file) != 4)) {
			return nullptr;
		}

		tz->tz_leaps[i].change = static_cast<int>(decode(x));
	}

	for (size_t i = 0; i < tz_head->tzh_ttisstdcnt; ++i) {

		int c = getc(file);

		if (UNLIKELY(c == EOF)) {
			return nullptr;
		}

		tz->tz_types[i].tt_isstd = c != 0;
	}

	for (size_t i = tz_head->tzh_ttisstdcnt; i < tz_head->tzh_typecnt; ++i) {
		tz->tz_types[i].tt_isstd = 0;
	}

	for (size_t i = 0; i < tz_head->tzh_ttisgmtcnt; ++i) {

		int c = getc(file);

		if (UNLIKELY(c == EOF)) {
			return nullptr;
		}

		tz->tz_types[i].tt_isgmt = c != 0;
	}

	for (size_t i = tz_head->tzh_ttisgmtcnt; i < tz_head->tzh_typecnt; ++i) {
		tz->tz_types[i].tt_isgmt = 0;
	}

	if (tz_specs) {

		assert(tzspec_len > 0);

		if (getc(file) != '\n' || (fread(tz->tz_specs, 1, tzspec_len - 1, file) != tzspec_len - 1)) {
			tz_specs.reset();
		}
		else {
			tz->tz_specs[tzspec_len - 1] = '\0';
		}
	}

	if (tz_specs && tz_specs[0] == '\0') {
		tz_specs.reset();
	}

	tz->tz_name[0] = nullptr;
	tz->tz_name[1] = nullptr;

	for (size_t i = tz_head->tzh_timecnt; i > 0;) {

		int type = tz->tz_typeidxs[--i];
		int dst = tz->tz_types[type].tt_isdst;

		if (tz->tz_name[dst] == nullptr) {

			int idx = tz->tz_types[type].tt_desigidx;
			tz->tz_name[dst] = &tz->tz_zonenames[idx];

			if (tz->tz_name[1 - dst] != nullptr) {
				break;
			}
		}
	}

	if (tz->tz_name[0] == nullptr) {
		assert(tz_head->tzh_typecnt == 1);
		tz->tz_name[0] = tz->tz_zonenames;
	}

	if (tz->tz_name[1] == nullptr) {
		tz->tz_name[1] = tz->tz_name[0];
	}

	int32_t rule_stdoff = 0;
	int32_t rule_dstoff = 0;

	if (tz_head->tzh_timecnt == 0) {
		rule_stdoff = rule_dstoff = tz->tz_types[0].tt_utoff;
	}
	else {

		int stdoff_set = 0, dstoff_set = 0;
		rule_stdoff = rule_dstoff = 0;
		size_t i = tz_head->tzh_timecnt - 1;

		do {
			if (!stdoff_set && !tz->tz_types[tz->tz_typeidxs[i]].tt_isdst) {
				stdoff_set = 1;
				rule_stdoff = tz->tz_types[tz->tz_typeidxs[i]].tt_utoff;
			}
			else if (!dstoff_set && tz->tz_types[tz->tz_typeidxs[i]].tt_isdst) {
				dstoff_set = 1;
				rule_dstoff = tz->tz_types[tz->tz_typeidxs[i]].tt_utoff;
			}

			if (stdoff_set && dstoff_set) {
				break;
			}
		}
		while (i-- > 0);

		if (!dstoff_set) {
			rule_dstoff = rule_stdoff;
		}
	}

	tz->tz_daylight = rule_stdoff != rule_dstoff;
	tz->tz_timezone = -rule_stdoff;
	tz->tz_stdoff = rule_stdoff;
	tz->tz_dstoff = rule_dstoff;
	tz->tz_transitions = tz_transitions.release();
	tz->tz_leaps = tz_leaps.release();
	tz->tz_types = tz_types.release();
	tz->tz_typeidxs = tz_typeidxs.release();
	tz->tz_zonenames = tz_zonenames.release();
	tz->tz_specs = tz_specs.release();

	if (extra) {
		*extra = tz_extra.release();
	}

	return tz.release();
}

void mint::timezone_free(tzfile *tz) {
	delete[] tz->tz_transitions;
	delete[] tz->tz_leaps;
	delete[] tz->tz_types;
	delete[] tz->tz_typeidxs;
	delete[] tz->tz_zonenames;
	delete[] tz->tz_specs;
	delete tz;
}

static TimeZone *tzfile_default(const char *std, const char *dst, int stdoff, int dstoff) {

	char *cptr = nullptr;
	size_t stdlen = strlen(std) + 1;
	size_t dstlen = strlen(dst) + 1;

	std::filesystem::path tz_dir = "/usr/share/zoneinfo";

	if (!std::filesystem::exists(tz_dir)) {
		tz_dir = "/usr/lib/zoneinfo";
	}

	std::unique_ptr<tzfile> tz;
	std::filesystem::path path = tz_dir / "posixrules";

	if (FILE *file = fopen(path.c_str(), "rce")) {
		tz.reset(mint::timezone_read(file, std::filesystem::file_size(path), &cptr, stdlen + dstlen));
		fclose(file);
	}

	if (tz->tz_head.tzh_typecnt < 2) {
		return nullptr;
	}

	mempcpy(mempcpy(cptr, std, stdlen), dst, dstlen);
	tz->tz_zonenames = cptr;

	tz->tz_head.tzh_typecnt = 2;

	bool isdst = false;

	for (size_t i = 0; i < tz->tz_head.tzh_timecnt; ++i) {

		struct ttinfo *trans_type = &tz->tz_types[tz->tz_typeidxs[i]];

		tz->tz_typeidxs[i] = trans_type->tt_isdst;

		if (!trans_type->tt_isgmt) {
			if (isdst && !trans_type->tt_isstd) {
				tz->tz_transitions[i] += dstoff - tz->tz_dstoff;
			}
			else {
				tz->tz_transitions[i] += stdoff - tz->tz_stdoff;
			}
		}

		isdst = trans_type->tt_isdst;
	}

	tz->tz_stdoff = stdoff;
	tz->tz_dstoff = dstoff;
	tz->tz_timezone = -tz->tz_types[0].tt_utoff;
	tz->tz_types[0].tt_desigidx = 0;
	tz->tz_types[0].tt_utoff = stdoff;
	tz->tz_types[0].tt_isdst = 0;
	tz->tz_types[1].tt_desigidx = static_cast<uint8_t>(stdlen);
	tz->tz_types[1].tt_utoff = dstoff;
	tz->tz_types[1].tt_isdst = 1;

	return tz.release();
}

static bool tzrule_parse_tzname(tzrule *tz_rules, const char **specs, int whichrule) {

	const char *start = *specs;
	const char *cptr = start;

	while (('a' <= *cptr && *cptr <= 'z') || ('A' <= *cptr && *cptr <= 'Z')) {
		++cptr;
	}

	auto length = static_cast<size_t>(cptr - start);

	if (length < 3) {

		cptr = *specs;

		if (UNLIKELY(*cptr++ != '<')) {
			return false;
		}

		start = cptr;

		while (('a' <= *cptr && *cptr <= 'z') || ('A' <= *cptr && *cptr <= 'Z') || ('0' <= *cptr && *cptr <= '9')
			   || *cptr == '+' || *cptr == '-') {
			++cptr;
		}

		length = static_cast<size_t>(cptr - start);

		if (*cptr++ != '>' || length < 3) {
			return false;
		}
	}

	const char *name = strndup(start, length); /// \todo handle free

	if (name == nullptr) {
		return false;
	}

	tz_rules[whichrule].name = name;
	*specs = cptr;
	return true;
}

static int compute_offset(int ss, int mm, int hh) {
	ss = std::min(ss, 59);
	mm = std::min(mm, 59);
	hh = std::min(hh, 24);
	return ss + mm * 60 + hh * 60 * 60;
}

static bool tzrule_parse_offset(tzrule *tz_rules, const char **specs, int whichrule) {

	const char *cptr = *specs;

	if (whichrule == 0 && (*cptr == '\0' || (*cptr != '+' && *cptr != '-' && !isdigit(*cptr)))) {
		return false;
	}

	int sign;

	if (*cptr == '-' || *cptr == '+') {
		sign = *cptr++ == '-' ? 1 : -1;
	}
	else {
		sign = -1;
	}

	*specs = cptr;

	unsigned short int hh;
	unsigned short mm = 0;
	unsigned short ss = 0;
	int consumed = 0;

	if (sscanf(cptr, "%hu%n:%hu%n:%hu%n", &hh, &consumed, &mm, &consumed, &ss, &consumed) > 0) {
		tz_rules[whichrule].offset = sign * compute_offset(ss, mm, hh);
	}
	else if (whichrule == 0) {
		tz_rules[0].offset = 0;
		return false;
	}
	else {
		tz_rules[1].offset = tz_rules[0].offset + (60 * 60);
	}

	*specs = cptr + consumed;
	return true;
}

static bool tzrule_parse_rule(tzrule *tz_rules, const char **specs, int whichrule) {

	const char *cptr = *specs;

	tzrule *rule = &tz_rules[whichrule];

	cptr += *cptr == ',';

	if (*cptr == 'J' || isdigit(*cptr)) {

		char *end = nullptr;

		rule->type = *cptr == 'J' ? tzrule::J1 : tzrule::J0;

		if (rule->type == tzrule::J1 && !isdigit(*++cptr)) {
			return false;
		}

		unsigned short int d = static_cast<unsigned short int>(strtoul(cptr, &end, 10));

		if (end == cptr || d > 365) {
			return false;
		}

		if (rule->type == tzrule::J1 && d == 0) {
			return false;
		}

		rule->d = d;
		cptr = end;
	}
	else if (*cptr == 'M') {

		int consumed;
		rule->type = tzrule::M;

		if (sscanf(cptr, "M%hu.%hu.%hu%n", &rule->m, &rule->n, &rule->d, &consumed) != 3 || rule->m < 1 || rule->m > 12
			|| rule->n < 1 || rule->n > 5 || rule->d > 6) {
			return false;
		}

		cptr += consumed;
	}
	else if (*cptr == '\0') {

		rule->type = tzrule::M;

		if (rule == &tz_rules[0]) {
			rule->m = 3;
			rule->n = 2;
			rule->d = 0;
		}
		else {
			rule->m = 11;
			rule->n = 1;
			rule->d = 0;
		}
	}
	else {
		return false;
	}

	if (*cptr != '\0' && *cptr != '/' && *cptr != ',') {
		return false;
	}
	else if (*cptr == '/') {

		int negative;
		++cptr;

		if (*cptr == '\0') {
			return false;
		}

		negative = *cptr == '-';
		cptr += negative;

		unsigned short hh = 2;
		unsigned short mm = 0;
		unsigned short ss = 0;
		int consumed = 0;

		sscanf(cptr, "%hu%n:%hu%n:%hu%n", &hh, &consumed, &mm, &consumed, &ss, &consumed);
		cptr += consumed;
		rule->secs = (negative ? -1 : 1) * ((hh * 60 * 60) + (mm * 60) + ss);
	}
	else {
		rule->secs = 2 * 60 * 60;
	}

	rule->computed_for = -1;
	*specs = cptr;
	return true;
}

static tzfile *tzrule_parse(tzrule *tz_rules, tzfile *tz) {

	const char *specs = tz->tz_specs;
	tz_rules[0].name = tz_rules[1].name = "";

	if (tzrule_parse_tzname(tz_rules, &specs, 0) && tzrule_parse_offset(tz_rules, &specs, 0)) {
		if (*specs != '\0') {
			if (tzrule_parse_tzname(tz_rules, &specs, 1)) {
				tzrule_parse_offset(tz_rules, &specs, 1);
				if (*specs == '\0' || (specs[0] == ',' && specs[1] == '\0')) {
					tz = tzfile_default(tz_rules[0].name, tz_rules[1].name, tz_rules[0].offset, tz_rules[1].offset);
				}
			}

			if (tzrule_parse_rule(tz_rules, &specs, 0)) {
				tzrule_parse_rule(tz_rules, &specs, 1);
			}
		}
		else {
			tz_rules[1].name = tz_rules[0].name;
			tz_rules[1].offset = tz_rules[0].offset;
		}
	}

	return tz;
}

static void compute_change(tzrule *rule, int year) {

	time_t t;

	if (year != -1 && rule->computed_for == year) {
		return;
	}

	if (year > EPOCH_YEAR) {
		t = ((year - EPOCH_YEAR) * 365 + ((year - 1) / 4 - EPOCH_YEAR / 4) - ((year - 1) / 100 - EPOCH_YEAR / 100)
			 + ((year - 1) / 400 - EPOCH_YEAR / 400))
			* SECS_PER_DAY;
	}
	else {
		t = 0;
	}

	switch (rule->type) {
	case tzrule::J1:
		t += (rule->d - 1) * SECS_PER_DAY;
		if (rule->d >= 60 && __isleap(year)) {
			t += SECS_PER_DAY;
		}
		break;

	case tzrule::J0:
		t += rule->d * SECS_PER_DAY;
		break;

	case tzrule::M:
		{
			int d, m1, yy0, yy1, yy2, dow;
			const time_t *myday = &MONTH_YEAR_DAY[__isleap(year)][rule->m];

			t += myday[-1] * SECS_PER_DAY;
			m1 = (rule->m + 9) % 12 + 1;
			yy0 = (rule->m <= 2) ? (year - 1) : year;
			yy1 = yy0 / 100;
			yy2 = yy0 % 100;
			dow = ((26 * m1 - 2) / 10 + 1 + yy2 + yy2 / 4 + yy1 / 4 - 2 * yy1) % 7;

			if (dow < 0) {
				dow += 7;
			}

			d = rule->d - dow;

			if (d < 0) {
				d += 7;
			}

			for (unsigned int i = 1; i < rule->n; ++i) {
				if (d + 7 >= static_cast<int>(myday[0] - myday[-1])) {
					break;
				}
				d += 7;
			}

			t += d * SECS_PER_DAY;
		}
		break;
	}

	rule->change = t - rule->offset + rule->secs;
	rule->computed_for = year;
}

static void tzrule_compute(tzrule *tz_rules, char **tzname, time_t timer, tm &time) {

	compute_change(&tz_rules[0], TM_YEAR_BASE + time.tm_year);
	compute_change(&tz_rules[1], TM_YEAR_BASE + time.tm_year);

	bool isdst;

	if (UNLIKELY(tz_rules[0].change > tz_rules[1].change)) {
		isdst = (timer < tz_rules[1].change || timer >= tz_rules[0].change);
	}
	else {
		isdst = (timer >= tz_rules[0].change && timer < tz_rules[1].change);
	}

	time.tm_isdst = isdst;

#ifdef __USE_MISC
	time.tm_zone = tzname[isdst];
	time.tm_gmtoff = tz_rules[isdst].offset;
#else
	time.__tm_zone = tzname[isdst];
	time.__tm_gmtoff = tz_rules[isdst].offset;
#endif
}

static bool offtime(time_t timer, tm &time, int leap_correct) {

	time_t days = timer / SECS_PER_DAY;
	time_t rem = timer % SECS_PER_DAY;

#ifdef __USE_MISC
	rem += time.tm_gmtoff - leap_correct;
#else
	rem += time.__tm_gmtoff - leap_correct;
#endif

	while (rem < 0) {
		rem += SECS_PER_DAY;
		--days;
	}

	while (rem >= SECS_PER_DAY) {
		rem -= SECS_PER_DAY;
		++days;
	}

	time.tm_hour = static_cast<int>(rem / SECS_PER_HOUR);
	rem %= SECS_PER_HOUR;
	time.tm_min = static_cast<int>(rem / 60);
	time.tm_sec = static_cast<int>(rem % 60);

	time.tm_wday = (4 + days) % 7;

	if (time.tm_wday < 0) {
		time.tm_wday += 7;
	}

	time_t y = EPOCH_YEAR;
#define DIV(a, b) ((a) / (b) - ((a) % (b) < 0))
#define LEAPS_THRU_END_OF(y) (DIV(y, 4) - DIV(y, 100) + DIV(y, 400))

	while (days < 0 || days >= (__isleap(y) ? 366 : 365)) {

		time_t yg = y + days / 365 - (days % 365 < 0);

		days -= ((yg - y) * 365 + LEAPS_THRU_END_OF(yg - 1) - LEAPS_THRU_END_OF(y - 1));
		y = yg;
	}

	time.tm_year = static_cast<int>(y - TM_YEAR_BASE);

	if (time.tm_year != y - TM_YEAR_BASE) {
		return false;
	}

	time.tm_yday = static_cast<int>(days);
	const time_t *ip = MONTH_YEAR_DAY[__isleap(y)];

	for (y = 11; days < ip[y]; --y) {
		continue;
	}

	days -= ip[y];
	time.tm_mon = static_cast<int>(y);
	time.tm_mday = static_cast<int>(days + 1);
	return true;
}

tm mint::timezone_localtime(TimeZone *tz, time_t timer, bool *ok) {

	tm time;
	size_t i = 0;
	tzfile *old_tz = tz;
	const char *tz_names[2];

	tz_names[0] = nullptr;
	tz_names[1] = nullptr;
	memset(&time, 0, sizeof(time));

	if (ok) {
		*ok = true;
	}

	if (UNLIKELY(tz->tz_head.tzh_timecnt == 0 || timer < tz->tz_transitions[0])) {
		while (i < tz->tz_head.tzh_typecnt && tz->tz_types[i].tt_isdst) {
			if (tz_names[1] == nullptr) {
				tz_names[1] = &tz->tz_zonenames[tz->tz_types[i].tt_desigidx];
			}
			++i;
		}

		if (i == tz->tz_head.tzh_typecnt) {
			i = 0;
		}

		tz_names[0] = &tz->tz_zonenames[tz->tz_types[i].tt_desigidx];

		if (tz_names[1] == nullptr) {
			for (size_t j = i; j < tz->tz_head.tzh_typecnt; ++j) {
				if (tz->tz_types[j].tt_isdst) {
					tz_names[1] = &tz->tz_zonenames[tz->tz_types[j].tt_desigidx];
					break;
				}
			}
		}
	}
	else if (UNLIKELY(timer >= tz->tz_transitions[tz->tz_head.tzh_timecnt - 1])) {

		tzrule tz_rules[2];

		memset(tz_rules, 0, sizeof(tz_rules));

		if (UNLIKELY(tz->tz_specs == nullptr)) {
			i = tz->tz_head.tzh_timecnt;
			goto found;
		}

		tz = tzrule_parse(tz_rules, tz);
		tz_names[0] = tz_rules[0].name;
		tz_names[1] = tz_rules[1].name;

		if (UNLIKELY(!offtime(timer, time, 0))) {
			i = tz->tz_head.tzh_timecnt;
			goto found;
		}

		tzrule_compute(tz_rules, tzname, timer, time);

		if (UNLIKELY(tz->tz_zonenames == reinterpret_cast<char *>(&tz->tz_leaps[tz->tz_head.tzh_leapcnt]))) {
			assert(tz->tz_head.tzh_typecnt == 2);
			tz_names[0] = tz->tz_zonenames;
			tz_names[1] = &tz->tz_zonenames[strlen(tz->tz_zonenames) + 1];
		}

		goto leap;
	}
	else {

		{
			size_t lo = 0;
			size_t hi = tz->tz_head.tzh_timecnt - 1;

			i = size_t(tz->tz_transitions[tz->tz_head.tzh_timecnt - 1] - timer) / 15778476;

			if (i < tz->tz_head.tzh_timecnt) {

				i = tz->tz_head.tzh_timecnt - 1 - i;

				if (timer < tz->tz_transitions[i]) {
					if (i < 10 || timer >= tz->tz_transitions[i - 10]) {
						while (timer < tz->tz_transitions[i - 1]) {
							--i;
						}
						goto found;
					}

					hi = i - 10;
				}
				else {
					if (i + 10 >= tz->tz_head.tzh_timecnt || timer < tz->tz_transitions[i + 10]) {
						while (timer >= tz->tz_transitions[i]) {
							++i;
						}
						goto found;
					}

					lo = i + 10;
				}
			}

			while (lo + 1 < hi) {

				i = (lo + hi) / 2;

				if (timer < tz->tz_transitions[i]) {
					hi = i;
				}
				else {
					lo = i;
				}
			}

			i = hi;
		}

	found:
		tz_names[tz->tz_types[tz->tz_typeidxs[i - 1]].tt_isdst] =
			&tz->tz_zonenames[tz->tz_types[tz->tz_typeidxs[i - 1]].tt_desigidx];

		for (size_t j = i; j < tz->tz_head.tzh_timecnt; ++j) {

			int type = tz->tz_typeidxs[j];
			int dst = tz->tz_types[type].tt_isdst;
			int idx = tz->tz_types[type].tt_desigidx;

			if (tz_names[dst] == nullptr) {

				tz_names[dst] = &tz->tz_zonenames[idx];

				if (tz_names[1 - dst] != nullptr) {
					break;
				}
			}
		}

		if (UNLIKELY(tz_names[0] == nullptr)) {
			tz_names[0] = tz_names[1];
		}

		i = tz->tz_typeidxs[i - 1];
	}

	{
		ttinfo *info = &tz->tz_types[i];

		if (tz_names[0] == nullptr) {
			assert(tz->tz_head.tzh_typecnt == 1);
			tz_names[0] = tz->tz_zonenames;
		}

		if (tz_names[1] == nullptr) {
			tz_names[1] = tz_names[0];
		}

		time.tm_isdst = info->tt_isdst;
		assert(strcmp(&tz->tz_zonenames[info->tt_desigidx], tz_names[time.tm_isdst]) == 0);

#ifdef __USE_MISC
		time.tm_zone = tz_names[time.tm_isdst];
		time.tm_gmtoff = info->tt_utoff;
#else
		time.__tm_zone = tzname[time.tm_isdst];
		time.__tm_gmtoff = info->tt_utoff;
#endif
	}

leap:
	bool leap_found = true;
	int leap_correct = 0L;
	int leap_hit = 0;

	i = tz->tz_head.tzh_leapcnt;

	do {
		if (i-- == 0) {
			leap_found = false;
			break;
		}
	}
	while (timer < tz->tz_leaps[i].transition);

	if (leap_found) {

		leap_correct = tz->tz_leaps[i].change;

		if (timer == tz->tz_leaps[i].transition
			&& ((i == 0 && tz->tz_leaps[i].change > 0) || tz->tz_leaps[i].change > tz->tz_leaps[i - 1].change)) {

			leap_hit = 1;

			while (i > 0 && tz->tz_leaps[i].transition == tz->tz_leaps[i - 1].transition + 1
				   && tz->tz_leaps[i].change == tz->tz_leaps[i - 1].change + 1) {
				++leap_hit;
				--i;
			}
		}
	}

	if (ok) {
		*ok = offtime(timer, time, leap_correct);
	}
	else {
		((void)offtime(timer, time, leap_correct));
	}

	if (old_tz != tz) {
		mint::timezone_free(tz);
	}

	return time;
}

static struct tm *convert_time(struct tm (*convert)(tzfile *, time_t, bool *), tzfile *tz, long int t, struct tm *tm) {

	bool ok = false;
	struct tm time = convert(tz, time_t(t), &ok);

	if (ok) {
		*tm = time;
		return tm;
	}

	return nullptr;
}

static long int shr(long int a, int b) {
	long int one = 1;
	return (-one >> 1 == -1 ? a >> b : a / (one << b) - (a % (one << b) < 0));
}

static bool isdst_differ(int a, int b) {
	return (!a != !b) && (0 <= a) && (0 <= b);
}

static long int ydhms_diff(long int year1, long int yday1, int hour1, int min1, int sec1, int year0, int yday0,
						   int hour0, int min0, int sec0) {

	static_assert(-1 / 2 == 0, "-1 / 2 should be 0");

	long int a4 = shr(year1, 2) + shr(TM_YEAR_BASE, 2) - !(year1 & 3);
	long int b4 = shr(year0, 2) + shr(TM_YEAR_BASE, 2) - !(year0 & 3);
	long int a100 = a4 / 25 - (a4 % 25 < 0);
	long int b100 = b4 / 25 - (b4 % 25 < 0);
	long int a400 = shr(a100, 2);
	long int b400 = shr(b100, 2);
	long int intervening_leap_days = (a4 - b4) - (a100 - b100) + (a400 - b400);

	long int years = year1 - year0;
	long int days = 365 * years + yday1 - yday0 + intervening_leap_days;
	long int hours = 24 * days + hour1 - hour0;
	long int minutes = 60 * hours + min1 - min0;
	long int seconds = 60 * minutes + sec1 - sec0;
	return seconds;
}

static long int long_int_avg(long int a, long int b) {
	return shr(a, 1) + shr(b, 1) + ((a | b) & 1);
}

static long int tm_diff(long int year, long int yday, int hour, int min, int sec, struct tm const *tp) {
	return ydhms_diff(year, yday, hour, min, sec, tp->tm_year, tp->tm_yday, tp->tm_hour, tp->tm_min, tp->tm_sec);
}

static constexpr const long int MKTIME_MIN = ((std::numeric_limits<time_t>::is_signed
											   && std::numeric_limits<time_t>::min()
													  < std::numeric_limits<long int>::min())
												  ? std::numeric_limits<long int>::min()
												  : std::numeric_limits<time_t>::min());
static constexpr const long int MKTIME_MAX = (std::numeric_limits<long int>::max() < std::numeric_limits<time_t>::max()
												  ? std::numeric_limits<long int>::max()
												  : std::numeric_limits<time_t>::max());

static struct tm *ranged_convert(struct tm (*convert)(tzfile *, time_t, bool *), tzfile *tz, long int *t,
								 struct tm *tp) {

	long int t1 = (*t < MKTIME_MIN ? MKTIME_MIN : *t <= MKTIME_MAX ? *t : MKTIME_MAX);
	struct tm *r = convert_time(convert, tz, t1, tp);

	if (r) {
		*t = t1;
		return r;
	}

	long int bad = t1;
	long int ok = 0;
	struct tm oktm;
	oktm.tm_sec = -1;

	for (;;) {

		long int mid = long_int_avg(ok, bad);

		if (mid == ok || mid == bad) {
			break;
		}

		if (convert_time(convert, tz, mid, tp)) {
			ok = mid;
			oktm = *tp;
		}
		else {
			bad = mid;
		}
	}

	if (oktm.tm_sec < 0) {
		return nullptr;
	}

	*t = ok;
	*tp = oktm;
	return tp;
}

time_t mint::timezone_mktime(TimeZone *tz, const tm &time, bool *ok) {

	struct tm tm;
	int remaining_probes = 6;
	int sec = time.tm_sec;
	int min = time.tm_min;
	int hour = time.tm_hour;
	int mday = time.tm_mday;
	int mon = time.tm_mon;
	int year_requested = time.tm_year;
	int isdst = time.tm_isdst;
	int dst2 = 0;
	int mon_remainder = mon % 12;
	int negative_mon_remainder = mon_remainder < 0;
	int mon_years = mon / 12 - negative_mon_remainder;
	long int lyear_requested = year_requested;
	long int year = lyear_requested + mon_years;
	int mon_yday = int((MONTH_YEAR_DAY[leap_year(year)][mon_remainder + 12 * negative_mon_remainder]) - 1);
	long int lmday = mday;
	long int yday = mon_yday + lmday;
	long int off = 0;
	int negative_offset_guess;
	int sec_requested = sec;

	if (ok) {
		*ok = true;
	}

	if (sec < 0) {
		sec = 0;
	}

	if (59 < sec) {
		sec = 59;
	}

	__builtin_sub_overflow(0, off, &negative_offset_guess);
	long int t0 = ydhms_diff(year, yday, hour, min, sec, EPOCH_YEAR - TM_YEAR_BASE, 0, 0, 0, negative_offset_guess);
	long int t = t0, t1 = t0, t2 = t0;

	for (;;) {
		if (!ranged_convert(mint::timezone_localtime, tz, &t, &tm)) {
			if (ok) {
				*ok = false;
			}
			return -1;
		}

		long int dt = tm_diff(year, yday, hour, min, sec, &tm);

		if (dt == 0) {
			break;
		}

		if (t == t1 && t != t2
			&& (tm.tm_isdst < 0 || (isdst < 0 ? dst2 <= (tm.tm_isdst != 0) : (isdst != 0) != (tm.tm_isdst != 0)))) {
			goto offset_found;
		}

		remaining_probes--;

		if (remaining_probes == 0) {
			if (ok) {
				*ok = false;
			}
			return -1;
		}

		t1 = t2;
		t2 = t;
		t += dt;
		dst2 = tm.tm_isdst != 0;
	}

	if (isdst_differ(isdst, tm.tm_isdst)) {

		int stride = 601200;
		int duration_max = 536454000;
		int delta_bound = duration_max / 2 + stride;
		int delta, direction;

		for (delta = stride; delta < delta_bound; delta += stride) {
			for (direction = -1; direction <= 1; direction += 2) {

				long int ot;

				if (!__builtin_add_overflow(t, delta * direction, &ot)) {

					struct tm otm;

					if (!ranged_convert(mint::timezone_localtime, tz, &ot, &otm)) {
						if (ok) {
							*ok = false;
						}
						return -1;
					}

					if (!isdst_differ(isdst, otm.tm_isdst)) {

						long int gt = ot + tm_diff(year, yday, hour, min, sec, &otm);

						if (MKTIME_MIN <= gt && gt <= MKTIME_MAX) {
							if (convert_time(mint::timezone_localtime, tz, gt, &tm)) {
								t = gt;
								goto offset_found;
							}
						}
					}
				}
			}
		}

		if (ok) {
			*ok = false;
		}

		return -1;
	}

offset_found:
	__builtin_sub_overflow(t, t0, &off);
	__builtin_sub_overflow(off, negative_offset_guess, &off);

	if (sec_requested != tm.tm_sec) {

		long int sec_adjustment = sec == 0 && tm.tm_sec == 60;
		sec_adjustment -= sec;
		sec_adjustment += sec_requested;

		if (__builtin_add_overflow(t, sec_adjustment, &t) || !(MKTIME_MIN <= t && t <= MKTIME_MAX)) {
			if (ok) {
				*ok = false;
			}
			return -1;
		}

		if (!convert_time(mint::timezone_localtime, tz, t, &tm)) {
			if (ok) {
				*ok = false;
			}
			return -1;
		}
	}

	return t;
}

bool mint::timezone_match(TimeZone *tz1, TimeZone *tz2) {
	return !strcmp(tz1->tz_name[0], tz2->tz_name[0]) && !strcmp(tz1->tz_name[1], tz2->tz_name[1]);
}

std::string mint::timezone_default_name() {

	static char g_default_name[TZ_NAME_MAX];

	const char *name = getenv("TZ");

	if (name && name[0] == ':') {
		name = name + 1;
	}

	if (name && !strcmp(name, "/etc/localtime")) {
		name = nullptr;
	}

	if (name == nullptr) {

		auto index = std::string::npos;
		std::string path = std::filesystem::read_symlink("/etc/localtime");

#if defined(SYMLOOP_MAX)
		int iteration = SYMLOOP_MAX;
#elif defined(MAXSYMLINKS)
		int iteration = MAXSYMLINKS;
#else
		int iteration = 20;
#endif

		while (iteration-- > 0 && !path.empty() && (index = path.find("/zoneinfo/")) == std::string::npos) {
			path = std::filesystem::read_symlink(path);
		}

		if (index != std::string::npos) {
			strncpy(g_default_name, path.substr(index + 10).c_str(), sizeof(g_default_name));
			g_default_name[TZ_NAME_MAX - 1] = '\0';
			name = g_default_name;
		}
	}

	if (name == nullptr) {
		if (std::filesystem::exists("/etc/timezone")) {
			if (std::ifstream stream("/etc/timezone"); stream.good()) {
				if (std::string path; getline(stream, path) && !path.empty()) {
					strncpy(g_default_name, path.c_str(), sizeof(g_default_name));
					g_default_name[TZ_NAME_MAX - 1] = '\0';
					name = g_default_name;
				}
			}
		}
	}

	if (name == nullptr) {
		if (std::ifstream stream("/etc/sysconfig/clock"); stream.good()) {

			std::string line;

			while (!name && !stream.eof() && stream.good()) {

				getline(stream, line);

				if (!line.compare(0, 5, "ZONE=")) {
					strncpy(g_default_name, line.substr(6, line.size() - 7).c_str(), sizeof(g_default_name));
					g_default_name[TZ_NAME_MAX - 1] = '\0';
					name = g_default_name;
				}
				else if (!line.compare(0, 9, "TIMEZONE=")) {
					strncpy(g_default_name, line.substr(10, line.size() - 11).c_str(), sizeof(g_default_name));
					g_default_name[TZ_NAME_MAX - 1] = '\0';
					name = g_default_name;
				}
			}
		}
	}

	if (name == nullptr) {
		if (std::ifstream stream("/etc/TZ"); stream.good()) {

			std::string line;
			getline(stream, line);

			if (!line.empty()) {
				line.erase(find_if(line.rbegin(), line.rend(),
								   [](char c) {
									   return !isspace(c);
								   })
							   .base(),
						   line.end());
				line.erase(line.begin(), find_if(line.begin(), line.end(), [](char c) {
							   return !isspace(c);
						   }));
				strncpy(g_default_name, line.c_str(), sizeof(g_default_name));
				g_default_name[TZ_NAME_MAX - 1] = '\0';
				name = g_default_name;
			}
		}
	}

	if (name == nullptr) {
		name = "UTC";
	}

	if (name != g_default_name) {
		strncpy(g_default_name, name, sizeof(g_default_name));
		g_default_name[TZ_NAME_MAX - 1] = '\0';
	}

	return g_default_name;
}

std::vector<std::string> mint::timezone_list_names() {
	std::vector<std::string> names;
	names.reserve(g_tz_files.size());
	for (const auto &tz : g_tz_files) {
		names.emplace_back(tz.first);
	}
	return names;
}

mint::TimeZone *mint::timezone_find(const char *time_zone) {

	std::filesystem::path tz_dir = "/usr/share/zoneinfo";

	if (!std::filesystem::exists(tz_dir)) {
		tz_dir = "/usr/lib/zoneinfo";
	}

	std::filesystem::path path = tz_dir / time_zone;

	if (FILE *file = fopen(path.c_str(), "rce")) {
		tzfile *tz = mint::timezone_read(file, std::filesystem::file_size(path));
		fclose(file);
		return tz;
	}

	return nullptr;
}

int mint::timezone_set_default(const char *time_zone) {

	std::filesystem::path tz_dir = "/usr/share/zoneinfo";

	if (!std::filesystem::exists(tz_dir)) {
		tz_dir = "/usr/lib/zoneinfo";
	}

	std::filesystem::path path = tz_dir / time_zone;

	if (std::filesystem::exists(tz_dir)) {
		// TODO : change system timezone
		return EPERM;
	}

	return EINVAL;
}
