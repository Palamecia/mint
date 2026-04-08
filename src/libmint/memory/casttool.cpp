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

#include "mint/memory/casttool.h"
#include "mint/ast/cursor.h"
#include "mint/ast/symbol.h"
#include "mint/memory/builtin/array.h"
#include "mint/memory/builtin/hash.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/memory/builtin/regex.h"
#include "mint/memory/builtin/string.h"
#include "mint/memory/data.h"
#include "mint/memory/functiontool.h"
#include "mint/memory/memorytool.h"
#include "mint/memory/object.h"
#include "mint/memory/reference.h"
#include "mint/system/assert.h"
#include "mint/system/error.h"
#include "mint/system/string.h"
#include "mint/system/utf8.h"
#include "mint/scheduler/scheduler.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <format>
#include <iterator>
#include <optional>
#include <regex>
#include <stdexcept>
#include <string>
#include <cmath>
#include <ranges>
#include <string_view>
#include <utility>

using namespace mint;

namespace {

std::string number_to_char(std::intmax_t number) {

	std::string result;

	while (number) {
		result.insert(result.begin(), static_cast<char>(number % (1 << 8)));
		number = number / (1 << 8);
	}

	return result;
}

}

double mint::to_number(Cursor& cursor, const Reference& ref) {
	switch (ref.data().format()) {
	case Data::Format::none:
		error("invalid conversion from 'none' to 'number'");
	case Data::Format::null:
		cursor.raise(ref);
		break;
	case Data::Format::number:
		return ref.data<Number>().value;
	case Data::Format::boolean:
		return ref.data<Boolean>().value;
	case Data::Format::object:
		switch (ref.data<Object>().metadata.metatype()) {
		case Class::Metatype::string:
			try {
				return to_signed_number(ref.data<String>().str);
			}
			catch (std::exception&) {
				return 0;
			}
		case Class::Metatype::iterator:
		case Class::Metatype::async_iterator:
			if (std::optional<WeakReference>&& item = iterator_get(ref.data<Iterator>())) {
				return to_number(cursor, *item);
			}
			return to_number(cursor, create_none());
		default:
			if (auto* method = ref.data<Object>().metadata.find_member(builtin_symbols::to_number)) {
				auto* scheduler = Scheduler::instance();
				assert_x(scheduler, __func__, "execution should be done using a scheduler");
				return to_number(cursor, scheduler->invoke(ref, builtin_symbols::to_number, *method));
			}
			error("invalid conversion from '{}' to 'number'", type_name(ref));
		}
		break;
	case Data::Format::package:
		error("invalid conversion from 'package' to 'number'");
	case Data::Format::function:
		error("invalid conversion from 'function' to 'number'");
	case Data::Format::coroutine:
		error("invalid conversion from 'coroutine' to 'number'");
	}
	return 0;
}

double mint::to_number(Cursor& cursor, Reference&& ref) {
	switch (ref.data().format()) {
	case Data::Format::none:
		error("invalid conversion from 'none' to 'number'");
	case Data::Format::null:
		cursor.raise(std::move(ref));
		break;
	case Data::Format::number:
		return ref.data<Number>().value;
	case Data::Format::boolean:
		return ref.data<Boolean>().value;
	case Data::Format::object:
		switch (ref.data<Object>().metadata.metatype()) {
		case Class::Metatype::string:
			try {
				return to_signed_number(ref.data<String>().str);
			}
			catch (std::exception&) {
				return 0;
			}
		case Class::Metatype::iterator:
		case Class::Metatype::async_iterator:
			if (std::optional<WeakReference>&& item = iterator_get(ref.data<Iterator>())) {
				return to_number(cursor, *item);
			}
			return to_number(cursor, create_none());
		default:
			if (auto* method = ref.data<Object>().metadata.find_member(builtin_symbols::to_number)) {
				auto* scheduler = Scheduler::instance();
				assert_x(scheduler, __func__, "execution should be done using a scheduler");
				return to_number(cursor, scheduler->invoke(ref, builtin_symbols::to_number, *method));
			}
			error("invalid conversion from '{}' to 'number'", type_name(ref));
		}
		break;
	case Data::Format::package:
		error("invalid conversion from 'package' to 'number'");
	case Data::Format::function:
		error("invalid conversion from 'function' to 'number'");
	case Data::Format::coroutine:
		error("invalid conversion from 'coroutine' to 'number'");
	}
	return 0;
}

std::intmax_t mint::to_signed_integer(Cursor& cursor, const Reference& ref) {
	switch (ref.data().format()) {
	case Data::Format::none:
		error("invalid conversion from 'none' to 'number'");
	case Data::Format::null:
		cursor.raise(ref);
		break;
	case Data::Format::number:
		return to_signed_integer(ref.data<Number>().value);
	case Data::Format::boolean:
		return ref.data<Boolean>().value;
	case Data::Format::object:
		switch (ref.data<Object>().metadata.metatype()) {
		case Class::Metatype::string:
			try {
				return to_signed_integer(ref.data<String>().str);
			}
			catch (std::exception&) {
				return 0;
			}
		case Class::Metatype::iterator:
		case Class::Metatype::async_iterator:
			if (std::optional<WeakReference>&& item = iterator_get(ref.data<Iterator>())) {
				return to_signed_integer(cursor, *item);
			}
			return to_signed_integer(cursor, create_none());
		default:
			if (auto* method = ref.data<Object>().metadata.find_member(builtin_symbols::to_number)) {
				auto* scheduler = Scheduler::instance();
				assert_x(scheduler, __func__, "execution should be done using a scheduler");
				return to_signed_integer(cursor, scheduler->invoke(ref, builtin_symbols::to_number, *method));
			}
			error("invalid conversion from '{}' to 'number'", type_name(ref));
		}
		break;
	case Data::Format::package:
		error("invalid conversion from 'package' to 'number'");
	case Data::Format::function:
		error("invalid conversion from 'function' to 'number'");
	case Data::Format::coroutine:
		error("invalid conversion from 'coroutine' to 'number'");
	}
	return 0;
}

std::intmax_t mint::to_signed_integer(Cursor& cursor, Reference&& ref) {
	switch (ref.data().format()) {
	case Data::Format::none:
		error("invalid conversion from 'none' to 'number'");
	case Data::Format::null:
		cursor.raise(std::move(ref));
		break;
	case Data::Format::number:
		return to_signed_integer(ref.data<Number>().value);
	case Data::Format::boolean:
		return ref.data<Boolean>().value;
	case Data::Format::object:
		switch (ref.data<Object>().metadata.metatype()) {
		case Class::Metatype::string:
			return to_signed_integer(ref.data<String>().str);
		case Class::Metatype::iterator:
		case Class::Metatype::async_iterator:
			try {
				if (std::optional<WeakReference>&& item = iterator_get(ref.data<Iterator>())) {
					return to_signed_integer(cursor, *item);
				}
				return to_signed_integer(cursor, create_none());
			}
			catch (std::exception&) {
				return 0;
			}
		default:
			if (auto* method = ref.data<Object>().metadata.find_member(builtin_symbols::to_number)) {
				auto* scheduler = Scheduler::instance();
				assert_x(scheduler, __func__, "execution should be done using a scheduler");
				return to_signed_integer(cursor, scheduler->invoke(ref, builtin_symbols::to_number, *method));
			}
			error("invalid conversion from '{}' to 'number'", type_name(ref));
		}
		break;
	case Data::Format::package:
		error("invalid conversion from 'package' to 'number'");
	case Data::Format::function:
		error("invalid conversion from 'function' to 'number'");
	case Data::Format::coroutine:
		error("invalid conversion from 'coroutine' to 'number'");
	}
	return 0;
}

std::uintmax_t mint::to_unsigned_integer(Cursor& cursor, const Reference& ref) {
	switch (ref.data().format()) {
	case Data::Format::none:
		error("invalid conversion from 'none' to 'number'");
	case Data::Format::null:
		cursor.raise(ref);
		break;
	case Data::Format::number:
		return to_unsigned_integer(ref.data<Number>().value);
	case Data::Format::boolean:
		return ref.data<Boolean>().value;
	case Data::Format::object:
		switch (ref.data<Object>().metadata.metatype()) {
		case Class::Metatype::string:
			try {
				return to_unsigned_integer(ref.data<String>().str);
			}
			catch (std::exception&) {
				return 0;
			}
		case Class::Metatype::iterator:
		case Class::Metatype::async_iterator:
			if (std::optional<WeakReference>&& item = iterator_get(ref.data<Iterator>())) {
				return to_unsigned_integer(cursor, *item);
			}
			return to_unsigned_integer(cursor, create_none());
		default:
			if (auto* method = ref.data<Object>().metadata.find_member(builtin_symbols::to_number)) {
				auto* scheduler = Scheduler::instance();
				assert_x(scheduler, __func__, "execution should be done using a scheduler");
				return to_unsigned_integer(cursor, scheduler->invoke(ref, builtin_symbols::to_number, *method));
			}
			error("invalid conversion from '{}' to 'number'", type_name(ref));
		}
		break;
	case Data::Format::package:
		error("invalid conversion from 'package' to 'number'");
	case Data::Format::function:
		error("invalid conversion from 'function' to 'number'");
	case Data::Format::coroutine:
		error("invalid conversion from 'coroutine' to 'number'");
	}
	return 0;
}

std::uintmax_t mint::to_unsigned_integer(Cursor& cursor, Reference&& ref) {
	switch (ref.data().format()) {
	case Data::Format::none:
		error("invalid conversion from 'none' to 'number'");
	case Data::Format::null:
		cursor.raise(std::move(ref));
		break;
	case Data::Format::number:
		return to_unsigned_integer(ref.data<Number>().value);
	case Data::Format::boolean:
		return ref.data<Boolean>().value;
	case Data::Format::object:
		switch (ref.data<Object>().metadata.metatype()) {
		case Class::Metatype::string:
			try {
				return to_unsigned_integer(ref.data<String>().str);
			}
			catch (std::exception&) {
				return 0;
			}
		case Class::Metatype::iterator:
		case Class::Metatype::async_iterator:
			if (std::optional<WeakReference>&& item = iterator_get(ref.data<Iterator>())) {
				return to_unsigned_integer(cursor, *item);
			}
			return to_unsigned_integer(cursor, create_none());
		default:
			if (auto* method = ref.data<Object>().metadata.find_member(builtin_symbols::to_number)) {
				auto* scheduler = Scheduler::instance();
				assert_x(scheduler, __func__, "execution should be done using a scheduler");
				return to_unsigned_integer(cursor, scheduler->invoke(ref, builtin_symbols::to_number, *method));
			}
			error("invalid conversion from '{}' to 'number'", type_name(ref));
		}
		break;
	case Data::Format::package:
		error("invalid conversion from 'package' to 'number'");
	case Data::Format::function:
		error("invalid conversion from 'function' to 'number'");
	case Data::Format::coroutine:
		error("invalid conversion from 'coroutine' to 'number'");
	}
	return 0;
}

bool mint::to_boolean(const Reference& ref) {
	switch (ref.data().format()) {
	case Data::Format::none:
	case Data::Format::null:
		return false;
	case Data::Format::number:
		return ref.data<Number>().value != 0.;
	case Data::Format::boolean:
		return ref.data<Boolean>().value;
	case Data::Format::object:
		switch (ref.data<Object>().metadata.metatype()) {
		case Class::Metatype::iterator:
		case Class::Metatype::async_iterator:
			return !ref.data<Iterator>().ctx.empty();
		default:
			if (auto* method = ref.data<Object>().metadata.find_member(builtin_symbols::to_boolean)) {
				auto* scheduler = Scheduler::instance();
				assert_x(scheduler, __func__, "execution should be done using a scheduler");
				return to_boolean(scheduler->invoke(ref, builtin_symbols::to_boolean, *method));
			}
			break;
		}
		break;
	default:
		break;
	}
	return true;
}

std::string mint::to_char(const Reference& ref) {
	switch (ref.data().format()) {
	case Data::Format::none:
	case Data::Format::null:
		return {};
	case Data::Format::number:
		return number_to_char(to_signed_integer(ref.data<Number>().value));
	case Data::Format::boolean:
		return ref.data<Boolean>().value ? "y" : "n";
	case Data::Format::object:
		if (ref.data<Object>().metadata.metatype() == Class::Metatype::string) {
			return *const_utf8iterator(ref.data<String>().str.begin());
		}
		if (auto* method = ref.data<Object>().metadata.find_member(builtin_symbols::to_string)) {
			auto* scheduler = Scheduler::instance();
			assert_x(scheduler, __func__, "execution should be done using a scheduler");
			return to_string(scheduler->invoke(ref, builtin_symbols::to_string, *method));
		}
		error("invalid conversion from '{}' to 'character'", type_name(ref));
	case Data::Format::package:
		error("invalid conversion from 'package' to 'character'");
	case Data::Format::function:
		error("invalid conversion from 'function' to 'character'");
	case Data::Format::coroutine:
		error("invalid conversion from 'coroutine' to 'character'");
	}
	return {};
}

std::string mint::to_string(const Reference& ref) {
	switch (ref.data().format()) {
	case Data::Format::none:
		return {};
	case Data::Format::null:
		return "(null)";
	case Data::Format::number:
		{
			double intpart = 0.;
			const auto fracpart = modf(ref.data<Number>().value, &intpart);
			if (fracpart != 0.) {
				return mint::to_string(intpart + fracpart);
			}
			return mint::to_string(to_signed_integer(intpart));
		}
	case Data::Format::boolean:
		return ref.data<Boolean>().value ? "true" : "false";
	case Data::Format::object:
		switch (ref.data<Object>().metadata.metatype()) {
		case Class::Metatype::string:
			return ref.data<String>().str;
		case Class::Metatype::regex:
			return ref.data<Regex>().initializer;
		case Class::Metatype::array:
			return std::format("[{}]", std::views::transform(ref.data<Array>().values,
			                               [](auto& item) {
				                               return to_string(item);
			                               })
			                               | std::views::join_with(std::string(", ")) | std::ranges::to<std::string>());
		case Class::Metatype::hash:
			return std::format("{{{}}}", std::views::transform(ref.data<Hash>().values,
			                                 [](auto& item) {
				                                 return to_string(item.first) + " : " + to_string(item.second);
			                                 })
			                                 | std::views::join_with(std::string(", "))
			                                 | std::ranges::to<std::string>());
		case Class::Metatype::iterator:
		case Class::Metatype::async_iterator:
			if (auto item = iterator_get(ref.data<Iterator>())) {
				return to_string(*item);
			}
			return to_string(create_none());
		case Class::Metatype::object:
			if (auto* method = ref.data<Object>().metadata.find_member(builtin_symbols::to_string)) {
				auto* scheduler = Scheduler::instance();
				assert_x(scheduler, __func__, "execution should be done using a scheduler");
				return to_string(scheduler->invoke(ref, builtin_symbols::to_string, *method));
			}
			return is_object(ref.data<Object>()) ? "(object)" : "(class)";
		case Class::Metatype::library:
			return "(library)";
		case Class::Metatype::libobject:
			return "(libobject)";
		}
	case Data::Format::package:
		return "(package)";
	case Data::Format::function:
		return "(function)";
	case Data::Format::coroutine:
		return "(coroutine)";
	}
	return {};
}

std::regex mint::to_regex(const Reference& ref) {
	switch (ref.data().format()) {
	case Data::Format::object:
		switch (ref.data<Object>().metadata.metatype()) {
		case Class::Metatype::regex:
			return ref.data<Regex>().expr;
		default:
			if (auto* method = ref.data<Object>().metadata.find_member(builtin_symbols::to_regex)) {
				auto* scheduler = Scheduler::instance();
				assert_x(scheduler, __func__, "execution should be done using a scheduler");
				return to_regex(scheduler->invoke(ref, builtin_symbols::to_regex, *method));
			}
			break;
		}
		[[fallthrough]];
	default:
		break;
	}

	try {
		return std::regex(to_string(ref));
	}
	catch (const std::regex_error&) {
		error("regular expression '/{}/' is not valid", to_string(ref));
	}
}

Array::values_type mint::to_array(const Reference& ref) {
	Array::values_type result;
	switch (ref.data().format()) {
	case Data::Format::none:
		return result;
	case Data::Format::object:
		switch (ref.data<Object>().metadata.metatype()) {
		case Class::Metatype::array:
			result.reserve(ref.data<Array>().values.size());
			std::ranges::transform(ref.data<Array>().values, std::back_inserter(result), [](auto& item) {
				return array_get_item(item);
			});
			return result;
		case Class::Metatype::hash:
			result.reserve(ref.data<Hash>().values.size());
			std::ranges::transform(ref.data<Hash>().values, std::back_inserter(result), [](const auto& item) {
				return hash_get_key(item);
			});
			return result;
		case Class::Metatype::iterator:
		case Class::Metatype::async_iterator:
			result.reserve(ref.data<Iterator>().ctx.size());
			std::ranges::transform(ref.data<Iterator>().ctx, std::back_inserter(result), [](const Reference& item) {
				return array_item(item);
			});
			return result;
		default:
			if (auto* method = ref.data<Object>().metadata.find_member(builtin_symbols::to_array)) {
				auto* scheduler = Scheduler::instance();
				assert_x(scheduler, __func__, "execution should be done using a scheduler");
				return to_array(scheduler->invoke(ref, builtin_symbols::to_array, *method));
			}
			break;
		}
		[[fallthrough]];
	default:
		result.emplace_back(array_item(ref));
	}
	return result;
}

Hash::values_type mint::to_hash(const Reference& ref) {
	Hash::values_type result;
	switch (ref.data().format()) {
	case Data::Format::none:
		return result;
	case Data::Format::object:
		switch (ref.data<Object>().metadata.metatype()) {
		case Class::Metatype::array:
			for (std::size_t i = 0; i < ref.data<Array>().values.size(); ++i) {
				constexpr auto flags = Reference::const_address | Reference::const_value | Reference::temporary;
				result.emplace(make_weak_reference<Number>(flags, i), array_get_item(ref.data<Array>().values.at(i)));
			}
			return result;
		case Class::Metatype::hash:
			for (auto& item : ref.data<Hash>().values) {
				result.emplace(hash_get_key(item), hash_get_value(item));
			}
			return result;
		case Class::Metatype::iterator:
		case Class::Metatype::async_iterator:
			for (const Reference& item : ref.data<Iterator>().ctx) {
				result.emplace(hash_key(item), WeakReference());
			}
			return result;
		default:
			if (auto* method = ref.data<Object>().metadata.find_member(builtin_symbols::to_hash)) {
				auto* scheduler = Scheduler::instance();
				assert_x(scheduler, __func__, "execution should be done using a scheduler");
				return to_hash(scheduler->invoke(ref, builtin_symbols::to_hash, *method));
			}
			break;
		}
		[[fallthrough]];
	default:
		result.emplace(hash_key(ref), WeakReference());
	}
	return result;
}

double mint::to_unsigned_number(std::string_view str) {

	double intpart = 0;

	if (str.starts_with('0') && str.length() > 1) {
		switch (str[1]) {
		case 'b':
		case 'B':
			for (const char ch : str.substr(2)) {
				switch (ch) {
				case '0':
					intpart = intpart * static_cast<double>(binary_base);
					break;
				case '1':
					intpart = (intpart * static_cast<double>(binary_base)) + 1;
					break;
				default:
					throw std::invalid_argument(__func__);
				}
			}
			return intpart;
		case 'o':
		case 'O':
			for (const char ch : str.substr(2)) {
				if ('0' <= ch && ch < '8') {
					intpart = (intpart * static_cast<double>(octal_base)) + (ch - '0');
				}
				else {
					throw std::invalid_argument(__func__);
				}
			}
			return intpart;

		case 'x':
		case 'X':
			for (const char ch : str.substr(2)) {
				if (ch >= 'A') {
					if (const int digit = ((ch - 'A') & (~('a' ^ 'A'))) + 10; digit < hexadecimal_base) {
						intpart = (intpart * static_cast<double>(hexadecimal_base)) + digit;
					}
					else {
						throw std::invalid_argument(__func__);
					}
				}
				else if (isdigit(ch)) {
					intpart = (intpart * static_cast<double>(hexadecimal_base)) + (ch - '0');
				}
				else {
					throw std::invalid_argument(__func__);
				}
			}
			return intpart;

		default:
			break;
		}
	}

	bool decimals = false;
	bool exponent = false;
	bool sign_expected = false;
	double fracpart = 0.;
	std::intmax_t fracexp = 0;
	std::intmax_t exppart = 0;
	std::intmax_t expsign = 0;

	for (const char ch : str) {
		switch (ch) {
		case '.':
			if (decimals || exponent) {
				throw std::invalid_argument(__func__);
			}
			decimals = true;
			break;
		case 'e':
		case 'E':
			if (exponent) {
				throw std::invalid_argument(__func__);
			}
			sign_expected = true;
			exponent = true;
			break;
		case '+':
			if (!sign_expected) {
				throw std::invalid_argument(__func__);
			}
			sign_expected = false;
			expsign = +1;
			break;
		case '-':
			if (!sign_expected) {
				throw std::invalid_argument(__func__);
			}
			sign_expected = false;
			expsign = -1;
			break;
		default:
			if (!isdigit(ch)) {
				throw std::invalid_argument(__func__);
			}
			if (exponent) {
				exppart = (exppart * decimal_base) + (ch - '0');
				sign_expected = false;
			}
			else if (decimals) {
				fracpart = (fracpart * static_cast<double>(decimal_base)) + (ch - '0');
				--fracexp;
			}
			else {
				intpart = (intpart * static_cast<double>(decimal_base)) + (ch - '0');
			}
		}
	}

	if (exponent) {
		return (fracpart * pow(static_cast<std::intmax_t>(decimal_base), fracexp) + intpart)
		       * pow(decimal_base, copysign(exppart, expsign));
	}

	if (decimals) {
		return (fracpart * pow(static_cast<std::intmax_t>(decimal_base), fracexp)) + intpart;
	}

	return intpart;
}

double mint::to_signed_number(std::string_view str) {
	return str.starts_with('-') ? -to_unsigned_number(str.substr(1)) : +to_unsigned_number(str);
}

std::uintmax_t mint::to_unsigned_integer(std::string_view str) {

	std::uintmax_t intpart = 0;

	if (str.starts_with('0') && str.length() > 1) {
		switch (str[1]) {
		case 'b':
		case 'B':
			for (const char ch : str.substr(2)) {
				switch (ch) {
				case '0':
					intpart = intpart << 1;
					break;
				case '1':
					intpart = (intpart << 1) + 1;
					break;
				default:
					throw std::invalid_argument(__func__);
				}
			}
			return intpart;
		case 'o':
		case 'O':
			for (const char ch : str.substr(2)) {
				if ('0' <= ch && ch < '8') {
					intpart = (intpart * octal_base) + (ch - '0');
				}
				else {
					throw std::invalid_argument(__func__);
				}
			}
			return intpart;

		case 'x':
		case 'X':
			for (const char ch : str.substr(2)) {
				if (ch >= 'A') {
					const int digit = ((ch - 'A') & (~('a' ^ 'A'))) + 10;
					if (digit < hexadecimal_base) {
						intpart = (intpart * hexadecimal_base) + digit;
					}
					else {
						throw std::invalid_argument(__func__);
					}
				}
				else if (isdigit(ch)) {
					intpart = (intpart * hexadecimal_base) + (ch - '0');
				}
				else {
					throw std::invalid_argument(__func__);
				}
			}
			return intpart;
		default:
			break;
		}
	}

	for (const char ch : str) {
		if ('0' <= ch && ch <= '9') {
			intpart = (intpart * decimal_base) + (ch - '0');
		}
		else {
			throw std::invalid_argument(__func__);
		}
	}

	return intpart;
}

std::intmax_t mint::to_signed_integer(std::string_view str) {
	return str.starts_with('-') ? -static_cast<std::intmax_t>(to_unsigned_integer(str.substr(1)))
	                            : +static_cast<std::intmax_t>(to_unsigned_integer(str));
}

double mint::to_signed_number(std::intmax_t value) {
	return static_cast<double>(value);
}

double mint::to_unsigned_number(std::uintmax_t value) {
	return static_cast<double>(value);
}

std::intmax_t mint::to_signed_integer(double value) {
	return static_cast<std::intmax_t>(value);
}

std::uintmax_t mint::to_unsigned_integer(double value) {
	return static_cast<std::uintmax_t>(value);
}
