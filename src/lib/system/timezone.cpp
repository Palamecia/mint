/**
 * Copyright (c) 2026 Gauvain CHERY.
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

#include "mint/ast/cursor.h"
#include "mint/ast/symbol.h"
#include "mint/memory/builtin/libobject.h"
#include "mint/memory/class.h"
#include "mint/memory/data.h"
#include "mint/memory/memory_tools.h"
#include "mint/memory/reference.h"
#include "mint/memory/function_tools.h"
#include "mint/memory/cast_tools.h"
#include <array>
#include <chrono>
#include <format>
#include <optional>
#include <ranges>
#include <regex>
#include <stdexcept>
#include <string>

#ifdef MINT_OS_WINDOWS
#include <__msvc_chrono.hpp>
#else
#include <bits/chrono.h>
#endif

namespace symbols {

const mint::Symbol system("System");
const mint::Symbol week_day("WeekDay");

const std::array days {
    mint::Symbol("Sunday"),
    mint::Symbol("Monday"),
    mint::Symbol("Tuesday"),
    mint::Symbol("Wednesday"),
    mint::Symbol("Thursday"),
    mint::Symbol("Friday"),
    mint::Symbol("Saturday"),
};

}

namespace {

std::optional<std::chrono::minutes> to_offset(const std::string& name) {
	if (std::smatch match; std::regex_match(name, match, std::regex(R"(([+-])(\d{2}):(\d{2}))"))) {
		const auto is_negative = match[1] == "-";
		const auto hours_offset = std::chrono::hours(std::stoi(match[2]));
		const auto minutes_offset = std::chrono::minutes(std::stoi(match[3]));
		const auto offset = std::chrono::minutes(minutes_offset + hours_offset);
		return is_negative ? -offset : +offset;
	}
	return {};
}

std::chrono::minutes to_offset(mint::Cursor& cursor, const mint::Reference& offset) {
	return std::chrono::minutes(mint::to_integer<std::chrono::minutes::rep>(cursor, offset));
}

template<class Duration, class TimePoint>
std::chrono::zoned_time<Duration> to_zoned_time(mint::Cursor& cursor, const mint::Reference& zoneinfo,
    TimePoint time_point) {
	if (mint::is_instance_of(zoneinfo, mint::Class::Metatype::libobject)) {
		return std::chrono::zoned_time<Duration>(zoneinfo.data<mint::LibObject<std::chrono::time_zone>>().ptr,
		    time_point);
	}
	if (mint::is_instance_of(zoneinfo, mint::Data::Format::number)) {
		return std::chrono::zoned_time<Duration>(time_point - to_offset(cursor, zoneinfo));
	}
	return {};
}

mint::Reference mint_timezone_locate(mint::Cursor& cursor, const mint::Reference& name) {
	const auto name_str = to_string(name);
	if (auto offset = to_offset(name_str)) {
		return mint::create_signed_number(offset->count());
	}
	try {
		return mint::create_c_object(cursor.ast(), std::chrono::locate_zone(name_str));
	}
	catch (std::runtime_error&) {
		return {};
	}
}

mint::Reference mint_timezone_get_name(mint::Cursor& cursor, const mint::Reference& d_ptr) {
	if (mint::is_instance_of(d_ptr, mint::Class::Metatype::libobject)) {
		return mint::create_string(cursor.ast(), d_ptr.data<mint::LibObject<std::chrono::time_zone>>().ptr->name());
	}
	if (mint::is_instance_of(d_ptr, mint::Data::Format::number)) {
		const auto offset = to_offset(cursor, d_ptr);
		const auto hours_offset = std::chrono::floor<std::chrono::hours>(offset);
		const auto minutes_offset = std::chrono::minutes(offset - hours_offset);
		return mint::create_string(cursor.ast(), std::format("{}{}:{}", offset < std::chrono::minutes(0) ? '-' : '+',
		                                             hours_offset.count(), minutes_offset.count()));
	}
	return {};
}

mint::Reference mint_timezone_match(mint::Cursor& cursor, const mint::Reference& self, const mint::Reference& other) {
	if (mint::is_instance_of(self, mint::Class::Metatype::libobject)
	    && mint::is_instance_of(other, mint::Class::Metatype::libobject)) {
		return mint::create_boolean(*self.data<mint::LibObject<std::chrono::time_zone>>().ptr
		                            == *other.data<mint::LibObject<std::chrono::time_zone>>().ptr);
	}
	if (mint::is_instance_of(self, mint::Data::Format::number)
	    && mint::is_instance_of(other, mint::Data::Format::number)) {
		return mint::create_boolean(to_offset(cursor, self) == to_offset(cursor, other));
	}
	return mint::create_boolean(false);
}

mint::Reference mint_timezone_current(mint::Cursor& cursor) {
	return mint::create_c_object(cursor.ast(), std::chrono::current_zone());
}

mint::Reference mint_timezone_list(mint::Cursor& cursor) {
	return mint::create_array(cursor.ast(),
	    {std::from_range, std::views::transform(std::chrono::get_tzdb().zones, [&](const auto& time_zone) {
		     return mint::create_string(cursor.ast(), time_zone.name());
	     })});
}

mint::Reference mint_timezone_seconds_since_epoch(mint::Cursor& cursor, const mint::Reference& zoneinfo,
    mint::Reference& year, const mint::Reference& mon, const mint::Reference& mday, const mint::Reference& hour,
    const mint::Reference& min, mint::Reference& sec) {

	const auto date = std::chrono::year_month_day(std::chrono::year(mint::to_integer<int>(cursor, year)),
	    std::chrono::month(mint::to_integer<int>(cursor, mon)), std::chrono::day(mint::to_integer<int>(cursor, mday)));
	if (!date.ok()) {
		return {};
	}

	const auto time = std::chrono::hh_mm_ss<std::chrono::seconds>(
	    std::chrono::hours(mint::to_integer<int>(cursor, hour))
	    + std::chrono::minutes(mint::to_integer<int>(cursor, min))
	    + std::chrono::seconds(mint::to_integer<int>(cursor, sec)));

	if (mint::is_instance_of(zoneinfo, mint::Class::Metatype::libobject)) {
		const auto date_time =
		    std::chrono::zoned_time<std::chrono::seconds>(zoneinfo.data<mint::LibObject<std::chrono::time_zone>>().ptr,
		        std::chrono::local_days(date) + time.to_duration())
		        .get_sys_time();
		return mint::create_signed_number(date_time.time_since_epoch().count());
	}
	if (mint::is_instance_of(zoneinfo, mint::Data::Format::number)) {
		const auto offset = to_offset(cursor, zoneinfo);
		const auto date_time = std::chrono::sys_time<std::chrono::seconds>(
		    std::chrono::sys_days(date) + time.to_duration() - offset);
		return mint::create_signed_number(date_time.time_since_epoch().count());
	}
	return {};
}

mint::Reference mint_timezone_milliseconds_since_epoch(mint::Cursor& cursor, const mint::Reference& zoneinfo,
    mint::Reference& year, const mint::Reference& mon, const mint::Reference& mday, const mint::Reference& hour,
    const mint::Reference& min, mint::Reference& sec, const mint::Reference& msec) {

	const auto date = std::chrono::year_month_day(std::chrono::year(mint::to_integer<int>(cursor, year)),
	    std::chrono::month(mint::to_integer<int>(cursor, mon)), std::chrono::day(mint::to_integer<int>(cursor, mday)));
	if (!date.ok()) {
		return {};
	}

	const auto time = std::chrono::hh_mm_ss<std::chrono::milliseconds>(
	    std::chrono::hours(mint::to_integer<int>(cursor, hour))
	    + std::chrono::minutes(mint::to_integer<int>(cursor, min))
	    + std::chrono::seconds(mint::to_integer<int>(cursor, sec))
	    + std::chrono::milliseconds(mint::to_integer<int>(cursor, msec)));

	if (mint::is_instance_of(zoneinfo, mint::Class::Metatype::libobject)) {
		const auto date_time = std::chrono::zoned_time<std::chrono::milliseconds>(
		    zoneinfo.data<mint::LibObject<std::chrono::time_zone>>().ptr,
		    std::chrono::local_days(date) + time.to_duration())
		                           .get_sys_time();
		return mint::create_signed_number(date_time.time_since_epoch().count());
	}
	if (mint::is_instance_of(zoneinfo, mint::Data::Format::number)) {
		const auto offset = to_offset(cursor, zoneinfo);
		const auto date_time = std::chrono::sys_time<std::chrono::milliseconds>(
		    std::chrono::sys_days(date) + time.to_duration() - offset);
		return mint::create_signed_number(date_time.time_since_epoch().count());
	}
	return {};
}

mint::Reference mint_timezone_time_from_time_point(mint::Cursor& cursor, const mint::Reference& zoneinfo,
    const mint::Reference& duration) {

	const auto zoned_time = to_zoned_time<std::chrono::milliseconds>(cursor, zoneinfo,
	    *duration.data<mint::LibObject<std::chrono::sys_time<std::chrono::milliseconds>>>().ptr);

	const auto days = std::chrono::floor<std::chrono::days>(zoned_time.get_local_time());
	const auto date = std::chrono::year_month_day(days);
	if (!date.ok()) {
		return {};
	}

	const auto time = std::chrono::hh_mm_ss<std::chrono::milliseconds>(
	    std::chrono::local_time(zoned_time.get_local_time() - days).time_since_epoch());

	return create_iterator_from(cursor, mint::create_signed_number(static_cast<int>(date.year())),
	    mint::create_signed_number(static_cast<unsigned>(date.month())),
	    mint::create_signed_number(static_cast<unsigned>(date.day())), mint::create_signed_number(time.hours().count()),
	    mint::create_signed_number(time.minutes().count()), mint::create_signed_number(time.seconds().count()),
	    mint::create_signed_number(static_cast<int>(time.subseconds().count())));
}

mint::Reference mint_timezone_time_from_seconds(mint::Cursor& cursor, const mint::Reference& zoneinfo,
    mint::Reference& duration) {

	const auto zoned_time = to_zoned_time<std::chrono::seconds>(cursor, zoneinfo,
	    std::chrono::sys_time(std::chrono::seconds(mint::to_integer<std::chrono::seconds::rep>(cursor, duration))));

	const auto days = std::chrono::floor<std::chrono::days>(zoned_time.get_local_time());
	const auto date = std::chrono::year_month_day(days);
	if (!date.ok()) {
		return {};
	}

	const auto time = std::chrono::hh_mm_ss<std::chrono::seconds>(
	    std::chrono::local_time(zoned_time.get_local_time() - days).time_since_epoch());

	return create_iterator_from(cursor, mint::create_signed_number(static_cast<int>(date.year())),
	    mint::create_signed_number(static_cast<unsigned>(date.month())),
	    mint::create_signed_number(static_cast<unsigned>(date.day())), mint::create_signed_number(time.hours().count()),
	    mint::create_signed_number(time.minutes().count()), mint::create_signed_number(time.seconds().count()));
}

mint::Reference mint_timezone_time_from_milliseconds(mint::Cursor& cursor, const mint::Reference& zoneinfo,
    mint::Reference& duration) {

	const auto zoned_time = to_zoned_time<std::chrono::milliseconds>(cursor, zoneinfo,
	    std::chrono::sys_time(
	        std::chrono::milliseconds(mint::to_integer<std::chrono::milliseconds::rep>(cursor, duration))));

	const auto days = std::chrono::floor<std::chrono::days>(zoned_time.get_local_time());
	const auto date = std::chrono::year_month_day(days);
	if (!date.ok()) {
		return {};
	}

	const auto time = std::chrono::hh_mm_ss<std::chrono::milliseconds>(
	    std::chrono::local_time(zoned_time.get_local_time() - days).time_since_epoch());

	return create_iterator_from(cursor, mint::create_signed_number(static_cast<int>(date.year())),
	    mint::create_signed_number(static_cast<unsigned>(date.month())),
	    mint::create_signed_number(static_cast<unsigned>(date.day())), mint::create_signed_number(time.hours().count()),
	    mint::create_signed_number(time.minutes().count()), mint::create_signed_number(time.seconds().count()),
	    mint::create_signed_number(static_cast<int>(time.subseconds().count())));
}

mint::Reference mint_timezone_week_day_from_time_point(mint::FunctionHelper& helper, const mint::Reference& zoneinfo,
    const mint::Reference& duration) {

	const auto zoned_time = to_zoned_time<std::chrono::milliseconds>(helper.cursor(), zoneinfo,
	    *duration.data<mint::LibObject<std::chrono::sys_time<std::chrono::milliseconds>>>().ptr);

	const auto weekday = std::chrono::weekday(std::chrono::floor<std::chrono::days>(zoned_time.get_local_time()));
	if (!weekday.ok()) {
		return {};
	}

	return helper.reference(symbols::system)
	    .member(symbols::week_day)
	    .member(symbols::days.at(weekday.c_encoding()))
	    .share();
}

mint::Reference mint_timezone_week_day_from_seconds(mint::FunctionHelper& helper, const mint::Reference& zoneinfo,
    mint::Reference& duration) {

	const auto zoned_time = to_zoned_time<std::chrono::seconds>(helper.cursor(), zoneinfo,
	    std::chrono::sys_time(
	        std::chrono::seconds(mint::to_integer<std::chrono::seconds::rep>(helper.cursor(), duration))));

	const auto weekday = std::chrono::weekday(std::chrono::floor<std::chrono::days>(zoned_time.get_local_time()));
	if (!weekday.ok()) {
		return {};
	}

	return helper.reference(symbols::system)
	    .member(symbols::week_day)
	    .member(symbols::days.at(weekday.c_encoding()))
	    .share();
}

mint::Reference mint_timezone_week_day_from_milliseconds(mint::FunctionHelper& helper, const mint::Reference& zoneinfo,
    const mint::Reference& duration) {

	const auto zoned_time = to_zoned_time<std::chrono::milliseconds>(helper.cursor(), zoneinfo,
	    std::chrono::sys_time(
	        std::chrono::milliseconds(mint::to_integer<std::chrono::milliseconds::rep>(helper.cursor(), duration))));

	const auto weekday = std::chrono::weekday(std::chrono::floor<std::chrono::days>(zoned_time.get_local_time()));
	if (!weekday.ok()) {
		return {};
	}

	return helper.reference(symbols::system)
	    .member(symbols::week_day)
	    .member(symbols::days.at(weekday.c_encoding()))
	    .share();
}

mint::Reference mint_timezone_year_day_from_time_point(mint::Cursor& cursor, const mint::Reference& zoneinfo,
    const mint::Reference& duration) {

	const auto zoned_time = to_zoned_time<std::chrono::milliseconds>(cursor, zoneinfo,
	    *duration.data<mint::LibObject<std::chrono::sys_time<std::chrono::milliseconds>>>().ptr);

	const auto days = std::chrono::floor<std::chrono::days>(zoned_time.get_local_time()).time_since_epoch();
	const auto day_of_year = (days
	                          - std::chrono::duration_cast<std::chrono::days>(
	                              std::chrono::floor<std::chrono::years>(days)));

	return mint::create_signed_number(day_of_year.count());
}

mint::Reference mint_timezone_year_day_from_seconds(mint::Cursor& cursor, const mint::Reference& zoneinfo,
    mint::Reference& duration) {

	const auto zoned_time = to_zoned_time<std::chrono::seconds>(cursor, zoneinfo,
	    std::chrono::sys_time(std::chrono::seconds(mint::to_integer<std::chrono::seconds::rep>(cursor, duration))));

	const auto days = std::chrono::floor<std::chrono::days>(zoned_time.get_local_time()).time_since_epoch();
	const auto day_of_year = (days
	                          - std::chrono::duration_cast<std::chrono::days>(
	                              std::chrono::floor<std::chrono::years>(days)));

	return mint::create_signed_number(day_of_year.count());
}

mint::Reference mint_timezone_year_day_from_milliseconds(mint::Cursor& cursor, const mint::Reference& zoneinfo,
    mint::Reference& duration) {

	const auto zoned_time = to_zoned_time<std::chrono::milliseconds>(cursor, zoneinfo,
	    std::chrono::sys_time(
	        std::chrono::milliseconds(mint::to_integer<std::chrono::milliseconds::rep>(cursor, duration))));

	const auto days = std::chrono::floor<std::chrono::days>(zoned_time.get_local_time()).time_since_epoch();
	const auto day_of_year = (days
	                          - std::chrono::duration_cast<std::chrono::days>(
	                              std::chrono::floor<std::chrono::years>(days)));

	return mint::create_signed_number(day_of_year.count());
}

mint::Reference mint_timezone_is_dst_from_time_point(mint::Cursor& cursor, const mint::Reference& zoneinfo,
    const mint::Reference& duration) {

	const auto zoned_time = to_zoned_time<std::chrono::milliseconds>(cursor, zoneinfo,
	    *duration.data<mint::LibObject<std::chrono::sys_time<std::chrono::milliseconds>>>().ptr);

	return mint::create_boolean(zoned_time.get_info().save != std::chrono::minutes(0));
}

mint::Reference mint_timezone_is_dst_from_seconds(mint::Cursor& cursor, const mint::Reference& zoneinfo,
    mint::Reference& duration) {

	const auto zoned_time = to_zoned_time<std::chrono::seconds>(cursor, zoneinfo,
	    std::chrono::sys_time(std::chrono::seconds(mint::to_integer<std::chrono::seconds::rep>(cursor, duration))));

	return mint::create_boolean(zoned_time.get_info().save != std::chrono::minutes(0));
}

mint::Reference mint_timezone_is_dst_from_milliseconds(mint::Cursor& cursor, const mint::Reference& zoneinfo,
    mint::Reference& duration) {

	const auto zoned_time = to_zoned_time<std::chrono::milliseconds>(cursor, zoneinfo,
	    std::chrono::sys_time(
	        std::chrono::milliseconds(mint::to_integer<std::chrono::milliseconds::rep>(cursor, duration))));

	return mint::create_boolean(zoned_time.get_info().save != std::chrono::minutes(0));
}

}

MINT_EXPORT_FUNCTION(mint_timezone_locate, 1);
MINT_EXPORT_FUNCTION(mint_timezone_get_name, 1);
MINT_EXPORT_FUNCTION(mint_timezone_match, 2);
MINT_EXPORT_FUNCTION(mint_timezone_current, 0);
MINT_EXPORT_FUNCTION(mint_timezone_list, 0);
MINT_EXPORT_FUNCTION(mint_timezone_seconds_since_epoch, 7);
MINT_EXPORT_FUNCTION(mint_timezone_milliseconds_since_epoch, 8);
MINT_EXPORT_FUNCTION(mint_timezone_time_from_time_point, 2);
MINT_EXPORT_FUNCTION(mint_timezone_time_from_seconds, 2);
MINT_EXPORT_FUNCTION(mint_timezone_time_from_milliseconds, 2);
MINT_EXPORT_FUNCTION(mint_timezone_week_day_from_time_point, 2);
MINT_EXPORT_FUNCTION(mint_timezone_week_day_from_seconds, 2);
MINT_EXPORT_FUNCTION(mint_timezone_week_day_from_milliseconds, 2);
MINT_EXPORT_FUNCTION(mint_timezone_year_day_from_time_point, 2);
MINT_EXPORT_FUNCTION(mint_timezone_year_day_from_seconds, 2);
MINT_EXPORT_FUNCTION(mint_timezone_year_day_from_milliseconds, 2);
MINT_EXPORT_FUNCTION(mint_timezone_is_dst_from_time_point, 2);
MINT_EXPORT_FUNCTION(mint_timezone_is_dst_from_seconds, 2);
MINT_EXPORT_FUNCTION(mint_timezone_is_dst_from_milliseconds, 2);
