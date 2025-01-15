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

#include "mint/memory/operatortool.h"
#include "mint/memory/memorytool.h"
#include "mint/memory/casttool.h"
#include "mint/memory/algorithm.hpp"
#include "mint/memory/functiontool.h"
#include "mint/memory/globaldata.h"
#include "mint/memory/builtin/string.h"
#include "mint/memory/builtin/regex.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/ast/cursor.h"
#include "mint/system/error.h"

#include <functional>
#include <optional>
#include <cmath>

using namespace mint;

static void do_function_call(int signature, const Function::Signature &function, Class *metadata, Cursor *cursor) {

	cursor->call(function.handle, signature, metadata);

	if (function.capture) {
		SymbolTable &symbols = cursor->symbols();
		for (auto &item : *function.capture) {
			symbols.emplace(item.first, WeakReference::share(item.second));
		}
	}
}

bool mint::call_overload(Cursor *cursor, Class::Operator operator_overload, int signature) {

	assert(signature >= 0);

	const size_t base = get_stack_base(cursor);
	Object *object = load_from_stack(cursor, base - static_cast<size_t>(signature)).data<Object>();

	if (Class::MemberInfo *info = object->metadata->find_operator(operator_overload)) {

		if (UNLIKELY(is_class(object))) {
			error("invalid use of class '%s' in an operation", object->metadata->full_name().c_str());
		}

		Reference &function = Class::MemberInfo::get(info, object);
		Class *metadata = info->owner;

		switch (function.data()->format) {
		case Data::FMT_NONE:
			error("invalid use of none value as a function");
		case Data::FMT_NULL:
			cursor->raise(WeakReference::share(function));
			break;
		case Data::FMT_NUMBER:
		case Data::FMT_BOOLEAN:
		case Data::FMT_OBJECT:
			if (signature == 0) {
				cursor->stack().back() = WeakReference::clone(function.data());
			}
			else {
				error("%s copy doesn't take %d argument(s)", type_name(function).c_str(), signature);
			}
			break;
		case Data::FMT_PACKAGE:
			error("invalid use of package in an operation");
		case Data::FMT_FUNCTION:
			if (!(function.flags() & Reference::GLOBAL)) {
				// add self to function arguments
				signature += 1;
			}
			auto it = find_function_signature(cursor, function.data<Function>()->mapping, signature);
			if (UNLIKELY(it == function.data<Function>()->mapping.end())) {
				error("called member doesn't take %d parameter(s)", signature);
			}
			do_function_call(it->first, it->second, metadata, cursor);
			break;
		}

		return true;
	}

	return false;
}

bool mint::call_overload(Cursor *cursor, const Symbol &operator_overload, int signature) {

	assert(signature >= 0);

	const size_t base = get_stack_base(cursor);
	Object *object = load_from_stack(cursor, base - static_cast<size_t>(signature)).data<Object>();
	auto it = object->metadata->members().find(operator_overload);

	if (it != object->metadata->members().end()) {

		if (UNLIKELY(is_class(object))) {
			error("invalid use of class '%s' in an operation", object->metadata->full_name().c_str());
		}

		Reference &function = Class::MemberInfo::get(it->second, object);
		Class *metadata = it->second->owner;

		switch (function.data()->format) {
		case Data::FMT_NONE:
			error("invalid use of none value as a function");
		case Data::FMT_NULL:
			cursor->raise(WeakReference::share(function));
			break;
		case Data::FMT_NUMBER:
		case Data::FMT_BOOLEAN:
		case Data::FMT_OBJECT:
			if (signature == 0) {
				cursor->stack().back() = WeakReference::clone(function.data());
			}
			else {
				error("%s copy doesn't take %d argument(s)", type_name(function).c_str(), signature);
			}
			break;
		case Data::FMT_PACKAGE:
			error("invalid use of package in an operation");
		case Data::FMT_FUNCTION:
			if (!(function.flags() & Reference::GLOBAL)) {
				// add self to function arguments
				signature += 1;
			}
			auto it = find_function_signature(cursor, function.data<Function>()->mapping, signature);
			if (UNLIKELY(it == function.data<Function>()->mapping.end())) {
				error("called member doesn't take %d parameter(s)", signature);
			}
			do_function_call(it->first, it->second, metadata, cursor);
			break;
		}

		return true;
	}

	return false;
}

void mint::move_operator(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);

	Reference &rvalue = load_from_stack(cursor, base);
	Reference &lvalue = load_from_stack(cursor, base - 1);

	if (UNLIKELY((lvalue.flags() & Reference::CONST_ADDRESS) && (lvalue.data()->format != Data::FMT_NONE))) {
		error("invalid modification of constant reference");
	}

	if (lvalue.flags() & Reference::CONST_VALUE) {
		lvalue.move_data(rvalue);
	}
	else if ((rvalue.flags() & (Reference::CONST_VALUE | Reference::TEMPORARY)) == Reference::CONST_VALUE) {
		lvalue.copy_data(rvalue);
	}
	else {
		lvalue.move_data(rvalue);
	}

	cursor->stack().pop_back();
}

void mint::copy_operator(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);

	Reference &rvalue = load_from_stack(cursor, base);
	Reference &lvalue = load_from_stack(cursor, base - 1);

	if (UNLIKELY(lvalue.flags() & Reference::CONST_VALUE)) {
		error("invalid modification of constant value");
	}

	switch (lvalue.data()->format) {
	case Data::FMT_NONE:
		error("invalid use of none value in an operation");
	case Data::FMT_NULL:
		cursor->raise(WeakReference::share(lvalue));
		break;
	case Data::FMT_NUMBER:
		lvalue.data<Number>()->value = to_number(cursor, rvalue);
		cursor->stack().pop_back();
		break;
	case Data::FMT_BOOLEAN:
		lvalue.data<Boolean>()->value = to_boolean(cursor, rvalue);
		cursor->stack().pop_back();
		break;
	case Data::FMT_FUNCTION:
		if (UNLIKELY(rvalue.data()->format != Data::FMT_FUNCTION)) {
			error("invalid conversion from '%s' to '%s'", type_name(rvalue).c_str(), type_name(lvalue).c_str());
		}
		lvalue.data<Function>()->mapping = rvalue.data<Function>()->mapping;
		cursor->stack().pop_back();
		break;
	case Data::FMT_OBJECT:
		if (!call_overload(cursor, Class::COPY_OPERATOR, 1)) {
			if (UNLIKELY(rvalue.data()->format != Data::FMT_OBJECT)) {
				error("cannot convert '%s' to '%s' in assignment", type_name(rvalue).c_str(), type_name(lvalue).c_str());
			}
			if (UNLIKELY(lvalue.data<Object>()->metadata != rvalue.data<Object>()->metadata)) {
				error("cannot convert '%s' to '%s' in assignment", type_name(rvalue).c_str(), type_name(lvalue).c_str());
			}
			delete[] lvalue.data<Object>()->data;
			lvalue.data<Object>()->construct(*rvalue.data<Object>());
		}
		break;
	case Data::FMT_PACKAGE:
		error("invalid use of package in an operation");
	}
}

void mint::call_operator(Cursor *cursor, int signature) {

	Cursor::Call call = std::move(cursor->waiting_calls().top());
	cursor->waiting_calls().pop();

	Cursor::Call::Flags flags = call.get_flags();
	Reference &function = call.function();
	Class *metadata = call.get_metadata();
	signature += call.extra_argument_count();

	switch (function.data()->format) {
	case Data::FMT_NONE:
		if (LIKELY(flags & Cursor::Call::MEMBER_CALL)) {
			if (UNLIKELY(signature)) {
				error("default constructors doesn't take %d argument(s)", signature);
			}
		}
		else {
			error("invalid use of none value as a function");
		}
		break;
	case Data::FMT_NULL:
		cursor->raise(WeakReference::share(function));
		break;
	case Data::FMT_NUMBER:
	case Data::FMT_BOOLEAN:
	case Data::FMT_OBJECT:
		if (signature == 0) {
			cursor->stack().emplace_back(WeakReference::clone(function.data()));
		}
		else {
			error("%s copy doesn't take %d argument(s)", type_name(function).c_str(), signature);
		}
		break;
	case Data::FMT_PACKAGE:
		error("invalid use of package in an operation");
	case Data::FMT_FUNCTION:
		if (flags & Cursor::Call::MEMBER_CALL) {
			// add self to function arguments
			signature += 1;
		}
		auto it = find_function_signature(cursor, function.data<Function>()->mapping, signature);
		if (UNLIKELY(it == function.data<Function>()->mapping.end())) {
			error("called function doesn't take %d parameter(s)", signature);
		}
		do_function_call(it->first, it->second, metadata, cursor);
		break;
	}
}

void mint::call_member_operator(Cursor *cursor, int signature) {

	Cursor::Call call = std::move(cursor->waiting_calls().top());
	cursor->waiting_calls().pop();

	Cursor::Call::Flags flags = call.get_flags();
	Reference &function = call.function();
	Class *metadata = call.get_metadata();
	signature += call.extra_argument_count();

	switch (function.data()->format) {
	case Data::FMT_NONE:
		if (LIKELY(flags & Cursor::Call::MEMBER_CALL)) {
			if (UNLIKELY(signature)) {
				error("default constructors doesn't take %d argument(s)", signature);
			}
		}
		else {
			error("invalid use of none value as a function");
		}
		break;
	case Data::FMT_NULL:
		cursor->raise(WeakReference::share(function));
		break;
	case Data::FMT_NUMBER:
	case Data::FMT_BOOLEAN:
	case Data::FMT_OBJECT:
		if (signature == 0) {
			cursor->stack().back() = WeakReference::clone(function.data());
		}
		else {
			error("%s copy doesn't take %d argument(s)", type_name(function).c_str(), signature);
		}
		break;
	case Data::FMT_PACKAGE:
		error("invalid use of package in an operation");
	case Data::FMT_FUNCTION:
		if (!(function.flags() & Reference::GLOBAL)) {
			// add self to function arguments
			signature += 1;
		}
		auto it = find_function_signature(cursor, function.data<Function>()->mapping, signature);
		if (UNLIKELY(it == function.data<Function>()->mapping.end())) {
			error("called member doesn't take %d parameter(s)", signature);
		}
		do_function_call(it->first, it->second, metadata, cursor);
		break;
	}
}

void mint::add_operator(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);

	Reference &rvalue = load_from_stack(cursor, base);
	Reference &lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data()->format) {
	case Data::FMT_NONE:
		error("invalid use of none value in an operation");
	case Data::FMT_NULL:
		cursor->raise(WeakReference::share(lvalue));
		break;
	case Data::FMT_NUMBER:
		if (lvalue.flags() & Reference::TEMPORARY) {
			lvalue.data<Number>()->value += to_number(cursor, rvalue);
			cursor->stack().pop_back();
		}
		else {
			Reference &&result = WeakReference::create<Number>(lvalue.data<Number>()->value + to_number(cursor, rvalue));
			cursor->stack().pop_back();
			cursor->stack().back() = std::move(result);
		}
		break;
	case Data::FMT_BOOLEAN:
		if (lvalue.flags() & Reference::TEMPORARY) {
			lvalue.data<Boolean>()->value += to_boolean(cursor, rvalue);
			cursor->stack().pop_back();
		}
		else {
			Reference &&result = WeakReference::create<Boolean>(lvalue.data<Boolean>()->value
																+ to_boolean(cursor, rvalue));
			cursor->stack().pop_back();
			cursor->stack().back() = std::move(result);
		}
		break;
	case Data::FMT_OBJECT:
		if (UNLIKELY(!call_overload(cursor, Class::ADD_OPERATOR, 1))) {
			error("class '%s' doesn't overload operator '+'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::FMT_PACKAGE:
		error("invalid use of package in an operation");
	case Data::FMT_FUNCTION:
		{
			Reference &&result = WeakReference::create<Function>();
			if (UNLIKELY(rvalue.data()->format != Data::FMT_FUNCTION)) {
				error("invalid use of operator '+' with '%s' and '%s' types", type_name(lvalue).c_str(),
					  type_name(rvalue).c_str());
			}
			for (const auto &item : lvalue.data<Function>()->mapping) {
				result.data<Function>()->mapping.insert(item);
			}
			for (const auto &item : rvalue.data<Function>()->mapping) {
				result.data<Function>()->mapping.insert(item);
			}
			cursor->stack().pop_back();
			cursor->stack().back() = std::move(result);
		}
		break;
	}
}

void mint::sub_operator(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);

	Reference &rvalue = load_from_stack(cursor, base);
	Reference &lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data()->format) {
	case Data::FMT_NONE:
		error("invalid use of none value in an operation");
	case Data::FMT_NULL:
		cursor->raise(WeakReference::share(lvalue));
		break;
	case Data::FMT_NUMBER:
		if (lvalue.flags() & Reference::TEMPORARY) {
			lvalue.data<Number>()->value -= to_number(cursor, rvalue);
			cursor->stack().pop_back();
		}
		else {
			Reference &&result = WeakReference::create<Number>(lvalue.data<Number>()->value - to_number(cursor, rvalue));
			cursor->stack().pop_back();
			cursor->stack().back() = std::move(result);
		}
		break;
	case Data::FMT_BOOLEAN:
		if (lvalue.flags() & Reference::TEMPORARY) {
			lvalue.data<Boolean>()->value -= to_boolean(cursor, rvalue);
			cursor->stack().pop_back();
		}
		else {
			Reference &&result = WeakReference::create<Boolean>(lvalue.data<Boolean>()->value
																- to_boolean(cursor, rvalue));
			cursor->stack().pop_back();
			cursor->stack().back() = std::move(result);
		}
		break;
	case Data::FMT_OBJECT:
		if (UNLIKELY(!call_overload(cursor, Class::SUB_OPERATOR, 1))) {
			error("class '%s' doesn't overload operator '-'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::FMT_PACKAGE:
		error("invalid use of package in an operation");
	case Data::FMT_FUNCTION:
		error("invalid use of '%s' type with operator '-'", type_name(lvalue).c_str());
	}
}

void mint::mul_operator(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);

	Reference &rvalue = load_from_stack(cursor, base);
	Reference &lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data()->format) {
	case Data::FMT_NONE:
		error("invalid use of none value in an operation");
	case Data::FMT_NULL:
		cursor->raise(WeakReference::share(lvalue));
		break;
	case Data::FMT_NUMBER:
		if (lvalue.flags() & Reference::TEMPORARY) {
			lvalue.data<Number>()->value *= to_number(cursor, rvalue);
			cursor->stack().pop_back();
		}
		else {
			Reference &&result = WeakReference::create<Number>(lvalue.data<Number>()->value * to_number(cursor, rvalue));
			cursor->stack().pop_back();
			cursor->stack().back() = std::move(result);
		}
		break;
	case Data::FMT_BOOLEAN:
		if (lvalue.flags() & Reference::TEMPORARY) {
			lvalue.data<Boolean>()->value = lvalue.data<Boolean>()->value && to_boolean(cursor, rvalue);
			cursor->stack().pop_back();
		}
		else {
			Reference &&result = WeakReference::create<Boolean>(lvalue.data<Boolean>()->value
																&& to_boolean(cursor, rvalue));
			cursor->stack().pop_back();
			cursor->stack().back() = std::move(result);
		}
		break;
	case Data::FMT_OBJECT:
		if (UNLIKELY(!call_overload(cursor, Class::MUL_OPERATOR, 1))) {
			error("class '%s' doesn't overload operator '*'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::FMT_PACKAGE:
		error("invalid use of package in an operation");
	case Data::FMT_FUNCTION:
		error("invalid use of '%s' type with operator '*'", type_name(lvalue).c_str());
	}
}

void mint::div_operator(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);

	Reference &rvalue = load_from_stack(cursor, base);
	Reference &lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data()->format) {
	case Data::FMT_NONE:
		error("invalid use of none value in an operation");
	case Data::FMT_NULL:
		cursor->raise(WeakReference::share(lvalue));
		break;
	case Data::FMT_NUMBER:
		if (lvalue.flags() & Reference::TEMPORARY) {
			lvalue.data<Number>()->value /= to_number(cursor, rvalue);
			cursor->stack().pop_back();
		}
		else {
			Reference &&result = WeakReference::create<Number>(lvalue.data<Number>()->value / to_number(cursor, rvalue));
			cursor->stack().pop_back();
			cursor->stack().back() = std::move(result);
		}
		break;
	case Data::FMT_BOOLEAN:
		if (lvalue.flags() & Reference::TEMPORARY) {
			lvalue.data<Boolean>()->value /= to_boolean(cursor, rvalue);
			cursor->stack().pop_back();
		}
		else {
			Reference &&result = WeakReference::create<Boolean>(lvalue.data<Boolean>()->value
																/ to_boolean(cursor, rvalue));
			cursor->stack().pop_back();
			cursor->stack().back() = std::move(result);
		}
		break;
	case Data::FMT_OBJECT:
		if (UNLIKELY(!call_overload(cursor, Class::DIV_OPERATOR, 1))) {
			error("class '%s' doesn't overload operator '/'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::FMT_PACKAGE:
		error("invalid use of package in an operation");
	case Data::FMT_FUNCTION:
		error("invalid use of '%s' type with operator '/'", type_name(lvalue).c_str());
	}
}

void mint::pow_operator(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);

	Reference &rvalue = load_from_stack(cursor, base);
	Reference &lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data()->format) {
	case Data::FMT_NONE:
		error("invalid use of none value in an operation");
	case Data::FMT_NULL:
		cursor->raise(WeakReference::share(lvalue));
		break;
	case Data::FMT_NUMBER:
		if (lvalue.flags() & Reference::TEMPORARY) {
			lvalue.data<Number>()->value = pow(lvalue.data<Number>()->value, to_number(cursor, rvalue));
			cursor->stack().pop_back();
		}
		else {
			Reference &&result = WeakReference::create<Number>(
				pow(lvalue.data<Number>()->value, to_number(cursor, rvalue)));
			cursor->stack().pop_back();
			cursor->stack().back() = std::move(result);
		}
		break;
	case Data::FMT_OBJECT:
		if (UNLIKELY(!call_overload(cursor, Class::POW_OPERATOR, 1))) {
			error("class '%s' doesn't overload operator '**'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::FMT_PACKAGE:
		error("invalid use of package in an operation");
	case Data::FMT_BOOLEAN:
	case Data::FMT_FUNCTION:
		error("invalid use of '%s' type with operator '**'", type_name(lvalue).c_str());
	}
}

void mint::mod_operator(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);

	Reference &rvalue = load_from_stack(cursor, base);
	Reference &lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data()->format) {
	case Data::FMT_NONE:
		error("invalid use of none value in an operation");
	case Data::FMT_NULL:
		cursor->raise(WeakReference::share(lvalue));
		break;
	case Data::FMT_NUMBER:
		if (intmax_t divider = to_integer(cursor, rvalue)) {
			if (lvalue.flags() & Reference::TEMPORARY) {
				lvalue.data<Number>()->value = static_cast<double>(to_integer(lvalue.data<Number>()->value) % divider);
				cursor->stack().pop_back();
			}
			else {
				Reference &&result = WeakReference::create<Number>(
					static_cast<double>(to_integer(lvalue.data<Number>()->value) % divider));
				cursor->stack().pop_back();
				cursor->stack().back() = std::move(result);
			}
		}
		else {
			error("modulo by zero");
		}
		break;
	case Data::FMT_OBJECT:
		if (UNLIKELY(!call_overload(cursor, Class::MOD_OPERATOR, 1))) {
			error("class '%s' doesn't overload operator '%%'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::FMT_PACKAGE:
		error("invalid use of package in an operation");
	case Data::FMT_BOOLEAN:
	case Data::FMT_FUNCTION:
		error("invalid use of '%s' type with operator '%%'", type_name(lvalue).c_str());
	}
}

void mint::is_operator(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);

	const Reference &rvalue = load_from_stack(cursor, base);
	const Reference &lvalue = load_from_stack(cursor, base - 1);

	WeakReference result = WeakReference::create<Boolean>(lvalue.data() == rvalue.data());
	cursor->stack().pop_back();
	cursor->stack().back() = std::move(result);
}

void mint::eq_operator(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);

	Reference &rvalue = load_from_stack(cursor, base);
	Reference &lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data()->format) {
	case Data::FMT_NONE:
		{
			Reference &&result = WeakReference::create<Boolean>(rvalue.data()->format == Data::FMT_NONE);
			cursor->stack().pop_back();
			cursor->stack().back() = std::move(result);
		}
		break;
	case Data::FMT_NULL:
		{
			Reference &&result = WeakReference::create<Boolean>(rvalue.data()->format == Data::FMT_NULL);
			cursor->stack().pop_back();
			cursor->stack().back() = std::move(result);
		}
		break;
	case Data::FMT_NUMBER:
		switch (rvalue.data()->format) {
		case Data::FMT_NONE:
		case Data::FMT_NULL:
			{
				Reference &&result = WeakReference::create<Boolean>(false);
				cursor->stack().pop_back();
				cursor->stack().back() = std::move(result);
			}
			break;
		default:
			Reference &&result = WeakReference::create<Boolean>(lvalue.data<Number>()->value
																== to_number(cursor, rvalue));
			cursor->stack().pop_back();
			cursor->stack().back() = std::move(result);
		}
		break;
	case Data::FMT_BOOLEAN:
		switch (rvalue.data()->format) {
		case Data::FMT_NONE:
		case Data::FMT_NULL:
			{
				Reference &&result = WeakReference::create<Boolean>(false);
				cursor->stack().pop_back();
				cursor->stack().back() = std::move(result);
			}
			break;
		default:
			Reference &&result = WeakReference::create<Boolean>(lvalue.data<Boolean>()->value
																== to_boolean(cursor, rvalue));
			cursor->stack().pop_back();
			cursor->stack().back() = std::move(result);
		}
		break;
	case Data::FMT_OBJECT:
		if (!call_overload(cursor, Class::EQ_OPERATOR, 1)) {
			switch (rvalue.data()->format) {
			case Data::FMT_NONE:
			case Data::FMT_NULL:
				{
					Reference &&result = WeakReference::create<Boolean>(false);
					cursor->stack().pop_back();
					cursor->stack().back() = std::move(result);
				}
				break;
			default:
				error("class '%s' doesn't overload operator '=='(1)", type_name(lvalue).c_str());
			}
		}
		break;
	case Data::FMT_PACKAGE:
		error("invalid use of package in an operation");
	case Data::FMT_FUNCTION:
		if (rvalue.data()->format == Data::FMT_FUNCTION) {
			Reference &&result = WeakReference::create<Boolean>(lvalue.data<Function>()->mapping
																== rvalue.data<Function>()->mapping);
			cursor->stack().pop_back();
			cursor->stack().back() = std::move(result);
		}
		else {
			error("invalid use of '%s' type with operator '=='", type_name(lvalue).c_str());
		}
	}
}

void mint::ne_operator(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);

	Reference &rvalue = load_from_stack(cursor, base);
	Reference &lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data()->format) {
	case Data::FMT_NONE:
		{
			Reference &&result = WeakReference::create<Boolean>(rvalue.data()->format != Data::FMT_NONE);
			cursor->stack().pop_back();
			cursor->stack().back() = std::move(result);
		}
		break;
	case Data::FMT_NULL:
		{
			Reference &&result = WeakReference::create<Boolean>(rvalue.data()->format != Data::FMT_NULL);
			cursor->stack().pop_back();
			cursor->stack().back() = std::move(result);
		}
		break;
	case Data::FMT_NUMBER:
		switch (rvalue.data()->format) {
		case Data::FMT_NONE:
		case Data::FMT_NULL:
			{
				Reference &&result = WeakReference::create<Boolean>(true);
				cursor->stack().pop_back();
				cursor->stack().back() = std::move(result);
			}
			break;
		default:
			Reference &&result = WeakReference::create<Boolean>(lvalue.data<Number>()->value
																!= to_number(cursor, rvalue));
			cursor->stack().pop_back();
			cursor->stack().back() = std::move(result);
		}
		break;
	case Data::FMT_BOOLEAN:
		switch (rvalue.data()->format) {
		case Data::FMT_NONE:
		case Data::FMT_NULL:
			{
				Reference &&result = WeakReference::create<Boolean>(true);
				cursor->stack().pop_back();
				cursor->stack().back() = std::move(result);
			}
			break;
		default:
			Reference &&result = WeakReference::create<Boolean>(lvalue.data<Boolean>()->value
																!= to_boolean(cursor, rvalue));
			cursor->stack().pop_back();
			cursor->stack().back() = std::move(result);
		}
		break;
	case Data::FMT_OBJECT:
		if (!call_overload(cursor, Class::NE_OPERATOR, 1)) {
			switch (rvalue.data()->format) {
			case Data::FMT_NONE:
			case Data::FMT_NULL:
				{
					Reference &&result = WeakReference::create<Boolean>(true);
					cursor->stack().pop_back();
					cursor->stack().back() = std::move(result);
				}
				break;
			default:
				error("class '%s' doesn't overload operator '!='(1)", type_name(lvalue).c_str());
			}
		}
		break;
	case Data::FMT_PACKAGE:
		error("invalid use of package in an operation");
	case Data::FMT_FUNCTION:
		if (rvalue.data()->format == Data::FMT_FUNCTION) {
			Reference &&result = WeakReference::create<Boolean>(lvalue.data<Function>()->mapping
																!= rvalue.data<Function>()->mapping);
			cursor->stack().pop_back();
			cursor->stack().back() = std::move(result);
		}
		else {
			error("invalid use of '%s' type with operator '!='", type_name(lvalue).c_str());
		}
	}
}

void mint::lt_operator(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);

	Reference &rvalue = load_from_stack(cursor, base);
	Reference &lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data()->format) {
	case Data::FMT_NONE:
		error("invalid use of none value in an operation");
	case Data::FMT_NULL:
		cursor->raise(WeakReference::share(lvalue));
		break;
	case Data::FMT_NUMBER:
		{
			Reference &&result = WeakReference::create<Boolean>(lvalue.data<Number>()->value
																< to_number(cursor, rvalue));
			cursor->stack().pop_back();
			cursor->stack().back() = std::move(result);
		}
		break;
	case Data::FMT_BOOLEAN:
		{
			Reference &&result = WeakReference::create<Boolean>(lvalue.data<Boolean>()->value
																< to_boolean(cursor, rvalue));
			cursor->stack().pop_back();
			cursor->stack().back() = std::move(result);
		}
		break;
	case Data::FMT_OBJECT:
		if (UNLIKELY(!call_overload(cursor, Class::LT_OPERATOR, 1))) {
			error("class '%s' doesn't overload operator '<'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::FMT_PACKAGE:
		error("invalid use of package in an operation");
	case Data::FMT_FUNCTION:
		error("invalid use of '%s' type with operator '<'", type_name(lvalue).c_str());
	}
}

void mint::gt_operator(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);

	Reference &rvalue = load_from_stack(cursor, base);
	Reference &lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data()->format) {
	case Data::FMT_NONE:
		error("invalid use of none value in an operation");
	case Data::FMT_NULL:
		cursor->raise(WeakReference::share(lvalue));
		break;
	case Data::FMT_NUMBER:
		{
			Reference &&result = WeakReference::create<Boolean>(lvalue.data<Number>()->value
																> to_number(cursor, rvalue));
			cursor->stack().pop_back();
			cursor->stack().back() = std::move(result);
		}
		break;
	case Data::FMT_BOOLEAN:
		{
			Reference &&result = WeakReference::create<Boolean>(lvalue.data<Boolean>()->value
																> to_boolean(cursor, rvalue));
			cursor->stack().pop_back();
			cursor->stack().back() = std::move(result);
		}
		break;
	case Data::FMT_OBJECT:
		if (UNLIKELY(!call_overload(cursor, Class::GT_OPERATOR, 1))) {
			error("class '%s' doesn't overload operator '>'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::FMT_PACKAGE:
		error("invalid use of package in an operation");
	case Data::FMT_FUNCTION:
		error("invalid use of '%s' type with operator '>'", type_name(lvalue).c_str());
	}
}

void mint::le_operator(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);

	Reference &rvalue = load_from_stack(cursor, base);
	Reference &lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data()->format) {
	case Data::FMT_NONE:
		error("invalid use of none value in an operation");
	case Data::FMT_NULL:
		cursor->raise(WeakReference::share(lvalue));
		break;
	case Data::FMT_NUMBER:
		{
			Reference &&result = WeakReference::create<Boolean>(lvalue.data<Number>()->value
																<= to_number(cursor, rvalue));
			cursor->stack().pop_back();
			cursor->stack().back() = std::move(result);
		}
		break;
	case Data::FMT_BOOLEAN:
		{
			Reference &&result = WeakReference::create<Boolean>(lvalue.data<Boolean>()->value
																<= to_boolean(cursor, rvalue));
			cursor->stack().pop_back();
			cursor->stack().back() = std::move(result);
		}
		break;
	case Data::FMT_OBJECT:
		if (UNLIKELY(!call_overload(cursor, Class::LE_OPERATOR, 1))) {
			error("class '%s' doesn't overload operator '<='(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::FMT_PACKAGE:
		error("invalid use of package in an operation");
	case Data::FMT_FUNCTION:
		error("invalid use of '%s' type with operator '<='", type_name(lvalue).c_str());
	}
}

void mint::ge_operator(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);

	Reference &rvalue = load_from_stack(cursor, base);
	Reference &lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data()->format) {
	case Data::FMT_NONE:
		error("invalid use of none value in an operation");
	case Data::FMT_NULL:
		cursor->raise(WeakReference::share(lvalue));
		break;
	case Data::FMT_NUMBER:
		{
			Reference &&result = WeakReference::create<Boolean>(lvalue.data<Number>()->value
																>= to_number(cursor, rvalue));
			cursor->stack().pop_back();
			cursor->stack().back() = std::move(result);
		}
		break;
	case Data::FMT_BOOLEAN:
		{
			Reference &&result = WeakReference::create<Boolean>(lvalue.data<Boolean>()->value
																>= to_boolean(cursor, rvalue));
			cursor->stack().pop_back();
			cursor->stack().back() = std::move(result);
		}
		break;
	case Data::FMT_OBJECT:
		if (UNLIKELY(!call_overload(cursor, Class::GE_OPERATOR, 1))) {
			error("class '%s' doesn't overload operator '>='(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::FMT_PACKAGE:
		error("invalid use of package in an operation");
	case Data::FMT_FUNCTION:
		error("invalid use of '%s' type with operator '>='", type_name(lvalue).c_str());
	}
}

void mint::and_pre_check(Cursor *cursor, size_t pos) {

	Reference &value = cursor->stack().back();

	switch (value.data()->format) {
	case Data::FMT_NONE:
	case Data::FMT_NULL:
		cursor->jmp(pos);
		break;
	case Data::FMT_NUMBER:
		if (value.data<Number>()->value == 0.) {
			cursor->jmp(pos);
		}
		break;
	case Data::FMT_BOOLEAN:
		if (!value.data<Boolean>()->value) {
			cursor->jmp(pos);
		}
		break;
	case Data::FMT_OBJECT:
		switch (value.data<Object>()->metadata->metatype()) {
		case Class::ITERATOR:
			if (value.data<Iterator>()->ctx.empty()) {
				cursor->jmp(pos);
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

void mint::and_operator(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);

	WeakReference &rvalue = load_from_stack(cursor, base);
	WeakReference &lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data()->format) {
	case Data::FMT_OBJECT:
		if (!call_overload(cursor, Class::AND_OPERATOR, 1)) {
			std::swap(lvalue, rvalue);
			cursor->stack().pop_back();
		}
		break;
	default:
		std::swap(lvalue, rvalue);
		cursor->stack().pop_back();
	}
}

void mint::or_pre_check(Cursor *cursor, size_t pos) {

	Reference &value = cursor->stack().back();

	switch (value.data()->format) {
	case Data::FMT_NONE:
	case Data::FMT_NULL:
		break;
	case Data::FMT_NUMBER:
		if (value.data<Number>()->value != 0.) {
			cursor->jmp(pos);
		}
		break;
	case Data::FMT_BOOLEAN:
		if (value.data<Boolean>()->value) {
			cursor->jmp(pos);
		}
		break;
	case Data::FMT_OBJECT:
		switch (value.data<Object>()->metadata->metatype()) {
		case Class::ITERATOR:
			if (!value.data<Iterator>()->ctx.empty()) {
				cursor->jmp(pos);
			}
			break;
		default:
			if (!value.data<Object>()->metadata->find_operator(Class::OR_OPERATOR)) {
				cursor->jmp(pos);
			}
			break;
		}
		break;
	default:
		cursor->jmp(pos);
		break;
	}
}

void mint::or_operator(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);

	WeakReference &rvalue = load_from_stack(cursor, base);
	WeakReference &lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data()->format) {
	case Data::FMT_OBJECT:
		if (!call_overload(cursor, Class::OR_OPERATOR, 1)) {
			std::swap(lvalue, rvalue);
			cursor->stack().pop_back();
		}
		break;
	default:
		std::swap(lvalue, rvalue);
		cursor->stack().pop_back();
	}
}

void mint::band_operator(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);

	Reference &rvalue = load_from_stack(cursor, base);
	Reference &lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data()->format) {
	case Data::FMT_NONE:
		error("invalid use of none value in an operation");
	case Data::FMT_NULL:
		cursor->raise(WeakReference::share(lvalue));
		break;
	case Data::FMT_NUMBER:
		if (lvalue.flags() & Reference::TEMPORARY) {
			lvalue.data<Number>()->value = static_cast<double>(to_integer(lvalue.data<Number>()->value)
															   & to_integer(cursor, rvalue));
			cursor->stack().pop_back();
		}
		else {
			Reference &&result = WeakReference::create<Number>(
				static_cast<double>(to_integer(lvalue.data<Number>()->value) & to_integer(cursor, rvalue)));
			cursor->stack().pop_back();
			cursor->stack().back() = std::move(result);
		}
		break;
	case Data::FMT_BOOLEAN:
		if (lvalue.flags() & Reference::TEMPORARY) {
			lvalue.data<Boolean>()->value &= to_boolean(cursor, rvalue);
			cursor->stack().pop_back();
		}
		else {
			Reference &&result = WeakReference::create<Boolean>(lvalue.data<Boolean>()->value
																&& to_boolean(cursor, rvalue));
			cursor->stack().pop_back();
			cursor->stack().back() = std::move(result);
		}
		break;
	case Data::FMT_OBJECT:
		if (UNLIKELY(!call_overload(cursor, Class::BAND_OPERATOR, 1))) {
			error("class '%s' doesn't overload operator '&'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::FMT_PACKAGE:
		error("invalid use of package in an operation");
	case Data::FMT_FUNCTION:
		error("invalid use of '%s' type with operator '&'", type_name(lvalue).c_str());
	}
}

void mint::bor_operator(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);

	Reference &rvalue = load_from_stack(cursor, base);
	Reference &lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data()->format) {
	case Data::FMT_NONE:
		error("invalid use of none value in an operation");
	case Data::FMT_NULL:
		cursor->raise(WeakReference::share(lvalue));
		break;
	case Data::FMT_NUMBER:
		if (lvalue.flags() & Reference::TEMPORARY) {
			lvalue.data<Number>()->value = static_cast<double>(to_integer(lvalue.data<Number>()->value)
															   | to_integer(cursor, rvalue));
			cursor->stack().pop_back();
		}
		else {
			Reference &&result = WeakReference::create<Number>(
				static_cast<double>(to_integer(lvalue.data<Number>()->value) | to_integer(cursor, rvalue)));
			cursor->stack().pop_back();
			cursor->stack().back() = std::move(result);
		}
		break;
	case Data::FMT_BOOLEAN:
		if (lvalue.flags() & Reference::TEMPORARY) {
			lvalue.data<Boolean>()->value |= to_boolean(cursor, rvalue);
			cursor->stack().pop_back();
		}
		else {
			Reference &&result = WeakReference::create<Boolean>(lvalue.data<Boolean>()->value
																|| to_boolean(cursor, rvalue));
			cursor->stack().pop_back();
			cursor->stack().back() = std::move(result);
		}
		break;
	case Data::FMT_OBJECT:
		if (UNLIKELY(!call_overload(cursor, Class::BOR_OPERATOR, 1))) {
			error("class '%s' doesn't overload operator '|'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::FMT_PACKAGE:
		error("invalid use of package in an operation");
	case Data::FMT_FUNCTION:
		error("invalid use of '%s' type with operator '|'", type_name(lvalue).c_str());
	}
}

void mint::xor_operator(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);

	Reference &rvalue = load_from_stack(cursor, base);
	Reference &lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data()->format) {
	case Data::FMT_NONE:
		error("invalid use of none value in an operation");
	case Data::FMT_NULL:
		cursor->raise(WeakReference::share(lvalue));
		break;
	case Data::FMT_NUMBER:
		if (lvalue.flags() & Reference::TEMPORARY) {
			lvalue.data<Number>()->value = static_cast<double>(to_integer(lvalue.data<Number>()->value)
															   ^ to_integer(cursor, rvalue));
			cursor->stack().pop_back();
		}
		else {
			Reference &&result = WeakReference::create<Number>(
				static_cast<double>(to_integer(lvalue.data<Number>()->value) ^ to_integer(cursor, rvalue)));
			cursor->stack().pop_back();
			cursor->stack().back() = std::move(result);
		}
		break;
	case Data::FMT_BOOLEAN:
		if (lvalue.flags() & Reference::TEMPORARY) {
			lvalue.data<Boolean>()->value ^= to_boolean(cursor, rvalue);
			cursor->stack().pop_back();
		}
		else {
			Reference &&result = WeakReference::create<Boolean>(to_integer(lvalue.data<Number>()->value)
																^ to_boolean(cursor, rvalue));
			cursor->stack().pop_back();
			cursor->stack().back() = std::move(result);
		}
		break;
	case Data::FMT_OBJECT:
		if (UNLIKELY(!call_overload(cursor, Class::XOR_OPERATOR, 1))) {
			error("class '%s' doesn't overload operator '^'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::FMT_PACKAGE:
		error("invalid use of package in an operation");
	case Data::FMT_FUNCTION:
		error("invalid use of '%s' type with operator '^'", type_name(lvalue).c_str());
	}
}

void mint::inc_operator(Cursor *cursor) {

	Reference &value = cursor->stack().back();

	if (UNLIKELY(value.flags() & Reference::CONST_VALUE)) {
		error("invalid modification of constant value");
	}

	switch (value.data()->format) {
	case Data::FMT_NONE:
		error("invalid use of none value in an operation");
	case Data::FMT_NULL:
		cursor->raise(WeakReference::share(value));
		break;
	case Data::FMT_NUMBER:
		value.move_data(WeakReference::create<Number>(value.data<Number>()->value + 1));
		break;
	case Data::FMT_BOOLEAN:
		value.move_data(WeakReference::create<Boolean>(value.data<Boolean>()->value + 1));
		break;
	case Data::FMT_OBJECT:
		if (UNLIKELY(!call_overload(cursor, Class::INC_OPERATOR, 0))) {
			error("class '%s' doesn't overload operator '++'(0)", type_name(value).c_str());
		}
		break;
	case Data::FMT_PACKAGE:
		error("invalid use of package in an operation");
	case Data::FMT_FUNCTION:
		error("invalid use of '%s' type with operator '++'", type_name(value).c_str());
	}
}

void mint::dec_operator(Cursor *cursor) {

	Reference &value = cursor->stack().back();

	if (UNLIKELY(value.flags() & Reference::CONST_VALUE)) {
		error("invalid modification of constant value");
	}

	switch (value.data()->format) {
	case Data::FMT_NONE:
		error("invalid use of none value in an operation");
	case Data::FMT_NULL:
		cursor->raise(WeakReference::share(value));
		break;
	case Data::FMT_NUMBER:
		value.move_data(WeakReference::create<Number>(value.data<Number>()->value - 1));
		break;
	case Data::FMT_BOOLEAN:
		value.move_data(WeakReference::create<Boolean>(value.data<Boolean>()->value - 1));
		break;
	case Data::FMT_OBJECT:
		if (UNLIKELY(!call_overload(cursor, Class::DEC_OPERATOR, 0))) {
			error("class '%s' doesn't overload operator '--'(0)", type_name(value).c_str());
		}
		break;
	case Data::FMT_PACKAGE:
		error("invalid use of package in an operation");
	case Data::FMT_FUNCTION:
		std::string type = type_name(value);
		error("invalid use of '%s' type with operator '--'", type.c_str());
	}
}

void mint::not_operator(Cursor *cursor) {

	Reference &value = cursor->stack().back();

	switch (value.data()->format) {
	case Data::FMT_NONE:
	case Data::FMT_NULL:
		cursor->stack().back() = WeakReference::create<Boolean>(true);
		break;
	case Data::FMT_NUMBER:
		cursor->stack().back() = WeakReference::create<Boolean>(value.data<Number>()->value == 0.);
		break;
	case Data::FMT_BOOLEAN:
		cursor->stack().back() = WeakReference::create<Boolean>(!value.data<Boolean>()->value);
		break;
	case Data::FMT_OBJECT:
		if (!call_overload(cursor, Class::NOT_OPERATOR, 0)) {
			cursor->stack().back() = WeakReference::create<Boolean>(!to_boolean(cursor, value));
		}
		break;
	case Data::FMT_PACKAGE:
		error("invalid use of package in an operation");
	case Data::FMT_FUNCTION:
		error("invalid use of '%s' type with operator '!'", type_name(value).c_str());
	}
}

void mint::compl_operator(Cursor *cursor) {

	Reference &value = cursor->stack().back();

	switch (value.data()->format) {
	case Data::FMT_NONE:
		error("invalid use of none value in an operation");
	case Data::FMT_NULL:
		cursor->raise(WeakReference::share(value));
		break;
	case Data::FMT_NUMBER:
		cursor->stack().back() = WeakReference::create<Number>(static_cast<double>(~(to_integer(cursor, value))));
		break;
	case Data::FMT_BOOLEAN:
		cursor->stack().back() = WeakReference::create<Boolean>(!value.data<Boolean>()->value);
		break;
	case Data::FMT_OBJECT:
		if (UNLIKELY(!call_overload(cursor, Class::COMPL_OPERATOR, 0))) {
			error("class '%s' doesn't overload operator '~'(0)", type_name(value).c_str());
		}
		break;
	case Data::FMT_PACKAGE:
		error("invalid use of package in an operation");
	case Data::FMT_FUNCTION:
		error("invalid use of '%s' type with operator '~'", type_name(value).c_str());
	}
}

void mint::pos_operator(Cursor *cursor) {

	Reference &value = cursor->stack().back();

	switch (value.data()->format) {
	case Data::FMT_NONE:
		error("invalid use of none value in an operation");
	case Data::FMT_NULL:
		cursor->raise(WeakReference::share(value));
		break;
	case Data::FMT_NUMBER:
		if (value.flags() & Reference::TEMPORARY) {
			value.data<Number>()->value = +(value.data<Number>()->value);
		}
		else {
			cursor->stack().back() = WeakReference::create<Number>(+(value.data<Number>()->value));
		}
		break;
	case Data::FMT_BOOLEAN:
		if (value.flags() & Reference::TEMPORARY) {
			value.data<Boolean>()->value = +(value.data<Boolean>()->value);
		}
		else {
			cursor->stack().back() = WeakReference::create<Boolean>(+(value.data<Boolean>()->value));
		}
		break;
	case Data::FMT_OBJECT:
		if (UNLIKELY(!call_overload(cursor, Class::ADD_OPERATOR, 0))) {
			error("class '%s' doesn't overload operator '+'(0)", type_name(value).c_str());
		}
		break;
	case Data::FMT_PACKAGE:
		error("invalid use of package in an operation");
	case Data::FMT_FUNCTION:
		error("invalid use of '%s' type with operator '+'", type_name(value).c_str());
	}
}

void mint::neg_operator(Cursor *cursor) {

	Reference &value = cursor->stack().back();

	switch (value.data()->format) {
	case Data::FMT_NONE:
		error("invalid use of none value in an operation");
	case Data::FMT_NULL:
		cursor->raise(WeakReference::share(value));
		break;
	case Data::FMT_NUMBER:
		if (value.flags() & Reference::TEMPORARY) {
			value.data<Number>()->value = -(value.data<Number>()->value);
		}
		else {
			cursor->stack().back() = WeakReference::create<Number>(-(value.data<Number>()->value));
		}
		break;
	case Data::FMT_BOOLEAN:
		if (value.flags() & Reference::TEMPORARY) {
			value.data<Boolean>()->value = -(value.data<Boolean>()->value);
		}
		else {
			cursor->stack().back() = WeakReference::create<Boolean>(-(value.data<Boolean>()->value));
		}
		break;
	case Data::FMT_OBJECT:
		if (UNLIKELY(!call_overload(cursor, Class::SUB_OPERATOR, 0))) {
			error("class '%s' doesn't overload operator '-'(0)", type_name(value).c_str());
		}
		break;
	case Data::FMT_PACKAGE:
		error("invalid use of package in an operation");
	case Data::FMT_FUNCTION:
		error("invalid use of '%s' type with operator '-'", type_name(value).c_str());
	}
}

void mint::shift_left_operator(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);

	Reference &rvalue = load_from_stack(cursor, base);
	Reference &lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data()->format) {
	case Data::FMT_NONE:
		error("invalid use of none value in an operation");
	case Data::FMT_NULL:
		cursor->raise(WeakReference::share(lvalue));
		break;
	case Data::FMT_NUMBER:
		{
			Reference &&result = WeakReference::create<Number>(
				static_cast<double>(to_integer(lvalue.data<Number>()->value) << to_integer(cursor, rvalue)));
			cursor->stack().pop_back();
			cursor->stack().back() = std::move(result);
		}
		break;
	case Data::FMT_BOOLEAN:
		{
			Reference &&result = WeakReference::create<Number>(lvalue.data<Boolean>()->value
															   << to_integer(cursor, rvalue));
			cursor->stack().pop_back();
			cursor->stack().back() = std::move(result);
		}
		break;
	case Data::FMT_OBJECT:
		if (UNLIKELY(!call_overload(cursor, Class::SHIFT_LEFT_OPERATOR, 1))) {
			error("class '%s' doesn't overload operator '<<'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::FMT_PACKAGE:
		error("invalid use of package in an operation");
	case Data::FMT_FUNCTION:
		error("invalid use of '%s' type with operator '<<'", type_name(lvalue).c_str());
	}
}

void mint::shift_right_operator(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);

	Reference &rvalue = load_from_stack(cursor, base);
	Reference &lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data()->format) {
	case Data::FMT_NONE:
		error("invalid use of none value in an operation");
	case Data::FMT_NULL:
		cursor->raise(WeakReference::share(lvalue));
		break;
	case Data::FMT_NUMBER:
		{
			Reference &&result = WeakReference::create<Number>(
				static_cast<double>(to_integer(lvalue.data<Number>()->value) >> to_integer(cursor, rvalue)));
			cursor->stack().pop_back();
			cursor->stack().back() = std::move(result);
		}
		break;
	case Data::FMT_BOOLEAN:
		{
			Reference &&result = WeakReference::create<Boolean>(lvalue.data<Boolean>()->value
																>> to_integer(cursor, rvalue));
			cursor->stack().pop_back();
			cursor->stack().back() = std::move(result);
		}
		break;
	case Data::FMT_OBJECT:
		if (UNLIKELY(!call_overload(cursor, Class::SHIFT_RIGHT_OPERATOR, 1))) {
			error("class '%s' doesn't overload operator '>>'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::FMT_PACKAGE:
		error("invalid use of package in an operation");
	case Data::FMT_FUNCTION:
		error("invalid use of '%s' type with operator '>>'", type_name(lvalue).c_str());
	}
}

void mint::inclusive_range_operator(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);

	Reference &rvalue = load_from_stack(cursor, base);
	Reference &lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data()->format) {
	case Data::FMT_NONE:
		error("invalid use of none value in an operation");
	case Data::FMT_NULL:
		cursor->raise(WeakReference::share(lvalue));
		break;
	case Data::FMT_NUMBER:
		{
			Reference &&result = Iterator::fromInclusiveRange(lvalue.data<Number>()->value, to_number(cursor, rvalue));
			cursor->stack().pop_back();
			cursor->stack().back() = std::move(result);
		}
		break;
	case Data::FMT_OBJECT:
		if (UNLIKELY(!call_overload(cursor, Class::INCLUSIVE_RANGE_OPERATOR, 1))) {
			error("class '%s' doesn't overload operator '..'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::FMT_PACKAGE:
		error("invalid use of package in an operation");
	case Data::FMT_BOOLEAN:
	case Data::FMT_FUNCTION:
		error("invalid use of '%s' type with operator '..'", type_name(lvalue).c_str());
	}
}

void mint::exclusive_range_operator(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);

	Reference &rvalue = load_from_stack(cursor, base);
	Reference &lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data()->format) {
	case Data::FMT_NONE:
		error("invalid use of none value in an operation");
	case Data::FMT_NULL:
		cursor->raise(WeakReference::share(lvalue));
		break;
	case Data::FMT_NUMBER:
		{
			Reference &&result = Iterator::fromExclusiveRange(lvalue.data<Number>()->value, to_number(cursor, rvalue));
			cursor->stack().pop_back();
			cursor->stack().back() = std::move(result);
		}
		break;
	case Data::FMT_OBJECT:
		if (UNLIKELY(!call_overload(cursor, Class::EXCLUSIVE_RANGE_OPERATOR, 1))) {
			error("class '%s' doesn't overload operator '...'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::FMT_PACKAGE:
		error("invalid use of package in an operation");
	case Data::FMT_BOOLEAN:
	case Data::FMT_FUNCTION:
		error("invalid use of '%s' type with operator '...'", type_name(lvalue).c_str());
	}
}

void mint::typeof_operator(Cursor *cursor) {
	cursor->stack().back() = create_string(type_name(std::forward<Reference>(cursor->stack().back())));
}

void mint::membersof_operator(Cursor *cursor) {

	Reference &value = cursor->stack().back();
	WeakReference result = create_array();

	switch (value.data()->format) {
	case Data::FMT_OBJECT:
		if (Object *object = value.data<Object>()) {
			Array *array = result.data<Array>();
			array->values.reserve(object->metadata->members().size());
			for (const auto &member : object->metadata->members()) {

				switch (member.second->value.flags() & Reference::VISIBILITY_MASK) {
				case Reference::PROTECTED_VISIBILITY:
					if (!is_protected_accessible(member.second->owner, cursor->symbols().get_metadata())) {
						continue;
					}
					break;
				case Reference::PRIVATE_VISIBILITY:
					if (member.second->owner != cursor->symbols().get_metadata()) {
						continue;
					}
					break;
				case Reference::PACKAGE_VISIBILITY:
					if (member.second->owner->get_package() != cursor->symbols().get_package()) {
						continue;
					}
					break;
				}
				array_append(array, create_string(member.first.str()));
			}
		}
		break;

	case Data::FMT_PACKAGE:
		if (Package *package = value.data<Package>()) {
			Array *array = result.data<Array>();
			array->values.reserve(package->data->symbols().size());
			for (auto &symbol : package->data->symbols()) {
				array_append(array, create_string(symbol.first.str()));
			}
		}
		break;

	default:
		break;
	}

	cursor->stack().back() = std::move(result);
}

void mint::subscript_operator(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);

	Reference &rvalue = load_from_stack(cursor, base);
	Reference &lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data()->format) {
	case Data::FMT_NONE:
		error("invalid use of none value in an operation");
	case Data::FMT_NULL:
		cursor->raise(WeakReference::share(lvalue));
		break;
	case Data::FMT_NUMBER:
		if (lvalue.flags() & Reference::TEMPORARY) {
			lvalue.data<Number>()->value = static_cast<double>(
				to_integer(lvalue.data<Number>()->value / pow(10, to_number(cursor, rvalue))) % 10);
			cursor->stack().pop_back();
		}
		else {
			WeakReference result = WeakReference::create<Number>(static_cast<double>(
				to_integer(lvalue.data<Number>()->value / pow(10, to_number(cursor, rvalue))) % 10));
			cursor->stack().pop_back();
			cursor->stack().back() = std::move(result);
		}
		break;
	case Data::FMT_BOOLEAN:
		error("invalid use of '%s' type with operator '[]'", type_name(lvalue).c_str());
	case Data::FMT_OBJECT:
		if (UNLIKELY(!call_overload(cursor, Class::SUBSCRIPT_OPERATOR, 1))) {
			error("class '%s' doesn't overload operator '[]'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::FMT_PACKAGE:
		error("invalid use of package in an operation");
	case Data::FMT_FUNCTION:
		auto signature = lvalue.data<Function>()->mapping.find(static_cast<int>(to_integer(cursor, rvalue)));
		if (signature != lvalue.data<Function>()->mapping.end()) {
			Reference &&result = WeakReference::create<Function>();
			result.data<Function>()->mapping.insert(*signature);
			cursor->stack().pop_back();
			cursor->stack().back() = std::move(result);
		}
		else {
			cursor->stack().pop_back();
			cursor->stack().back() = WeakReference::create<None>();
		}
		break;
	}
}

void mint::subscript_move_operator(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);

	Reference &rvalue = load_from_stack(cursor, base);
	Reference &kvalue = load_from_stack(cursor, base - 1);
	Reference &lvalue = load_from_stack(cursor, base - 2);

	if (UNLIKELY(lvalue.flags() & Reference::CONST_VALUE)) {
		error("invalid modification of constant value");
	}

	switch (lvalue.data()->format) {
	case Data::FMT_NONE:
		error("invalid use of none value in an operation");
	case Data::FMT_NULL:
		cursor->raise(WeakReference::share(lvalue));
		break;
	case Data::FMT_NUMBER:
		lvalue.data<Number>()->value -=
			(static_cast<double>(to_integer(lvalue.data<Number>()->value / pow(10, to_number(cursor, kvalue))) % 10)
			 * pow(10, to_number(cursor, kvalue)));
		lvalue.data<Number>()->value += to_number(cursor, rvalue) * pow(10, to_number(cursor, kvalue));
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		break;
	case Data::FMT_BOOLEAN:
		error("invalid use of '%s' type with operator '[]='", type_name(lvalue).c_str());
	case Data::FMT_OBJECT:
		if (UNLIKELY(!call_overload(cursor, Class::SUBSCRIPT_MOVE_OPERATOR, 2))) {
			error("class '%s' doesn't overload operator '[]='(2)", type_name(lvalue).c_str());
		}
		break;
	case Data::FMT_PACKAGE:
		error("invalid use of package in an operation");
	case Data::FMT_FUNCTION:
		error("invalid use of '%s' type with operator '[]='", type_name(lvalue).c_str());
	}
}

void mint::regex_match(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);
	Reference &lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data()->format) {
	case Data::FMT_NONE:
		error("invalid use of none value in an operation");
	case Data::FMT_NULL:
		cursor->raise(WeakReference::share(lvalue));
		break;
	case Data::FMT_OBJECT:
		if (UNLIKELY(!call_overload(cursor, Class::REGEX_MATCH_OPERATOR, 1))) {
			error("class '%s' doesn't overload operator '=~'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::FMT_PACKAGE:
		error("invalid use of package in an operation");
	case Data::FMT_NUMBER:
	case Data::FMT_BOOLEAN:
	case Data::FMT_FUNCTION:
		error("invalid use of '%s' type with operator '=~'", type_name(lvalue).c_str());
	}
}

void mint::regex_unmatch(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);
	Reference &lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data()->format) {
	case Data::FMT_NONE:
		error("invalid use of none value in an operation");
	case Data::FMT_NULL:
		cursor->raise(WeakReference::share(lvalue));
		break;
	case Data::FMT_OBJECT:
		if (UNLIKELY(!call_overload(cursor, Class::REGEX_UNMATCH_OPERATOR, 1))) {
			error("class '%s' doesn't overload operator '!~'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::FMT_PACKAGE:
		error("invalid use of package in an operation");
	case Data::FMT_NUMBER:
	case Data::FMT_BOOLEAN:
	case Data::FMT_FUNCTION:
		error("invalid use of '%s' type with operator '!~'", type_name(lvalue).c_str());
	}
}

void mint::strict_eq_operator(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);

	Reference &rvalue = load_from_stack(cursor, base);
	Reference &lvalue = load_from_stack(cursor, base - 1);

	if (lvalue.data()->format == rvalue.data()->format) {
		switch (lvalue.data()->format) {
		case Data::FMT_NONE:
		case Data::FMT_NULL:
			{
				cursor->stack().pop_back();
				cursor->stack().back() = WeakReference::create<Boolean>(true);
			}
			break;
		case Data::FMT_NUMBER:
			{
				Reference &&result = WeakReference::create<Boolean>(lvalue.data<Number>()->value
																	== rvalue.data<Number>()->value);
				cursor->stack().pop_back();
				cursor->stack().back() = std::move(result);
			}
			break;
		case Data::FMT_BOOLEAN:
			{
				Reference &&result = WeakReference::create<Boolean>(lvalue.data<Boolean>()->value
																	== rvalue.data<Boolean>()->value);
				cursor->stack().pop_back();
				cursor->stack().back() = std::move(result);
			}
			break;
		case Data::FMT_OBJECT:
			if (!call_overload(cursor, Class::EQ_OPERATOR, 1)) {
				error("class '%s' doesn't overload operator '=='(1)", type_name(lvalue).c_str());
			}
			break;
		case Data::FMT_PACKAGE:
			error("invalid use of package in an operation");
		case Data::FMT_FUNCTION:
			{
				Reference &&result = WeakReference::create<Boolean>(lvalue.data<Function>()->mapping
																	== rvalue.data<Function>()->mapping);
				cursor->stack().pop_back();
				cursor->stack().back() = std::move(result);
			}
		}
	}
	else {
		cursor->stack().pop_back();
		cursor->stack().back() = WeakReference::create<Boolean>(false);
	}
}

void mint::strict_ne_operator(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);

	Reference &rvalue = load_from_stack(cursor, base);
	Reference &lvalue = load_from_stack(cursor, base - 1);

	if (lvalue.data()->format == rvalue.data()->format) {
		switch (lvalue.data()->format) {
		case Data::FMT_NONE:
		case Data::FMT_NULL:
			{
				cursor->stack().pop_back();
				cursor->stack().back() = WeakReference::create<Boolean>(false);
			}
			break;
		case Data::FMT_NUMBER:
			{
				Reference &&result = WeakReference::create<Boolean>(lvalue.data<Number>()->value
																	!= rvalue.data<Number>()->value);
				cursor->stack().pop_back();
				cursor->stack().back() = std::move(result);
			}
			break;
		case Data::FMT_BOOLEAN:
			{
				Reference &&result = WeakReference::create<Boolean>(lvalue.data<Boolean>()->value
																	!= rvalue.data<Boolean>()->value);
				cursor->stack().pop_back();
				cursor->stack().back() = std::move(result);
			}
			break;
		case Data::FMT_OBJECT:
			if (!call_overload(cursor, Class::NE_OPERATOR, 1)) {
				error("class '%s' doesn't overload operator '!='(1)", type_name(lvalue).c_str());
			}
			break;
		case Data::FMT_PACKAGE:
			error("invalid use of package in an operation");
		case Data::FMT_FUNCTION:
			{
				Reference &&result = WeakReference::create<Boolean>(lvalue.data<Function>()->mapping
																	!= rvalue.data<Function>()->mapping);
				cursor->stack().pop_back();
				cursor->stack().back() = std::move(result);
			}
		}
	}
	else {
		cursor->stack().pop_back();
		cursor->stack().back() = WeakReference::create<Boolean>(true);
	}
}

void mint::find_defined_symbol(Cursor *cursor, const Symbol &symbol) {

	if (auto it = cursor->symbols().find(symbol); it != cursor->symbols().end()) {
		cursor->stack().emplace_back(WeakReference::share(it->second));
		return;
	}

	GlobalData *globalData = GlobalData::instance();
	if (auto it = globalData->symbols().find(symbol); it != globalData->symbols().end()) {
		cursor->stack().emplace_back(WeakReference::share(it->second));
		return;
	}

	cursor->stack().emplace_back(WeakReference::create<None>());
}

void mint::find_defined_member(Cursor *cursor, const Symbol &symbol) {

	if (cursor->stack().back().data()->format != Data::FMT_NONE) {

		WeakReference value = std::move(cursor->stack().back());
		cursor->stack().pop_back();

		switch (value.data()->format) {
		case Data::FMT_PACKAGE:
			if (Package *package = value.data<Package>()) {

				if (auto it = package->data->symbols().find(symbol); it != package->data->symbols().end()) {
					cursor->stack().emplace_back(WeakReference::share(it->second));
					return;
				}
			}

			cursor->stack().emplace_back(WeakReference::create<None>());
			break;

		case Data::FMT_OBJECT:
			if (Object *object = value.data<Object>()) {

				if (auto it = object->metadata->members().find(symbol); it != object->metadata->members().end()) {
					cursor->stack().emplace_back(WeakReference::share(Class::MemberInfo::get(it->second, object)));
					return;
				}

				if (auto it = object->metadata->globals().find(symbol); it != object->metadata->globals().end()) {
					cursor->stack().emplace_back(WeakReference::share(it->second->value));
					return;
				}
			}

			cursor->stack().emplace_back(WeakReference::create<None>());
			break;

		default:
			cursor->stack().emplace_back(WeakReference::create<None>());
			break;
		}
	}
}

void mint::check_defined(Cursor *cursor) {
	WeakReference value = std::move(cursor->stack().back());
	cursor->stack().back() = WeakReference::create<Boolean>(value.data()->format != Data::FMT_NONE);
}

void mint::find_operator(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);

	Reference &range = load_from_stack(cursor, base);
	Reference &value = load_from_stack(cursor, base - 1);

	switch (range.data()->format) {
	case Data::FMT_OBJECT:
		cursor->stack().emplace_back(WeakReference::share(value));
		if (!call_overload(cursor, Class::IN_OPERATOR, 1)) {
			cursor->stack().pop_back();
			cursor->stack().back() = WeakReference::create(iterator_init(range));
		}
		break;

	default:
		cursor->stack().back() = WeakReference::create(iterator_init(range));
		break;
	}
}

void mint::find_init(Cursor *cursor) {

	Reference &range = cursor->stack().back();

	if (range.data()->format != Data::FMT_BOOLEAN) {
		cursor->stack().back() = WeakReference::create(iterator_init(range));
	}
}

void mint::find_next(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);

	Reference &range = load_from_stack(cursor, base);
	Reference &value = load_from_stack(cursor, base - 1);

	if (range.data()->format == Data::FMT_BOOLEAN) {
		cursor->stack().emplace_back(WeakReference::create(range.data()));
	}
	else {
		Iterator *iterator = range.data<Iterator>();
		assert(iterator != nullptr);
		if (std::optional<WeakReference> &&item = iterator_next(iterator)) {
			cursor->stack().emplace_back(WeakReference::share(value));
			cursor->stack().emplace_back(WeakReference::share(*item));
			eq_operator(cursor);
		}
		else {
			cursor->stack().emplace_back(WeakReference::create<Boolean>(false));
		}
	}
}

void mint::find_check(Cursor *cursor, size_t pos) {

	const size_t base = get_stack_base(cursor);

	WeakReference found = move_from_stack(cursor, base);
	Reference &range = load_from_stack(cursor, base - 1);

	if (range.data()->format == Data::FMT_BOOLEAN) {
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().back() = std::move(found);
		cursor->jmp(pos);
	}
	else if (to_boolean(cursor, found)) {
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().back() = std::move(found);
		cursor->jmp(pos);
	}
	else if (range.data<Iterator>()->ctx.empty()) {
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().back() = std::move(found);
		cursor->jmp(pos);
	}
	else {
		cursor->stack().pop_back();
	}
}

void mint::in_operator(Cursor *cursor) {

	const Reference &range = cursor->stack().back();

	if (is_instance_of(range, Data::FMT_OBJECT)) {
		call_overload(cursor, Class::IN_OPERATOR, 0);
	}
}

void mint::range_init(Cursor *cursor) {

	Reference &range = cursor->stack().back();

	if (!is_instance_of(range, Class::ITERATOR)) {
		cursor->stack().back() = WeakReference::create(iterator_init(std::forward<Reference>(range)));
	}
}

void mint::range_next(Cursor *cursor) {
	assert(is_instance_of(cursor->stack().back(), Class::ITERATOR));
	cursor->stack().back().data<Iterator>()->ctx.pop();
}

void mint::range_check(Cursor *cursor, size_t pos) {

	const size_t base = get_stack_base(cursor);

	Reference &range = load_from_stack(cursor, base);
	Reference &target = load_from_stack(cursor, base - 1);

	if (std::optional<WeakReference> &&item = iterator_get(range.data<Iterator>())) {

		if (UNLIKELY((target.flags() & Reference::CONST_ADDRESS) && (target.data()->format != Data::FMT_NONE))) {
			error("invalid modification of constant reference");
		}

		if ((item->flags() & (Reference::CONST_VALUE | Reference::TEMPORARY)) == Reference::CONST_VALUE) {
			target.copy_data(*item);
		}
		else {
			target.move_data(*item);
		}
	}
	else {
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->jmp(pos);
	}
}

void mint::range_iterator_check(Cursor *cursor, size_t pos) {

	const size_t base = get_stack_base(cursor);

	Reference &range = load_from_stack(cursor, base);
	Reference &target = load_from_stack(cursor, base - 1);

	if (std::optional<WeakReference> &&item = iterator_get(range.data<Iterator>())) {

		Iterator::ctx_type::iterator it = target.data<Iterator>()->ctx.begin();
		const Iterator::ctx_type::iterator end = target.data<Iterator>()->ctx.end();

		if (is_instance_of(*item, Class::ITERATOR)) {
			item->data<Iterator>()->ctx.finalize();
		}

		for_each_if(*item, [&it, &end](const Reference &item) -> bool {
			if (it != end) {
				if (UNLIKELY((it->flags() & Reference::CONST_ADDRESS) && (it->data()->format != Data::FMT_NONE))) {
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
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->jmp(pos);
	}
}

namespace mint {
#if !defined(__x86_64__) && !defined(_WIN64)
static constexpr const size_t FNV_PRIME = 16777619u;
static constexpr const size_t OFFSET_BASIS = 2166136261u;
#else
static constexpr const size_t FNV_PRIME = 1099511628211u;
static constexpr const size_t OFFSET_BASIS = 14695981039346656037u;
#endif
}

size_t Hash::hash::operator()(const Hash::key_type &value) const {

	switch (value.data()->format) {
	case Data::FMT_NONE:
		return size_t {};

	case Data::FMT_NULL:
#if (__cplusplus >= 201703L) || (defined(_MSC_VER) && _MSC_VER >= 1911)
		return std::hash<std::nullptr_t> {}(nullptr);
#else
		return std::hash<void *> {}(nullptr);
#endif

	case Data::FMT_NUMBER:
		return std::hash<double> {}(value.data<Number>()->value);

	case Data::FMT_BOOLEAN:
		return std::hash<bool> {}(value.data<Boolean>()->value);

	case Data::FMT_OBJECT:
		switch (value.data<Object>()->metadata->metatype()) {
		case Class::OBJECT:
			return std::hash<WeakReference *> {}(value.data<Object>()->data);

		case Class::STRING:
			return std::hash<std::string> {}(value.data<String>()->str);

		case Class::REGEX:
			return std::hash<std::string> {}(value.data<Regex>()->initializer);

		case Class::ARRAY:
			return [this, &value] {
				size_t hash = OFFSET_BASIS;
				for (auto i = value.data<Array>()->values.begin(); i != value.data<Array>()->values.end(); ++i) {
					hash = hash * FNV_PRIME;
					hash = hash ^ operator()(array_get_item(i));
				}
				return hash;
			}();

		case Class::HASH:
		case Class::ITERATOR:
		case Class::LIBRARY:
		case Class::LIBOBJECT:
			error("invalid use of '%s' type as hash key", type_name(value).c_str());
		}
		break;
	case Data::FMT_PACKAGE:
	case Data::FMT_FUNCTION:
		error("invalid use of '%s' type as hash key", type_name(value).c_str());
	}

	return false;
}

bool Hash::equal_to::operator()(const Hash::key_type &lvalue, const Hash::key_type &rvalue) const {

	if (lvalue.data()->format != rvalue.data()->format) {
		return false;
	}

	switch (lvalue.data()->format) {
	case Data::FMT_NONE:
	case Data::FMT_NULL:
		return true;

	case Data::FMT_NUMBER:
		return lvalue.data<Number>()->value == rvalue.data<Number>()->value;

	case Data::FMT_BOOLEAN:
		return lvalue.data<Boolean>()->value == rvalue.data<Boolean>()->value;

	case Data::FMT_OBJECT:
		if (lvalue.data<Object>()->metadata->metatype() != rvalue.data<Object>()->metadata->metatype()) {
			return false;
		}

		switch (lvalue.data<Object>()->metadata->metatype()) {
		case Class::OBJECT:
			if (lvalue.data<Object>()->metadata != rvalue.data<Object>()->metadata) {
				return false;
			}
			return lvalue.data<Object>()->data == rvalue.data<Object>()->data;

		case Class::STRING:
			return lvalue.data<String>()->str == rvalue.data<String>()->str;

		case Class::REGEX:
			return lvalue.data<Regex>()->initializer == rvalue.data<Regex>()->initializer;

		case Class::ARRAY:
			if (lvalue.data<Array>()->values.size() != rvalue.data<Array>()->values.size()) {
				return false;
			}
			for (auto i = lvalue.data<Array>()->values.begin(), j = rvalue.data<Array>()->values.begin();
				 i != lvalue.data<Array>()->values.end() && j != rvalue.data<Array>()->values.end(); ++i, ++j) {
				if (!operator()(array_get_item(i), array_get_item(j))) {
					return false;
				}
			}
			return true;

		case Class::HASH:
		case Class::ITERATOR:
		case Class::LIBRARY:
		case Class::LIBOBJECT:
			error("invalid use of '%s' type as hash key", type_name(lvalue).c_str());
		}
		break;
	case Data::FMT_PACKAGE:
	case Data::FMT_FUNCTION:
		error("invalid use of '%s' type as hash key", type_name(lvalue).c_str());
	}

	return false;
}

bool Hash::compare_to::operator()(const Hash::key_type &lvalue, const Hash::key_type &rvalue) const {

	if (lvalue.data()->format != rvalue.data()->format) {
		return lvalue.data()->format < rvalue.data()->format;
	}

	switch (lvalue.data()->format) {
	case Data::FMT_NONE:
	case Data::FMT_NULL:
		return false;

	case Data::FMT_NUMBER:
		return lvalue.data<Number>()->value < rvalue.data<Number>()->value;

	case Data::FMT_BOOLEAN:
		return lvalue.data<Boolean>()->value < rvalue.data<Boolean>()->value;

	case Data::FMT_OBJECT:
		if (lvalue.data<Object>()->metadata->metatype() != rvalue.data<Object>()->metadata->metatype()) {
			return lvalue.data<Object>()->metadata->metatype() < rvalue.data<Object>()->metadata->metatype();
		}

		switch (lvalue.data<Object>()->metadata->metatype()) {
		case Class::OBJECT:
			if (lvalue.data<Object>()->metadata != rvalue.data<Object>()->metadata) {
				return lvalue.data<Object>()->metadata < rvalue.data<Object>()->metadata;
			}
			return lvalue.data<Object>()->data < rvalue.data<Object>()->data;

		case Class::STRING:
			return lvalue.data<String>()->str < rvalue.data<String>()->str;

		case Class::REGEX:
			return lvalue.data<Regex>()->initializer < rvalue.data<Regex>()->initializer;

		case Class::ARRAY:
			for (auto i = lvalue.data<Array>()->values.begin(), j = rvalue.data<Array>()->values.begin();
				 i != lvalue.data<Array>()->values.end() && j != rvalue.data<Array>()->values.end(); ++i, ++j) {
				if (operator()(array_get_item(i), array_get_item(j))) {
					return true;
				}
				if (operator()(array_get_item(j), array_get_item(i))) {
					return false;
				}
			}
			return lvalue.data<Array>()->values.size() < rvalue.data<Array>()->values.size();

		case Class::HASH:
		case Class::ITERATOR:
		case Class::LIBRARY:
		case Class::LIBOBJECT:
			error("invalid use of '%s' type as hash key", type_name(lvalue).c_str());
		}
		break;
	case Data::FMT_PACKAGE:
	case Data::FMT_FUNCTION:
		error("invalid use of '%s' type as hash key", type_name(lvalue).c_str());
	}

	return false;
}
