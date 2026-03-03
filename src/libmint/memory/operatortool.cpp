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
#include "mint/memory/algorithm.h"
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
			error("invalid use of class '{}' with operator '{}'({})", object.metadata.full_name(),
			    get_operator_symbol(operator_overload).str(), signature);
		}

		const auto& function = Class::MemberInfo::get(*info, object);
		Class& metadata = info->owner.get();

		switch (function.data().format()) {
		case Data::Format::none:
			error("invalid use of none value as a function");
		case Data::Format::null:
			cursor.raise(WeakReference(function));
			break;
		case Data::Format::number:
		case Data::Format::boolean:
		case Data::Format::object:
			if (signature == 0) {
				cursor.stack().back() = WeakReference(copy_from, function);
			}
			else {
				error("{} copy doesn't take {} argument(s)", type_name(function), signature);
			}
			break;
		case Data::Format::package:
			error("invalid use of package '{}' as a function", function.data<Package>().data.full_name());
		case Data::Format::coroutine:
			error("invalid use of coroutine as a function");
		case Data::Format::function:
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
			error("invalid use of class '{}' with operator '{}'({})", object.metadata.full_name(),
			    operator_overload.str(), signature);
		}

		const auto& function = Class::MemberInfo::get(*info, object);
		Class& metadata = info->owner.get();

		switch (function.data().format()) {
		case Data::Format::none:
			error("invalid use of none value as a function");
		case Data::Format::null:
			cursor.raise(WeakReference(function));
			break;
		case Data::Format::number:
		case Data::Format::boolean:
		case Data::Format::object:
			if (signature == 0) {
				cursor.stack().back() = WeakReference(copy_from, function);
			}
			else {
				error("{} copy doesn't take {} argument(s)", type_name(function), signature);
			}
			break;
		case Data::Format::package:
			error("invalid use of package '{}' as a function", function.data<Package>().data.full_name());
		case Data::Format::coroutine:
			error("invalid use of coroutine as a function");
		case Data::Format::function:
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

	const auto& rhs = load_from_stack(cursor, base);
	auto& lhs = load_from_stack(cursor, base - 1);

	if ((lhs.flags() & Reference::const_address) && (lhs.data().format() != Data::Format::none)) [[unlikely]] {
		error("invalid modification of constant reference");
	}

	if (lhs.flags() & Reference::const_value) {
		lhs.move_data(rhs);
	}
	else if ((rhs.flags() & (Reference::const_value | Reference::temporary)) == Reference::const_value) {
		lhs.copy_data(rhs);
	}
	else {
		lhs.move_data(rhs);
	}

	cursor.stack().pop_back();
}

void mint::copy_operator(Cursor& cursor) {

	const auto base = get_stack_base(cursor);

	const auto& rhs = load_from_stack(cursor, base);
	const auto& lhs = load_from_stack(cursor, base - 1);

	if (lhs.flags() & Reference::const_value) [[unlikely]] {
		error("invalid modification of constant value");
	}

	switch (lhs.data().format()) {
	case Data::Format::none:
		error("invalid use of none value in assignment");
	case Data::Format::null:
		cursor.raise(WeakReference(lhs));
		break;
	case Data::Format::number:
		lhs.data<Number>().value = to_number(cursor, rhs);
		cursor.stack().pop_back();
		break;
	case Data::Format::boolean:
		lhs.data<Boolean>().value = to_boolean(rhs);
		cursor.stack().pop_back();
		break;
	case Data::Format::function:
		if (rhs.data().format() != Data::Format::function) [[unlikely]] {
			error("invalid conversion from '{}' to '{}'", type_name(rhs), type_name(lhs));
		}
		lhs.data<Function>().mapping = rhs.data<Function>().mapping;
		cursor.stack().pop_back();
		break;
	case Data::Format::object:
		if (!call_overload(cursor, Class::copy_operator, 1)) {
			if (rhs.data().format() != Data::Format::object) [[unlikely]] {
				error("cannot convert '{}' to '{}' in assignment", type_name(rhs), type_name(lhs));
			}
			if (&lhs.data<Object>().metadata != &rhs.data<Object>().metadata) [[unlikely]] {
				error("cannot convert '{}' to '{}' in assignment", type_name(rhs), type_name(lhs));
			}
			lhs.data<Object>().destroy();
			lhs.data<Object>().construct(rhs.data<Object>());
		}
		break;
	case Data::Format::package:
		error("invalid use of package '{}' in assignment", lhs.data<Package>().data.full_name());
	case Data::Format::coroutine:
		error("invalid use of coroutine in assignment");
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
	case Data::Format::none:
		if (flags & Cursor::Call::member_call) [[likely]] {
			if (signature) [[unlikely]] {
				error("default constructors doesn't take {} argument(s)", signature);
			}
		}
		else {
			error("invalid use of none value as a function");
		}
		break;
	case Data::Format::null:
		cursor.raise(WeakReference(function));
		break;
	case Data::Format::number:
	case Data::Format::boolean:
	case Data::Format::object:
		if (signature == 0) {
			cursor.stack().emplace_back(copy_from, function);
		}
		else {
			error("{} copy doesn't take {} argument(s)", type_name(function), signature);
		}
		break;
	case Data::Format::package:
		error("invalid use of package '{}' as a function", function.data<Package>().data.full_name());
	case Data::Format::coroutine:
		error("invalid use of coroutine as a function");
	case Data::Format::function:
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
	case Data::Format::none:
		if (flags & Cursor::Call::member_call) [[likely]] {
			if (signature) [[unlikely]] {
				error("default constructors doesn't take {} argument(s)", signature);
			}
		}
		else {
			error("invalid use of none value as a function");
		}
		break;
	case Data::Format::null:
		cursor.raise(WeakReference(function));
		break;
	case Data::Format::number:
	case Data::Format::boolean:
	case Data::Format::object:
		if (signature == 0) {
			cursor.stack().back() = WeakReference(copy_from, function);
		}
		else {
			error("{} copy doesn't take {} argument(s)", type_name(function), signature);
		}
		break;
	case Data::Format::package:
		error("invalid use of package '{}' as a function", function.data<Package>().data.full_name());
	case Data::Format::coroutine:
		error("invalid use of coroutine as a function");
	case Data::Format::function:
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

	const auto& rhs = load_from_stack(cursor, base);
	const auto& lhs = load_from_stack(cursor, base - 1);

	switch (lhs.data().format()) {
	case Data::Format::none:
		error("invalid use of none value with operator '+'(1)");
	case Data::Format::null:
		cursor.raise(WeakReference(lhs));
		break;
	case Data::Format::number:
		if (lhs.flags() & Reference::temporary) {
			lhs.data<Number>().value += to_number(cursor, rhs);
			cursor.stack().pop_back();
		}
		else {
			Reference&& result = create_number(lhs.data<Number>().value + to_number(cursor, rhs));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::Format::boolean:
		if (lhs.flags() & Reference::temporary) {
			lhs.data<Boolean>().value += to_boolean(rhs);
			cursor.stack().pop_back();
		}
		else {
			Reference&& result = create_boolean(lhs.data<Boolean>().value + to_boolean(rhs));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::Format::object:
		if (!call_overload(cursor, Class::add_operator, 1)) [[unlikely]] {
			error("class '{}' doesn't overload operator '+'(1)", type_name(lhs));
		}
		break;
	case Data::Format::package:
		error("invalid use of package '{}' with operator '+'(1)", lhs.data<Package>().data.full_name());
	case Data::Format::coroutine:
		error("invalid use of coroutine with operator '+'(1)");
	case Data::Format::function:
		{
			if (rhs.data().format() != Data::Format::function) [[unlikely]] {
				error("invalid use of operator '+'(1) with '{}' and '{}' types", type_name(lhs), type_name(rhs));
			}
			Reference&& result = create_function();
			for (const auto& item : lhs.data<Function>().mapping) {
				result.data<Function>().mapping.insert(item);
			}
			for (const auto& item : rhs.data<Function>().mapping) {
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

	const auto& rhs = load_from_stack(cursor, base);
	const auto& lhs = load_from_stack(cursor, base - 1);

	switch (lhs.data().format()) {
	case Data::Format::none:
		error("invalid use of none value with operator '-'(1)");
	case Data::Format::null:
		cursor.raise(WeakReference(lhs));
		break;
	case Data::Format::number:
		if (lhs.flags() & Reference::temporary) {
			lhs.data<Number>().value -= to_number(cursor, rhs);
			cursor.stack().pop_back();
		}
		else {
			Reference&& result = create_number(lhs.data<Number>().value - to_number(cursor, rhs));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::Format::boolean:
		if (lhs.flags() & Reference::temporary) {
			lhs.data<Boolean>().value -= to_boolean(rhs);
			cursor.stack().pop_back();
		}
		else {
			Reference&& result = create_boolean(lhs.data<Boolean>().value - to_boolean(rhs));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::Format::object:
		if (!call_overload(cursor, Class::sub_operator, 1)) [[unlikely]] {
			error("class '{}' doesn't overload operator '-'(1)", type_name(lhs));
		}
		break;
	case Data::Format::package:
		error("invalid use of package '{}' with operator '-'(1)", lhs.data<Package>().data.full_name());
	case Data::Format::coroutine:
		error("invalid use of coroutine with operator '-'(1)");
	case Data::Format::function:
		error("invalid use of '{}' type with operator '-'(1)", type_name(lhs));
	}
}

void mint::mul_operator(Cursor& cursor) {

	const auto base = get_stack_base(cursor);

	const auto& rhs = load_from_stack(cursor, base);
	const auto& lhs = load_from_stack(cursor, base - 1);

	switch (lhs.data().format()) {
	case Data::Format::none:
		error("invalid use of none value with operator '*'(1)");
	case Data::Format::null:
		cursor.raise(WeakReference(lhs));
		break;
	case Data::Format::number:
		if (lhs.flags() & Reference::temporary) {
			lhs.data<Number>().value *= to_number(cursor, rhs);
			cursor.stack().pop_back();
		}
		else {
			Reference&& result = create_number(lhs.data<Number>().value * to_number(cursor, rhs));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::Format::boolean:
		if (lhs.flags() & Reference::temporary) {
			lhs.data<Boolean>().value = lhs.data<Boolean>().value && to_boolean(rhs);
			cursor.stack().pop_back();
		}
		else {
			Reference&& result = create_boolean(lhs.data<Boolean>().value && to_boolean(rhs));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::Format::object:
		if (!call_overload(cursor, Class::mul_operator, 1)) [[unlikely]] {
			error("class '{}' doesn't overload operator '*'(1)", type_name(lhs));
		}
		break;
	case Data::Format::package:
		error("invalid use of package '{}' with operator '*'(1)", lhs.data<Package>().data.full_name());
	case Data::Format::coroutine:
		error("invalid use of coroutine with operator '*'(1)");
	case Data::Format::function:
		error("invalid use of '{}' type with operator '*'(1)", type_name(lhs));
	}
}

void mint::div_operator(Cursor& cursor) {

	const auto base = get_stack_base(cursor);

	const auto& rhs = load_from_stack(cursor, base);
	const auto& lhs = load_from_stack(cursor, base - 1);

	switch (lhs.data().format()) {
	case Data::Format::none:
		error("invalid use of none value with operator '/'(1)");
	case Data::Format::null:
		cursor.raise(WeakReference(lhs));
		break;
	case Data::Format::number:
		if (lhs.flags() & Reference::temporary) {
			lhs.data<Number>().value /= to_number(cursor, rhs);
			cursor.stack().pop_back();
		}
		else {
			Reference&& result = create_number(lhs.data<Number>().value / to_number(cursor, rhs));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::Format::boolean:
		if (lhs.flags() & Reference::temporary) {
			lhs.data<Boolean>().value /= to_boolean(rhs);
			cursor.stack().pop_back();
		}
		else {
			Reference&& result = create_boolean(lhs.data<Boolean>().value / to_boolean(rhs));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::Format::object:
		if (!call_overload(cursor, Class::div_operator, 1)) [[unlikely]] {
			error("class '{}' doesn't overload operator '/'(1)", type_name(lhs));
		}
		break;
	case Data::Format::package:
		error("invalid use of package '{}' with operator '/'(1)", lhs.data<Package>().data.full_name());
	case Data::Format::coroutine:
		error("invalid use of coroutine with operator '/'(1)");
	case Data::Format::function:
		error("invalid use of '{}' type with operator '/'(1)", type_name(lhs));
	}
}

void mint::pow_operator(Cursor& cursor) {

	const auto base = get_stack_base(cursor);

	const auto& rhs = load_from_stack(cursor, base);
	const auto& lhs = load_from_stack(cursor, base - 1);

	switch (lhs.data().format()) {
	case Data::Format::none:
		error("invalid use of none value with operator '**'(1)");
	case Data::Format::null:
		cursor.raise(WeakReference(lhs));
		break;
	case Data::Format::number:
		if (lhs.flags() & Reference::temporary) {
			lhs.data<Number>().value = pow(lhs.data<Number>().value, to_number(cursor, rhs));
			cursor.stack().pop_back();
		}
		else {
			Reference&& result = create_number(pow(lhs.data<Number>().value, to_number(cursor, rhs)));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::Format::object:
		if (!call_overload(cursor, Class::pow_operator, 1)) [[unlikely]] {
			error("class '{}' doesn't overload operator '**'(1)", type_name(lhs));
		}
		break;
	case Data::Format::package:
		error("invalid use of package '{}' with operator '**'(1)", lhs.data<Package>().data.full_name());
	case Data::Format::coroutine:
		error("invalid use of coroutine with operator '**'(1)");
	case Data::Format::boolean:
	case Data::Format::function:
		error("invalid use of '{}' type with operator '**'(1)", type_name(lhs));
	}
}

void mint::mod_operator(Cursor& cursor) {

	const auto base = get_stack_base(cursor);

	const auto& rhs = load_from_stack(cursor, base);
	const auto& lhs = load_from_stack(cursor, base - 1);

	switch (lhs.data().format()) {
	case Data::Format::none:
		error("invalid use of none value with operator '%'(1)");
	case Data::Format::null:
		cursor.raise(WeakReference(lhs));
		break;
	case Data::Format::number:
		if (const auto divider = to_signed_integer(cursor, rhs)) {
			if (lhs.flags() & Reference::temporary) {
				lhs.data<Number>().value = to_number(to_signed_integer(lhs.data<Number>().value) % divider);
				cursor.stack().pop_back();
			}
			else {
				Reference&& result = create_signed_number(to_signed_integer(lhs.data<Number>().value) % divider);
				cursor.stack().pop_back();
				cursor.stack().back() = std::move(result);
			}
		}
		else {
			error("modulo by zero");
		}
		break;
	case Data::Format::object:
		if (!call_overload(cursor, Class::mod_operator, 1)) [[unlikely]] {
			error("class '{}' doesn't overload operator '%'(1)", type_name(lhs));
		}
		break;
	case Data::Format::package:
		error("invalid use of package '{}' with operator '%'(1)", lhs.data<Package>().data.full_name());
	case Data::Format::coroutine:
		error("invalid use of coroutine with operator '%'(1)");
	case Data::Format::boolean:
	case Data::Format::function:
		error("invalid use of '{}' type with operator '%'(1)", type_name(lhs));
	}
}

void mint::is_operator(Cursor& cursor) {

	const auto base = get_stack_base(cursor);

	const auto& rhs = load_from_stack(cursor, base);
	const auto& lhs = load_from_stack(cursor, base - 1);

	WeakReference result = create_boolean(&lhs.data() == &rhs.data());
	cursor.stack().pop_back();
	cursor.stack().back() = std::move(result);
}

void mint::eq_operator(Cursor& cursor) {

	const auto base = get_stack_base(cursor);

	const auto& rhs = load_from_stack(cursor, base);
	auto& lhs = load_from_stack(cursor, base - 1);

	switch (lhs.data().format()) {
	case Data::Format::none:
		{
			Reference&& result = create_boolean(rhs.data().format() == Data::Format::none);
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::Format::null:
		{
			Reference&& result = create_boolean(rhs.data().format() == Data::Format::null);
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::Format::number:
		switch (rhs.data().format()) {
		case Data::Format::none:
		case Data::Format::null:
			{
				Reference&& result = create_boolean(false);
				cursor.stack().pop_back();
				cursor.stack().back() = std::move(result);
			}
			break;
		default:
			Reference&& result = create_boolean(lhs.data<Number>().value == to_number(cursor, rhs));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::Format::boolean:
		switch (rhs.data().format()) {
		case Data::Format::none:
		case Data::Format::null:
			{
				Reference&& result = create_boolean(false);
				cursor.stack().pop_back();
				cursor.stack().back() = std::move(result);
			}
			break;
		default:
			Reference&& result = create_boolean(lhs.data<Boolean>().value == to_boolean(rhs));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::Format::object:
		if (!call_overload(cursor, Class::eq_operator, 1)) {
			switch (rhs.data().format()) {
			case Data::Format::none:
			case Data::Format::null:
				{
					Reference&& result = create_boolean(false);
					cursor.stack().pop_back();
					cursor.stack().back() = std::move(result);
				}
				break;
			default:
				error("class '{}' doesn't overload operator '=='(1)", type_name(lhs));
			}
		}
		break;
	case Data::Format::package:
		error("invalid use of package '{}' with operator '=='(1)", lhs.data<Package>().data.full_name());
	case Data::Format::coroutine:
		error("invalid use of coroutine with operator '=='(1)");
	case Data::Format::function:
		if (rhs.data().format() == Data::Format::function) {
			Reference&& result = create_boolean(lhs.data<Function>().mapping == rhs.data<Function>().mapping);
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		else {
			error("invalid use of '{}' type with operator '=='(1)", type_name(lhs));
		}
	}
}

void mint::ne_operator(Cursor& cursor) {

	const auto base = get_stack_base(cursor);

	const auto& rhs = load_from_stack(cursor, base);
	auto& lhs = load_from_stack(cursor, base - 1);

	switch (lhs.data().format()) {
	case Data::Format::none:
		{
			Reference&& result = create_boolean(rhs.data().format() != Data::Format::none);
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::Format::null:
		{
			Reference&& result = create_boolean(rhs.data().format() != Data::Format::null);
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::Format::number:
		switch (rhs.data().format()) {
		case Data::Format::none:
		case Data::Format::null:
			{
				Reference&& result = create_boolean(true);
				cursor.stack().pop_back();
				cursor.stack().back() = std::move(result);
			}
			break;
		default:
			Reference&& result = create_boolean(lhs.data<Number>().value != to_number(cursor, rhs));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::Format::boolean:
		switch (rhs.data().format()) {
		case Data::Format::none:
		case Data::Format::null:
			{
				Reference&& result = create_boolean(true);
				cursor.stack().pop_back();
				cursor.stack().back() = std::move(result);
			}
			break;
		default:
			Reference&& result = create_boolean(lhs.data<Boolean>().value != to_boolean(rhs));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::Format::object:
		if (!call_overload(cursor, Class::ne_operator, 1)) {
			switch (rhs.data().format()) {
			case Data::Format::none:
			case Data::Format::null:
				{
					Reference&& result = create_boolean(true);
					cursor.stack().pop_back();
					cursor.stack().back() = std::move(result);
				}
				break;
			default:
				error("class '{}' doesn't overload operator '!='(1)", type_name(lhs));
			}
		}
		break;
	case Data::Format::package:
		error("invalid use of package '{}' with operator '!='(1)", lhs.data<Package>().data.full_name());
	case Data::Format::coroutine:
		error("invalid use of coroutine with operator '!='(1)");
	case Data::Format::function:
		if (rhs.data().format() == Data::Format::function) {
			Reference&& result = create_boolean(lhs.data<Function>().mapping != rhs.data<Function>().mapping);
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		else {
			error("invalid use of '{}' type with operator '!='(1)", type_name(lhs));
		}
	}
}

void mint::lt_operator(Cursor& cursor) {

	const auto base = get_stack_base(cursor);

	const auto& rhs = load_from_stack(cursor, base);
	const auto& lhs = load_from_stack(cursor, base - 1);

	switch (lhs.data().format()) {
	case Data::Format::none:
		error("invalid use of none value with operator '<'(1)");
	case Data::Format::null:
		cursor.raise(WeakReference(lhs));
		break;
	case Data::Format::number:
		{
			Reference&& result = create_boolean(lhs.data<Number>().value < to_number(cursor, rhs));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::Format::boolean:
		{
			Reference&& result = create_boolean(lhs.data<Boolean>().value < to_boolean(rhs));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::Format::object:
		if (!call_overload(cursor, Class::lt_operator, 1)) [[unlikely]] {
			error("class '{}' doesn't overload operator '<'(1)", type_name(lhs));
		}
		break;
	case Data::Format::package:
		error("invalid use of package '{}' with operator '<'(1)", lhs.data<Package>().data.full_name());
	case Data::Format::coroutine:
		error("invalid use of coroutine with operator '<'(1)");
	case Data::Format::function:
		error("invalid use of '{}' type with operator '<'(1)", type_name(lhs));
	}
}

void mint::gt_operator(Cursor& cursor) {

	const auto base = get_stack_base(cursor);

	const auto& rhs = load_from_stack(cursor, base);
	const auto& lhs = load_from_stack(cursor, base - 1);

	switch (lhs.data().format()) {
	case Data::Format::none:
		error("invalid use of none value with operator '>'(1)");
	case Data::Format::null:
		cursor.raise(WeakReference(lhs));
		break;
	case Data::Format::number:
		{
			Reference&& result = create_boolean(lhs.data<Number>().value > to_number(cursor, rhs));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::Format::boolean:
		{
			Reference&& result = create_boolean(lhs.data<Boolean>().value > to_boolean(rhs));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::Format::object:
		if (!call_overload(cursor, Class::gt_operator, 1)) [[unlikely]] {
			error("class '{}' doesn't overload operator '>'(1)", type_name(lhs));
		}
		break;
	case Data::Format::package:
		error("invalid use of package '{}' with operator '>'(1)", lhs.data<Package>().data.full_name());
	case Data::Format::coroutine:
		error("invalid use of coroutine with operator '>'(1)");
	case Data::Format::function:
		error("invalid use of '{}' type with operator '>'(1)", type_name(lhs));
	}
}

void mint::le_operator(Cursor& cursor) {

	const auto base = get_stack_base(cursor);

	const auto& rhs = load_from_stack(cursor, base);
	const auto& lhs = load_from_stack(cursor, base - 1);

	switch (lhs.data().format()) {
	case Data::Format::none:
		error("invalid use of none value with operator '<='(1)");
	case Data::Format::null:
		cursor.raise(WeakReference(lhs));
		break;
	case Data::Format::number:
		{
			Reference&& result = create_boolean(lhs.data<Number>().value <= to_number(cursor, rhs));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::Format::boolean:
		{
			Reference&& result = create_boolean(lhs.data<Boolean>().value <= to_boolean(rhs));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::Format::object:
		if (!call_overload(cursor, Class::le_operator, 1)) [[unlikely]] {
			error("class '{}' doesn't overload operator '<='(1)", type_name(lhs));
		}
		break;
	case Data::Format::package:
		error("invalid use of package '{}' with operator '<='(1)", lhs.data<Package>().data.full_name());
	case Data::Format::coroutine:
		error("invalid use of coroutine with operator '<='(1)");
	case Data::Format::function:
		error("invalid use of '{}' type with operator '<='(1)", type_name(lhs));
	}
}

void mint::ge_operator(Cursor& cursor) {

	const auto base = get_stack_base(cursor);

	const auto& rhs = load_from_stack(cursor, base);
	const auto& lhs = load_from_stack(cursor, base - 1);

	switch (lhs.data().format()) {
	case Data::Format::none:
		error("invalid use of none value with operator '>='(1)");
	case Data::Format::null:
		cursor.raise(WeakReference(lhs));
		break;
	case Data::Format::number:
		{
			Reference&& result = create_boolean(lhs.data<Number>().value >= to_number(cursor, rhs));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::Format::boolean:
		{
			Reference&& result = create_boolean(lhs.data<Boolean>().value >= to_boolean(rhs));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::Format::object:
		if (!call_overload(cursor, Class::ge_operator, 1)) [[unlikely]] {
			error("class '{}' doesn't overload operator '>='(1)", type_name(lhs));
		}
		break;
	case Data::Format::package:
		error("invalid use of package '{}' with operator '>='(1)", lhs.data<Package>().data.full_name());
	case Data::Format::coroutine:
		error("invalid use of coroutine with operator '>='(1)");
	case Data::Format::function:
		error("invalid use of '{}' type with operator '>='(1)", type_name(lhs));
	}
}

void mint::and_pre_check(Cursor& cursor, std::size_t pos) {

	const auto& arg = cursor.stack().back();

	switch (arg.data().format()) {
	case Data::Format::none:
	case Data::Format::null:
		cursor.jmp(pos);
		break;
	case Data::Format::number:
		if (arg.data<Number>().value == 0.) {
			cursor.jmp(pos);
		}
		break;
	case Data::Format::boolean:
		if (!arg.data<Boolean>().value) {
			cursor.jmp(pos);
		}
		break;
	case Data::Format::object:
		switch (arg.data<Object>().metadata.metatype()) {
		case Class::Metatype::iterator:
			if (arg.data<Iterator>().ctx.empty()) {
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

	auto& rhs = load_from_stack(cursor, base);
	auto& lhs = load_from_stack(cursor, base - 1);

	switch (lhs.data().format()) {
	case Data::Format::object:
		if (!call_overload(cursor, Class::and_operator, 1)) {
			std::swap(lhs, rhs);
			cursor.stack().pop_back();
		}
		break;
	default:
		std::swap(lhs, rhs);
		cursor.stack().pop_back();
	}
}

void mint::or_pre_check(Cursor& cursor, std::size_t pos) {

	const auto& arg = cursor.stack().back();

	switch (arg.data().format()) {
	case Data::Format::none:
	case Data::Format::null:
		break;
	case Data::Format::number:
		if (arg.data<Number>().value != 0.) {
			cursor.jmp(pos);
		}
		break;
	case Data::Format::boolean:
		if (arg.data<Boolean>().value) {
			cursor.jmp(pos);
		}
		break;
	case Data::Format::object:
		switch (arg.data<Object>().metadata.metatype()) {
		case Class::Metatype::iterator:
			if (!arg.data<Iterator>().ctx.empty()) {
				cursor.jmp(pos);
			}
			break;
		default:
			if (!arg.data<Object>().metadata.find_operator(Class::or_operator)) {
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

	auto& rhs = load_from_stack(cursor, base);
	auto& lhs = load_from_stack(cursor, base - 1);

	switch (lhs.data().format()) {
	case Data::Format::object:
		if (!call_overload(cursor, Class::or_operator, 1)) {
			std::swap(lhs, rhs);
			cursor.stack().pop_back();
		}
		break;
	default:
		std::swap(lhs, rhs);
		cursor.stack().pop_back();
	}
}

void mint::band_operator(Cursor& cursor) {

	const auto base = get_stack_base(cursor);

	const auto& rhs = load_from_stack(cursor, base);
	const auto& lhs = load_from_stack(cursor, base - 1);

	switch (lhs.data().format()) {
	case Data::Format::none:
		error("invalid use of none value with operator '&'(1)");
	case Data::Format::null:
		cursor.raise(WeakReference(lhs));
		break;
	case Data::Format::number:
		if (lhs.flags() & Reference::temporary) {
			lhs.data<Number>().value = to_number(
			    to_unsigned_integer(lhs.data<Number>().value) & to_unsigned_integer(cursor, rhs));
			cursor.stack().pop_back();
		}
		else {
			Reference&& result = create_unsigned_number(
			    to_unsigned_integer(lhs.data<Number>().value) & to_unsigned_integer(cursor, rhs));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::Format::boolean:
		if (lhs.flags() & Reference::temporary) {
			lhs.data<Boolean>().value &= to_boolean(rhs);
			cursor.stack().pop_back();
		}
		else {
			Reference&& result = create_boolean(lhs.data<Boolean>().value && to_boolean(rhs));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::Format::object:
		if (!call_overload(cursor, Class::band_operator, 1)) [[unlikely]] {
			error("class '{}' doesn't overload operator '&'(1)", type_name(lhs));
		}
		break;
	case Data::Format::package:
		error("invalid use of package '{}' with operator '&'(1)", lhs.data<Package>().data.full_name());
	case Data::Format::coroutine:
		error("invalid use of coroutine with operator '&'(1)");
	case Data::Format::function:
		error("invalid use of '{}' type with operator '&'(1)", type_name(lhs));
	}
}

void mint::bor_operator(Cursor& cursor) {

	const auto base = get_stack_base(cursor);

	const auto& rhs = load_from_stack(cursor, base);
	const auto& lhs = load_from_stack(cursor, base - 1);

	switch (lhs.data().format()) {
	case Data::Format::none:
		error("invalid use of none value with operator '|'(1)");
	case Data::Format::null:
		cursor.raise(WeakReference(lhs));
		break;
	case Data::Format::number:
		if (lhs.flags() & Reference::temporary) {
			lhs.data<Number>().value = to_number(
			    to_unsigned_integer(lhs.data<Number>().value) | to_unsigned_integer(cursor, rhs));
			cursor.stack().pop_back();
		}
		else {
			Reference&& result = create_unsigned_number(
			    to_unsigned_integer(lhs.data<Number>().value) | to_unsigned_integer(cursor, rhs));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::Format::boolean:
		if (lhs.flags() & Reference::temporary) {
			lhs.data<Boolean>().value |= to_boolean(rhs);
			cursor.stack().pop_back();
		}
		else {
			Reference&& result = create_boolean(lhs.data<Boolean>().value || to_boolean(rhs));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::Format::object:
		if (!call_overload(cursor, Class::bor_operator, 1)) [[unlikely]] {
			error("class '{}' doesn't overload operator '|'(1)", type_name(lhs));
		}
		break;
	case Data::Format::package:
		error("invalid use of package '{}' with operator '|'(1)", lhs.data<Package>().data.full_name());
	case Data::Format::coroutine:
		error("invalid use of coroutine with operator '|'(1)");
	case Data::Format::function:
		error("invalid use of '{}' type with operator '|'(1)", type_name(lhs));
	}
}

void mint::xor_operator(Cursor& cursor) {

	const auto base = get_stack_base(cursor);

	const auto& rhs = load_from_stack(cursor, base);
	const auto& lhs = load_from_stack(cursor, base - 1);

	switch (lhs.data().format()) {
	case Data::Format::none:
		error("invalid use of none value with operator '^'(1)");
	case Data::Format::null:
		cursor.raise(WeakReference(lhs));
		break;
	case Data::Format::number:
		if (lhs.flags() & Reference::temporary) {
			lhs.data<Number>().value = to_number(
			    to_unsigned_integer(lhs.data<Number>().value) ^ to_unsigned_integer(cursor, rhs));
			cursor.stack().pop_back();
		}
		else {
			Reference&& result = create_unsigned_number(
			    to_unsigned_integer(lhs.data<Number>().value) ^ to_unsigned_integer(cursor, rhs));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::Format::boolean:
		if (lhs.flags() & Reference::temporary) {
			lhs.data<Boolean>().value ^= to_boolean(rhs);
			cursor.stack().pop_back();
		}
		else {
			Reference&& result = create_boolean(to_unsigned_integer(lhs.data<Number>().value) ^ to_boolean(rhs));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::Format::object:
		if (!call_overload(cursor, Class::xor_operator, 1)) [[unlikely]] {
			error("class '{}' doesn't overload operator '^'(1)", type_name(lhs));
		}
		break;
	case Data::Format::package:
		error("invalid use of package '{}' with operator '^'(1)", lhs.data<Package>().data.full_name());
	case Data::Format::coroutine:
		error("invalid use of coroutine with operator '^'(1)");
	case Data::Format::function:
		error("invalid use of '{}' type with operator '^'(1)", type_name(lhs));
	}
}

void mint::inc_operator(Cursor& cursor) {

	auto& arg = cursor.stack().back();

	if (arg.flags() & Reference::const_value) [[unlikely]] {
		error("invalid modification of constant value");
	}

	switch (arg.data().format()) {
	case Data::Format::none:
		error("invalid use of none value with operator '++'(0)");
	case Data::Format::null:
		cursor.raise(WeakReference(arg));
		break;
	case Data::Format::number:
		arg.move_data(create_number(arg.data<Number>().value + 1));
		break;
	case Data::Format::boolean:
		arg.move_data(create_boolean(arg.data<Boolean>().value + 1));
		break;
	case Data::Format::object:
		if (!call_overload(cursor, Class::inc_operator, 0)) [[unlikely]] {
			error("class '{}' doesn't overload operator '++'(0)", type_name(arg));
		}
		break;
	case Data::Format::package:
		error("invalid use of package '{}' with operator '++'(0)", arg.data<Package>().data.full_name());
	case Data::Format::coroutine:
		error("invalid use of coroutine with operator '++'(0)");
	case Data::Format::function:
		error("invalid use of '{}' type with operator '++'(0)", type_name(arg));
	}
}

void mint::dec_operator(Cursor& cursor) {

	auto& arg = cursor.stack().back();

	if (arg.flags() & Reference::const_value) [[unlikely]] {
		error("invalid modification of constant value");
	}

	switch (arg.data().format()) {
	case Data::Format::none:
		error("invalid use of none value with operator '--'(0)");
	case Data::Format::null:
		cursor.raise(WeakReference(arg));
		break;
	case Data::Format::number:
		arg.move_data(create_number(arg.data<Number>().value - 1));
		break;
	case Data::Format::boolean:
		arg.move_data(create_boolean(arg.data<Boolean>().value - 1));
		break;
	case Data::Format::object:
		if (!call_overload(cursor, Class::dec_operator, 0)) [[unlikely]] {
			error("class '{}' doesn't overload operator '--'(0)", type_name(arg));
		}
		break;
	case Data::Format::package:
		error("invalid use of package '{}' with operator '--'(0)", arg.data<Package>().data.full_name());
	case Data::Format::coroutine:
		error("invalid use of coroutine with operator '--'(0)");
	case Data::Format::function:
		std::string type = type_name(arg);
		error("invalid use of '{}' type with operator '--'(0)", type);
	}
}

void mint::not_operator(Cursor& cursor) {

	const auto& arg = cursor.stack().back();

	switch (arg.data().format()) {
	case Data::Format::none:
	case Data::Format::null:
		cursor.stack().back() = create_boolean(true);
		break;
	case Data::Format::number:
		cursor.stack().back() = create_boolean(arg.data<Number>().value == 0.);
		break;
	case Data::Format::boolean:
		cursor.stack().back() = create_boolean(!arg.data<Boolean>().value);
		break;
	case Data::Format::object:
		if (!call_overload(cursor, Class::not_operator, 0)) {
			cursor.stack().back() = create_boolean(!to_boolean(arg));
		}
		break;
	case Data::Format::package:
		error("invalid use of package '{}' with operator '!'(0)", arg.data<Package>().data.full_name());
	case Data::Format::coroutine:
		error("invalid use of coroutine with operator '!'(0)");
	case Data::Format::function:
		error("invalid use of '{}' type with operator '!'(0)", type_name(arg));
	}
}

void mint::compl_operator(Cursor& cursor) {

	const auto& arg = cursor.stack().back();

	switch (arg.data().format()) {
	case Data::Format::none:
		error("invalid use of none value with operator '~'(0)");
	case Data::Format::null:
		cursor.raise(WeakReference(arg));
		break;
	case Data::Format::number:
		cursor.stack().back() = create_signed_number(~to_signed_integer(cursor, arg));
		break;
	case Data::Format::boolean:
		cursor.stack().back() = create_boolean(!arg.data<Boolean>().value);
		break;
	case Data::Format::object:
		if (!call_overload(cursor, Class::compl_operator, 0)) [[unlikely]] {
			error("class '{}' doesn't overload operator '~'(0)", type_name(arg));
		}
		break;
	case Data::Format::package:
		error("invalid use of package '{}' with operator '~'(0)", arg.data<Package>().data.full_name());
	case Data::Format::coroutine:
		error("invalid use of coroutine with operator '~'(0)");
	case Data::Format::function:
		error("invalid use of '{}' type with operator '~'(0)", type_name(arg));
	}
}

void mint::pos_operator(Cursor& cursor) {

	const auto& arg = cursor.stack().back();

	switch (arg.data().format()) {
	case Data::Format::none:
		error("invalid use of none value with operator '+'(0)");
	case Data::Format::null:
		cursor.raise(WeakReference(arg));
		break;
	case Data::Format::number:
		if (arg.flags() & Reference::temporary) {
			arg.data<Number>().value = +(arg.data<Number>().value);
		}
		else {
			cursor.stack().back() = create_number(+(arg.data<Number>().value));
		}
		break;
	case Data::Format::boolean:
		if (arg.flags() & Reference::temporary) {
			arg.data<Boolean>().value = +(arg.data<Boolean>().value);
		}
		else {
			cursor.stack().back() = create_boolean(+(arg.data<Boolean>().value));
		}
		break;
	case Data::Format::object:
		if (!call_overload(cursor, Class::add_operator, 0)) [[unlikely]] {
			error("class '{}' doesn't overload operator '+'(0)", type_name(arg));
		}
		break;
	case Data::Format::package:
		error("invalid use of package '{}' with operator '+'(0)", arg.data<Package>().data.full_name());
	case Data::Format::coroutine:
		error("invalid use of coroutine with operator '+'(0)");
	case Data::Format::function:
		error("invalid use of '{}' type with operator '+'(0)", type_name(arg));
	}
}

void mint::neg_operator(Cursor& cursor) {

	const auto& arg = cursor.stack().back();

	switch (arg.data().format()) {
	case Data::Format::none:
		error("invalid use of none value with operator '-'(0)");
	case Data::Format::null:
		cursor.raise(WeakReference(arg));
		break;
	case Data::Format::number:
		if (arg.flags() & Reference::temporary) {
			arg.data<Number>().value = -(arg.data<Number>().value);
		}
		else {
			cursor.stack().back() = create_number(-(arg.data<Number>().value));
		}
		break;
	case Data::Format::boolean:
		if (arg.flags() & Reference::temporary) {
			arg.data<Boolean>().value = -(arg.data<Boolean>().value);
		}
		else {
			cursor.stack().back() = create_boolean(-(arg.data<Boolean>().value));
		}
		break;
	case Data::Format::object:
		if (!call_overload(cursor, Class::sub_operator, 0)) [[unlikely]] {
			error("class '{}' doesn't overload operator '-'(0)", type_name(arg));
		}
		break;
	case Data::Format::package:
		error("invalid use of package '{}' with operator '-'(0)", arg.data<Package>().data.full_name());
	case Data::Format::coroutine:
		error("invalid use of coroutine with operator '-'(0)");
	case Data::Format::function:
		error("invalid use of '{}' type with operator '-'(0)", type_name(arg));
	}
}

void mint::shift_left_operator(Cursor& cursor) {

	const auto base = get_stack_base(cursor);

	const auto& rhs = load_from_stack(cursor, base);
	const auto& lhs = load_from_stack(cursor, base - 1);

	switch (lhs.data().format()) {
	case Data::Format::none:
		error("invalid use of none value with operator '<<'(1)");
	case Data::Format::null:
		cursor.raise(WeakReference(lhs));
		break;
	case Data::Format::number:
		{
			Reference&& result = create_unsigned_number(
			    to_unsigned_integer(lhs.data<Number>().value) << to_unsigned_integer(cursor, rhs));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::Format::boolean:
		{
			Reference&& result = create_unsigned_number(
			    to_unsigned_integer(lhs.data<Boolean>().value) << to_unsigned_integer(cursor, rhs));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::Format::object:
		if (!call_overload(cursor, Class::shift_left_operator, 1)) [[unlikely]] {
			error("class '{}' doesn't overload operator '<<'(1)", type_name(lhs));
		}
		break;
	case Data::Format::package:
		error("invalid use of package '{}' with operator '<<'(1)", lhs.data<Package>().data.full_name());
	case Data::Format::coroutine:
		error("invalid use of coroutine with operator '<<'(1)");
	case Data::Format::function:
		error("invalid use of '{}' type with operator '<<'(1)", type_name(lhs));
	}
}

void mint::shift_right_operator(Cursor& cursor) {

	const auto base = get_stack_base(cursor);

	const auto& rhs = load_from_stack(cursor, base);
	const auto& lhs = load_from_stack(cursor, base - 1);

	switch (lhs.data().format()) {
	case Data::Format::none:
		error("invalid use of none value with operator '>>'(1)");
	case Data::Format::null:
		cursor.raise(WeakReference(lhs));
		break;
	case Data::Format::number:
		{
			Reference&& result = create_unsigned_number(
			    to_unsigned_integer(lhs.data<Number>().value) >> to_unsigned_integer(cursor, rhs));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::Format::boolean:
		{
			Reference&& result = create_boolean(lhs.data<Boolean>().value >> to_unsigned_integer(cursor, rhs));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::Format::object:
		if (!call_overload(cursor, Class::shift_right_operator, 1)) [[unlikely]] {
			error("class '{}' doesn't overload operator '>>'(1)", type_name(lhs));
		}
		break;
	case Data::Format::package:
		error("invalid use of package '{}' with operator '>>'(1)", lhs.data<Package>().data.full_name());
	case Data::Format::coroutine:
		error("invalid use of coroutine with operator '>>'(1)");
	case Data::Format::function:
		error("invalid use of '{}' type with operator '>>'(1)", type_name(lhs));
	}
}

void mint::inclusive_range_operator(Cursor& cursor) {

	const auto base = get_stack_base(cursor);

	const auto& rhs = load_from_stack(cursor, base);
	const auto& lhs = load_from_stack(cursor, base - 1);

	switch (lhs.data().format()) {
	case Data::Format::none:
		error("invalid use of none value with operator '..'(1)");
	case Data::Format::null:
		cursor.raise(WeakReference(lhs));
		break;
	case Data::Format::number:
		{
			auto result = create_iterator(from_inclusive_range, cursor.ast(), lhs.data<Number>().value,
			    to_number(cursor, rhs));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::Format::object:
		if (!call_overload(cursor, Class::inclusive_range_operator, 1)) [[unlikely]] {
			error("class '{}' doesn't overload operator '..'(1)", type_name(lhs));
		}
		break;
	case Data::Format::package:
		error("invalid use of package '{}' with operator '..'(1)", lhs.data<Package>().data.full_name());
	case Data::Format::coroutine:
		error("invalid use of coroutine with operator '..'(1)");
	case Data::Format::boolean:
	case Data::Format::function:
		error("invalid use of '{}' type with operator '..'(1)", type_name(lhs));
	}
}

void mint::exclusive_range_operator(Cursor& cursor) {

	const auto base = get_stack_base(cursor);

	const auto& rhs = load_from_stack(cursor, base);
	const auto& lhs = load_from_stack(cursor, base - 1);

	switch (lhs.data().format()) {
	case Data::Format::none:
		error("invalid use of none value with operator '...'(1)");
	case Data::Format::null:
		cursor.raise(WeakReference(lhs));
		break;
	case Data::Format::number:
		{
			auto result = create_iterator(from_exclusive_range, cursor.ast(), lhs.data<Number>().value,
			    to_number(cursor, rhs));
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::Format::object:
		if (!call_overload(cursor, Class::exclusive_range_operator, 1)) [[unlikely]] {
			error("class '{}' doesn't overload operator '...'(1)", type_name(lhs));
		}
		break;
	case Data::Format::package:
		error("invalid use of package '{}' with operator '...'(1)", lhs.data<Package>().data.full_name());
	case Data::Format::coroutine:
		error("invalid use of coroutine with operator '...'(1)");
	case Data::Format::boolean:
	case Data::Format::function:
		error("invalid use of '{}' type with operator '...'(1)", type_name(lhs));
	}
}

void mint::typeof_operator(Cursor& cursor) {
	cursor.stack().back() = create_string(cursor.ast(), type_name(std::forward<Reference>(cursor.stack().back())));
}

void mint::membersof_operator(Cursor& cursor) {

	auto& arg = cursor.stack().back();
	WeakReference result = create_array(cursor.ast());

	switch (arg.data().format()) {
	case Data::Format::object:
		{
			auto& object = arg.data<Object>();
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

	case Data::Format::package:
		{
			auto& package = arg.data<Package>();
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

	const auto& rhs = load_from_stack(cursor, base);
	const auto& lhs = load_from_stack(cursor, base - 1);

	switch (lhs.data().format()) {
	case Data::Format::none:
		error("invalid use of none value with operator '[]'(1)");
	case Data::Format::null:
		cursor.raise(WeakReference(lhs));
		break;
	case Data::Format::number:
		if (lhs.flags() & Reference::temporary) {
			lhs.data<Number>().value = to_number(
			    to_unsigned_integer(lhs.data<Number>().value / pow(decimal_base, to_number(cursor, rhs)))
			    % decimal_base);
			cursor.stack().pop_back();
		}
		else {
			WeakReference result = create_unsigned_number(
			    to_unsigned_integer(lhs.data<Number>().value / pow(decimal_base, to_number(cursor, rhs)))
			    % decimal_base);
			cursor.stack().pop_back();
			cursor.stack().back() = std::move(result);
		}
		break;
	case Data::Format::boolean:
		error("invalid use of '{}' type with operator '[]'(1)", type_name(lhs));
	case Data::Format::object:
		if (!call_overload(cursor, Class::subscript_operator, 1)) [[unlikely]] {
			error("class '{}' doesn't overload operator '[]'(1)", type_name(lhs));
		}
		break;
	case Data::Format::package:
		error("invalid use of package '{}' with operator '[]'(1)", lhs.data<Package>().data.full_name());
	case Data::Format::coroutine:
		error("invalid use of coroutine with operator '[]'(1)");
	case Data::Format::function:
		auto signature = lhs.data<Function>().mapping.find(to_integer<int>(cursor, rhs));
		if (signature != lhs.data<Function>().mapping.end()) {
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

	const auto& rhs = load_from_stack(cursor, base);
	const auto& kvalue = load_from_stack(cursor, base - 1);
	const auto& lhs = load_from_stack(cursor, base - 2);

	if (lhs.flags() & Reference::const_value) [[unlikely]] {
		error("invalid modification of constant value");
	}

	switch (lhs.data().format()) {
	case Data::Format::none:
		error("invalid use of none value with operator '[]='(2)");
	case Data::Format::null:
		cursor.raise(WeakReference(lhs));
		break;
	case Data::Format::number:
		lhs.data<Number>().value -= (to_number(to_unsigned_integer(lhs.data<Number>().value
		                                                           / pow(decimal_base, to_number(cursor, kvalue)))
		                                       % decimal_base)
		                             * pow(decimal_base, to_number(cursor, kvalue)));
		lhs.data<Number>().value += to_number(cursor, rhs) * pow(decimal_base, to_number(cursor, kvalue));
		cursor.stack().pop_back();
		cursor.stack().pop_back();
		break;
	case Data::Format::boolean:
		error("invalid use of '{}' type with operator '[]='(2)", type_name(lhs));
	case Data::Format::object:
		if (!call_overload(cursor, Class::subscript_move_operator, 2)) [[unlikely]] {
			error("class '{}' doesn't overload operator '[]='(2)", type_name(lhs));
		}
		break;
	case Data::Format::package:
		error("invalid use of package '{}' with operator '[]='(2)", lhs.data<Package>().data.full_name());
	case Data::Format::coroutine:
		error("invalid use of coroutine with operator '[]='(2)");
	case Data::Format::function:
		error("invalid use of '{}' type with operator '[]='(2)", type_name(lhs));
	}
}

void mint::regex_match(Cursor& cursor) {

	const auto base = get_stack_base(cursor);
	const auto& lhs = load_from_stack(cursor, base - 1);

	switch (lhs.data().format()) {
	case Data::Format::none:
		error("invalid use of none value with operator '=~'(1)");
	case Data::Format::null:
		cursor.raise(WeakReference(lhs));
		break;
	case Data::Format::object:
		if (!call_overload(cursor, Class::regex_match_operator, 1)) [[unlikely]] {
			error("class '{}' doesn't overload operator '=~'(1)", type_name(lhs));
		}
		break;
	case Data::Format::package:
		error("invalid use of package '{}' with operator '=~'(1)", lhs.data<Package>().data.full_name());
	case Data::Format::coroutine:
		error("invalid use of coroutine with operator '=~'(1)");
	case Data::Format::number:
	case Data::Format::boolean:
	case Data::Format::function:
		error("invalid use of '{}' type with operator '=~'(1)", type_name(lhs));
	}
}

void mint::regex_unmatch(Cursor& cursor) {

	const auto base = get_stack_base(cursor);
	const auto& lhs = load_from_stack(cursor, base - 1);

	switch (lhs.data().format()) {
	case Data::Format::none:
		error("invalid use of none value with operator '!~'(1)");
	case Data::Format::null:
		cursor.raise(WeakReference(lhs));
		break;
	case Data::Format::object:
		if (!call_overload(cursor, Class::regex_unmatch_operator, 1)) [[unlikely]] {
			error("class '{}' doesn't overload operator '!~'(1)", type_name(lhs));
		}
		break;
	case Data::Format::package:
		error("invalid use of package '{}' with operator '!~'(1)", lhs.data<Package>().data.full_name());
	case Data::Format::coroutine:
		error("invalid use of coroutine with operator '!~'(1)");
	case Data::Format::number:
	case Data::Format::boolean:
	case Data::Format::function:
		error("invalid use of '{}' type with operator '!~'(1)", type_name(lhs));
	}
}

void mint::strict_eq_operator(Cursor& cursor) {

	const auto base = get_stack_base(cursor);

	const auto& rhs = load_from_stack(cursor, base);
	auto& lhs = load_from_stack(cursor, base - 1);

	if (lhs.data().format() == rhs.data().format()) {
		switch (lhs.data().format()) {
		case Data::Format::none:
		case Data::Format::null:
			{
				cursor.stack().pop_back();
				cursor.stack().back() = create_boolean(true);
			}
			break;
		case Data::Format::number:
			{
				Reference&& result = create_boolean(lhs.data<Number>().value == rhs.data<Number>().value);
				cursor.stack().pop_back();
				cursor.stack().back() = std::move(result);
			}
			break;
		case Data::Format::boolean:
			{
				Reference&& result = create_boolean(lhs.data<Boolean>().value == rhs.data<Boolean>().value);
				cursor.stack().pop_back();
				cursor.stack().back() = std::move(result);
			}
			break;
		case Data::Format::object:
			if (!call_overload(cursor, Class::eq_operator, 1)) {
				error("class '{}' doesn't overload operator '=='(1)", type_name(lhs));
			}
			break;
		case Data::Format::package:
			error("invalid use of package '{}' with operator '=='(1)", lhs.data<Package>().data.full_name());
		case Data::Format::coroutine:
			error("invalid use of coroutine with operator '=='(1)");
		case Data::Format::function:
			{
				Reference&& result = create_boolean(lhs.data<Function>().mapping == rhs.data<Function>().mapping);
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

	const auto& rhs = load_from_stack(cursor, base);
	auto& lhs = load_from_stack(cursor, base - 1);

	if (lhs.data().format() == rhs.data().format()) {
		switch (lhs.data().format()) {
		case Data::Format::none:
		case Data::Format::null:
			{
				cursor.stack().pop_back();
				cursor.stack().back() = create_boolean(false);
			}
			break;
		case Data::Format::number:
			{
				Reference&& result = create_boolean(lhs.data<Number>().value != rhs.data<Number>().value);
				cursor.stack().pop_back();
				cursor.stack().back() = std::move(result);
			}
			break;
		case Data::Format::boolean:
			{
				Reference&& result = create_boolean(lhs.data<Boolean>().value != rhs.data<Boolean>().value);
				cursor.stack().pop_back();
				cursor.stack().back() = std::move(result);
			}
			break;
		case Data::Format::object:
			if (!call_overload(cursor, Class::ne_operator, 1)) {
				error("class '{}' doesn't overload operator '!='(1)", type_name(lhs));
			}
			break;
		case Data::Format::package:
			error("invalid use of package '{}' with operator '!='(1)", lhs.data<Package>().data.full_name());
		case Data::Format::coroutine:
			error("invalid use of coroutine with operator '!='(1)");
		case Data::Format::function:
			{
				Reference&& result = create_boolean(lhs.data<Function>().mapping != rhs.data<Function>().mapping);
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

	if (cursor.stack().back().data().format() != Data::Format::none) {

		const auto arg = std::move(cursor.stack().back());
		cursor.stack().pop_back();

		switch (arg.data().format()) {
		case Data::Format::package:
			{
				auto& package = arg.data<Package>();
				if (auto it = package.data.symbols().find(symbol); it != package.data.symbols().end()) {
					cursor.stack().emplace_back(it->second);
					return;
				}
			}

			cursor.stack().emplace_back(create_none());
			break;

		case Data::Format::object:
			{
				auto& object = arg.data<Object>();
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
	const auto arg = std::move(cursor.stack().back());
	cursor.stack().back() = create_boolean(arg.data().format() != Data::Format::none);
}

void mint::find_operator(Cursor& cursor) {

	const auto base = get_stack_base(cursor);

	const auto& range = load_from_stack(cursor, base);
	auto& arg = load_from_stack(cursor, base - 1);

	switch (range.data().format()) {
	case Data::Format::object:
		cursor.stack().emplace_back(arg);
		if (!call_overload(cursor, Class::in_operator, 1)) {
			cursor.stack().pop_back();
			cursor.stack().back() = create_iterator_over(cursor, range);
		}
		break;

	default:
		cursor.stack().back() = create_iterator_over(cursor, range);
		break;
	}
}

void mint::find_init(Cursor& cursor) {

	const auto& range = cursor.stack().back();

	if (range.data().format() != Data::Format::boolean) {
		cursor.stack().back() = create_iterator_over(cursor, range);
	}
}

void mint::find_next(Cursor& cursor) {

	const auto base = get_stack_base(cursor);

	const auto& range = load_from_stack(cursor, base);
	auto& arg = load_from_stack(cursor, base - 1);

	if (range.data().format() == Data::Format::boolean) {
		cursor.stack().emplace_back(range);
	}
	else {
		assert(is_instance_of(range, Class::Metatype::iterator));
		auto& iterator = range.data<Iterator>();
		if (std::optional<WeakReference>&& item = iterator_next(cursor, iterator)) {
			cursor.stack().emplace_back(arg);
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

	if (range.data().format() == Data::Format::boolean || to_boolean(found) || range.data<Iterator>().ctx.empty()) {
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
	if (is_instance_of(range, Data::Format::object)) {
		call_overload(cursor, Class::in_operator, 0);
	}
}

void mint::range_init(Cursor& cursor) {
	auto& range = cursor.stack().back();
	if (!is_instance_of(range, Class::Metatype::iterator)) {
		cursor.stack().back() = create_iterator_over(cursor, std::move(range));
	}
}

void mint::range_next(Cursor& cursor) {
	assert(is_instance_of(cursor.stack().back(), Class::Metatype::iterator));
	cursor.stack().back().data<Iterator>().ctx.next(cursor);
}

void mint::range_check(Cursor& cursor, std::size_t pos) {

	const auto base = get_stack_base(cursor);
	const auto& range = load_from_stack(cursor, base);
	auto& target = load_from_stack(cursor, base - 1);

	assert(is_instance_of(range, Class::Metatype::iterator));

	if (auto item = iterator_get(range.data<Iterator>())) {

		if ((target.flags() & Reference::const_address) && (target.data().format() != Data::Format::none))
		    [[unlikely]] {
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

		if (is_instance_of(*item, Class::Metatype::iterator)) {
			item->data<Iterator>().ctx.finalize(cursor);
		}

		for_each_if(cursor, *item, [&it, &end](const Reference& item) -> bool {
			if (it != end) {
				if ((it->flags() & Reference::const_address) && (it->data().format() != Data::Format::none))
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
	case Data::Format::none:
		return std::size_t {};

	case Data::Format::null:
#if (__cplusplus >= 201703L) || (defined(_MSC_VER) && _MSC_VER >= 1911)
		return std::hash<std::nullptr_t> {}(nullptr);
#else
		return std::hash<void*> {}(nullptr);
#endif

	case Data::Format::number:
		return std::hash<double> {}(value.data<Number>().value);

	case Data::Format::boolean:
		return std::hash<bool> {}(value.data<Boolean>().value);

	case Data::Format::object:
		switch (value.data<Object>().metadata.metatype()) {
		case Class::Metatype::object:
			return std::hash<WeakReference*> {}(value.data<Object>().data);

		case Class::Metatype::string:
			return std::hash<std::string> {}(value.data<String>().str);

		case Class::Metatype::regex:
			return std::hash<std::string> {}(value.data<Regex>().initializer);

		case Class::Metatype::array:
			return [this, &value] {
				std::size_t hash = offset_basis;
				for (auto i = value.data<Array>().values.begin(); i != value.data<Array>().values.end(); ++i) {
					hash = hash * fnv_prime;
					hash = hash ^ operator()(array_get_item(i));
				}
				return hash;
			}();

		case Class::Metatype::hash:
		case Class::Metatype::iterator:
		case Class::Metatype::library:
		case Class::Metatype::libobject:
			error("invalid use of '{}' type as hash key", type_name(value));
		}
		break;
	case Data::Format::package:
	case Data::Format::function:
	case Data::Format::coroutine:
		error("invalid use of '{}' type as hash key", type_name(value));
	}

	return false;
}

bool Hash::equal_to::operator()(const Hash::key_type& lhs, const Hash::key_type& rhs) const {

	if (lhs.data().format() != rhs.data().format()) {
		return false;
	}

	switch (lhs.data().format()) {
	case Data::Format::none:
	case Data::Format::null:
		return true;

	case Data::Format::number:
		return lhs.data<Number>().value == rhs.data<Number>().value;

	case Data::Format::boolean:
		return lhs.data<Boolean>().value == rhs.data<Boolean>().value;

	case Data::Format::object:
		if (lhs.data<Object>().metadata.metatype() != rhs.data<Object>().metadata.metatype()) {
			return false;
		}

		switch (lhs.data<Object>().metadata.metatype()) {
		case Class::Metatype::object:
			if (&lhs.data<Object>().metadata != &rhs.data<Object>().metadata) {
				return false;
			}
			return lhs.data<Object>().data == rhs.data<Object>().data;

		case Class::Metatype::string:
			return lhs.data<String>().str == rhs.data<String>().str;

		case Class::Metatype::regex:
			return lhs.data<Regex>().initializer == rhs.data<Regex>().initializer;

		case Class::Metatype::array:
			if (lhs.data<Array>().values.size() != rhs.data<Array>().values.size()) {
				return false;
			}
			for (auto i = lhs.data<Array>().values.begin(), j = rhs.data<Array>().values.begin();
			    i != lhs.data<Array>().values.end() && j != rhs.data<Array>().values.end(); ++i, ++j) {
				if (!operator()(array_get_item(i), array_get_item(j))) {
					return false;
				}
			}
			return true;

		case Class::Metatype::hash:
		case Class::Metatype::iterator:
		case Class::Metatype::library:
		case Class::Metatype::libobject:
			error("invalid use of '{}' type as hash key", type_name(lhs));
		}
		break;
	case Data::Format::package:
	case Data::Format::function:
	case Data::Format::coroutine:
		error("invalid use of '{}' type as hash key", type_name(lhs));
	}

	return false;
}

bool Hash::compare_to::operator()(const Hash::key_type& lhs, const Hash::key_type& rhs) const {

	if (lhs.data().format() != rhs.data().format()) {
		return lhs.data().format() < rhs.data().format();
	}

	switch (lhs.data().format()) {
	case Data::Format::none:
	case Data::Format::null:
		return false;

	case Data::Format::number:
		return lhs.data<Number>().value < rhs.data<Number>().value;

	case Data::Format::boolean:
		return lhs.data<Boolean>().value < rhs.data<Boolean>().value;

	case Data::Format::object:
		if (lhs.data<Object>().metadata.metatype() != rhs.data<Object>().metadata.metatype()) {
			return lhs.data<Object>().metadata.metatype() < rhs.data<Object>().metadata.metatype();
		}

		switch (lhs.data<Object>().metadata.metatype()) {
		case Class::Metatype::object:
			if (&lhs.data<Object>().metadata != &rhs.data<Object>().metadata) {
				return &lhs.data<Object>().metadata < &rhs.data<Object>().metadata;
			}
			return lhs.data<Object>().data < rhs.data<Object>().data;

		case Class::Metatype::string:
			return lhs.data<String>().str < rhs.data<String>().str;

		case Class::Metatype::regex:
			return lhs.data<Regex>().initializer < rhs.data<Regex>().initializer;

		case Class::Metatype::array:
			for (auto i = lhs.data<Array>().values.begin(), j = rhs.data<Array>().values.begin();
			    i != lhs.data<Array>().values.end() && j != rhs.data<Array>().values.end(); ++i, ++j) {
				if (operator()(array_get_item(i), array_get_item(j))) {
					return true;
				}
				if (operator()(array_get_item(j), array_get_item(i))) {
					return false;
				}
			}
			return lhs.data<Array>().values.size() < rhs.data<Array>().values.size();

		case Class::Metatype::hash:
		case Class::Metatype::iterator:
		case Class::Metatype::library:
		case Class::Metatype::libobject:
			error("invalid use of '{}' type as hash key", type_name(lhs));
		}
		break;
	case Data::Format::package:
	case Data::Format::function:
	case Data::Format::coroutine:
		error("invalid use of '{}' type as hash key", type_name(lhs));
	}

	return false;
}
