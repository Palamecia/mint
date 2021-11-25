#include <memory/functiontool.h>
#include <memory/casttool.h>
#include <system/filesystem.h>
#include <chrono>

#ifdef OS_UNIX
#include "tzfile.h"
#else
#include "wintz.h"
#endif

using namespace std;
using namespace mint;

namespace symbols {

static const Symbol System("System");
static const Symbol WeekDay("WeekDay");

static const Symbol days[] {
	Symbol("sunday"),
	Symbol("monday"),
	Symbol("tuesday"),
	Symbol("wednesday"),
	Symbol("thursday"),
	Symbol("friday"),
	Symbol("saturday")
};

}

MINT_FUNCTION(mint_timezone_open, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &name = helper.popParameter();

#ifdef OS_UNIX
	if (tzfile *tz = tzfile_find(to_string(name).c_str())) {
		helper.returnValue(create_object(tz));
	}
#else
	if (TimeZone *tz = wintz_find(to_string(name).c_str())) {
		helper.returnValue(create_object(tz));
	}
#endif
}

MINT_FUNCTION(mint_timezone_close, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &zoneinfo = helper.popParameter();

#ifdef OS_UNIX
	tzfile_free(zoneinfo.data<LibObject<TimeZone>>()->impl);
#else
	wintz_free(zoneinfo.data<LibObject<TimeZone>>()->impl);
#endif
}

MINT_FUNCTION(mint_timezone_match, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &other = helper.popParameter();
	Reference &self = helper.popParameter();

#ifdef OS_UNIX
	helper.returnValue(create_boolean(tzfile_match(self.data<LibObject<TimeZone>>()->impl, other.data<LibObject<TimeZone>>()->impl)));
#else
	helper.returnValue(create_boolean(wintz_match(self.data<LibObject<TimeZone>>()->impl, other.data<LibObject<TimeZone>>()->impl)));
#endif
}

MINT_FUNCTION(mint_timezone_current_name, 0, cursor) {

	FunctionHelper helper(cursor, 0);

#ifdef OS_UNIX
	helper.returnValue(create_string(tzfile_default_name()));
#else
	helper.returnValue(create_string(wintz_default_name()));
#endif
}

MINT_FUNCTION(mint_timezone_list, 0, cursor) {

	FunctionHelper helper(cursor, 0);
	WeakReference result = create_array();

#ifdef OS_UNIX
	for (const string &name : tzfile_list_names()) {
		array_append(result.data<Array>(), create_string(name));
	}
#else
	for (const string &name : wintz_list_names()) {
		array_append(result.data<Array>(), create_string(name));
	}
#endif

	helper.returnValue(move(result));
}

MINT_FUNCTION(mint_timezone_seconds_since_epoch, 7, cursor) {

	FunctionHelper helper(cursor, 7);
	tm time;

	memset(&time, 0, sizeof (time));

	time.tm_sec = static_cast<int>(to_integer(cursor, helper.popParameter()));
	time.tm_min = static_cast<int>(to_integer(cursor, helper.popParameter()));
	time.tm_hour = static_cast<int>(to_integer(cursor, helper.popParameter()));
	time.tm_mday = static_cast<int>(to_integer(cursor, helper.popParameter()));
	time.tm_mon = static_cast<int>(to_integer(cursor, helper.popParameter())) - 1;
	time.tm_year = static_cast<int>(to_integer(cursor, helper.popParameter())) - TM_YEAR_BASE;
	Reference &zoneinfo = helper.popParameter();

#ifdef OS_UNIX
	bool ok = true;
	time_t seconds = tzfile_mktime(zoneinfo.data<LibObject<TimeZone>>()->impl, time, &ok);
#else
	bool ok = true;
	time_t seconds = wintz_mktime(zoneinfo.data<LibObject<TimeZone>>()->impl, time, &ok);
#endif

	if (ok) {
		helper.returnValue(create_number(seconds));
	}
}

MINT_FUNCTION(mint_timezone_milliseconds_since_epoch, 8, cursor) {

	FunctionHelper helper(cursor, 8);
	tm time;

	memset(&time, 0, sizeof (time));

	int msec = static_cast<int>(to_integer(cursor, helper.popParameter()));
	time.tm_sec = static_cast<int>(to_integer(cursor, helper.popParameter()));
	time.tm_min = static_cast<int>(to_integer(cursor, helper.popParameter()));
	time.tm_hour = static_cast<int>(to_integer(cursor, helper.popParameter()));
	time.tm_mday = static_cast<int>(to_integer(cursor, helper.popParameter()));
	time.tm_mon = static_cast<int>(to_integer(cursor, helper.popParameter())) - 1;
	time.tm_year = static_cast<int>(to_integer(cursor, helper.popParameter())) - TM_YEAR_BASE;
	Reference &zoneinfo = helper.popParameter();

#ifdef OS_UNIX
	bool ok = true;
	intmax_t milliseconds = tzfile_mktime(zoneinfo.data<LibObject<TimeZone>>()->impl, time, &ok);
#else
	bool ok = true;
	intmax_t milliseconds = wintz_mktime(zoneinfo.data<LibObject<TimeZone>>()->impl, time, &ok);
#endif

	if (ok) {
		milliseconds *= 1000;
		milliseconds += msec;
		helper.returnValue(create_number(milliseconds));
	}
}

MINT_FUNCTION(mint_timezone_time_from_duration, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &timepoint = helper.popParameter();
	Reference &zoneinfo = helper.popParameter();

	bool ok = true;
	int msec = timepoint.data<LibObject<chrono::milliseconds>>()->impl->count() % 1000;
	time_t seconds = static_cast<time_t>(chrono::duration_cast<chrono::seconds>(*timepoint.data<LibObject<chrono::milliseconds>>()->impl).count());

#ifdef OS_UNIX
	tm &&time = tzfile_localtime(zoneinfo.data<LibObject<TimeZone>>()->impl, seconds, &ok);
#else
	tm &&time = wintz_localtime(zoneinfo.data<LibObject<TimeZone>>()->impl, seconds, &ok);
#endif

	if (ok) {
		WeakReference result = create_iterator();
		iterator_insert(result.data<Iterator>(), create_number(time.tm_year + TM_YEAR_BASE));
		iterator_insert(result.data<Iterator>(), create_number(time.tm_mon + 1));
		iterator_insert(result.data<Iterator>(), create_number(time.tm_mday));
		iterator_insert(result.data<Iterator>(), create_number(time.tm_hour));
		iterator_insert(result.data<Iterator>(), create_number(time.tm_min));
		iterator_insert(result.data<Iterator>(), create_number(time.tm_sec));
		iterator_insert(result.data<Iterator>(), create_number(msec));
		helper.returnValue(move(result));
	}
}

MINT_FUNCTION(mint_timezone_time_from_seconds, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &timepoint = helper.popParameter();
	Reference &zoneinfo = helper.popParameter();

	bool ok = true;
	time_t seconds = static_cast<time_t>(to_integer(cursor, timepoint));

#ifdef OS_UNIX
	tm &&time = tzfile_localtime(zoneinfo.data<LibObject<TimeZone>>()->impl, seconds, &ok);
#else
	tm &&time = wintz_localtime(zoneinfo.data<LibObject<TimeZone>>()->impl, seconds, &ok);
#endif

	if (ok) {
		WeakReference result = create_iterator();
		iterator_insert(result.data<Iterator>(), create_number(time.tm_year + TM_YEAR_BASE));
		iterator_insert(result.data<Iterator>(), create_number(time.tm_mon + 1));
		iterator_insert(result.data<Iterator>(), create_number(time.tm_mday));
		iterator_insert(result.data<Iterator>(), create_number(time.tm_hour));
		iterator_insert(result.data<Iterator>(), create_number(time.tm_min));
		iterator_insert(result.data<Iterator>(), create_number(time.tm_sec));
		helper.returnValue(move(result));
	}
}

MINT_FUNCTION(mint_timezone_time_from_milliseconds, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &timepoint = helper.popParameter();
	Reference &zoneinfo = helper.popParameter();

	bool ok = true;
	int msec = static_cast<int>(to_integer(cursor, timepoint) % 1000);
	time_t seconds = static_cast<time_t>(to_integer(cursor, timepoint) / 1000);

#ifdef OS_UNIX
	tm &&time = tzfile_localtime(zoneinfo.data<LibObject<TimeZone>>()->impl, seconds, &ok);
#else
	tm &&time = wintz_localtime(zoneinfo.data<LibObject<TimeZone>>()->impl, seconds, &ok);
#endif

	if (ok) {
		WeakReference result = create_iterator();
		iterator_insert(result.data<Iterator>(), create_number(time.tm_year + TM_YEAR_BASE));
		iterator_insert(result.data<Iterator>(), create_number(time.tm_mon + 1));
		iterator_insert(result.data<Iterator>(), create_number(time.tm_mday));
		iterator_insert(result.data<Iterator>(), create_number(time.tm_hour));
		iterator_insert(result.data<Iterator>(), create_number(time.tm_min));
		iterator_insert(result.data<Iterator>(), create_number(time.tm_sec));
		iterator_insert(result.data<Iterator>(), create_number(msec));
		helper.returnValue(move(result));
	}
}

MINT_FUNCTION(mint_timezone_week_day_from_duration, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &timepoint = helper.popParameter();
	Reference &zoneinfo = helper.popParameter();

	bool ok = true;
	time_t seconds = static_cast<time_t>(chrono::duration_cast<chrono::seconds>(*timepoint.data<LibObject<chrono::milliseconds>>()->impl).count());

#ifdef OS_UNIX
	tm &&time = tzfile_localtime(zoneinfo.data<LibObject<TimeZone>>()->impl, seconds, &ok);
#else
	tm &&time = wintz_localtime(zoneinfo.data<LibObject<TimeZone>>()->impl, seconds, &ok);
#endif

	if (ok) {
		helper.returnValue(helper.reference(symbols::System).member(symbols::WeekDay).member(symbols::days[time.tm_wday]));
	}
}

MINT_FUNCTION(mint_timezone_week_day_from_seconds, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &timepoint = helper.popParameter();
	Reference &zoneinfo = helper.popParameter();

	bool ok = true;
	time_t seconds = static_cast<time_t>(to_integer(cursor, timepoint));

#ifdef OS_UNIX
	tm &&time = tzfile_localtime(zoneinfo.data<LibObject<TimeZone>>()->impl, seconds, &ok);
#else
	tm &&time = wintz_localtime(zoneinfo.data<LibObject<TimeZone>>()->impl, seconds, &ok);
#endif

	if (ok) {
		helper.returnValue(helper.reference(symbols::System).member(symbols::WeekDay).member(symbols::days[time.tm_wday]));
	}
}

MINT_FUNCTION(mint_timezone_week_day_from_milliseconds, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &timepoint = helper.popParameter();
	Reference &zoneinfo = helper.popParameter();

	bool ok = true;
	time_t seconds = static_cast<time_t>(to_integer(cursor, timepoint) / 1000);

#ifdef OS_UNIX
	tm &&time = tzfile_localtime(zoneinfo.data<LibObject<TimeZone>>()->impl, seconds, &ok);
#else
	tm &&time = wintz_localtime(zoneinfo.data<LibObject<TimeZone>>()->impl, seconds, &ok);
#endif

	if (ok) {
		helper.returnValue(helper.reference(symbols::System).member(symbols::WeekDay).member(symbols::days[time.tm_wday]));
	}
}

MINT_FUNCTION(mint_timezone_year_day_from_duration, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &timepoint = helper.popParameter();
	Reference &zoneinfo = helper.popParameter();

	bool ok = true;
	time_t seconds = static_cast<time_t>(chrono::duration_cast<chrono::seconds>(*timepoint.data<LibObject<chrono::milliseconds>>()->impl).count());

#ifdef OS_UNIX
	tm &&time = tzfile_localtime(zoneinfo.data<LibObject<TimeZone>>()->impl, seconds, &ok);
#else
	tm &&time = wintz_localtime(zoneinfo.data<LibObject<TimeZone>>()->impl, seconds, &ok);
#endif

	if (ok) {
		helper.returnValue(create_number(time.tm_yday));
	}
}

MINT_FUNCTION(mint_timezone_year_day_from_seconds, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &timepoint = helper.popParameter();
	Reference &zoneinfo = helper.popParameter();

	bool ok = true;
	time_t seconds = static_cast<time_t>(to_integer(cursor, timepoint));

#ifdef OS_UNIX
	tm &&time = tzfile_localtime(zoneinfo.data<LibObject<TimeZone>>()->impl, seconds, &ok);
#else
	tm &&time = wintz_localtime(zoneinfo.data<LibObject<TimeZone>>()->impl, seconds, &ok);
#endif

	if (ok) {
		helper.returnValue(create_number(time.tm_yday));
	}
}

MINT_FUNCTION(mint_timezone_year_day_from_milliseconds, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &timepoint = helper.popParameter();
	Reference &zoneinfo = helper.popParameter();

	bool ok = true;
	time_t seconds = static_cast<time_t>(to_integer(cursor, timepoint) / 1000);

#ifdef OS_UNIX
	tm &&time = tzfile_localtime(zoneinfo.data<LibObject<TimeZone>>()->impl, seconds, &ok);
#else
	tm &&time = wintz_localtime(zoneinfo.data<LibObject<TimeZone>>()->impl, seconds, &ok);
#endif

	if (ok) {
		helper.returnValue(create_number(time.tm_yday));
	}
}
