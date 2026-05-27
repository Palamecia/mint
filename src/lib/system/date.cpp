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

#include "mint/memory/builtin/libobject.h"
#include "mint/memory/reference.h"
#include "mint/memory/function_tools.h"
#include "mint/memory/cast_tools.h"
#include "mint/system/errno.h"
#include <cstdint>
#include <chrono>
#include <cstdlib>
#include <format>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>

#ifdef MINT_OS_WINDOWS
#include <utility>
#include <Windows.h>
#include <__msvc_chrono.hpp>
#include <minwinbase.h>
#include <minwindef.h>
#include <sysinfoapi.h>
#else
#include <bits/chrono.h>
#include <bits/types/struct_timeval.h>
#include <ctime>
#include <sys/time.h>
#include <sys/types.h>
#endif

using namespace std::chrono_literals;

namespace {

template<class Duration>
Duration to_duration(const std::string& str, Duration min, Duration max) {
	const auto duration = Duration(std::stoi(str));
	if (duration < min || duration > max) {
		throw std::runtime_error("date format is not valid");
	}
	return duration;
}

std::tuple<std::chrono::month, std::chrono::day> week_number_to_month_and_day(std::chrono::year year, int week_number,
    std::chrono::weekday day) {
	const auto date = std::chrono::year_month_day(std::chrono::sys_days(year / 1 / std::chrono::Thursday[1])
	                                              - (std::chrono::Thursday - std::chrono::Monday)
	                                              + std::chrono::weeks(week_number - 1) + (day - std::chrono::Monday));
	if (!date.ok()) {
		throw std::runtime_error("date format is not valid");
	}
	return {date.month(), date.day()};
}

std::tuple<std::chrono::month, std::chrono::day> year_day_to_month_and_day(std::chrono::year year, int year_day) {
	const auto date = std::chrono::year_month_day(
	    std::chrono::sys_days(year / std::chrono::January / 1) + std::chrono::days(year_day - 1));
	if (!date.ok()) {
		throw std::runtime_error("date format is not valid");
	}
	return {date.month(), date.day()};
}

std::string offset_to_timezone(const std::chrono::minutes& offset,
    std::chrono::sys_time<std::chrono::milliseconds>& time_point) {

	const auto hours_offset = std::chrono::floor<std::chrono::hours>(offset);
	const auto minutes_offset = std::chrono::minutes(offset - hours_offset);

	if (minutes_offset == std::chrono::minutes {0}) {
		const auto name = std::format("Etc/GMT{}{}", offset < std::chrono::minutes(0) ? '-' : '+',
		    std::abs(hours_offset.count()));
		time_point -= offset;
		return name;
	}

	time_point -= offset;
	return std::format("{}{}:{}", offset < std::chrono::minutes(0) ? '-' : '+', hours_offset.count(),
	    minutes_offset.count());
}

std::tuple<std::string, std::chrono::sys_time<std::chrono::milliseconds>> parse_iso_date(std::string_view str) {

	enum State : std::uint8_t {
		read_start,
		read_year_fraction,
		read_month_day,
		read_week,
		read_week_day,
		read_time,
		read_minutes,
		read_seconds,
		read_seconds_fraction,
		read_positive_offset,
		read_positive_offset_minutes,
		read_negative_offset,
		read_negative_offset_minutes,
		read_end
	};

	std::chrono::year year {1970};
	std::chrono::month month {std::chrono::January};
	std::chrono::day day {1};
	int week_number = -1;
	std::chrono::milliseconds time {0};
	std::chrono::minutes offset {0};

	State state = read_start;
	std::string token;

	for (const auto ch : str) {
		switch (ch) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			token += ch;
			break;
		case ':':
			switch (state) {
			case read_start:
			case read_time:
				switch (token.length()) {
				case 2:
					time += to_duration(token, 0h, 23h);
					break;
				default:
					throw std::runtime_error("date format is not valid");
				}
				state = read_minutes;
				token.clear();
				break;
			case read_minutes:
				switch (token.length()) {
				case 2:
					time += to_duration(token, 0min, 59min);
					break;
				default:
					throw std::runtime_error("date format is not valid");
				}
				state = read_seconds;
				token.clear();
				break;
			case read_negative_offset:
				switch (token.length()) {
				case 2:
					offset -= to_duration(token, 0h, 23h);
					break;
				default:
					throw std::runtime_error("date format is not valid");
				}
				state = read_negative_offset_minutes;
				token.clear();
				break;
			case read_positive_offset:
				switch (token.length()) {
				case 2:
					offset += to_duration(token, 0h, 23h);
					break;
				default:
					throw std::runtime_error("date format is not valid");
				}
				state = read_positive_offset_minutes;
				token.clear();
				break;
			default:
				throw std::runtime_error("date format is not valid");
			}
			break;
		case '-':
			switch (state) {
			case read_start:
				switch (token.length()) {
				case 4:
					year = std::chrono::year(std::stoi(token));
					break;
				default:
					throw std::runtime_error("date format is not valid");
				}
				state = read_year_fraction;
				token.clear();
				break;
			case read_year_fraction:
				switch (token.length()) {
				case 2:
					month = std::chrono::month(std::stoi(token));
					break;
				default:
					throw std::runtime_error("date format is not valid");
				}
				state = read_month_day;
				token.clear();
				break;
			case read_week:
				switch (token.length()) {
				case 2:
					week_number = std::stoi(token);
					break;
				default:
					throw std::runtime_error("date format is not valid");
				}
				state = read_week_day;
				token.clear();
				break;
			case read_time:
				switch (token.length()) {
				case 2:
					time += to_duration(token, 0h, 23h);
					break;
				case 4:
					time += to_duration(token.substr(0, 2), 0h, 23h);
					time += to_duration(token.substr(2, 2), 0min, 59min);
					break;
				case 6:
					time += to_duration(token.substr(0, 2), 0h, 23h);
					time += to_duration(token.substr(2, 2), 0min, 59min);
					time += to_duration(token.substr(4, 2), 0s, 59s);
					break;
				default:
					throw std::runtime_error("date format is not valid");
				}
				state = read_negative_offset;
				token.clear();
				break;
			case read_minutes:
				switch (token.length()) {
				case 2:
					time += to_duration(token, 0min, 59min);
					break;
				default:
					throw std::runtime_error("date format is not valid");
				}
				state = read_negative_offset;
				token.clear();
				break;
			case read_seconds:
				switch (token.length()) {
				case 2:
					time += to_duration(token, 0s, 59s);
					break;
				default:
					throw std::runtime_error("date format is not valid");
				}
				state = read_negative_offset;
				token.clear();
				break;
			case read_seconds_fraction:
				while (token.length() < 3) {
					token += "0";
				}
				time += to_duration(token.substr(0, 3), 0ms, 999ms);
				state = read_negative_offset;
				token.clear();
				break;
			default:
				throw std::runtime_error("date format is not valid");
			}
			break;
		case '+':
			switch (state) {
			case read_time:
				switch (token.length()) {
				case 2:
					time += to_duration(token, 0h, 23h);
					break;
				case 4:
					time += to_duration(token.substr(0, 2), 0h, 23h);
					time += to_duration(token.substr(2, 2), 0min, 59min);
					break;
				case 6:
					time += to_duration(token.substr(0, 2), 0h, 23h);
					time += to_duration(token.substr(2, 2), 0min, 59min);
					time += to_duration(token.substr(4, 2), 0s, 59s);
					break;
				default:
					throw std::runtime_error("date format is not valid");
				}
				state = read_positive_offset;
				token.clear();
				break;
			case read_minutes:
				switch (token.length()) {
				case 2:
					time += to_duration(token, 0min, 59min);
					break;
				default:
					throw std::runtime_error("date format is not valid");
				}
				state = read_positive_offset;
				token.clear();
				break;
			case read_seconds:
				switch (token.length()) {
				case 2:
					time += to_duration(token, 0s, 59s);
					break;
				default:
					throw std::runtime_error("date format is not valid");
				}
				state = read_positive_offset;
				token.clear();
				break;
			case read_seconds_fraction:
				while (token.length() < 3) {
					token += "0";
				}
				time += to_duration(token.substr(0, 3), 0ms, 999ms);
				state = read_positive_offset;
				token.clear();
				break;
			default:
				throw std::runtime_error("date format is not valid");
			}
			break;
		case 'T':
			switch (state) {
			case read_start:
				switch (token.length()) {
				case 0:
					{
						const auto date = std::chrono::year_month_day(
						    std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now()));
						year = date.year();
						month = date.month();
						day = date.day();
					}
					break;
				case 4:
					year = std::chrono::year(std::stoi(token));
					break;
				case 6:
					year = std::chrono::year(std::stoi(token.substr(0, 4)));
					month = std::chrono::month(std::stoi(token.substr(4, 2)));
					break;
				case 7:
					year = std::chrono::year(std::stoi(token.substr(0, 4)));
					std::tie(month, day) = year_day_to_month_and_day(year, std::stoi(token.substr(4, 3)));
					break;
				case 8:
					year = std::chrono::year(std::stoi(token.substr(0, 4)));
					month = std::chrono::month(std::stoi(token.substr(4, 2)));
					day = std::chrono::day(std::stoi(token.substr(6, 2)));
					break;
				default:
					throw std::runtime_error("date format is not valid");
				}
				state = read_time;
				token.clear();
				break;
			case read_year_fraction:
				switch (token.length()) {
				case 2:
					month = std::chrono::month(std::stoi(token));
					break;
				case 3:
					std::tie(month, day) = year_day_to_month_and_day(year, std::stoi(token));
					break;
				default:
					throw std::runtime_error("date format is not valid");
				}
				state = read_time;
				token.clear();
				break;
			case read_month_day:
				switch (token.length()) {
				case 2:
					day = std::chrono::day(std::stoi(token));
					break;
				default:
					throw std::runtime_error("date format is not valid");
				}
				state = read_time;
				token.clear();
				break;
			case read_week:
				switch (token.length()) {
				case 3:
					std::tie(month, day) = week_number_to_month_and_day(year, std::stoi(token.substr(0, 2)),
					    std::chrono::weekday(std::stoi(token.substr(2, 1))));
					break;
				default:
					throw std::runtime_error("date format is not valid");
				}
				state = read_time;
				token.clear();
				break;
			case read_week_day:
				switch (token.length()) {
				case 1:
					std::tie(month, day) = week_number_to_month_and_day(year, week_number,
					    std::chrono::weekday(std::stoi(token)));
					break;
				default:
					throw std::runtime_error("date format is not valid");
				}
				state = read_time;
				token.clear();
				break;
			default:
				throw std::runtime_error("date format is not valid");
			}
			break;
		case 'W':
			switch (state) {
			case read_start:
				switch (token.length()) {
				case 4:
					year = std::chrono::year(std::stoi(token));
					break;
				default:
					throw std::runtime_error("date format is not valid");
				}
				break;
			case read_year_fraction:
				break;
			default:
				throw std::runtime_error("date format is not valid");
			}
			state = read_week;
			token.clear();
			break;
		case 'Z':
			switch (state) {
			case read_time:
				switch (token.length()) {
				case 2:
					time += to_duration(token, 0h, 23h);
					break;
				case 4:
					time += to_duration(token.substr(0, 2), 0h, 23h);
					time += to_duration(token.substr(2, 2), 0min, 59min);
					break;
				case 6:
					time += to_duration(token.substr(0, 2), 0h, 23h);
					time += to_duration(token.substr(2, 2), 0min, 59min);
					time += to_duration(token.substr(4, 2), 0s, 59s);
					break;
				default:
					throw std::runtime_error("date format is not valid");
				}
				state = read_end;
				token.clear();
				break;
			case read_minutes:
				switch (token.length()) {
				case 2:
					time += to_duration(token, 0min, 59min);
					break;
				default:
					throw std::runtime_error("date format is not valid");
				}
				state = read_end;
				token.clear();
				break;
			case read_seconds:
				switch (token.length()) {
				case 2:
					time += to_duration(token, 0s, 59s);
					break;
				default:
					throw std::runtime_error("date format is not valid");
				}
				state = read_end;
				token.clear();
				break;
			case read_seconds_fraction:
				while (token.length() < 3) {
					token += "0";
				}
				time += to_duration(token.substr(0, 3), 0ms, 999ms);
				state = read_end;
				token.clear();
				break;
			default:
				throw std::runtime_error("date format is not valid");
			}
			break;
		case '.':
		case ',':
			switch (state) {
			case read_time:
				switch (token.length()) {
				case 2:
					time += to_duration(token, 0h, 23h);
					break;
				case 4:
					time += to_duration(token.substr(0, 2), 0h, 23h);
					time += to_duration(token.substr(2, 2), 0min, 59min);
					break;
				case 6:
					time += to_duration(token.substr(0, 2), 0h, 23h);
					time += to_duration(token.substr(2, 2), 0min, 59min);
					time += to_duration(token.substr(4, 2), 0s, 59s);
					break;
				default:
					throw std::runtime_error("date format is not valid");
				}
				break;
			case read_seconds:
				switch (token.length()) {
				case 2:
					time += to_duration(token, 0s, 59s);
					break;
				default:
					throw std::runtime_error("date format is not valid");
				}
				break;
			default:
				throw std::runtime_error("date format is not valid");
			}
			state = read_seconds_fraction;
			token.clear();
			break;
		default:
			throw std::runtime_error("date format is not valid");
		}
	}

	if (!token.empty()) {
		switch (state) {
		case read_start:
			switch (token.length()) {
			case 4:
				year = std::chrono::year(std::stoi(token));
				break;
			case 6:
				year = std::chrono::year(std::stoi(token.substr(0, 4)));
				month = std::chrono::month(std::stoi(token.substr(4, 2)));
				break;
			case 7:
				year = std::chrono::year(std::stoi(token.substr(0, 4)));
				std::tie(month, day) = year_day_to_month_and_day(year, std::stoi(token.substr(4, 3)));
				break;
			case 8:
				year = std::chrono::year(std::stoi(token.substr(0, 4)));
				month = std::chrono::month(std::stoi(token.substr(4, 2)));
				day = std::chrono::day(std::stoi(token.substr(6, 2)));
				break;
			default:
				throw std::runtime_error("date format is not valid");
			}
			break;
		case read_year_fraction:
			switch (token.length()) {
			case 2:
				month = std::chrono::month(std::stoi(token));
				break;
			case 3:
				std::tie(month, day) = year_day_to_month_and_day(year, std::stoi(token));
				break;
			default:
				throw std::runtime_error("date format is not valid");
			}
			break;
		case read_month_day:
			switch (token.length()) {
			case 2:
				day = std::chrono::day(std::stoi(token));
				break;
			default:
				throw std::runtime_error("date format is not valid");
			}
			break;
		case read_week:
			switch (token.length()) {
			case 3:
				std::tie(month, day) = week_number_to_month_and_day(year, std::stoi(token.substr(0, 2)),
				    std::chrono::weekday(std::stoi(token.substr(2, 1))));
				break;
			default:
				throw std::runtime_error("date format is not valid");
			}
			break;
		case read_week_day:
			switch (token.length()) {
			case 1:
				std::tie(month, day) = week_number_to_month_and_day(year, week_number,
				    std::chrono::weekday(std::stoi(token)));
				break;
			default:
				throw std::runtime_error("date format is not valid");
			}
			break;
		case read_time:
			switch (token.length()) {
			case 2:
				time += to_duration(token, 0h, 23h);
				break;
			case 4:
				time += to_duration(token.substr(0, 2), 0h, 23h);
				time += to_duration(token.substr(2, 2), 0min, 59min);
				break;
			case 6:
				time += to_duration(token.substr(0, 2), 0h, 23h);
				time += to_duration(token.substr(2, 2), 0min, 59min);
				time += to_duration(token.substr(4, 2), 0s, 59s);
				break;
			default:
				throw std::runtime_error("date format is not valid");
			}
			break;
		case read_minutes:
			switch (token.length()) {
			case 2:
				time += to_duration(token, 0min, 59min);
				break;
			default:
				throw std::runtime_error("date format is not valid");
			}
			break;
		case read_seconds:
			switch (token.length()) {
			case 2:
				time += to_duration(token, 0s, 59s);
				break;
			default:
				throw std::runtime_error("date format is not valid");
			}
			break;
		case read_seconds_fraction:
			while (token.length() < 3) {
				token += "0";
			}
			time += to_duration(token.substr(0, 3), 0ms, 999ms);
			break;
		case read_negative_offset:
			switch (token.length()) {
			case 4:
				offset -= to_duration(token.substr(0, 2), 0h, 23h);
				offset -= to_duration(token.substr(2, 2), 0min, 59min);
				break;
			default:
				throw std::runtime_error("date format is not valid");
			}
			break;
		case read_negative_offset_minutes:
			switch (token.length()) {
			case 2:
				offset -= to_duration(token, 0min, 59min);
				break;
			default:
				throw std::runtime_error("date format is not valid");
			}
			break;
		case read_positive_offset:
			switch (token.length()) {
			case 4:
				offset += to_duration(token.substr(0, 2), 0h, 23h);
				offset += to_duration(token.substr(2, 2), 0min, 59min);
				break;
			default:
				throw std::runtime_error("date format is not valid");
			}
			break;
		case read_positive_offset_minutes:
			switch (token.length()) {
			case 2:
				offset += to_duration(token, 0min, 59min);
				break;
			default:
				throw std::runtime_error("date format is not valid");
			}
			break;
		default:
			throw std::runtime_error("date format is not valid");
		}
	}

	const auto date = std::chrono::year_month_day(year, month, day);
	if (!date.ok()) {
		throw std::runtime_error("date format is not valid");
	}

	auto time_point = std::chrono::sys_days(date) + time;
	const auto time_zone = (offset != std::chrono::minutes(0)) ? offset_to_timezone(offset, time_point)
	                                                           : std::string("UTC");

	return {time_zone, time_point};
}

mint::Reference mint_date_current_timepoint(mint::Cursor& cursor) {
	return mint::create_c_object(cursor.ast(),
	    new std::chrono::sys_time<std::chrono::milliseconds>(std::chrono::duration_cast<std::chrono::milliseconds>(
	        std::chrono::system_clock::now().time_since_epoch())));
}

mint::Reference mint_date_set_current(mint::Cursor& /*cursor*/, const mint::Reference& duration) {

	const auto time_point = std::chrono::sys_time(
	    *duration.data<mint::LibObject<std::chrono::sys_time<std::chrono::milliseconds>>>().ptr);

#ifdef MINT_OS_WINDOWS

	const auto date = std::chrono::year_month_day(std::chrono::floor<std::chrono::days>(time_point));
	if (!date.ok()) {
		return mint::create_number(EINVAL);
	}

	const auto weekday = std::chrono::weekday(std::chrono::floor<std::chrono::days>(time_point));
	if (!weekday.ok()) {
		return {};
	}

	const auto time = std::chrono::hh_mm_ss<std::chrono::milliseconds>(time_point.time_since_epoch());

	const auto system_time = SYSTEMTIME {
	    .wYear = static_cast<WORD>(static_cast<int>(date.year())),
	    .wMonth = static_cast<WORD>(static_cast<unsigned>(date.month())),
	    .wDayOfWeek = static_cast<WORD>(weekday.c_encoding()),
	    .wDay = static_cast<WORD>(static_cast<unsigned>(date.day())),
	    .wHour = static_cast<WORD>(time.hours().count()),
	    .wMinute = static_cast<WORD>(time.minutes().count()),
	    .wSecond = static_cast<WORD>(time.seconds().count()),
	    .wMilliseconds = static_cast<WORD>(time.subseconds().count()),
	};

	if (!SetSystemTime(&system_time)) {
		return mint::create_number(mint::errno_from_error_code(mint::last_error_code()));
	}
#else
	timeval tv {
	    .tv_sec = static_cast<time_t>(
	        std::chrono::duration_cast<std::chrono::seconds>(time_point.time_since_epoch()).count()),
	    .tv_usec = static_cast<suseconds_t>((time_point.time_since_epoch().count() % 1000) * 1000),
	};

	if (settimeofday(&tv, nullptr)) {
		return mint::create_number(errno);
	}
#endif

	return {};
}

mint::Reference mint_date_delete(mint::Cursor& /*cursor*/, const mint::Reference& duration) {
	delete duration.data<mint::LibObject<std::chrono::sys_time<std::chrono::milliseconds>>>().ptr;
	return {};
}

mint::Reference mint_date_set_seconds(mint::Cursor& cursor, const mint::Reference& duration, mint::Reference& value) {
	*duration.data<mint::LibObject<std::chrono::sys_time<std::chrono::milliseconds>>>().ptr =
	    std::chrono::sys_time<std::chrono::milliseconds>(std::chrono::duration_cast<std::chrono::milliseconds>(
	        std::chrono::seconds(to_integer<std::chrono::seconds::rep>(cursor, value))));
	return {};
}

mint::Reference mint_date_timepoint_to_seconds(mint::Cursor& /*cursor*/, const mint::Reference& duration) {
	return mint::create_signed_number(std::chrono::duration_cast<std::chrono::seconds>(
	    duration.data<mint::LibObject<std::chrono::sys_time<std::chrono::milliseconds>>>().ptr->time_since_epoch())
	        .count());
}

mint::Reference mint_date_seconds_to_timepoint(mint::Cursor& cursor, const mint::Reference& value) {
	return mint::create_c_object(cursor.ast(),
	    new std::chrono::sys_time<std::chrono::milliseconds>(std::chrono::duration_cast<std::chrono::milliseconds>(
	        std::chrono::seconds(to_integer<std::chrono::seconds::rep>(cursor, value)))));
}

mint::Reference mint_date_set_milliseconds(mint::Cursor& cursor, const mint::Reference& duration,
    mint::Reference& value) {
	*duration.data<mint::LibObject<std::chrono::sys_time<std::chrono::milliseconds>>>().ptr =
	    std::chrono::sys_time<std::chrono::milliseconds>(
	        std::chrono::milliseconds(to_integer<std::chrono::milliseconds::rep>(cursor, value)));
	return {};
}

mint::Reference mint_date_timepoint_to_milliseconds(mint::Cursor& /*cursor*/, const mint::Reference& duration) {
	return mint::create_signed_number(duration.data<mint::LibObject<std::chrono::sys_time<std::chrono::milliseconds>>>()
	        .ptr->time_since_epoch()
	        .count());
}

mint::Reference mint_date_milliseconds_to_timepoint(mint::Cursor& cursor, const mint::Reference& value) {
	return mint::create_c_object(cursor.ast(),
	    new std::chrono::sys_time<std::chrono::milliseconds>(
	        std::chrono::milliseconds(to_integer<std::chrono::milliseconds::rep>(cursor, value))));
}

mint::Reference mint_date_equals(mint::Cursor& /*cursor*/, const mint::Reference& self, const mint::Reference& other) {
	return mint::create_boolean(
	    (*self.data<mint::LibObject<std::chrono::sys_time<std::chrono::milliseconds>>>().ptr)
	    == (*other.data<mint::LibObject<std::chrono::sys_time<std::chrono::milliseconds>>>().ptr));
}

mint::Reference mint_parse_iso_date(mint::Cursor& cursor, const mint::Reference& date) {
	try {
		const auto [timezone, timepoint] = parse_iso_date(mint::to_string(date));
		return mint::create_iterator_from(cursor,
		    mint::create_c_object(cursor.ast(), new std::chrono::sys_time<std::chrono::milliseconds>(timepoint)),
		    mint::create_string(cursor.ast(), timezone));
	}
	catch (const std::runtime_error&) {
		return {};
	}
}

mint::Reference mint_date_is_leap(mint::Cursor& cursor, const mint::Reference& year) {
	return mint::create_boolean(std::chrono::year(mint::to_integer<int>(cursor, year)).is_leap());
}

mint::Reference mint_date_days_in_month(mint::Cursor& cursor, const mint::Reference& year,
    const mint::Reference& month) {
	return mint::create_unsigned_number(
	    static_cast<unsigned>(std::chrono::year_month_day_last(std::chrono::year(mint::to_integer<int>(cursor, year)),
	        std::chrono::month_day_last(std::chrono::month(mint::to_integer<unsigned>(cursor, month))))
	            .day()));
}

}

MINT_EXPORT_FUNCTION(mint_date_current_timepoint, 0);
MINT_EXPORT_FUNCTION(mint_date_set_current, 1);
MINT_EXPORT_FUNCTION(mint_date_delete, 1);
MINT_EXPORT_FUNCTION(mint_date_set_seconds, 2);
MINT_EXPORT_FUNCTION(mint_date_timepoint_to_seconds, 1);
MINT_EXPORT_FUNCTION(mint_date_seconds_to_timepoint, 1);
MINT_EXPORT_FUNCTION(mint_date_set_milliseconds, 2);
MINT_EXPORT_FUNCTION(mint_date_timepoint_to_milliseconds, 1);
MINT_EXPORT_FUNCTION(mint_date_milliseconds_to_timepoint, 1);
MINT_EXPORT_FUNCTION(mint_date_equals, 2);
MINT_EXPORT_FUNCTION(mint_parse_iso_date, 1);
MINT_EXPORT_FUNCTION(mint_date_is_leap, 1);
MINT_EXPORT_FUNCTION(mint_date_days_in_month, 2);
