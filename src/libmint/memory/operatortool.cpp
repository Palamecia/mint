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

#include "mint/memory/operatortool.h"
#include "mint/ast/abstractsyntaxtree.h"
#include "mint/ast/cursor.h"
#include "mint/ast/symbol.h"
#include "mint/memory/algorithm.hpp"
#include "mint/memory/builtin/array.h"
#include "mint/memory/builtin/hash.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/memory/builtin/regex.h"
#include "mint/memory/builtin/string.h"
#include "mint/memory/casttool.h"
#include "mint/memory/class.h"
#include "mint/memory/data.h"
#include "mint/memory/functiontool.h"
#include "mint/memory/globaldata.h"
#include "mint/memory/memorytool.h"
#include "mint/memory/object.h"
#include "mint/memory/reference.h"
#include "mint/system/error.h"
#include "mint/system/string.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <cmath>
#include <optional>
#include <string>
#include <utility>

using namespace mint;

bool mint::call_overload(Cursor& cursor, Class::Operator operator_overload, int signature) {

	assert(signature >= 0);

	const auto base = get_stack_base(cursor);
	auto& object = load_from_stack(cursor, base - static_cast<std::size_t>(signature)).data<Object>();

	if (Class::MemberInfo* info = object.metadata.find_operator(operator_overload)) {

		if (is_class(object)) [[unlikely]] {
			error("invalid use of class '{}' in an operation", object.metadata.full_name());
		}

		const auto& function = Class::MemberInfo::get(*info, object);
		Class& metadata = info->owner.get();

		switch (function.data().format()) {
		case Data::none_format:
			error("invalid use of none value as a function");
		case Data::null_format:
			cursor.raise(WeakReference(function));
			break;
		case Data::number_format:
		case Data::boolean_format:
		case Data::object_format:
			if (signature == 0) {
				cursor.stack().back() = WeakReference(copy_from, function);
			}
			else {
				error("{} copy doesn't take {} argument(s)", type_name(function), signature);
			}
			break;
		case Data::package_format:
			error("invalid use of package in an operation");
		case Data::function_format:
			if (!(function.flags() & Reference::global)) {
				// add self to function arguments
				signature += 1;
			}
			auto it = find_function_signature(cursor, function.data<Function>().mapping, signature);
			if (it == function.data<Function>().mapping.end()) [[unlikely]] {
				error("called member doesn't take {} parameter(s)", signature);
			}
			it->second.call(it->first, &metadata, cursor);
			break;
		}

		return true;
	}

	return false;
}

bool mint::call_overload(Cursor& cursor, const Symbol& operator_overload, int signature) {

	assert(signature >= 0);

	const auto base = get_stack_base(cursor);
	auto& object = load_from_stack(cursor, base - static_cast<std::size_t>(signature)).data<Object>();

	if (auto* info = object.metadata.find_member(operator_overload)) {

		if (is_class(object)) [[unlikely]] {
			error("invalid use of class '{}' in an operation", object.metadata.full_name());
		}

		const auto& function = Class::MemberInfo::get(*info, object);
		Class& metadata = info->owner.get();

		switch (function.data().format()) {
		case Data::none_format:
			error("invalid use of none value as a function");
		case Data::null_format:
			cursor.raise(WeakReference(function));
			break;
		case Data::number_format:
		case Data::boolean_format:
		case Data::object_format:
			if (signature == 0) {
				cursor.stack().back() = WeakReference(copy_from, function);
			}
			else {
				error("{} copy doesn't take {} argument(s)", type_name(function), signature);
			}
			break;
		case Data::package_format:
			error("invalid use of package in an operation");
		case Data::function_format:
			if (!(function.flags() & Reference::global)) {
				// add self to function arguments
				signature += 1;
			}
			auto it = find_function_signature(cursor, function.data<Function>().mapping, signature);
			if (it == function.data<Function>().mapping.end()) [[unlikely]] {
				error("called member doesn't take {} parameter(s)", signature);
			}
			it->second.call(it->first, &metadata, cursor);
			break;
		}

		return true;
	}

	return false;
}

void mint::move_operator(Cursor& cursor) {

	const auto base = get_stack_base(cursor);

	const auto& rvalue = load_from_stack(cursor, base);
	auto& lvalue = load_from_stack(cursor, base - 1);

	if ((lvalue.flags() & Reference::const_address) && (lvalue.data().format() != Data::none_format)) [[unlikely]] {
		error("invalid modification of constant reference");
	}

	if (lvalue.flags() & Reference::const_value) {
		lvalue.move_data(rvalue);
	}
	else if ((rvalue.flags() & (Reference::const_value | Reference::temporary)) == Reference::const_value) {
		lvalue.copy_data(rvalue);
	}
	else {
		lvalue.move_data(rvalue);
	}

	cursor.stack().pop_back();
}

void mint::copy_operator(Cursor& cursor) {

	const auto base = get_stack_base(cursor);

	const auto& rvalue = load_from_stack(cursor, base);
	const auto& lvalue = load_from_stack(cursor, base - 1);

	if (lvalue.flags() & Reference::const_value) [[unlikely]] {
		error("invalid modification of constant value");
	}

	switch (lvalue.data().format()) {
	case Data::none_format:
		error("invalid use of none value in an operation");
	case Data::null_format:
		cursor.raise(WeakReference(lvalue));
		break;
	case Data::number_format:
		lvalue.data<Number>().value = to_number(cursor, rvalue);
		cursor.stack().pop_back();
		break;
	case Data::boolean_format:
		lvalue.data<Boolean>().value = to_boolean(rvalue);
		cursor.stack().pop_back();
		break;
	case Data::function_format:
		if (rvalue.data().format() != Data::function_format) [[unlikely]] {
			error("invalid conversion from '{}' to '{}'", type_name(rvalue), type_name(lvalue));
		}
		lvalue.data<Function>().mapping = rvalue.data<Function>().mapping;
		cursor.stack().pop_back();
		break;
	case Data::object_format:
		if (!call_overload(cursor, Class::copy_operator, 1)) {
			if (rvalue.data().format() != Data::object_format) [[unlikely]] {
				error("cannot convert '{}' to '{}' in assignment", type_name(rvalue), type_name(lvalue));
			}
			if (&lvalue.data<Object>().metadata != &rvalue.data<Object>().metadata) [[unlikely]] {
				error("cannot convert '{}' to '{}' in assignment", type_name(rvalue), type_name(lvalue));
			}
			lvalue.data<Object>().destroy();
			lvalue.data<Object>().construct(rvalue.data<Object>());
		}
		break;
	case Data::package_format:
		error("invalid use of package in an operation");
	}
}

void mint::call_operator(Cursor& cursor, int signature) {

	Cursor::Call call = std::move(cursor.waiting_calls().top());
	cursor.waiting_calls().pop();

	const auto flags = call.get_flags();
	const auto& function = call.function();
	Class* metadata = call.get_metadata();
	signature += call.extra_argument_count();

	switch (function.data().format()) {
	case Data::none_format:
		if (flags & Cursor::Call::member_call) [[likely]] {
			if (signature) [[unlikely]] {
				error("default constructors doesn't take {} argument(s)", signature);
			}
		}
		else {
			error("invalid use of none value as a function");
		}
		break;
	case Data::null_format:
		cursor.raise(WeakReference(function));
		break;
	case Data::number_format:
	case Data::boolean_format:
	case Data::object_format:
		if (signature == 0) {
			cursor.stack().emplace_back(copy_from, function);
		}
		else {
			error("{} copy doesn't take {} argument(s)", type_name(function), signature);
		}
		break;
	case Data::package_format:
		error("invalid use of package in an operation");
	case Data::function_format:
		if (flags & Cursor::Call::member_call) {
			// add self to function arguments
			signature += 1;
		}
		auto it = find_function_signature(cursor, function.data<Function>().mapping, signature);
		if (it == function.data<Function>().mapping.end()) [[unlikely]] {
			error("called function doesn't take {} parameter(s)", signature);
		}
		it->second.call(it->first, metadata, cursor);
		break;
	}
}

void mint::call_member_operator(Cursor& cursor, int signature) {

	Cursor::Call call = std::move(cursor.waiting_calls().top());
	cursor.waiting_calls().pop();

	const auto flags = call.get_flags();
	const auto& function = call.function();
	Class* metadata = call.get_metadata();
	signature += call.extra_argument_count();

	switch (function.data().format()) {
	case Data::none_format:
		if (flags & Cursor::Call::member_call) [[likely]] {
			if (signature) [[unlikely]] {
				error("default constructors doesn't take {} argument(s)", signature);
			}
		}
		else {
			error("invalid use of none value as a function");
		}
		break;
	case Data::null_format:
		cursor.raise(WeakReference(function));
		break;
	case Data::number_format:
	case Data::boolean_format:
	case Data::object_format:
		if (signature == 0) {
			cursor.stack().back() = WeakReference(copy_from, function);
		}
		else {
			error("{} copy doesn't take {} argument(s)", type_name(function), signature);
		}
		break;
	case Data::package_format:
		error("invalid use of package in an operation");
	case Data::function_format:
		if (!(function.flags() & Reference::global)) {
			// add self to function arguments
			signature += 1;
		}
		auto it = find_function_signature(cursor, function.data<Function>().mapping, signature);
		if (it == function.data<Function>().mapping.end()) [[unlikely]] {
			error("called member doesn't take {} parameter(s)", signature);
		}
		it->second.call(it->first, metadata, cursor);
		break;
	}
}

void mint::add_operator(Cursor& cursor) {

	const auto base = get_stack_base(cursor);

	const auto& rvalue = load_from_stack(cursor, base);
	const auto& lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data().format()) {
	case Data::none_format:
		error("invalid use of none value in an operation");
	case Data::null_format:
		cursor.raise(WeakReference(lvalue));
		break;
	case Data::number_format:
		if (lvalue.flags() & Reference::temporary) {
			lvalue.data<Number>().value += to_number(cursor, rvalue);
			cursor.stack().pop_back();
		}
		else {
			Reference&& result = create_number(lvalue.data<Number>().value + to_number(cursor, rvalue));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::boolean_format:
		if (lvalue.flags() & Reference::temporary) {
			lvalue.data<Boolean>().value += to_boolean(rvalue);
			cursor.stack().pop_back();
		}
		else {
			Reference&& result = create_boolean(lvalue.data<Boolean>().value + to_boolean(rvalue));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::object_format:
		if (!call_overload(cursor, Class::add_operator, 1)) [[unlikely]] {
			error("class '{}' doesn't overload operator '+'(1)", type_name(lvalue));
		}
		break;
	case Data::package_format:
		error("invalid use of package in an operation");
	case Data::function_format:
		{
			if (rvalue.data().format() != Data::function_format) [[unlikely]] {
				error("invalid use of operator '+' with '{}' and '{}' types", type_name(lvalue), type_name(rvalue));
			}
			Reference&& result = create_function();
			for (const auto& item : lvalue.data<Function>().mapping) {
				result.data<Function>().mapping.insert(item);
			}
			for (const auto& item : rvalue.data<Function>().mapping) {
				result.data<Function>().mapping.insert(item);
			}
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	}
}

void mint::sub_operator(Cursor& cursor) {

	const auto base = get_stack_base(cursor);

	const auto& rvalue = load_from_stack(cursor, base);
	const auto& lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data().format()) {
	case Data::none_format:
		error("invalid use of none value in an operation");
	case Data::null_format:
		cursor.raise(WeakReference(lvalue));
		break;
	case Data::number_format:
		if (lvalue.flags() & Reference::temporary) {
			lvalue.data<Number>().value -= to_number(cursor, rvalue);
			cursor.stack().pop_back();
		}
		else {
			Reference&& result = create_number(lvalue.data<Number>().value - to_number(cursor, rvalue));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::boolean_format:
		if (lvalue.flags() & Reference::temporary) {
			lvalue.data<Boolean>().value -= to_boolean(rvalue);
			cursor.stack().pop_back();
		}
		else {
			Reference&& result = create_boolean(lvalue.data<Boolean>().value - to_boolean(rvalue));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::object_format:
		if (!call_overload(cursor, Class::sub_operator, 1)) [[unlikely]] {
			error("class '{}' doesn't overload operator '-'(1)", type_name(lvalue));
		}
		break;
	case Data::package_format:
		error("invalid use of package in an operation");
	case Data::function_format:
		error("invalid use of '{}' type with operator '-'", type_name(lvalue));
	}
}

void mint::mul_operator(Cursor& cursor) {

	const auto base = get_stack_base(cursor);

	const auto& rvalue = load_from_stack(cursor, base);
	const auto& lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data().format()) {
	case Data::none_format:
		error("invalid use of none value in an operation");
	case Data::null_format:
		cursor.raise(WeakReference(lvalue));
		break;
	case Data::number_format:
		if (lvalue.flags() & Reference::temporary) {
			lvalue.data<Number>().value *= to_number(cursor, rvalue);
			cursor.stack().pop_back();
		}
		else {
			Reference&& result = create_number(lvalue.data<Number>().value * to_number(cursor, rvalue));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::boolean_format:
		if (lvalue.flags() & Reference::temporary) {
			lvalue.data<Boolean>().value = lvalue.data<Boolean>().value && to_boolean(rvalue);
			cursor.stack().pop_back();
		}
		else {
			Reference&& result = create_boolean(lvalue.data<Boolean>().value && to_boolean(rvalue));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::object_format:
		if (!call_overload(cursor, Class::mul_operator, 1)) [[unlikely]] {
			error("class '{}' doesn't overload operator '*'(1)", type_name(lvalue));
		}
		break;
	case Data::package_format:
		error("invalid use of package in an operation");
	case Data::function_format:
		error("invalid use of '{}' type with operator '*'", type_name(lvalue));
	}
}

void mint::div_operator(Cursor& cursor) {

	const auto base = get_stack_base(cursor);

	const auto& rvalue = load_from_stack(cursor, base);
	const auto& lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data().format()) {
	case Data::none_format:
		error("invalid use of none value in an operation");
	case Data::null_format:
		cursor.raise(WeakReference(lvalue));
		break;
	case Data::number_format:
		if (lvalue.flags() & Reference::temporary) {
			lvalue.data<Number>().value /= to_number(cursor, rvalue);
			cursor.stack().pop_back();
		}
		else {
			Reference&& result = create_number(lvalue.data<Number>().value / to_number(cursor, rvalue));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::boolean_format:
		if (lvalue.flags() & Reference::temporary) {
			lvalue.data<Boolean>().value /= to_boolean(rvalue);
			cursor.stack().pop_back();
		}
		else {
			Reference&& result = create_boolean(lvalue.data<Boolean>().value / to_boolean(rvalue));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::object_format:
		if (!call_overload(cursor, Class::div_operator, 1)) [[unlikely]] {
			error("class '{}' doesn't overload operator '/'(1)", type_name(lvalue));
		}
		break;
	case Data::package_format:
		error("invalid use of package in an operation");
	case Data::function_format:
		error("invalid use of '{}' type with operator '/'", type_name(lvalue));
	}
}

void mint::pow_operator(Cursor& cursor) {

	const auto base = get_stack_base(cursor);

	const auto& rvalue = load_from_stack(cursor, base);
	const auto& lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data().format()) {
	case Data::none_format:
		error("invalid use of none value in an operation");
	case Data::null_format:
		cursor.raise(WeakReference(lvalue));
		break;
	case Data::number_format:
		if (lvalue.flags() & Reference::temporary) {
			lvalue.data<Number>().value = pow(lvalue.data<Number>().value, to_number(cursor, rvalue));
			cursor.stack().pop_back();
		}
		else {
			Reference&& result = create_number(pow(lvalue.data<Number>().value, to_number(cursor, rvalue)));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::object_format:
		if (!call_overload(cursor, Class::pow_operator, 1)) [[unlikely]] {
			error("class '{}' doesn't overload operator '**'(1)", type_name(lvalue));
		}
		break;
	case Data::package_format:
		error("invalid use of package in an operation");
	case Data::boolean_format:
	case Data::function_format:
		error("invalid use of '{}' type with operator '**'", type_name(lvalue));
	}
}

void mint::mod_operator(Cursor& cursor) {

	const auto base = get_stack_base(cursor);

	const auto& rvalue = load_from_stack(cursor, base);
	const auto& lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data().format()) {
	case Data::none_format:
		error("invalid use of none value in an operation");
	case Data::null_format:
		cursor.raise(WeakReference(lvalue));
		break;
	case Data::number_format:
		if (const auto divider = to_signed_integer(cursor, rvalue)) {
			if (lvalue.flags() & Reference::temporary) {
				lvalue.data<Number>().value = to_number(to_signed_integer(lvalue.data<Number>().value) % divider);
				cursor.stack().pop_back();
			}
			else {
				Reference&& result = create_signed_number(to_signed_integer(lvalue.data<Number>().value) % divider);
				cursor.stack().pop_back();
				cursor.stack().back() = std::move(result);
			}
		}
		else {
			error("modulo by zero");
		}
		break;
	case Data::object_format:
		if (!call_overload(cursor, Class::mod_operator, 1)) [[unlikely]] {
			error("class '{}' doesn't overload operator '%%'(1)", type_name(lvalue));
		}
		break;
	case Data::package_format:
		error("invalid use of package in an operation");
	case Data::boolean_format:
	case Data::function_format:
		error("invalid use of '{}' type with operator '%%'", type_name(lvalue));
	}
}

void mint::is_operator(Cursor& cursor) {

	const auto base = get_stack_base(cursor);

	const auto& rvalue = load_from_stack(cursor, base);
	const auto& lvalue = load_from_stack(cursor, base - 1);

	WeakReference result = create_boolean(&lvalue.data() == &rvalue.data());
	cursor.stack().pop_back();
	cursor.stack().back() = std::move(result);
}

void mint::eq_operator(Cursor& cursor) {

	const auto base = get_stack_base(cursor);

	const auto& rvalue = load_from_stack(cursor, base);
	auto& lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data().format()) {
	case Data::none_format:
		{
			Reference&& result = create_boolean(rvalue.data().format() == Data::none_format);
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::null_format:
		{
			Reference&& result = create_boolean(rvalue.data().format() == Data::null_format);
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::number_format:
		switch (rvalue.data().format()) {
		case Data::none_format:
		case Data::null_format:
			{
				Reference&& result = create_boolean(false);
				cursor.stack().pop_back();
				cursor.stack().back() = std::move(result);
			}
			break;
		default:
			Reference&& result = create_boolean(lvalue.data<Number>().value == to_number(cursor, rvalue));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::boolean_format:
		switch (rvalue.data().format()) {
		case Data::none_format:
		case Data::null_format:
			{
				Reference&& result = create_boolean(false);
				cursor.stack().pop_back();
				cursor.stack().back() = std::move(result);
			}
			break;
		default:
			Reference&& result = create_boolean(lvalue.data<Boolean>().value == to_boolean(rvalue));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::object_format:
		if (!call_overload(cursor, Class::eq_operator, 1)) {
			switch (rvalue.data().format()) {
			case Data::none_format:
			case Data::null_format:
				{
					Reference&& result = create_boolean(false);
					cursor.stack().pop_back();
					cursor.stack().back() = std::move(result);
				}
				break;
			default:
				error("class '{}' doesn't overload operator '=='(1)", type_name(lvalue));
			}
		}
		break;
	case Data::package_format:
		error("invalid use of package in an operation");
	case Data::function_format:
		if (rvalue.data().format() == Data::function_format) {
			Reference&& result = create_boolean(lvalue.data<Function>().mapping == rvalue.data<Function>().mapping);
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		else {
			error("invalid use of '{}' type with operator '=='", type_name(lvalue));
		}
	}
}

void mint::ne_operator(Cursor& cursor) {

	const auto base = get_stack_base(cursor);

	const auto& rvalue = load_from_stack(cursor, base);
	auto& lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data().format()) {
	case Data::none_format:
		{
			Reference&& result = create_boolean(rvalue.data().format() != Data::none_format);
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::null_format:
		{
			Reference&& result = create_boolean(rvalue.data().format() != Data::null_format);
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::number_format:
		switch (rvalue.data().format()) {
		case Data::none_format:
		case Data::null_format:
			{
				Reference&& result = create_boolean(true);
				cursor.stack().pop_back();
				cursor.stack().back() = std::move(result);
			}
			break;
		default:
			Reference&& result = create_boolean(lvalue.data<Number>().value != to_number(cursor, rvalue));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::boolean_format:
		switch (rvalue.data().format()) {
		case Data::none_format:
		case Data::null_format:
			{
				Reference&& result = create_boolean(true);
				cursor.stack().pop_back();
				cursor.stack().back() = std::move(result);
			}
			break;
		default:
			Reference&& result = create_boolean(lvalue.data<Boolean>().value != to_boolean(rvalue));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::object_format:
		if (!call_overload(cursor, Class::ne_operator, 1)) {
			switch (rvalue.data().format()) {
			case Data::none_format:
			case Data::null_format:
				{
					Reference&& result = create_boolean(true);
					cursor.stack().pop_back();
					cursor.stack().back() = std::move(result);
				}
				break;
			default:
				error("class '{}' doesn't overload operator '!='(1)", type_name(lvalue));
			}
		}
		break;
	case Data::package_format:
		error("invalid use of package in an operation");
	case Data::function_format:
		if (rvalue.data().format() == Data::function_format) {
			Reference&& result = create_boolean(lvalue.data<Function>().mapping != rvalue.data<Function>().mapping);
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		else {
			error("invalid use of '{}' type with operator '!='", type_name(lvalue));
		}
	}
}

void mint::lt_operator(Cursor& cursor) {

	const auto base = get_stack_base(cursor);

	const auto& rvalue = load_from_stack(cursor, base);
	const auto& lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data().format()) {
	case Data::none_format:
		error("invalid use of none value in an operation");
	case Data::null_format:
		cursor.raise(WeakReference(lvalue));
		break;
	case Data::number_format:
		{
			Reference&& result = create_boolean(lvalue.data<Number>().value < to_number(cursor, rvalue));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::boolean_format:
		{
			Reference&& result = create_boolean(lvalue.data<Boolean>().value < to_boolean(rvalue));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::object_format:
		if (!call_overload(cursor, Class::lt_operator, 1)) [[unlikely]] {
			error("class '{}' doesn't overload operator '<'(1)", type_name(lvalue));
		}
		break;
	case Data::package_format:
		error("invalid use of package in an operation");
	case Data::function_format:
		error("invalid use of '{}' type with operator '<'", type_name(lvalue));
	}
}

void mint::gt_operator(Cursor& cursor) {

	const auto base = get_stack_base(cursor);

	const auto& rvalue = load_from_stack(cursor, base);
	const auto& lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data().format()) {
	case Data::none_format:
		error("invalid use of none value in an operation");
	case Data::null_format:
		cursor.raise(WeakReference(lvalue));
		break;
	case Data::number_format:
		{
			Reference&& result = create_boolean(lvalue.data<Number>().value > to_number(cursor, rvalue));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::boolean_format:
		{
			Reference&& result = create_boolean(lvalue.data<Boolean>().value > to_boolean(rvalue));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::object_format:
		if (!call_overload(cursor, Class::gt_operator, 1)) [[unlikely]] {
			error("class '{}' doesn't overload operator '>'(1)", type_name(lvalue));
		}
		break;
	case Data::package_format:
		error("invalid use of package in an operation");
	case Data::function_format:
		error("invalid use of '{}' type with operator '>'", type_name(lvalue));
	}
}

void mint::le_operator(Cursor& cursor) {

	const auto base = get_stack_base(cursor);

	const auto& rvalue = load_from_stack(cursor, base);
	const auto& lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data().format()) {
	case Data::none_format:
		error("invalid use of none value in an operation");
	case Data::null_format:
		cursor.raise(WeakReference(lvalue));
		break;
	case Data::number_format:
		{
			Reference&& result = create_boolean(lvalue.data<Number>().value <= to_number(cursor, rvalue));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::boolean_format:
		{
			Reference&& result = create_boolean(lvalue.data<Boolean>().value <= to_boolean(rvalue));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::object_format:
		if (!call_overload(cursor, Class::le_operator, 1)) [[unlikely]] {
			error("class '{}' doesn't overload operator '<='(1)", type_name(lvalue));
		}
		break;
	case Data::package_format:
		error("invalid use of package in an operation");
	case Data::function_format:
		error("invalid use of '{}' type with operator '<='", type_name(lvalue));
	}
}

void mint::ge_operator(Cursor& cursor) {

	const auto base = get_stack_base(cursor);

	const auto& rvalue = load_from_stack(cursor, base);
	const auto& lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data().format()) {
	case Data::none_format:
		error("invalid use of none value in an operation");
	case Data::null_format:
		cursor.raise(WeakReference(lvalue));
		break;
	case Data::number_format:
		{
			Reference&& result = create_boolean(lvalue.data<Number>().value >= to_number(cursor, rvalue));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::boolean_format:
		{
			Reference&& result = create_boolean(lvalue.data<Boolean>().value >= to_boolean(rvalue));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::object_format:
		if (!call_overload(cursor, Class::ge_operator, 1)) [[unlikely]] {
			error("class '{}' doesn't overload operator '>='(1)", type_name(lvalue));
		}
		break;
	case Data::package_format:
		error("invalid use of package in an operation");
	case Data::function_format:
		error("invalid use of '{}' type with operator '>='", type_name(lvalue));
	}
}

void mint::and_pre_check(Cursor& cursor, std::size_t pos) {

	const auto& value = cursor.stack().back();

	switch (value.data().format()) {
	case Data::none_format:
	case Data::null_format:
		cursor.jmp(pos);
		break;
	case Data::number_format:
		if (value.data<Number>().value == 0.) {
			cursor.jmp(pos);
		}
		break;
	case Data::boolean_format:
		if (!value.data<Boolean>().value) {
			cursor.jmp(pos);
		}
		break;
	case Data::object_format:
		switch (value.data<Object>().metadata.metatype()) {
		case Class::iterator:
			if (value.data<Iterator>().ctx.empty()) {
				cursor.jmp(pos);
			}
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}

void mint::and_operator(Cursor& cursor) {

	const auto base = get_stack_base(cursor);

	auto& rvalue = load_from_stack(cursor, base);
	auto& lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data().format()) {
	case Data::object_format:
		if (!call_overload(cursor, Class::and_operator, 1)) {
			std::swap(lvalue, rvalue);
			cursor.stack().pop_back();
		}
		break;
	default:
		std::swap(lvalue, rvalue);
		cursor.stack().pop_back();
	}
}

void mint::or_pre_check(Cursor& cursor, std::size_t pos) {

	const auto& value = cursor.stack().back();

	switch (value.data().format()) {
	case Data::none_format:
	case Data::null_format:
		break;
	case Data::number_format:
		if (value.data<Number>().value != 0.) {
			cursor.jmp(pos);
		}
		break;
	case Data::boolean_format:
		if (value.data<Boolean>().value) {
			cursor.jmp(pos);
		}
		break;
	case Data::object_format:
		switch (value.data<Object>().metadata.metatype()) {
		case Class::iterator:
			if (!value.data<Iterator>().ctx.empty()) {
				cursor.jmp(pos);
			}
			break;
		default:
			if (!value.data<Object>().metadata.find_operator(Class::or_operator)) {
				cursor.jmp(pos);
			}
			break;
		}
		break;
	default:
		cursor.jmp(pos);
		break;
	}
}

void mint::or_operator(Cursor& cursor) {

	const auto base = get_stack_base(cursor);

	auto& rvalue = load_from_stack(cursor, base);
	auto& lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data().format()) {
	case Data::object_format:
		if (!call_overload(cursor, Class::or_operator, 1)) {
			std::swap(lvalue, rvalue);
			cursor.stack().pop_back();
		}
		break;
	default:
		std::swap(lvalue, rvalue);
		cursor.stack().pop_back();
	}
}

void mint::band_operator(Cursor& cursor) {

	const auto base = get_stack_base(cursor);

	const auto& rvalue = load_from_stack(cursor, base);
	const auto& lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data().format()) {
	case Data::none_format:
		error("invalid use of none value in an operation");
	case Data::null_format:
		cursor.raise(WeakReference(lvalue));
		break;
	case Data::number_format:
		if (lvalue.flags() & Reference::temporary) {
			lvalue.data<Number>().value = to_number(
			    to_unsigned_integer(lvalue.data<Number>().value) & to_unsigned_integer(cursor, rvalue));
			cursor.stack().pop_back();
		}
		else {
			Reference&& result = create_unsigned_number(
			    to_unsigned_integer(lvalue.data<Number>().value) & to_unsigned_integer(cursor, rvalue));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::boolean_format:
		if (lvalue.flags() & Reference::temporary) {
			lvalue.data<Boolean>().value &= to_boolean(rvalue);
			cursor.stack().pop_back();
		}
		else {
			Reference&& result = create_boolean(lvalue.data<Boolean>().value && to_boolean(rvalue));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::object_format:
		if (!call_overload(cursor, Class::band_operator, 1)) [[unlikely]] {
			error("class '{}' doesn't overload operator '&'(1)", type_name(lvalue));
		}
		break;
	case Data::package_format:
		error("invalid use of package in an operation");
	case Data::function_format:
		error("invalid use of '{}' type with operator '&'", type_name(lvalue));
	}
}

void mint::bor_operator(Cursor& cursor) {

	const auto base = get_stack_base(cursor);

	const auto& rvalue = load_from_stack(cursor, base);
	const auto& lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data().format()) {
	case Data::none_format:
		error("invalid use of none value in an operation");
	case Data::null_format:
		cursor.raise(WeakReference(lvalue));
		break;
	case Data::number_format:
		if (lvalue.flags() & Reference::temporary) {
			lvalue.data<Number>().value = to_number(
			    to_unsigned_integer(lvalue.data<Number>().value) | to_unsigned_integer(cursor, rvalue));
			cursor.stack().pop_back();
		}
		else {
			Reference&& result = create_unsigned_number(
			    to_unsigned_integer(lvalue.data<Number>().value) | to_unsigned_integer(cursor, rvalue));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::boolean_format:
		if (lvalue.flags() & Reference::temporary) {
			lvalue.data<Boolean>().value |= to_boolean(rvalue);
			cursor.stack().pop_back();
		}
		else {
			Reference&& result = create_boolean(lvalue.data<Boolean>().value || to_boolean(rvalue));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::object_format:
		if (!call_overload(cursor, Class::bor_operator, 1)) [[unlikely]] {
			error("class '{}' doesn't overload operator '|'(1)", type_name(lvalue));
		}
		break;
	case Data::package_format:
		error("invalid use of package in an operation");
	case Data::function_format:
		error("invalid use of '{}' type with operator '|'", type_name(lvalue));
	}
}

void mint::xor_operator(Cursor& cursor) {

	const auto base = get_stack_base(cursor);

	const auto& rvalue = load_from_stack(cursor, base);
	const auto& lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data().format()) {
	case Data::none_format:
		error("invalid use of none value in an operation");
	case Data::null_format:
		cursor.raise(WeakReference(lvalue));
		break;
	case Data::number_format:
		if (lvalue.flags() & Reference::temporary) {
			lvalue.data<Number>().value = to_number(
			    to_unsigned_integer(lvalue.data<Number>().value) ^ to_unsigned_integer(cursor, rvalue));
			cursor.stack().pop_back();
		}
		else {
			Reference&& result = create_unsigned_number(
			    to_unsigned_integer(lvalue.data<Number>().value) ^ to_unsigned_integer(cursor, rvalue));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::boolean_format:
		if (lvalue.flags() & Reference::temporary) {
			lvalue.data<Boolean>().value ^= to_boolean(rvalue);
			cursor.stack().pop_back();
		}
		else {
			Reference&& result = create_boolean(to_unsigned_integer(lvalue.data<Number>().value) ^ to_boolean(rvalue));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::object_format:
		if (!call_overload(cursor, Class::xor_operator, 1)) [[unlikely]] {
			error("class '{}' doesn't overload operator '^'(1)", type_name(lvalue));
		}
		break;
	case Data::package_format:
		error("invalid use of package in an operation");
	case Data::function_format:
		error("invalid use of '{}' type with operator '^'", type_name(lvalue));
	}
}

void mint::inc_operator(Cursor& cursor) {

	auto& value = cursor.stack().back();

	if (value.flags() & Reference::const_value) [[unlikely]] {
		error("invalid modification of constant value");
	}

	switch (value.data().format()) {
	case Data::none_format:
		error("invalid use of none value in an operation");
	case Data::null_format:
		cursor.raise(WeakReference(value));
		break;
	case Data::number_format:
		value.move_data(create_number(value.data<Number>().value + 1));
		break;
	case Data::boolean_format:
		value.move_data(create_boolean(value.data<Boolean>().value + 1));
		break;
	case Data::object_format:
		if (!call_overload(cursor, Class::inc_operator, 0)) [[unlikely]] {
			error("class '{}' doesn't overload operator '++'(0)", type_name(value));
		}
		break;
	case Data::package_format:
		error("invalid use of package in an operation");
	case Data::function_format:
		error("invalid use of '{}' type with operator '++'", type_name(value));
	}
}

void mint::dec_operator(Cursor& cursor) {

	auto& value = cursor.stack().back();

	if (value.flags() & Reference::const_value) [[unlikely]] {
		error("invalid modification of constant value");
	}

	switch (value.data().format()) {
	case Data::none_format:
		error("invalid use of none value in an operation");
	case Data::null_format:
		cursor.raise(WeakReference(value));
		break;
	case Data::number_format:
		value.move_data(create_number(value.data<Number>().value - 1));
		break;
	case Data::boolean_format:
		value.move_data(create_boolean(value.data<Boolean>().value - 1));
		break;
	case Data::object_format:
		if (!call_overload(cursor, Class::dec_operator, 0)) [[unlikely]] {
			error("class '{}' doesn't overload operator '--'(0)", type_name(value));
		}
		break;
	case Data::package_format:
		error("invalid use of package in an operation");
	case Data::function_format:
		std::string type = type_name(value);
		error("invalid use of '{}' type with operator '--'", type);
	}
}

void mint::not_operator(Cursor& cursor) {

	const auto& value = cursor.stack().back();

	switch (value.data().format()) {
	case Data::none_format:
	case Data::null_format:
		cursor.stack().back() = create_boolean(true);
		break;
	case Data::number_format:
		cursor.stack().back() = create_boolean(value.data<Number>().value == 0.);
		break;
	case Data::boolean_format:
		cursor.stack().back() = create_boolean(!value.data<Boolean>().value);
		break;
	case Data::object_format:
		if (!call_overload(cursor, Class::not_operator, 0)) {
			cursor.stack().back() = create_boolean(!to_boolean(value));
		}
		break;
	case Data::package_format:
		error("invalid use of package in an operation");
	case Data::function_format:
		error("invalid use of '{}' type with operator '!'", type_name(value));
	}
}

void mint::compl_operator(Cursor& cursor) {

	const auto& value = cursor.stack().back();

	switch (value.data().format()) {
	case Data::none_format:
		error("invalid use of none value in an operation");
	case Data::null_format:
		cursor.raise(WeakReference(value));
		break;
	case Data::number_format:
		cursor.stack().back() = create_signed_number(~to_signed_integer(cursor, value));
		break;
	case Data::boolean_format:
		cursor.stack().back() = create_boolean(!value.data<Boolean>().value);
		break;
	case Data::object_format:
		if (!call_overload(cursor, Class::compl_operator, 0)) [[unlikely]] {
			error("class '{}' doesn't overload operator '~'(0)", type_name(value));
		}
		break;
	case Data::package_format:
		error("invalid use of package in an operation");
	case Data::function_format:
		error("invalid use of '{}' type with operator '~'", type_name(value));
	}
}

void mint::pos_operator(Cursor& cursor) {

	const auto& value = cursor.stack().back();

	switch (value.data().format()) {
	case Data::none_format:
		error("invalid use of none value in an operation");
	case Data::null_format:
		cursor.raise(WeakReference(value));
		break;
	case Data::number_format:
		if (value.flags() & Reference::temporary) {
			value.data<Number>().value = +(value.data<Number>().value);
		}
		else {
			cursor.stack().back() = create_number(+(value.data<Number>().value));
		}
		break;
	case Data::boolean_format:
		if (value.flags() & Reference::temporary) {
			value.data<Boolean>().value = +(value.data<Boolean>().value);
		}
		else {
			cursor.stack().back() = create_boolean(+(value.data<Boolean>().value));
		}
		break;
	case Data::object_format:
		if (!call_overload(cursor, Class::add_operator, 0)) [[unlikely]] {
			error("class '{}' doesn't overload operator '+'(0)", type_name(value));
		}
		break;
	case Data::package_format:
		error("invalid use of package in an operation");
	case Data::function_format:
		error("invalid use of '{}' type with operator '+'", type_name(value));
	}
}

void mint::neg_operator(Cursor& cursor) {

	const auto& value = cursor.stack().back();

	switch (value.data().format()) {
	case Data::none_format:
		error("invalid use of none value in an operation");
	case Data::null_format:
		cursor.raise(WeakReference(value));
		break;
	case Data::number_format:
		if (value.flags() & Reference::temporary) {
			value.data<Number>().value = -(value.data<Number>().value);
		}
		else {
			cursor.stack().back() = create_number(-(value.data<Number>().value));
		}
		break;
	case Data::boolean_format:
		if (value.flags() & Reference::temporary) {
			value.data<Boolean>().value = -(value.data<Boolean>().value);
		}
		else {
			cursor.stack().back() = create_boolean(-(value.data<Boolean>().value));
		}
		break;
	case Data::object_format:
		if (!call_overload(cursor, Class::sub_operator, 0)) [[unlikely]] {
			error("class '{}' doesn't overload operator '-'(0)", type_name(value));
		}
		break;
	case Data::package_format:
		error("invalid use of package in an operation");
	case Data::function_format:
		error("invalid use of '{}' type with operator '-'", type_name(value));
	}
}

void mint::shift_left_operator(Cursor& cursor) {

	const auto base = get_stack_base(cursor);

	const auto& rvalue = load_from_stack(cursor, base);
	const auto& lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data().format()) {
	case Data::none_format:
		error("invalid use of none value in an operation");
	case Data::null_format:
		cursor.raise(WeakReference(lvalue));
		break;
	case Data::number_format:
		{
			Reference&& result = create_unsigned_number(
			    to_unsigned_integer(lvalue.data<Number>().value) << to_unsigned_integer(cursor, rvalue));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::boolean_format:
		{
			Reference&& result = create_unsigned_number(
			    to_unsigned_integer(lvalue.data<Boolean>().value) << to_unsigned_integer(cursor, rvalue));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::object_format:
		if (!call_overload(cursor, Class::shift_left_operator, 1)) [[unlikely]] {
			error("class '{}' doesn't overload operator '<<'(1)", type_name(lvalue));
		}
		break;
	case Data::package_format:
		error("invalid use of package in an operation");
	case Data::function_format:
		error("invalid use of '{}' type with operator '<<'", type_name(lvalue));
	}
}

void mint::shift_right_operator(Cursor& cursor) {

	const auto base = get_stack_base(cursor);

	const auto& rvalue = load_from_stack(cursor, base);
	const auto& lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data().format()) {
	case Data::none_format:
		error("invalid use of none value in an operation");
	case Data::null_format:
		cursor.raise(WeakReference(lvalue));
		break;
	case Data::number_format:
		{
			Reference&& result = create_unsigned_number(
			    to_unsigned_integer(lvalue.data<Number>().value) >> to_unsigned_integer(cursor, rvalue));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::boolean_format:
		{
			Reference&& result = create_boolean(lvalue.data<Boolean>().value >> to_unsigned_integer(cursor, rvalue));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::object_format:
		if (!call_overload(cursor, Class::shift_right_operator, 1)) [[unlikely]] {
			error("class '{}' doesn't overload operator '>>'(1)", type_name(lvalue));
		}
		break;
	case Data::package_format:
		error("invalid use of package in an operation");
	case Data::function_format:
		error("invalid use of '{}' type with operator '>>'", type_name(lvalue));
	}
}

void mint::inclusive_range_operator(Cursor& cursor) {

	const auto base = get_stack_base(cursor);

	const auto& rvalue = load_from_stack(cursor, base);
	const auto& lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data().format()) {
	case Data::none_format:
		error("invalid use of none value in an operation");
	case Data::null_format:
		cursor.raise(WeakReference(lvalue));
		break;
	case Data::number_format:
		{
			auto result = create_iterator(from_inclusive_range, cursor.ast(), lvalue.data<Number>().value,
			    to_number(cursor, rvalue));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::object_format:
		if (!call_overload(cursor, Class::inclusive_range_operator, 1)) [[unlikely]] {
			error("class '{}' doesn't overload operator '..'(1)", type_name(lvalue));
		}
		break;
	case Data::package_format:
		error("invalid use of package in an operation");
	case Data::boolean_format:
	case Data::function_format:
		error("invalid use of '{}' type with operator '..'", type_name(lvalue));
	}
}

void mint::exclusive_range_operator(Cursor& cursor) {

	const auto base = get_stack_base(cursor);

	const auto& rvalue = load_from_stack(cursor, base);
	const auto& lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data().format()) {
	case Data::none_format:
		error("invalid use of none value in an operation");
	case Data::null_format:
		cursor.raise(WeakReference(lvalue));
		break;
	case Data::number_format:
		{
			auto result = create_iterator(from_exclusive_range, cursor.ast(), lvalue.data<Number>().value,
			    to_number(cursor, rvalue));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::object_format:
		if (!call_overload(cursor, Class::exclusive_range_operator, 1)) [[unlikely]] {
			error("class '{}' doesn't overload operator '...'(1)", type_name(lvalue));
		}
		break;
	case Data::package_format:
		error("invalid use of package in an operation");
	case Data::boolean_format:
	case Data::function_format:
		error("invalid use of '{}' type with operator '...'", type_name(lvalue));
	}
}

void mint::typeof_operator(Cursor& cursor) {
	cursor.stack().back() = create_string(cursor.ast(), type_name(std::forward<Reference>(cursor.stack().back())));
}

void mint::membersof_operator(Cursor& cursor) {

	auto& value = cursor.stack().back();
	WeakReference result = create_array(cursor.ast());

	switch (value.data().format()) {
	case Data::object_format:
		{
			auto& object = value.data<Object>();
			auto& array = result.data<Array>();
			array.values.reserve(object.metadata.members().size());
			for (const auto& [symbol, info] : object.metadata.members()) {

				switch (info.get().value.flags() & Reference::visibility_mask) {
				case Reference::protected_visibility:
					if (!is_protected_accessible(info.get().owner, cursor.symbols().get_metadata())) {
						continue;
					}
					break;
				case Reference::private_visibility:
					if (&info.get().owner.get() != cursor.symbols().get_metadata()) {
						continue;
					}
					break;
				case Reference::package_visibility:
					if (&info.get().owner.get().get_package() != &cursor.symbols().get_package()) {
						continue;
					}
					break;
				default:
					break;
				}
				array_append(array, create_string(cursor.ast(), symbol.str()));
			}
		}
		break;

	case Data::package_format:
		{
			auto& package = value.data<Package>();
			auto& array = result.data<Array>();
			array.values.reserve(package.data.symbols().size());
			for (const auto& [symbol, _] : package.data.symbols()) {
				array_append(array, create_string(cursor.ast(), symbol.str()));
			}
		}
		break;

	default:
		break;
	}

	cursor.stack().back() = std::move(result);
}

void mint::subscript_operator(Cursor& cursor) {

	const auto base = get_stack_base(cursor);

	const auto& rvalue = load_from_stack(cursor, base);
	const auto& lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data().format()) {
	case Data::none_format:
		error("invalid use of none value in an operation");
	case Data::null_format:
		cursor.raise(WeakReference(lvalue));
		break;
	case Data::number_format:
		if (lvalue.flags() & Reference::temporary) {
			lvalue.data<Number>().value = to_number(
			    to_unsigned_integer(lvalue.data<Number>().value / pow(decimal_base, to_number(cursor, rvalue)))
			    % decimal_base);
			cursor.stack().pop_back();
		}
		else {
			WeakReference result = create_unsigned_number(
			    to_unsigned_integer(lvalue.data<Number>().value / pow(decimal_base, to_number(cursor, rvalue)))
			    % decimal_base);
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::boolean_format:
		error("invalid use of '{}' type with operator '[]'", type_name(lvalue));
	case Data::object_format:
		if (!call_overload(cursor, Class::subscript_operator, 1)) [[unlikely]] {
			error("class '{}' doesn't overload operator '[]'(1)", type_name(lvalue));
		}
		break;
	case Data::package_format:
		error("invalid use of package in an operation");
	case Data::function_format:
		auto signature = lvalue.data<Function>().mapping.find(to_integer<int>(cursor, rvalue));
		if (signature != lvalue.data<Function>().mapping.end()) {
			auto result = create_function(*signature);
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		else {
			cursor.stack().pop_back();
			cursor.stack().back() = create_none();
		}
		break;
	}
}

void mint::subscript_move_operator(Cursor& cursor) {

	const auto base = get_stack_base(cursor);

	const auto& rvalue = load_from_stack(cursor, base);
	const auto& kvalue = load_from_stack(cursor, base - 1);
	const auto& lvalue = load_from_stack(cursor, base - 2);

	if (lvalue.flags() & Reference::const_value) [[unlikely]] {
		error("invalid modification of constant value");
	}

	switch (lvalue.data().format()) {
	case Data::none_format:
		error("invalid use of none value in an operation");
	case Data::null_format:
		cursor.raise(WeakReference(lvalue));
		break;
	case Data::number_format:
		lvalue.data<Number>().value -= (to_number(to_unsigned_integer(lvalue.data<Number>().value
		                                                              / pow(decimal_base, to_number(cursor, kvalue)))
		                                          % decimal_base)
		                                * pow(decimal_base, to_number(cursor, kvalue)));
		lvalue.data<Number>().value += to_number(cursor, rvalue) * pow(decimal_base, to_number(cursor, kvalue));
		cursor.stack().pop_back();
		cursor.stack().pop_back();
		break;
	case Data::boolean_format:
		error("invalid use of '{}' type with operator '[]='", type_name(lvalue));
	case Data::object_format:
		if (!call_overload(cursor, Class::subscript_move_operator, 2)) [[unlikely]] {
			error("class '{}' doesn't overload operator '[]='(2)", type_name(lvalue));
		}
		break;
	case Data::package_format:
		error("invalid use of package in an operation");
	case Data::function_format:
		error("invalid use of '{}' type with operator '[]='", type_name(lvalue));
	}
}

void mint::regex_match(Cursor& cursor) {

	const auto base = get_stack_base(cursor);
	const auto& lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data().format()) {
	case Data::none_format:
		error("invalid use of none value in an operation");
	case Data::null_format:
		cursor.raise(WeakReference(lvalue));
		break;
	case Data::object_format:
		if (!call_overload(cursor, Class::regex_match_operator, 1)) [[unlikely]] {
			error("class '{}' doesn't overload operator '=~'(1)", type_name(lvalue));
		}
		break;
	case Data::package_format:
		error("invalid use of package in an operation");
	case Data::number_format:
	case Data::boolean_format:
	case Data::function_format:
		error("invalid use of '{}' type with operator '=~'", type_name(lvalue));
	}
}

void mint::regex_unmatch(Cursor& cursor) {

	const auto base = get_stack_base(cursor);
	const auto& lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data().format()) {
	case Data::none_format:
		error("invalid use of none value in an operation");
	case Data::null_format:
		cursor.raise(WeakReference(lvalue));
		break;
	case Data::object_format:
		if (!call_overload(cursor, Class::regex_unmatch_operator, 1)) [[unlikely]] {
			error("class '{}' doesn't overload operator '!~'(1)", type_name(lvalue));
		}
		break;
	case Data::package_format:
		error("invalid use of package in an operation");
	case Data::number_format:
	case Data::boolean_format:
	case Data::function_format:
		error("invalid use of '{}' type with operator '!~'", type_name(lvalue));
	}
}

void mint::strict_eq_operator(Cursor& cursor) {

	const auto base = get_stack_base(cursor);

	const auto& rvalue = load_from_stack(cursor, base);
	auto& lvalue = load_from_stack(cursor, base - 1);

	if (lvalue.data().format() == rvalue.data().format()) {
		switch (lvalue.data().format()) {
		case Data::none_format:
		case Data::null_format:
			{
				cursor.stack().pop_back();
				cursor.stack().back() = create_boolean(true);
			}
			break;
		case Data::number_format:
			{
				Reference&& result = create_boolean(lvalue.data<Number>().value == rvalue.data<Number>().value);
				cursor.stack().pop_back();
				cursor.stack().back() = std::move(result);
			}
			break;
		case Data::boolean_format:
			{
				Reference&& result = create_boolean(lvalue.data<Boolean>().value == rvalue.data<Boolean>().value);
				cursor.stack().pop_back();
				cursor.stack().back() = std::move(result);
			}
			break;
		case Data::object_format:
			if (!call_overload(cursor, Class::eq_operator, 1)) {
				error("class '{}' doesn't overload operator '=='(1)", type_name(lvalue));
			}
			break;
		case Data::package_format:
			error("invalid use of package in an operation");
		case Data::function_format:
			{
				Reference&& result = create_boolean(lvalue.data<Function>().mapping == rvalue.data<Function>().mapping);
				cursor.stack().pop_back();
				cursor.stack().back() = std::move(result);
			}
		}
	}
	else {
		cursor.stack().pop_back();
		cursor.stack().back() = create_boolean(false);
	}
}

void mint::strict_ne_operator(Cursor& cursor) {

	const auto base = get_stack_base(cursor);

	const auto& rvalue = load_from_stack(cursor, base);
	auto& lvalue = load_from_stack(cursor, base - 1);

	if (lvalue.data().format() == rvalue.data().format()) {
		switch (lvalue.data().format()) {
		case Data::none_format:
		case Data::null_format:
			{
				cursor.stack().pop_back();
				cursor.stack().back() = create_boolean(false);
			}
			break;
		case Data::number_format:
			{
				Reference&& result = create_boolean(lvalue.data<Number>().value != rvalue.data<Number>().value);
				cursor.stack().pop_back();
				cursor.stack().back() = std::move(result);
			}
			break;
		case Data::boolean_format:
			{
				Reference&& result = create_boolean(lvalue.data<Boolean>().value != rvalue.data<Boolean>().value);
				cursor.stack().pop_back();
				cursor.stack().back() = std::move(result);
			}
			break;
		case Data::object_format:
			if (!call_overload(cursor, Class::ne_operator, 1)) {
				error("class '{}' doesn't overload operator '!='(1)", type_name(lvalue));
			}
			break;
		case Data::package_format:
			error("invalid use of package in an operation");
		case Data::function_format:
			{
				Reference&& result = create_boolean(lvalue.data<Function>().mapping != rvalue.data<Function>().mapping);
				cursor.stack().pop_back();
				cursor.stack().back() = std::move(result);
			}
		}
	}
	else {
		cursor.stack().pop_back();
		cursor.stack().back() = create_boolean(true);
	}
}

void mint::find_defined_symbol(Cursor& cursor, const Symbol& symbol) {

	if (auto it = cursor.symbols().find(symbol); it != cursor.symbols().end()) {
		cursor.stack().emplace_back(it->second);
		return;
	}

	GlobalData& global_data = cursor.ast().global_data();
	if (auto it = global_data.symbols().find(symbol); it != global_data.symbols().end()) {
		cursor.stack().emplace_back(it->second);
		return;
	}

	cursor.stack().emplace_back(create_none());
}

void mint::find_defined_member(Cursor& cursor, const Symbol& symbol) {

	if (cursor.stack().back().data().format() != Data::none_format) {

		const auto value = std::move(cursor.stack().back());
		cursor.stack().pop_back();

		switch (value.data().format()) {
		case Data::package_format:
			{
				auto& package = value.data<Package>();
				if (auto it = package.data.symbols().find(symbol); it != package.data.symbols().end()) {
					cursor.stack().emplace_back(it->second);
					return;
				}
			}

			cursor.stack().emplace_back(create_none());
			break;

		case Data::object_format:
			{
				auto& object = value.data<Object>();
				if (auto* info = object.metadata.find_member(symbol)) {
					cursor.stack().emplace_back(Class::MemberInfo::get(*info, object));
					return;
				}

				if (auto* info = object.metadata.find_global(symbol)) {
					cursor.stack().emplace_back(info->value);
					return;
				}
			}

			cursor.stack().emplace_back(create_none());
			break;

		default:
			cursor.stack().emplace_back(create_none());
			break;
		}
	}
}

void mint::check_defined(Cursor& cursor) {
	const auto value = std::move(cursor.stack().back());
	cursor.stack().back() = create_boolean(value.data().format() != Data::none_format);
}

void mint::find_operator(Cursor& cursor) {

	const auto base = get_stack_base(cursor);

	const auto& range = load_from_stack(cursor, base);
	auto& value = load_from_stack(cursor, base - 1);

	switch (range.data().format()) {
	case Data::object_format:
		cursor.stack().emplace_back(value);
		if (!call_overload(cursor, Class::in_operator, 1)) {
			cursor.stack().pop_back();
			cursor.stack().back() = create_iterator_over(cursor.ast(), range);
		}
		break;

	default:
		cursor.stack().back() = create_iterator_over(cursor.ast(), range);
		break;
	}
}

void mint::find_init(Cursor& cursor) {

	const auto& range = cursor.stack().back();

	if (range.data().format() != Data::boolean_format) {
		cursor.stack().back() = create_iterator_over(cursor.ast(), range);
	}
}

void mint::find_next(Cursor& cursor) {

	const auto base = get_stack_base(cursor);

	const auto& range = load_from_stack(cursor, base);
	auto& value = load_from_stack(cursor, base - 1);

	if (range.data().format() == Data::boolean_format) {
		cursor.stack().emplace_back(range);
	}
	else {
		assert(is_instance_of(range, Class::iterator));
		auto& iterator = range.data<Iterator>();
		if (std::optional<WeakReference>&& item = iterator_next(iterator)) {
			cursor.stack().emplace_back(value);
			cursor.stack().emplace_back(*item);
			eq_operator(cursor);
		}
		else {
			cursor.stack().emplace_back(create_boolean(false));
		}
	}
}

void mint::find_check(Cursor& cursor, std::size_t pos) {

	const auto base = get_stack_base(cursor);

	auto found = move_from_stack(cursor, base);
	const auto& range = load_from_stack(cursor, base - 1);

	if (range.data().format() == Data::boolean_format || to_boolean(found) || range.data<Iterator>().ctx.empty()) {
		cursor.stack().pop_back();
		cursor.stack().pop_back();
		cursor.stack().back() = std::move(found);
		cursor.jmp(pos);
	}
	else {
		cursor.stack().pop_back();
	}
}

void mint::in_operator(Cursor& cursor) {
	const auto& range = cursor.stack().back();
	if (is_instance_of(range, Data::object_format)) {
		call_overload(cursor, Class::in_operator, 0);
	}
}

void mint::range_init(Cursor& cursor) {
	auto& range = cursor.stack().back();
	if (!is_instance_of(range, Class::iterator)) {
		cursor.stack().back() = create_iterator_over(cursor.ast(), std::move(range));
	}
}

void mint::range_next(Cursor& cursor) {
	assert(is_instance_of(cursor.stack().back(), Class::iterator));
	cursor.stack().back().data<Iterator>().ctx.next();
}

void mint::range_check(Cursor& cursor, std::size_t pos) {

	const auto base = get_stack_base(cursor);
	const auto& range = load_from_stack(cursor, base);
	auto& target = load_from_stack(cursor, base - 1);

	if (std::optional<WeakReference>&& item = iterator_get(range.data<Iterator>())) {

		if ((target.flags() & Reference::const_address) && (target.data().format() != Data::none_format)) [[unlikely]] {
			error("invalid modification of constant reference");
		}

		if ((item->flags() & (Reference::const_value | Reference::temporary)) == Reference::const_value) {
			target.copy_data(*item);
		}
		else {
			target.move_data(*item);
		}
	}
	else {
		cursor.stack().pop_back();
		cursor.stack().pop_back();
		cursor.jmp(pos);
	}
}

void mint::range_iterator_check(Cursor& cursor, std::size_t pos) {

	const auto base = get_stack_base(cursor);

	const auto& range = load_from_stack(cursor, base);
	const auto& target = load_from_stack(cursor, base - 1);

	if (std::optional<WeakReference> item = iterator_get(range.data<Iterator>())) {

		auto target_context = target.data<Iterator>().ctx;
		auto it = target_context.begin();
		const auto end = target_context.end();

		if (is_instance_of(*item, Class::iterator)) {
			item->data<Iterator>().ctx.finalize();
		}

		for_each_if(cursor.ast(), *item, [&it, &end](const Reference& item) -> bool {
			if (it != end) {
				if ((it->flags() & Reference::const_address) && (it->data().format() != Data::none_format))
				    [[unlikely]] {
					error("invalid modification of constant reference");
				}

				it->move_data(item);
				++it;
				return true;
			}

			return false;
		});
	}
	else {
		cursor.stack().pop_back();
		cursor.stack().pop_back();
		cursor.jmp(pos);
	}
}

namespace mint {

#if !defined(__x86_64__) && !defined(_WIN64)
static constexpr const std::size_t fnv_prime = 16777619u;
static constexpr const std::size_t offset_basis = 2166136261u;
#else
static constexpr const std::size_t fnv_prime = 1099511628211u;
static constexpr const std::size_t offset_basis = 14695981039346656037u;
#endif

}

std::size_t Hash::hash::operator()(const Hash::key_type& value) const {

	switch (value.data().format()) {
	case Data::none_format:
		return std::size_t {};

	case Data::null_format:
#if (__cplusplus >= 201703L) || (defined(_MSC_VER) && _MSC_VER >= 1911)
		return std::hash<std::nullptr_t> {}(nullptr);
#else
		return std::hash<void*> {}(nullptr);
#endif

	case Data::number_format:
		return std::hash<double> {}(value.data<Number>().value);

	case Data::boolean_format:
		return std::hash<bool> {}(value.data<Boolean>().value);

	case Data::object_format:
		switch (value.data<Object>().metadata.metatype()) {
		case Class::object:
			return std::hash<WeakReference*> {}(value.data<Object>().data);

		case Class::string:
			return std::hash<std::string> {}(value.data<String>().str);

		case Class::regex:
			return std::hash<std::string> {}(value.data<Regex>().initializer);

		case Class::array:
			return [this, &value] {
				std::size_t hash = offset_basis;
				for (auto i = value.data<Array>().values.begin(); i != value.data<Array>().values.end(); ++i) {
					hash = hash * fnv_prime;
					hash = hash ^ operator()(array_get_item(i));
				}
				return hash;
			}();

		case Class::hash:
		case Class::iterator:
		case Class::library:
		case Class::libobject:
			error("invalid use of '{}' type as hash key", type_name(value));
		}
		break;
	case Data::package_format:
	case Data::function_format:
		error("invalid use of '{}' type as hash key", type_name(value));
	}

	return false;
}

bool Hash::equal_to::operator()(const Hash::key_type& lvalue, const Hash::key_type& rvalue) const {

	if (lvalue.data().format() != rvalue.data().format()) {
		return false;
	}

	switch (lvalue.data().format()) {
	case Data::none_format:
	case Data::null_format:
		return true;

	case Data::number_format:
		return lvalue.data<Number>().value == rvalue.data<Number>().value;

	case Data::boolean_format:
		return lvalue.data<Boolean>().value == rvalue.data<Boolean>().value;

	case Data::object_format:
		if (lvalue.data<Object>().metadata.metatype() != rvalue.data<Object>().metadata.metatype()) {
			return false;
		}

		switch (lvalue.data<Object>().metadata.metatype()) {
		case Class::object:
			if (&lvalue.data<Object>().metadata != &rvalue.data<Object>().metadata) {
				return false;
			}
			return lvalue.data<Object>().data == rvalue.data<Object>().data;

		case Class::string:
			return lvalue.data<String>().str == rvalue.data<String>().str;

		case Class::regex:
			return lvalue.data<Regex>().initializer == rvalue.data<Regex>().initializer;

		case Class::array:
			if (lvalue.data<Array>().values.size() != rvalue.data<Array>().values.size()) {
				return false;
			}
			for (auto i = lvalue.data<Array>().values.begin(), j = rvalue.data<Array>().values.begin();
			    i != lvalue.data<Array>().values.end() && j != rvalue.data<Array>().values.end(); ++i, ++j) {
				if (!operator()(array_get_item(i), array_get_item(j))) {
					return false;
				}
			}
			return true;

		case Class::hash:
		case Class::iterator:
		case Class::library:
		case Class::libobject:
			error("invalid use of '{}' type as hash key", type_name(lvalue));
		}
		break;
	case Data::package_format:
	case Data::function_format:
		error("invalid use of '{}' type as hash key", type_name(lvalue));
	}

	return false;
}

bool Hash::compare_to::operator()(const Hash::key_type& lvalue, const Hash::key_type& rvalue) const {

	if (lvalue.data().format() != rvalue.data().format()) {
		return lvalue.data().format() < rvalue.data().format();
	}

	switch (lvalue.data().format()) {
	case Data::none_format:
	case Data::null_format:
		return false;

	case Data::number_format:
		return lvalue.data<Number>().value < rvalue.data<Number>().value;

	case Data::boolean_format:
		return lvalue.data<Boolean>().value < rvalue.data<Boolean>().value;

	case Data::object_format:
		if (lvalue.data<Object>().metadata.metatype() != rvalue.data<Object>().metadata.metatype()) {
			return lvalue.data<Object>().metadata.metatype() < rvalue.data<Object>().metadata.metatype();
		}

		switch (lvalue.data<Object>().metadata.metatype()) {
		case Class::object:
			if (&lvalue.data<Object>().metadata != &rvalue.data<Object>().metadata) {
				return &lvalue.data<Object>().metadata < &rvalue.data<Object>().metadata;
			}
			return lvalue.data<Object>().data < rvalue.data<Object>().data;

		case Class::string:
			return lvalue.data<String>().str < rvalue.data<String>().str;

		case Class::regex:
			return lvalue.data<Regex>().initializer < rvalue.data<Regex>().initializer;

		case Class::array:
			for (auto i = lvalue.data<Array>().values.begin(), j = rvalue.data<Array>().values.begin();
			    i != lvalue.data<Array>().values.end() && j != rvalue.data<Array>().values.end(); ++i, ++j) {
				if (operator()(array_get_item(i), array_get_item(j))) {
					return true;
				}
				if (operator()(array_get_item(j), array_get_item(i))) {
					return false;
				}
			}
			return lvalue.data<Array>().values.size() < rvalue.data<Array>().values.size();

		case Class::hash:
		case Class::iterator:
		case Class::library:
		case Class::libobject:
			error("invalid use of '{}' type as hash key", type_name(lvalue));
		}
		break;
	case Data::package_format:
	case Data::function_format:
		error("invalid use of '{}' type as hash key", type_name(lvalue));
	}

	return false;
}
