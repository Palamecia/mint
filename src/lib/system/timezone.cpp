/**
 * Copyright (c) 2024 Gauvain CHERY.
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
#include <mint/system/filesystem.h>
#include <chrono>

#ifdef OS_UNIX
#include "unix/tzfile.h"
#else
#include "win32/wintz.h"
#endif

using namespace std;
using namespace mint;

namespace symbols {

static const Symbol System("System");
static const Symbol WeekDay("WeekDay");

static const Symbol days[] {
	Symbol("Sunday"),
	Symbol("Monday"),
	Symbol("Tuesday"),
	Symbol("Wednesday"),
	Symbol("Thursday"),
	Symbol("Friday"),
	Symbol("Saturday")
};

}

MINT_FUNCTION(mint_timezone_open, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &name = helper.pop_parameter();

	string name_str = to_string(name);
	if (TimeZone *tz = mint::timezone_find(name_str.c_str())) {
		helper.return_value(create_object(tz));
	}
}

MINT_FUNCTION(mint_timezone_close, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &zoneinfo = helper.pop_parameter();

	mint::timezone_free(zoneinfo.data<LibObject<TimeZone>>()->impl);
}

MINT_FUNCTION(mint_timezone_match, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &other = helper.pop_parameter();
	Reference &self = helper.pop_parameter();
	
	helper.return_value(create_boolean(mint::timezone_match(self.data<LibObject<TimeZone>>()->impl, other.data<LibObject<TimeZone>>()->impl)));
}

MINT_FUNCTION(mint_timezone_current_name, 0, cursor) {

	FunctionHelper helper(cursor, 0);
	
	helper.return_value(create_string(mint::timezone_default_name()));
}

MINT_FUNCTION(mint_timezone_set_current, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &name = helper.pop_parameter();

	string name_str = to_string(name);
	if (int error = mint::timezone_set_default(name_str.c_str())) {
		helper.return_value(create_number(error));
	}
}

MINT_FUNCTION(mint_timezone_list, 0, cursor) {

	FunctionHelper helper(cursor, 0);
	WeakReference result = create_array();

	for (const string &name : mint::timezone_list_names()) {
		array_append(result.data<Array>(), create_string(name));
	}
	
	helper.return_value(std::move(result));
}

MINT_FUNCTION(mint_timezone_seconds_since_epoch, 7, cursor) {

	FunctionHelper helper(cursor, 7);
	tm time;

	memset(&time, 0, sizeof (time));
	
	time.tm_sec = static_cast<int>(to_integer(cursor, helper.pop_parameter()));
	time.tm_min = static_cast<int>(to_integer(cursor, helper.pop_parameter()));
	time.tm_hour = static_cast<int>(to_integer(cursor, helper.pop_parameter()));
	time.tm_mday = static_cast<int>(to_integer(cursor, helper.pop_parameter()));
	time.tm_mon = static_cast<int>(to_integer(cursor, helper.pop_parameter())) - 1;
	time.tm_year = static_cast<int>(to_integer(cursor, helper.pop_parameter())) - TM_YEAR_BASE;
	Reference &zoneinfo = helper.pop_parameter();

	bool ok = true;
	time_t seconds = mint::timezone_mktime(zoneinfo.data<LibObject<TimeZone>>()->impl, time, &ok);

	if (ok) {
		helper.return_value(create_number(seconds));
	}
}

MINT_FUNCTION(mint_timezone_milliseconds_since_epoch, 8, cursor) {

	FunctionHelper helper(cursor, 8);
	tm time;

	memset(&time, 0, sizeof (time));
	
	int msec = static_cast<int>(to_integer(cursor, helper.pop_parameter()));
	time.tm_sec = static_cast<int>(to_integer(cursor, helper.pop_parameter()));
	time.tm_min = static_cast<int>(to_integer(cursor, helper.pop_parameter()));
	time.tm_hour = static_cast<int>(to_integer(cursor, helper.pop_parameter()));
	time.tm_mday = static_cast<int>(to_integer(cursor, helper.pop_parameter()));
	time.tm_mon = static_cast<int>(to_integer(cursor, helper.pop_parameter())) - 1;
	time.tm_year = static_cast<int>(to_integer(cursor, helper.pop_parameter())) - TM_YEAR_BASE;
	Reference &zoneinfo = helper.pop_parameter();

	bool ok = true;
	intmax_t milliseconds = mint::timezone_mktime(zoneinfo.data<LibObject<TimeZone>>()->impl, time, &ok);

	if (ok) {
		milliseconds *= 1000;
		milliseconds += msec;
		helper.return_value(create_number(milliseconds));
	}
}

MINT_FUNCTION(mint_timezone_time_from_duration, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &timepoint = helper.pop_parameter();
	Reference &zoneinfo = helper.pop_parameter();

	bool ok = true;
	int msec = timepoint.data<LibObject<chrono::milliseconds>>()->impl->count() % 1000;
	time_t seconds = static_cast<time_t>(chrono::duration_cast<chrono::seconds>(*timepoint.data<LibObject<chrono::milliseconds>>()->impl).count());

	tm &&time = mint::timezone_localtime(zoneinfo.data<LibObject<TimeZone>>()->impl, seconds, &ok);

	if (ok) {
		WeakReference result = create_iterator();
		iterator_insert(result.data<Iterator>(), create_number(time.tm_year + TM_YEAR_BASE));
		iterator_insert(result.data<Iterator>(), create_number(time.tm_mon + 1));
		iterator_insert(result.data<Iterator>(), create_number(time.tm_mday));
		iterator_insert(result.data<Iterator>(), create_number(time.tm_hour));
		iterator_insert(result.data<Iterator>(), create_number(time.tm_min));
		iterator_insert(result.data<Iterator>(), create_number(time.tm_sec));
		iterator_insert(result.data<Iterator>(), create_number(msec));
		helper.return_value(std::move(result));
	}
}

MINT_FUNCTION(mint_timezone_time_from_seconds, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &timepoint = helper.pop_parameter();
	Reference &zoneinfo = helper.pop_parameter();

	bool ok = true;
	time_t seconds = static_cast<time_t>(to_integer(cursor, timepoint));

	tm &&time = mint::timezone_localtime(zoneinfo.data<LibObject<TimeZone>>()->impl, seconds, &ok);

	if (ok) {
		WeakReference result = create_iterator();
		iterator_insert(result.data<Iterator>(), create_number(time.tm_year + TM_YEAR_BASE));
		iterator_insert(result.data<Iterator>(), create_number(time.tm_mon + 1));
		iterator_insert(result.data<Iterator>(), create_number(time.tm_mday));
		iterator_insert(result.data<Iterator>(), create_number(time.tm_hour));
		iterator_insert(result.data<Iterator>(), create_number(time.tm_min));
		iterator_insert(result.data<Iterator>(), create_number(time.tm_sec));
		helper.return_value(std::move(result));
	}
}

MINT_FUNCTION(mint_timezone_time_from_milliseconds, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &timepoint = helper.pop_parameter();
	Reference &zoneinfo = helper.pop_parameter();

	bool ok = true;
	int msec = static_cast<int>(to_integer(cursor, timepoint) % 1000);
	time_t seconds = static_cast<time_t>(to_integer(cursor, timepoint) / 1000);

	tm &&time = mint::timezone_localtime(zoneinfo.data<LibObject<TimeZone>>()->impl, seconds, &ok);

	if (ok) {
		WeakReference result = create_iterator();
		iterator_insert(result.data<Iterator>(), create_number(time.tm_year + TM_YEAR_BASE));
		iterator_insert(result.data<Iterator>(), create_number(time.tm_mon + 1));
		iterator_insert(result.data<Iterator>(), create_number(time.tm_mday));
		iterator_insert(result.data<Iterator>(), create_number(time.tm_hour));
		iterator_insert(result.data<Iterator>(), create_number(time.tm_min));
		iterator_insert(result.data<Iterator>(), create_number(time.tm_sec));
		iterator_insert(result.data<Iterator>(), create_number(msec));
		helper.return_value(std::move(result));
	}
}

MINT_FUNCTION(mint_timezone_week_day_from_duration, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &timepoint = helper.pop_parameter();
	Reference &zoneinfo = helper.pop_parameter();

	bool ok = true;
	time_t seconds = static_cast<time_t>(chrono::duration_cast<chrono::seconds>(*timepoint.data<LibObject<chrono::milliseconds>>()->impl).count());

	tm &&time = mint::timezone_localtime(zoneinfo.data<LibObject<TimeZone>>()->impl, seconds, &ok);

	if (ok) {
		helper.return_value(helper.reference(symbols::System).member(symbols::WeekDay).member(symbols::days[time.tm_wday]));
	}
}

MINT_FUNCTION(mint_timezone_week_day_from_seconds, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &timepoint = helper.pop_parameter();
	Reference &zoneinfo = helper.pop_parameter();

	bool ok = true;
	time_t seconds = static_cast<time_t>(to_integer(cursor, timepoint));

	tm &&time = mint::timezone_localtime(zoneinfo.data<LibObject<TimeZone>>()->impl, seconds, &ok);

	if (ok) {
		helper.return_value(helper.reference(symbols::System).member(symbols::WeekDay).member(symbols::days[time.tm_wday]));
	}
}

MINT_FUNCTION(mint_timezone_week_day_from_milliseconds, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &timepoint = helper.pop_parameter();
	Reference &zoneinfo = helper.pop_parameter();

	bool ok = true;
	time_t seconds = static_cast<time_t>(to_integer(cursor, timepoint) / 1000);

	tm &&time = mint::timezone_localtime(zoneinfo.data<LibObject<TimeZone>>()->impl, seconds, &ok);

	if (ok) {
		helper.return_value(helper.reference(symbols::System).member(symbols::WeekDay).member(symbols::days[time.tm_wday]));
	}
}

MINT_FUNCTION(mint_timezone_year_day_from_duration, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &timepoint = helper.pop_parameter();
	Reference &zoneinfo = helper.pop_parameter();

	bool ok = true;
	time_t seconds = static_cast<time_t>(chrono::duration_cast<chrono::seconds>(*timepoint.data<LibObject<chrono::milliseconds>>()->impl).count());

	tm &&time = mint::timezone_localtime(zoneinfo.data<LibObject<TimeZone>>()->impl, seconds, &ok);

	if (ok) {
		helper.return_value(create_number(time.tm_yday));
	}
}

MINT_FUNCTION(mint_timezone_year_day_from_seconds, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &timepoint = helper.pop_parameter();
	Reference &zoneinfo = helper.pop_parameter();

	bool ok = true;
	time_t seconds = static_cast<time_t>(to_integer(cursor, timepoint));

	tm &&time = mint::timezone_localtime(zoneinfo.data<LibObject<TimeZone>>()->impl, seconds, &ok);

	if (ok) {
		helper.return_value(create_number(time.tm_yday));
	}
}

MINT_FUNCTION(mint_timezone_year_day_from_milliseconds, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &timepoint = helper.pop_parameter();
	Reference &zoneinfo = helper.pop_parameter();

	bool ok = true;
	time_t seconds = static_cast<time_t>(to_integer(cursor, timepoint) / 1000);

	tm &&time = mint::timezone_localtime(zoneinfo.data<LibObject<TimeZone>>()->impl, seconds, &ok);

	if (ok) {
		helper.return_value(create_number(time.tm_yday));
	}
}

MINT_FUNCTION(mint_timezone_is_dst_from_duration, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &timepoint = helper.pop_parameter();
	Reference &zoneinfo = helper.pop_parameter();

	bool ok = true;
	time_t seconds = static_cast<time_t>(chrono::duration_cast<chrono::seconds>(*timepoint.data<LibObject<chrono::milliseconds>>()->impl).count());

	tm &&time = mint::timezone_localtime(zoneinfo.data<LibObject<TimeZone>>()->impl, seconds, &ok);

	if (ok) {
		helper.return_value(create_boolean(time.tm_isdst));
	}
}

MINT_FUNCTION(mint_timezone_is_dst_from_seconds, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &timepoint = helper.pop_parameter();
	Reference &zoneinfo = helper.pop_parameter();

	bool ok = true;
	time_t seconds = static_cast<time_t>(to_integer(cursor, timepoint));

	tm &&time = mint::timezone_localtime(zoneinfo.data<LibObject<TimeZone>>()->impl, seconds, &ok);

	if (ok) {
		helper.return_value(create_boolean(time.tm_isdst));
	}
}

MINT_FUNCTION(mint_timezone_is_dst_from_milliseconds, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &timepoint = helper.pop_parameter();
	Reference &zoneinfo = helper.pop_parameter();

	bool ok = true;
	time_t seconds = static_cast<time_t>(to_integer(cursor, timepoint) / 1000);

	tm &&time = mint::timezone_localtime(zoneinfo.data<LibObject<TimeZone>>()->impl, seconds, &ok);

	if (ok) {
		helper.return_value(create_boolean(time.tm_isdst));
	}
}
