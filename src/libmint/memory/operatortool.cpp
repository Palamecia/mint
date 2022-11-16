#include "memory/operatortool.h"
#include "memory/memorytool.h"
#include "memory/algorithm.hpp"
#include "memory/functiontool.h"
#include "memory/globaldata.h"
#include "memory/builtin/string.h"
#include "memory/builtin/regex.h"
#include "memory/builtin/libobject.h"
#include "ast/cursor.h"
#include "scheduler/scheduler.h"
#include "scheduler/processor.h"
#include "system/assert.h"
#include "system/error.h"

#include <functional>
#include <cmath>

using namespace std;
using namespace mint;

static void do_function_call(int signature, const Function::Signature &function, Class *metadata, Cursor *cursor) {

	cursor->call(function.handle, signature, metadata);

	if (function.capture) {
		SymbolTable &symbols = cursor->symbols();
		for (const auto &item : *function.capture) {
			symbols.insert(item);
		}
	}
}

bool mint::call_overload(Cursor *cursor, Class::Operator operator_overload, int signature) {

	assert(signature >= 0);

	const size_t base = get_stack_base(cursor);
	Object *object = load_from_stack(cursor, base - static_cast<size_t>(signature)).data<Object>();

	if (Class::MemberInfo *info = object->metadata->findOperator(operator_overload)) {

		if (UNLIKELY(is_class(object))) {
			error("invalid use of class in an operation");
		}

		Reference &function = object->data[info->offset];
		Class *metadata = info->owner;

		switch (function.data()->format) {
		case Data::fmt_none:
			error("invalid use of none value as a function");
		case Data::fmt_null:
			cursor->raise(WeakReference::share(function));
			break;
		case Data::fmt_number:
		case Data::fmt_boolean:
		case Data::fmt_object:
			if (signature == 0) {
				cursor->stack().back() = WeakReference::clone(function.data());
			}
			else {
				string type = type_name(function);
				error("%s copy doesn't take %d argument(s)", type.c_str(), signature);
			}
			break;
		case Data::fmt_package:
			error("invalid use of package in an operation");
		case Data::fmt_function:
			if (!(function.flags() & Reference::global)) {
				// add self to function arguments
				signature += 1;
			}
			auto it = find_function_signature(cursor, function.data<Function>()->mapping, signature);
			if (UNLIKELY(it == function.data<Function>()->mapping.end())) {
				error("called member doesn't take %d parameter(s)", signature);
			}
			do_function_call(signature, it->second, metadata, cursor);
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
			error("invalid use of class in an operation");
		}

		Reference &function = object->data[it->second->offset];
		Class *metadata = it->second->owner;

		switch (function.data()->format) {
		case Data::fmt_none:
			error("invalid use of none value as a function");
		case Data::fmt_null:
			cursor->raise(WeakReference::share(function));
			break;
		case Data::fmt_number:
		case Data::fmt_boolean:
		case Data::fmt_object:
			if (signature == 0) {
				cursor->stack().back() = WeakReference::clone(function.data());
			}
			else {
				string type = type_name(function);
				error("%s copy doesn't take %d argument(s)", type.c_str(), signature);
			}
			break;
		case Data::fmt_package:
			error("invalid use of package in an operation");
		case Data::fmt_function:
			if (!(function.flags() & Reference::global)) {
				// add self to function arguments
				signature += 1;
			}
			auto it = find_function_signature(cursor, function.data<Function>()->mapping, signature);
			if (UNLIKELY(it == function.data<Function>()->mapping.end())) {
				error("called member doesn't take %d parameter(s)", signature);
			}
			do_function_call(signature, it->second, metadata, cursor);
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

	if (UNLIKELY((lvalue.flags() & Reference::const_address) && (lvalue.data()->format != Data::fmt_none))) {
		error("invalid modification of constant reference");
	}

	if (lvalue.flags() & Reference::const_value) {
		lvalue.move(rvalue);
	}
	else if ((rvalue.flags() & (Reference::const_value | Reference::temporary)) == Reference::const_value) {
		lvalue.copy(rvalue);
	}
	else {
		lvalue.move(rvalue);
	}

	cursor->stack().pop_back();
}

void mint::copy_operator(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);

	Reference &rvalue = load_from_stack(cursor, base);
	Reference &lvalue = load_from_stack(cursor, base - 1);

	if (UNLIKELY(lvalue.flags() & Reference::const_value)) {
		error("invalid modification of constant value");
	}

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
	case Data::fmt_null:
		cursor->raise(WeakReference::share(lvalue));
		break;
	case Data::fmt_number:
		lvalue.data<Number>()->value = to_number(cursor, rvalue);
		cursor->stack().pop_back();
		break;
	case Data::fmt_boolean:
		lvalue.data<Boolean>()->value = to_boolean(cursor, rvalue);
		cursor->stack().pop_back();
		break;
	case Data::fmt_function:
		if (UNLIKELY(rvalue.data()->format != Data::fmt_function)) {
			string rtype = type_name(rvalue);
			string ltype = type_name(lvalue);
			error("invalid conversion from '%s' to '%s'", rtype.c_str(), ltype.c_str());
		}
		lvalue.data<Function>()->mapping = rvalue.data<Function>()->mapping;
		cursor->stack().pop_back();
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, Class::copy_operator, 1)) {
			if (UNLIKELY(rvalue.data()->format != Data::fmt_object)) {
				string rtype = type_name(rvalue);
				string ltype = type_name(lvalue);
				error("cannot convert '%s' to '%s' in assignment", rtype.c_str(), ltype.c_str());
			}
			if (UNLIKELY(lvalue.data<Object>()->metadata != rvalue.data<Object>()->metadata)) {
				string rtype = type_name(rvalue);
				string ltype = type_name(lvalue);
				error("cannot convert '%s' to '%s' in assignment", rtype.c_str(), ltype.c_str());
			}
			delete [] lvalue.data<Object>()->data;
			lvalue.data<Object>()->construct(*rvalue.data<Object>());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	}
}

void mint::call_operator(Cursor *cursor, int signature) {

	Cursor::Call &call_infos = cursor->waitingCalls().top();
	WeakReference function = move(call_infos.function());
	Cursor::Call::Flags flags = call_infos.getFlags();
	Class *metadata = call_infos.getMetadata();
	signature += call_infos.extraArgumentCount();
	cursor->waitingCalls().pop();

	switch (function.data()->format) {
	case Data::fmt_none:
		if (LIKELY(flags & Cursor::Call::member_call)) {
			if (UNLIKELY(signature)) {
				error("default constructors doesn't take %d argument(s)", signature);
			}
		}
		else {
			error("invalid use of none value as a function");
		}
		break;
	case Data::fmt_null:
		cursor->raise(WeakReference::share(function));
		break;
	case Data::fmt_number:
	case Data::fmt_boolean:
	case Data::fmt_object:
		if (signature == 0) {
			cursor->stack().back() = WeakReference::clone(function.data());
		}
		else {
			string type = type_name(function);
			error("%s copy doesn't take %d argument(s)", type.c_str(), signature);
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	case Data::fmt_function:
		if (flags & Cursor::Call::member_call) {
			// add self to function arguments
			signature += 1;
		}
		auto it = find_function_signature(cursor, function.data<Function>()->mapping, signature);
		if (UNLIKELY(it == function.data<Function>()->mapping.end())) {
			error("called function doesn't take %d parameter(s)", signature);
		}
		do_function_call(signature, it->second, metadata, cursor);
		break;
	}
}

void mint::call_member_operator(Cursor *cursor, int signature) {

	Cursor::Call &call_infos = cursor->waitingCalls().top();
	WeakReference function = move(call_infos.function());
	Cursor::Call::Flags flags = call_infos.getFlags();
	Class *metadata = call_infos.getMetadata();
	signature += call_infos.extraArgumentCount();
	cursor->waitingCalls().pop();

	switch (function.data()->format) {
	case Data::fmt_none:
		if (LIKELY(flags & Cursor::Call::member_call)) {
			if (UNLIKELY(signature)) {
				error("default constructors doesn't take %d argument(s)", signature);
			}
		}
		else {
			error("invalid use of none value as a function");
		}
		break;
	case Data::fmt_null:
		cursor->raise(WeakReference::share(function));
		break;
	case Data::fmt_number:
	case Data::fmt_boolean:
	case Data::fmt_object:
		if (signature == 0) {
			cursor->stack().back() = WeakReference::clone(function.data());
		}
		else {
			string type = type_name(function);
			error("%s copy doesn't take %d argument(s)", type.c_str(), signature);
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	case Data::fmt_function:
		if (!(function.flags() & Reference::global)) {
			// add self to function arguments
			signature += 1;
		}
		auto it = find_function_signature(cursor, function.data<Function>()->mapping, signature);
		if (UNLIKELY(it == function.data<Function>()->mapping.end())) {
			error("called member doesn't take %d parameter(s)", signature);
		}
		do_function_call(signature, it->second, metadata, cursor);
		break;
	}
}

void mint::add_operator(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);

	Reference &rvalue = load_from_stack(cursor, base);
	Reference &lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
	case Data::fmt_null:
		cursor->raise(WeakReference::share(lvalue));
		break;
	case Data::fmt_number:
		if (lvalue.flags() & Reference::temporary) {
			lvalue.data<Number>()->value += to_number(cursor, rvalue);
			cursor->stack().pop_back();
		}
		else {
			Reference &&result = WeakReference::create<Number>(lvalue.data<Number>()->value + to_number(cursor, rvalue));
			cursor->stack().pop_back();
			cursor->stack().back() = move(result);
		}
		break;
	case Data::fmt_boolean:
		if (lvalue.flags() & Reference::temporary) {
			lvalue.data<Boolean>()->value += to_boolean(cursor, rvalue);
			cursor->stack().pop_back();
		}
		else {
			Reference &&result = WeakReference::create<Boolean>(lvalue.data<Boolean>()->value + to_boolean(cursor, rvalue));
			cursor->stack().pop_back();
			cursor->stack().back() = move(result);
		}
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Class::add_operator, 1))) {
			string ltype = type_name(lvalue);
			error("class '%s' dosen't ovreload operator '+'(1)", ltype.c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	case Data::fmt_function:
	{
		Reference &&result = WeakReference::create<Function>();
		if (UNLIKELY(rvalue.data()->format != Data::fmt_function)) {
			string ltype = type_name(lvalue);
			string rtype = type_name(rvalue);
			error("invalid use of operator '+' with '%s' and '%s' types", ltype.c_str(), rtype.c_str());
		}
		for (const auto &item : lvalue.data<Function>()->mapping) {
			result.data<Function>()->mapping.insert(item);
		}
		for (const auto &item : rvalue.data<Function>()->mapping) {
			result.data<Function>()->mapping.insert(item);
		}
		cursor->stack().pop_back();
		cursor->stack().back() = move(result);
	}
		break;
	}
}

void mint::sub_operator(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);

	Reference &rvalue = load_from_stack(cursor, base);
	Reference &lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
	case Data::fmt_null:
		cursor->raise(WeakReference::share(lvalue));
		break;
	case Data::fmt_number:
		if (lvalue.flags() & Reference::temporary) {
			lvalue.data<Number>()->value -= to_number(cursor, rvalue);
			cursor->stack().pop_back();
		}
		else {
			Reference &&result = WeakReference::create<Number>(lvalue.data<Number>()->value - to_number(cursor, rvalue));
			cursor->stack().pop_back();
			cursor->stack().back() = move(result);
		}
		break;
	case Data::fmt_boolean:
		if (lvalue.flags() & Reference::temporary) {
			lvalue.data<Boolean>()->value -= to_boolean(cursor, rvalue);
			cursor->stack().pop_back();
		}
		else {
			Reference &&result = WeakReference::create<Boolean>(lvalue.data<Boolean>()->value - to_boolean(cursor, rvalue));
			cursor->stack().pop_back();
			cursor->stack().back() = move(result);
		}
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Class::sub_operator, 1))) {
			string ltype = type_name(lvalue);
			error("class '%s' dosen't ovreload operator '-'(1)", ltype.c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	case Data::fmt_function:
		string ltype = type_name(lvalue);
		error("invalid use of '%s' type with operator '-'", ltype.c_str());
	}
}

void mint::mul_operator(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);

	Reference &rvalue = load_from_stack(cursor, base);
	Reference &lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
	case Data::fmt_null:
		cursor->raise(WeakReference::share(lvalue));
		break;
	case Data::fmt_number:
		if (lvalue.flags() & Reference::temporary) {
			lvalue.data<Number>()->value *= to_number(cursor, rvalue);
			cursor->stack().pop_back();
		}
		else {
			Reference &&result = WeakReference::create<Number>(lvalue.data<Number>()->value * to_number(cursor, rvalue));
			cursor->stack().pop_back();
			cursor->stack().back() = move(result);
		}
		break;
	case Data::fmt_boolean:
		if (lvalue.flags() & Reference::temporary) {
			lvalue.data<Boolean>()->value = lvalue.data<Boolean>()->value && to_boolean(cursor, rvalue);
			cursor->stack().pop_back();
		}
		else {
			Reference &&result = WeakReference::create<Boolean>(lvalue.data<Boolean>()->value && to_boolean(cursor, rvalue));
			cursor->stack().pop_back();
			cursor->stack().back() = move(result);
		}
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Class::mul_operator, 1))) {
			string ltype = type_name(lvalue);
			error("class '%s' dosen't ovreload operator '*'(1)", ltype.c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	case Data::fmt_function:
		string ltype = type_name(lvalue);
		error("invalid use of '%s' type with operator '*'", ltype.c_str());
	}
}

void mint::div_operator(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);

	Reference &rvalue = load_from_stack(cursor, base);
	Reference &lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
	case Data::fmt_null:
		cursor->raise(WeakReference::share(lvalue));
		break;
	case Data::fmt_number:
		if (lvalue.flags() & Reference::temporary) {
			lvalue.data<Number>()->value /= to_number(cursor, rvalue);
			cursor->stack().pop_back();
		}
		else {
			Reference &&result = WeakReference::create<Number>(lvalue.data<Number>()->value / to_number(cursor, rvalue));
			cursor->stack().pop_back();
			cursor->stack().back() = move(result);
		}
		break;
	case Data::fmt_boolean:
		if (lvalue.flags() & Reference::temporary) {
			lvalue.data<Boolean>()->value /= to_boolean(cursor, rvalue);
			cursor->stack().pop_back();
		}
		else {
			Reference &&result = WeakReference::create<Boolean>(lvalue.data<Boolean>()->value / to_boolean(cursor, rvalue));
			cursor->stack().pop_back();
			cursor->stack().back() = move(result);
		}
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Class::div_operator, 1))) {
			string ltype = type_name(lvalue);
			error("class '%s' dosen't ovreload operator '/'(1)", ltype.c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	case Data::fmt_function:
		string ltype = type_name(lvalue);
		error("invalid use of '%s' type with operator '/'", ltype.c_str());
	}
}

void mint::pow_operator(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);

	Reference &rvalue = load_from_stack(cursor, base);
	Reference &lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
	case Data::fmt_null:
		cursor->raise(WeakReference::share(lvalue));
		break;
	case Data::fmt_number:
		if (lvalue.flags() & Reference::temporary) {
			lvalue.data<Number>()->value = pow(lvalue.data<Number>()->value, to_number(cursor, rvalue));
			cursor->stack().pop_back();
		}
		else {
			Reference &&result = WeakReference::create<Number>(pow(lvalue.data<Number>()->value, to_number(cursor, rvalue)));
			cursor->stack().pop_back();
			cursor->stack().back() = move(result);
		}
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Class::pow_operator, 1))) {
			string ltype = type_name(lvalue);
			error("class '%s' dosen't ovreload operator '**'(1)", ltype.c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	case Data::fmt_boolean:
	case Data::fmt_function:
		string ltype = type_name(lvalue);
		error("invalid use of '%s' type with operator '**'", ltype.c_str());
	}
}

void mint::mod_operator(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);

	Reference &rvalue = load_from_stack(cursor, base);
	Reference &lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
	case Data::fmt_null:
		cursor->raise(WeakReference::share(lvalue));
		break;
	case Data::fmt_number:
		if (intmax_t divider = to_integer(cursor, rvalue)) {
			if (lvalue.flags() & Reference::temporary) {
				lvalue.data<Number>()->value = static_cast<double>(to_integer(lvalue.data<Number>()->value) % divider);
				cursor->stack().pop_back();
			}
			else {
				Reference &&result = WeakReference::create<Number>(static_cast<double>(to_integer(lvalue.data<Number>()->value) % divider));
				cursor->stack().pop_back();
				cursor->stack().back() = move(result);
			}
		}
		else {
			error("modulo by zero");
		}
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Class::mod_operator, 1))) {
			string ltype = type_name(lvalue);
			error("class '%s' dosen't ovreload operator '%%'(1)", ltype.c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	case Data::fmt_boolean:
	case Data::fmt_function:
		string ltype = type_name(lvalue);
		error("invalid use of '%s' type with operator '%%'", ltype.c_str());
	}
}

void mint::is_operator(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);

	Reference &rvalue = load_from_stack(cursor, base);
	Reference &lvalue = load_from_stack(cursor, base - 1);

	WeakReference result = WeakReference::create<Boolean>();
	result.data<Boolean>()->value = lvalue.data() == rvalue.data();
	cursor->stack().pop_back();
	cursor->stack().back() = move(result);
}

void mint::eq_operator(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);

	Reference &rvalue = load_from_stack(cursor, base);
	Reference &lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data()->format) {
	case Data::fmt_none:
	{
		Reference &&result = WeakReference::create<Boolean>(rvalue.data()->format == Data::fmt_none);
		cursor->stack().pop_back();
		cursor->stack().back() = move(result);
	}
		break;
	case Data::fmt_null:
	{
		Reference &&result = WeakReference::create<Boolean>(rvalue.data()->format == Data::fmt_null);
		cursor->stack().pop_back();
		cursor->stack().back() = move(result);
	}
		break;
	case Data::fmt_number:
		switch (rvalue.data()->format) {
		case Data::fmt_none:
		case Data::fmt_null:
		{
			Reference &&result = WeakReference::create<Boolean>(false);
			cursor->stack().pop_back();
			cursor->stack().back() = move(result);
		}
			break;
		default:
			Reference &&result = WeakReference::create<Boolean>(lvalue.data<Number>()->value == to_number(cursor, rvalue));
			cursor->stack().pop_back();
			cursor->stack().back() = move(result);
		}
		break;
	case Data::fmt_boolean:
		switch (rvalue.data()->format) {
		case Data::fmt_none:
		case Data::fmt_null:
		{
			Reference &&result = WeakReference::create<Boolean>(false);
			cursor->stack().pop_back();
			cursor->stack().back() = move(result);
		}
			break;
		default:
			Reference &&result = WeakReference::create<Boolean>(lvalue.data<Boolean>()->value == to_boolean(cursor, rvalue));
			cursor->stack().pop_back();
			cursor->stack().back() = move(result);
		}
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, Class::eq_operator, 1)) {
			switch (rvalue.data()->format) {
			case Data::fmt_none:
			case Data::fmt_null:
			{
				Reference &&result = WeakReference::create<Boolean>(false);
				cursor->stack().pop_back();
				cursor->stack().back() = move(result);
			}
				break;
			default:
				string ltype = type_name(lvalue);
				error("class '%s' dosen't ovreload operator '=='(1)", ltype.c_str());
			}
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	case Data::fmt_function:
		if (rvalue.data()->format == Data::fmt_function) {
			Reference &&result = WeakReference::create<Boolean>(lvalue.data<Function>()->mapping == rvalue.data<Function>()->mapping);
			cursor->stack().pop_back();
			cursor->stack().back() = move(result);
		}
		else {
			string ltype = type_name(lvalue);
			error("invalid use of '%s' type with operator '=='", ltype.c_str());
		}
	}
}

void mint::ne_operator(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);

	Reference &rvalue = load_from_stack(cursor, base);
	Reference &lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data()->format) {
	case Data::fmt_none:
	{
		Reference &&result = WeakReference::create<Boolean>(rvalue.data()->format != Data::fmt_none);
		cursor->stack().pop_back();
		cursor->stack().back() = move(result);
	}
		break;
	case Data::fmt_null:
	{
		Reference &&result = WeakReference::create<Boolean>(rvalue.data()->format != Data::fmt_null);
		cursor->stack().pop_back();
		cursor->stack().back() = move(result);
	}
		break;
	case Data::fmt_number:
		switch (rvalue.data()->format) {
		case Data::fmt_none:
		case Data::fmt_null:
		{
			Reference &&result = WeakReference::create<Boolean>(true);
			cursor->stack().pop_back();
			cursor->stack().back() = move(result);
		}
			break;
		default:
			Reference &&result = WeakReference::create<Boolean>(lvalue.data<Number>()->value != to_number(cursor, rvalue));
			cursor->stack().pop_back();
			cursor->stack().back() = move(result);
		}
		break;
	case Data::fmt_boolean:
		switch (rvalue.data()->format) {
		case Data::fmt_none:
		case Data::fmt_null:
		{
			Reference &&result = WeakReference::create<Boolean>(true);
			cursor->stack().pop_back();
			cursor->stack().back() = move(result);
		}
			break;
		default:
			Reference &&result = WeakReference::create<Boolean>(lvalue.data<Boolean>()->value != to_boolean(cursor, rvalue));
			cursor->stack().pop_back();
			cursor->stack().back() = move(result);
		}
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, Class::ne_operator, 1)) {
			switch (rvalue.data()->format) {
			case Data::fmt_none:
			case Data::fmt_null:
			{
				Reference &&result = WeakReference::create<Boolean>(true);
				cursor->stack().pop_back();
				cursor->stack().back() = move(result);
			}
				break;
			default:
				string ltype = type_name(lvalue);
				error("class '%s' dosen't ovreload operator '!='(1)", ltype.c_str());
			}
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	case Data::fmt_function:
		if (rvalue.data()->format == Data::fmt_function) {
			Reference &&result = WeakReference::create<Boolean>(lvalue.data<Function>()->mapping != rvalue.data<Function>()->mapping);
			cursor->stack().pop_back();
			cursor->stack().back() = move(result);
		}
		else {
			string ltype = type_name(lvalue);
			error("invalid use of '%s' type with operator '!='", ltype.c_str());
		}
	}
}

void mint::lt_operator(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);

	Reference &rvalue = load_from_stack(cursor, base);
	Reference &lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
	case Data::fmt_null:
		cursor->raise(WeakReference::share(lvalue));
		break;
	case Data::fmt_number:
	{
		Reference &&result = WeakReference::create<Boolean>(lvalue.data<Number>()->value < to_number(cursor, rvalue));
		cursor->stack().pop_back();
		cursor->stack().back() = move(result);
	}
		break;
	case Data::fmt_boolean:
	{
		Reference &&result = WeakReference::create<Boolean>(lvalue.data<Boolean>()->value < to_boolean(cursor, rvalue));
		cursor->stack().pop_back();
		cursor->stack().back() = move(result);
	}
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Class::lt_operator, 1))) {
			string ltype = type_name(lvalue);
			error("class '%s' dosen't ovreload operator '<'(1)", ltype.c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	case Data::fmt_function:
		string ltype = type_name(lvalue);
		error("invalid use of '%s' type with operator '<'", ltype.c_str());
	}
}

void mint::gt_operator(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);

	Reference &rvalue = load_from_stack(cursor, base);
	Reference &lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
	case Data::fmt_null:
		cursor->raise(WeakReference::share(lvalue));
		break;
	case Data::fmt_number:
	{
		Reference &&result = WeakReference::create<Boolean>(lvalue.data<Number>()->value > to_number(cursor, rvalue));
		cursor->stack().pop_back();
		cursor->stack().back() = move(result);
	}
		break;
	case Data::fmt_boolean:
	{
		Reference &&result = WeakReference::create<Boolean>(lvalue.data<Boolean>()->value > to_boolean(cursor, rvalue));
		cursor->stack().pop_back();
		cursor->stack().back() = move(result);
	}
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Class::gt_operator, 1))) {
			string ltype = type_name(lvalue);
			error("class '%s' dosen't ovreload operator '>'(1)", ltype.c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	case Data::fmt_function:
		string ltype = type_name(lvalue);
		error("invalid use of '%s' type with operator '>'", ltype.c_str());
	}
}

void mint::le_operator(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);

	Reference &rvalue = load_from_stack(cursor, base);
	Reference &lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
	case Data::fmt_null:
		cursor->raise(WeakReference::share(lvalue));
		break;
	case Data::fmt_number:
	{
		Reference &&result = WeakReference::create<Boolean>(lvalue.data<Number>()->value <= to_number(cursor, rvalue));
		cursor->stack().pop_back();
		cursor->stack().back() = move(result);
	}
		break;
	case Data::fmt_boolean:
	{
		Reference &&result = WeakReference::create<Boolean>(lvalue.data<Boolean>()->value <= to_boolean(cursor, rvalue));
		cursor->stack().pop_back();
		cursor->stack().back() = move(result);
	}
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Class::le_operator, 1))) {
			string ltype = type_name(lvalue);
			error("class '%s' dosen't ovreload operator '<='(1)", ltype.c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	case Data::fmt_function:
		string ltype = type_name(lvalue);
		error("invalid use of '%s' type with operator '<='", ltype.c_str());
	}
}

void mint::ge_operator(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);

	Reference &rvalue = load_from_stack(cursor, base);
	Reference &lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
	case Data::fmt_null:
		cursor->raise(WeakReference::share(lvalue));
		break;
	case Data::fmt_number:
	{
		Reference &&result = WeakReference::create<Boolean>(lvalue.data<Number>()->value >= to_number(cursor, rvalue));
		cursor->stack().pop_back();
		cursor->stack().back() = move(result);
	}
		break;
	case Data::fmt_boolean:
	{
		Reference &&result = WeakReference::create<Boolean>(lvalue.data<Boolean>()->value >= to_boolean(cursor, rvalue));
		cursor->stack().pop_back();
		cursor->stack().back() = move(result);
	}
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Class::ge_operator, 1))) {
			string ltype = type_name(lvalue);
			error("class '%s' dosen't ovreload operator '>='(1)", ltype.c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	case Data::fmt_function:
		string ltype = type_name(lvalue);
		error("invalid use of '%s' type with operator '>='", ltype.c_str());
	}
}

void mint::and_pre_check(Cursor *cursor, size_t pos) {

	Reference &value = cursor->stack().back();

	switch (value.data()->format) {
	case Data::fmt_none:
	case Data::fmt_null:
		cursor->jmp(pos);
		break;
	case Data::fmt_number:
		if (value.data<Number>()->value == 0.) {
			cursor->jmp(pos);
		}
		break;
	case Data::fmt_boolean:
		if (!value.data<Boolean>()->value) {
			cursor->jmp(pos);
		}
		break;
	case Data::fmt_object:
		switch (value.data<Object>()->metadata->metatype()) {
		case Class::iterator:
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
	case Data::fmt_object:
		if (!call_overload(cursor, Class::and_operator, 1)) {
			swap(lvalue, rvalue);
			cursor->stack().pop_back();
		}
		break;
	default:
		swap(lvalue, rvalue);
		cursor->stack().pop_back();
	}
}

void mint::or_pre_check(Cursor *cursor, size_t pos) {

	Reference &value = cursor->stack().back();

	switch (value.data()->format) {
	case Data::fmt_none:
	case Data::fmt_null:
		break;
	case Data::fmt_number:
		if (value.data<Number>()->value != 0.) {
			cursor->jmp(pos);
		}
		break;
	case Data::fmt_boolean:
		if (value.data<Boolean>()->value) {
			cursor->jmp(pos);
		}
		break;
	case Data::fmt_object:
		switch (value.data<Object>()->metadata->metatype()) {
		case Class::iterator:
			if (!value.data<Iterator>()->ctx.empty()) {
				cursor->jmp(pos);
			}
			break;
		default:
			if (!value.data<Object>()->metadata->findOperator(Class::or_operator)) {
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
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Class::or_operator, 1))) {
			string ltype = type_name(lvalue);
			error("class '%s' dosen't ovreload operator '||'(1)", ltype.c_str());
		}
		break;
	default:
		swap(lvalue, rvalue);
		cursor->stack().pop_back();
	}
}

void mint::band_operator(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);

	Reference &rvalue = load_from_stack(cursor, base);
	Reference &lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
	case Data::fmt_null:
		cursor->raise(WeakReference::share(lvalue));
		break;
	case Data::fmt_number:
		if (lvalue.flags() & Reference::temporary) {
			lvalue.data<Number>()->value = static_cast<double>(to_integer(lvalue.data<Number>()->value) & to_integer(cursor, rvalue));
			cursor->stack().pop_back();
		}
		else {
			Reference &&result = WeakReference::create<Number>(static_cast<double>(to_integer(lvalue.data<Number>()->value) & to_integer(cursor, rvalue)));
			cursor->stack().pop_back();
			cursor->stack().back() = move(result);
		}
		break;
	case Data::fmt_boolean:
		if (lvalue.flags() & Reference::temporary) {
			lvalue.data<Boolean>()->value &= to_boolean(cursor, rvalue);
			cursor->stack().pop_back();
		}
		else {
			Reference &&result = WeakReference::create<Boolean>();
			result.data<Boolean>()->value = lvalue.data<Boolean>()->value && to_boolean(cursor, rvalue);
			cursor->stack().pop_back();
			cursor->stack().back() = move(result);
		}
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Class::band_operator, 1))) {
			string ltype = type_name(lvalue);
			error("class '%s' dosen't ovreload operator '&'(1)", ltype.c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	case Data::fmt_function:
		string ltype = type_name(lvalue);
		error("invalid use of '%s' type with operator '&'", ltype.c_str());
	}
}

void mint::bor_operator(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);

	Reference &rvalue = load_from_stack(cursor, base);
	Reference &lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
	case Data::fmt_null:
		cursor->raise(WeakReference::share(lvalue));
		break;
	case Data::fmt_number:
		if (lvalue.flags() & Reference::temporary) {
			lvalue.data<Number>()->value = static_cast<double>(to_integer(lvalue.data<Number>()->value) | to_integer(cursor, rvalue));
			cursor->stack().pop_back();
		}
		else {
			Reference &&result = WeakReference::create<Number>(static_cast<double>(to_integer(lvalue.data<Number>()->value) | to_integer(cursor, rvalue)));
			cursor->stack().pop_back();
			cursor->stack().back() = move(result);
		}
		break;
	case Data::fmt_boolean:
		if (lvalue.flags() & Reference::temporary) {
			lvalue.data<Boolean>()->value |= to_boolean(cursor, rvalue);
			cursor->stack().pop_back();
		}
		else {
			Reference &&result = WeakReference::create<Boolean>(lvalue.data<Boolean>()->value || to_boolean(cursor, rvalue));
			cursor->stack().pop_back();
			cursor->stack().back() = move(result);
		}
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Class::bor_operator, 1))) {
			string ltype = type_name(lvalue);
			error("class '%s' dosen't ovreload operator '|'(1)", ltype.c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	case Data::fmt_function:
		string ltype = type_name(lvalue);
		error("invalid use of '%s' type with operator '|'", ltype.c_str());
	}
}

void mint::xor_operator(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);

	Reference &rvalue = load_from_stack(cursor, base);
	Reference &lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
	case Data::fmt_null:
		cursor->raise(WeakReference::share(lvalue));
		break;
	case Data::fmt_number:
		if (lvalue.flags() & Reference::temporary) {
			lvalue.data<Number>()->value = static_cast<double>(to_integer(lvalue.data<Number>()->value) ^ to_integer(cursor, rvalue));
			cursor->stack().pop_back();
		}
		else {
			Reference &&result = WeakReference::create<Number>(static_cast<double>(to_integer(lvalue.data<Number>()->value) ^ to_integer(cursor, rvalue)));
			cursor->stack().pop_back();
			cursor->stack().back() = move(result);
		}
		break;
	case Data::fmt_boolean:
		if (lvalue.flags() & Reference::temporary) {
			lvalue.data<Boolean>()->value ^= to_boolean(cursor, rvalue);
			cursor->stack().pop_back();
		}
		else {
			Reference &&result = WeakReference::create<Boolean>(to_integer(lvalue.data<Number>()->value) ^ to_boolean(cursor, rvalue));
			cursor->stack().pop_back();
			cursor->stack().back() = move(result);
		}
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Class::xor_operator, 1))) {
			string ltype = type_name(lvalue);
			error("class '%s' dosen't ovreload operator '^'(1)", ltype.c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	case Data::fmt_function:
		string ltype = type_name(lvalue);
		error("invalid use of '%s' type with operator '^'", ltype.c_str());
	}
}

void mint::inc_operator(Cursor *cursor) {

	Reference &value = cursor->stack().back();

	if (UNLIKELY(value.flags() & Reference::const_value)) {
		error("invalid modification of constant value");
	}

	switch (value.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
	case Data::fmt_null:
		cursor->raise(WeakReference::share(value));
		break;
	case Data::fmt_number:
		value.move(WeakReference::create<Number>(value.data<Number>()->value + 1));
		break;
	case Data::fmt_boolean:
		value.move(WeakReference::create<Boolean>(value.data<Boolean>()->value + 1));
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Class::inc_operator, 0))) {
			string type = type_name(value);
			error("class '%s' dosen't ovreload operator '++'(0)", type.c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	case Data::fmt_function:
		string type = type_name(value);
		error("invalid use of '%s' type with operator '++'", type.c_str());
	}
}

void mint::dec_operator(Cursor *cursor) {

	Reference &value = cursor->stack().back();

	if (UNLIKELY(value.flags() & Reference::const_value)) {
		error("invalid modification of constant value");
	}

	switch (value.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
	case Data::fmt_null:
		cursor->raise(WeakReference::share(value));
		break;
	case Data::fmt_number:
		value.move(WeakReference::create<Number>(value.data<Number>()->value - 1));
		break;
	case Data::fmt_boolean:
		value.move(WeakReference::create<Boolean>(value.data<Boolean>()->value - 1));
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Class::dec_operator, 0))) {
			string type = type_name(value);
			error("class '%s' dosen't ovreload operator '--'(0)", type.c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	case Data::fmt_function:
		string type = type_name(value);
		error("invalid use of '%s' type with operator '--'", type.c_str());
	}
}

void mint::not_operator(Cursor *cursor) {

	Reference &value = cursor->stack().back();
	WeakReference result = WeakReference::create<Boolean>();

	switch (value.data()->format) {
	case Data::fmt_none:
	case Data::fmt_null:
		cursor->stack().back() = WeakReference::create<Boolean>(true);
		break;
	case Data::fmt_number:
		cursor->stack().back() = WeakReference::create<Boolean>(value.data<Number>()->value == 0.);
		break;
	case Data::fmt_boolean:
		cursor->stack().back() = WeakReference::create<Boolean>(!value.data<Boolean>()->value);
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Class::not_operator, 0))) {
			string type = type_name(value);
			error("class '%s' dosen't ovreload operator '!'(0)", type.c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	case Data::fmt_function:
		string type = type_name(value);
		error("invalid use of '%s' type with operator '!'", type.c_str());
	}
}

void mint::compl_operator(Cursor *cursor) {

	Reference &value = cursor->stack().back();

	switch (value.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
	case Data::fmt_null:
		cursor->raise(WeakReference::share(value));
		break;
	case Data::fmt_number:
		cursor->stack().back() = WeakReference::create<Number>(static_cast<double>(~(to_integer(cursor, value))));
		break;
	case Data::fmt_boolean:
		cursor->stack().back() = WeakReference::create<Boolean>(!value.data<Boolean>()->value);
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Class::compl_operator, 0))) {
			string type = type_name(value);
			error("class '%s' dosen't ovreload operator '~'(0)", type.c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	case Data::fmt_function:
		string type = type_name(value);
		error("invalid use of '%s' type with operator '~'", type.c_str());
	}
}

void mint::pos_operator(Cursor *cursor) {

	Reference &value = cursor->stack().back();

	switch (value.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
	case Data::fmt_null:
		cursor->raise(WeakReference::share(value));
		break;
	case Data::fmt_number:
		if (value.flags() & Reference::temporary) {
			value.data<Number>()->value = +(value.data<Number>()->value);
		}
		else {
			cursor->stack().back() = WeakReference::create<Number>(+(value.data<Number>()->value));
		}
		break;
	case Data::fmt_boolean:
		if (value.flags() & Reference::temporary) {
			value.data<Boolean>()->value = +(value.data<Boolean>()->value);
		}
		else {
			cursor->stack().back() = WeakReference::create<Boolean>(+(value.data<Boolean>()->value));
		}
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Class::add_operator, 0))) {
			string type = type_name(value);
			error("class '%s' dosen't ovreload operator '+'(0)", type.c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	case Data::fmt_function:
		string type = type_name(value);
		error("invalid use of '%s' type with operator '+'", type.c_str());
	}
}

void mint::neg_operator(Cursor *cursor) {

	Reference &value = cursor->stack().back();

	switch (value.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
	case Data::fmt_null:
		cursor->raise(WeakReference::share(value));
		break;
	case Data::fmt_number:
		if (value.flags() & Reference::temporary) {
			value.data<Number>()->value = -(value.data<Number>()->value);
		}
		else {
			cursor->stack().back() = WeakReference::create<Number>(-(value.data<Number>()->value));
		}
		break;
	case Data::fmt_boolean:
		if (value.flags() & Reference::temporary) {
			value.data<Boolean>()->value = -(value.data<Boolean>()->value);
		}
		else {
			cursor->stack().back() = WeakReference::create<Boolean>(-(value.data<Boolean>()->value));
		}
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Class::sub_operator, 0))) {
			string type = type_name(value);
			error("class '%s' dosen't ovreload operator '-'(0)", type.c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	case Data::fmt_function:
		string type = type_name(value);
		error("invalid use of '%s' type with operator '-'", type.c_str());
	}
}

void mint::shift_left_operator(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);

	Reference &rvalue = load_from_stack(cursor, base);
	Reference &lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
	case Data::fmt_null:
		cursor->raise(WeakReference::share(lvalue));
		break;
	case Data::fmt_number:
	{
		Reference &&result = WeakReference::create<Number>(static_cast<double>(to_integer(lvalue.data<Number>()->value) << to_integer(cursor, rvalue)));
		cursor->stack().pop_back();
		cursor->stack().back() = move(result);
	}
		break;
	case Data::fmt_boolean:
	{
		Reference &&result = WeakReference::create<Number>(lvalue.data<Boolean>()->value << to_integer(cursor, rvalue));
		cursor->stack().pop_back();
		cursor->stack().back() = move(result);
	}
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Class::shift_left_operator, 1))) {
			string ltype = type_name(lvalue);
			error("class '%s' dosen't ovreload operator '<<'(1)", ltype.c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	case Data::fmt_function:
		string ltype = type_name(lvalue);
		error("invalid use of '%s' type with operator '<<'",ltype.c_str());
	}
}

void mint::shift_right_operator(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);

	Reference &rvalue = load_from_stack(cursor, base);
	Reference &lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
	case Data::fmt_null:
		cursor->raise(WeakReference::share(lvalue));
		break;
	case Data::fmt_number:
	{
		Reference &&result = WeakReference::create<Number>(static_cast<double>(to_integer(lvalue.data<Number>()->value) >> to_integer(cursor, rvalue)));
		cursor->stack().pop_back();
		cursor->stack().back() = move(result);
	}
		break;
	case Data::fmt_boolean:
	{
		Reference &&result = WeakReference::create<Boolean>(lvalue.data<Boolean>()->value >> to_integer(cursor, rvalue));
		cursor->stack().pop_back();
		cursor->stack().back() = move(result);
	}
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Class::shift_right_operator, 1))) {
			string ltype = type_name(lvalue);
			error("class '%s' dosen't ovreload operator '>>'(1)", ltype.c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	case Data::fmt_function:
		string ltype = type_name(lvalue);
		error("invalid use of '%s' type with operator '>>'", ltype.c_str());
	}
}

void mint::inclusive_range_operator(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);

	Reference &rvalue = load_from_stack(cursor, base);
	Reference &lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
	case Data::fmt_null:
		cursor->raise(WeakReference::share(lvalue));
		break;
	case Data::fmt_number:
	{
		Reference &&result = Iterator::fromInclusiveRange(lvalue.data<Number>()->value, to_number(cursor, rvalue));
		cursor->stack().pop_back();
		cursor->stack().back() = move(result);
	}
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Class::inclusive_range_operator, 1))) {
			string ltype = type_name(lvalue);
			error("class '%s' dosen't ovreload operator '..'(1)", ltype.c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	case Data::fmt_boolean:
	case Data::fmt_function:
		string ltype = type_name(lvalue);
		error("invalid use of '%s' type with operator '..'", ltype.c_str());
	}
}

void mint::exclusive_range_operator(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);

	Reference &rvalue = load_from_stack(cursor, base);
	Reference &lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
	case Data::fmt_null:
		cursor->raise(WeakReference::share(lvalue));
		break;
	case Data::fmt_number:
	{
		Reference &&result = Iterator::fromExclusiveRange(lvalue.data<Number>()->value, to_number(cursor, rvalue));
		cursor->stack().pop_back();
		cursor->stack().back() = move(result);
	}
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Class::exclusive_range_operator, 1))) {
			string ltype = type_name(lvalue);
			error("class '%s' dosen't ovreload operator '...'(1)", ltype.c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	case Data::fmt_boolean:
	case Data::fmt_function:
		string ltype = type_name(lvalue);
		error("invalid use of '%s' type with operator '...'", ltype.c_str());
	}
}

void mint::typeof_operator(Cursor *cursor) {
	cursor->stack().back() = create_string(type_name(forward<Reference>(cursor->stack().back())));
}

void mint::membersof_operator(Cursor *cursor) {

	Reference &value = cursor->stack().back();
	WeakReference result = WeakReference::create<Array>();

	switch (value.data()->format) {
	case Data::fmt_object:
		if (Object *object = value.data<Object>()) {

			Array *array = result.data<Array>();

			array->construct();
			array->values.reserve(object->metadata->members().size());

			for (const auto &member : object->metadata->members()) {

				switch (member.second->value.flags() & Reference::visibility_mask) {
				case Reference::protected_visibility:
					if (!is_protected_accessible(member.second->owner, cursor->symbols().getMetadata())) {
						continue;
					}
					break;
				case Reference::private_visibility:
					if (member.second->owner != cursor->symbols().getMetadata()) {
						continue;
					}
					break;
				case Reference::package_visibility:
					if (member.second->owner->getPackage() != cursor->symbols().getPackage()) {
						continue;
					}
					break;
				}

				array_append(array, create_string(member.first.str()));
			}
		}
		break;

	case Data::fmt_package:
		if (Package *package = value.data<Package>()) {

			Array *array = result.data<Array>();

			array->construct();
			array->values.reserve(package->data->symbols().size());

			for (auto &symbol : package->data->symbols()) {
				array_append(array, create_string(symbol.first.str()));
			}
		}
		break;

	default:
		break;
	}

	cursor->stack().back() = move(result);
}

void mint::subscript_operator(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);

	Reference &rvalue = load_from_stack(cursor, base);
	Reference &lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
	case Data::fmt_null:
		cursor->raise(WeakReference::share(lvalue));
		break;
	case Data::fmt_number:
		if (lvalue.flags() & Reference::temporary) {
			lvalue.data<Number>()->value = static_cast<double>(to_integer(lvalue.data<Number>()->value / pow(10, to_number(cursor, rvalue))) % 10);
			cursor->stack().pop_back();
		}
		else {
			WeakReference result = WeakReference::create<Number>(static_cast<double>(to_integer(lvalue.data<Number>()->value / pow(10, to_number(cursor, rvalue))) % 10));
			cursor->stack().pop_back();
			cursor->stack().back() = move(result);
		}
		break;
	case Data::fmt_boolean:
		{
			string ltype = type_name(lvalue);
			error("invalid use of '%s' type with operator '[]'", ltype.c_str());
		}
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Class::subscript_operator, 1))) {
			string ltype = type_name(lvalue);
			error("class '%s' dosen't ovreload operator '[]'(1)", ltype.c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	case Data::fmt_function:
		auto signature = lvalue.data<Function>()->mapping.find(static_cast<int>(to_integer(cursor, rvalue)));
		if (signature != lvalue.data<Function>()->mapping.end()) {
			Reference &&result = WeakReference::create<Function>();
			result.data<Function>()->mapping.insert(*signature);
			cursor->stack().pop_back();
			cursor->stack().back() = move(result);
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

	if (UNLIKELY(lvalue.flags() & Reference::const_value)) {
		error("invalid modification of constant value");
	}

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
	case Data::fmt_null:
		cursor->raise(WeakReference::share(lvalue));
		break;
	case Data::fmt_number:
		lvalue.data<Number>()->value -= (static_cast<double>(to_integer(lvalue.data<Number>()->value / pow(10, to_number(cursor, kvalue))) % 10) * pow(10, to_number(cursor, kvalue)));
		lvalue.data<Number>()->value += to_number(cursor, rvalue) * pow(10, to_number(cursor, kvalue));
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		break;
	case Data::fmt_boolean:
		{
			string ltype = type_name(lvalue);
			error("invalid use of '%s' type with operator '[]='", ltype.c_str());
		}
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Class::subscript_move_operator, 2))) {
			string ltype = type_name(lvalue);
			error("class '%s' dosen't ovreload operator '[]='(2)", ltype.c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	case Data::fmt_function:
		string ltype = type_name(lvalue);
		error("invalid use of '%s' type with operator '[]='", ltype.c_str());
	}
}

void mint::regex_match(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);
	Reference &lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
	case Data::fmt_null:
		cursor->raise(WeakReference::share(lvalue));
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Class::regex_match_operator, 1))) {
			string ltype = type_name(lvalue);
			error("class '%s' dosen't ovreload operator '=~'(1)", ltype.c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	case Data::fmt_number:
	case Data::fmt_boolean:
	case Data::fmt_function:
		string ltype = type_name(lvalue);
		error("invalid use of '%s' type with operator '=~'", ltype.c_str());
	}
}

void mint::regex_unmatch(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);
	Reference &lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
	case Data::fmt_null:
		cursor->raise(WeakReference::share(lvalue));
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Class::regex_unmatch_operator, 1))) {
			string ltype = type_name(lvalue);
			error("class '%s' dosen't ovreload operator '!~'(1)", ltype.c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	case Data::fmt_number:
	case Data::fmt_boolean:
	case Data::fmt_function:
		string ltype = type_name(lvalue);
		error("invalid use of '%s' type with operator '!~'", ltype.c_str());
	}
}

void mint::find_defined_symbol(Cursor *cursor, const Symbol &symbol) {

	auto it_local = cursor->symbols().find(symbol);
	if (it_local != cursor->symbols().end()) {
		cursor->stack().emplace_back(WeakReference::share(it_local->second));
		return;
	}

	GlobalData *globalData = GlobalData::instance();
	auto it_global = globalData->symbols().find(symbol);
	if (it_global != globalData->symbols().end()) {
		cursor->stack().emplace_back(WeakReference::share(it_global->second));
		return;
	}

	cursor->stack().emplace_back(WeakReference::create<None>());
}

void mint::find_defined_member(Cursor *cursor, const Symbol &symbol) {

	if (cursor->stack().back().data()->format != Data::fmt_none) {

		WeakReference value = move(cursor->stack().back());
		cursor->stack().pop_back();

		switch (value.data()->format) {
		case Data::fmt_package:
			if (Package *package = value.data<Package>()) {

				auto it_package = package->data->symbols().find(symbol);
				if (it_package != package->data->symbols().end()) {
					cursor->stack().emplace_back(WeakReference::share(it_package->second));
					return;
				}
			}

			cursor->stack().emplace_back(WeakReference::create<None>());
			break;

		case Data::fmt_object:
			if (Object *object = value.data<Object>()) {

				auto it_local = object->metadata->members().find(symbol);
				if (it_local != object->metadata->members().end()) {
					cursor->stack().emplace_back(WeakReference::share(object->data[it_local->second->offset]));
					return;
				}

				auto it_global = object->metadata->globals().find(symbol);
				if (it_global != object->metadata->globals().end()) {
					cursor->stack().emplace_back(WeakReference::share(it_global->second->value));
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
	WeakReference value = move(cursor->stack().back());
	cursor->stack().back() = WeakReference::create<Boolean>(value.data()->format != Data::fmt_none);
}

void mint::find_operator(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);

	Reference &range = load_from_stack(cursor, base);
	Reference &value = load_from_stack(cursor, base - 1);

	switch (range.data()->format) {
	case Data::fmt_object:
		cursor->stack().emplace_back(WeakReference::share(value));
		if (!call_overload(cursor, Class::in_operator, 1)) {
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

	if (range.data()->format != Data::fmt_boolean) {
		cursor->stack().back() = WeakReference::create(iterator_init(range));
	}
}

void mint::find_next(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);

	Reference &range = load_from_stack(cursor, base);
	Reference &value = load_from_stack(cursor, base - 1);

	if (range.data()->format == Data::fmt_boolean) {
		cursor->stack().emplace_back(WeakReference::create(range.data()));
	}
	else {
		Iterator *iterator = range.data<Iterator>();
		assert(iterator != nullptr);
		if (optional<WeakReference> &&item = iterator_next(iterator)) {
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

	if (range.data()->format == Data::fmt_boolean) {
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().back() = move(found);
		cursor->jmp(pos);
	}
	else if (to_boolean(cursor, found)) {
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().back() = move(found);
		cursor->jmp(pos);
	}
	else if (range.data<Iterator>()->ctx.empty()) {
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().back() = move(found);
		cursor->jmp(pos);
	}
	else {
		cursor->stack().pop_back();
	}
}

void mint::in_operator(Cursor *cursor) {

	Reference &range = cursor->stack().back();

	if (range.data()->format == Data::fmt_object) {
		call_overload(cursor, Class::in_operator, 0);
	}
}

void mint::range_init(Cursor *cursor) {

	Reference &range = cursor->stack().back();

	if (range.data()->format != Data::fmt_object || range.data<Object>()->metadata->metatype() != Class::iterator) {
		cursor->stack().back() = WeakReference::create(iterator_init(forward<Reference>(range)));
	}
}

void mint::range_next(Cursor *cursor) {
	cursor->stack().back().data<Iterator>()->ctx.pop_front();
}

void mint::range_check(Cursor *cursor, size_t pos) {

	const size_t base = get_stack_base(cursor);

	Reference &range = load_from_stack(cursor, base);
	Reference &target = load_from_stack(cursor, base - 1);

	if (optional<WeakReference> &&item = iterator_get(range.data<Iterator>())) {

		if (UNLIKELY((target.flags() & Reference::const_address) && (target.data()->format != Data::fmt_none))) {
			error("invalid modification of constant reference");
		}

		if ((item->flags() & (Reference::const_value | Reference::temporary)) == Reference::const_value) {
			target.copy(*item);
		}
		else {
			target.move(*item);
		}
	}
	else {
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->jmp(pos);
	}
}

void mint::range_iterator_finalize(Cursor *cursor) {

	Reference &range = cursor->stack().back();

	if (optional<WeakReference> &&item = iterator_get(range.data<Iterator>())) {
		if (item->data()->format == Data::fmt_object && item->data<Object>()->metadata->metatype() == Class::iterator) {
			item->data<Iterator>()->ctx.finalize();
		}
	}
}

void mint::range_iterator_check(Cursor *cursor, size_t pos) {

	const size_t base = get_stack_base(cursor);

	Reference &range = load_from_stack(cursor, base);
	Reference &target = load_from_stack(cursor, base - 1);

	if (optional<WeakReference> &&item = iterator_get(range.data<Iterator>())) {

		Iterator::ctx_type::iterator it = target.data<Iterator>()->ctx.begin();
		const Iterator::ctx_type::iterator end = target.data<Iterator>()->ctx.end();

		for_each_if(*item, [&it, &end] (const Reference &item) -> bool {
			if (it != end) {
				if (UNLIKELY((it->flags() & Reference::const_address) && (it->data()->format != Data::fmt_none))) {
					error("invalid modification of constant reference");
				}

				it->move(item);
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
#if !defined (__x86_64__) && !defined (_WIN64)
static constexpr const size_t fnv_prime = 16777619u;
static constexpr const size_t offset_basis = 2166136261u;
#else
static constexpr const size_t fnv_prime = 1099511628211u;
static constexpr const size_t offset_basis = 14695981039346656037u;
#endif
}

size_t Hash::hash::operator ()(const Hash::key_type &value) const {

	switch (value.data()->format) {
	case Data::fmt_none:
		return size_t{};

	case Data::fmt_null:
#if (__cplusplus >= 201703L) || (defined(_MSC_VER) && _MSC_VER >= 1911)
		return std::hash<nullptr_t>{}(nullptr);
#else
		return std::hash<void *>{}(nullptr);
#endif

	case Data::fmt_number:
		return std::hash<double>{}(value.data<Number>()->value);

	case Data::fmt_boolean:
		return std::hash<bool>{}(value.data<Boolean>()->value);

	case Data::fmt_object:
		switch (value.data<Object>()->metadata->metatype()) {
		case Class::object:
			return std::hash<WeakReference *>{}(value.data<Object>()->data);

		case Class::string:
			return std::hash<std::string>{}(value.data<String>()->str);

		case Class::regex:
			return std::hash<std::string>{}(value.data<Regex>()->initializer);

		case Class::array:
			return [this, &value] {
				size_t hash = offset_basis;
				for (auto i = value.data<Array>()->values.begin(); i != value.data<Array>()->values.end(); ++i) {
					hash = hash * fnv_prime;
					hash = hash ^ operator ()(array_get_item(i));
				}
				return hash;
			} ();

		case Class::hash:
		case Class::iterator:
		case Class::library:
		case Class::libobject:
			string type = type_name(value);
			error("invalid use of '%s' type as hash key", type.c_str());
		}
		break;
	case Data::fmt_package:
	case Data::fmt_function:
		string type = type_name(value);
		error("invalid use of '%s' type as hash key", type.c_str());
	}

	return false;
}

bool Hash::equal_to::operator ()(const Hash::key_type &lvalue, const Hash::key_type &rvalue) const {

	if (lvalue.data()->format != rvalue.data()->format) {
		return false;
	}

	switch (lvalue.data()->format) {
	case Data::fmt_none:
	case Data::fmt_null:
		return true;

	case Data::fmt_number:
		return lvalue.data<Number>()->value == rvalue.data<Number>()->value;

	case Data::fmt_boolean:
		return lvalue.data<Boolean>()->value == rvalue.data<Boolean>()->value;

	case Data::fmt_object:
		if (lvalue.data<Object>()->metadata->metatype() != rvalue.data<Object>()->metadata->metatype()) {
			return false;
		}

		switch (lvalue.data<Object>()->metadata->metatype()) {
		case Class::object:
			if (lvalue.data<Object>()->metadata != rvalue.data<Object>()->metadata) {
				return false;
			}
			return lvalue.data<Object>()->data == rvalue.data<Object>()->data;

		case Class::string:
			return lvalue.data<String>()->str == rvalue.data<String>()->str;

		case Class::regex:
			return lvalue.data<Regex>()->initializer == rvalue.data<Regex>()->initializer;

		case Class::array:
			if (lvalue.data<Array>()->values.size() != rvalue.data<Array>()->values.size()) {
				return false;
			}
			for (auto i = lvalue.data<Array>()->values.begin(), j = rvalue.data<Array>()->values.begin();
				 i != lvalue.data<Array>()->values.end() && j != rvalue.data<Array>()->values.end(); ++i, ++j) {
				if (!operator ()(array_get_item(i), array_get_item(j))) {
					return false;
				}
			}
			return true;

		case Class::hash:
		case Class::iterator:
		case Class::library:
		case Class::libobject:
			string ltype = type_name(lvalue);
			error("invalid use of '%s' type as hash key", ltype.c_str());
		}
		break;
	case Data::fmt_package:
	case Data::fmt_function:
		string ltype = type_name(lvalue);
		error("invalid use of '%s' type as hash key", ltype.c_str());
	}

	return false;
}

bool Hash::compare_to::operator ()(const Hash::key_type &lvalue, const Hash::key_type &rvalue) const {

	if (lvalue.data()->format != rvalue.data()->format) {
		return lvalue.data()->format < rvalue.data()->format;
	}

	switch (lvalue.data()->format) {
	case Data::fmt_none:
	case Data::fmt_null:
		return false;

	case Data::fmt_number:
		return lvalue.data<Number>()->value < rvalue.data<Number>()->value;

	case Data::fmt_boolean:
		return lvalue.data<Boolean>()->value < rvalue.data<Boolean>()->value;

	case Data::fmt_object:
		if (lvalue.data<Object>()->metadata->metatype() != rvalue.data<Object>()->metadata->metatype()) {
			return lvalue.data<Object>()->metadata->metatype() < rvalue.data<Object>()->metadata->metatype();
		}

		switch (lvalue.data<Object>()->metadata->metatype()) {
		case Class::object:
			if (lvalue.data<Object>()->metadata != rvalue.data<Object>()->metadata) {
				return lvalue.data<Object>()->metadata < rvalue.data<Object>()->metadata;
			}
			return lvalue.data<Object>()->data < rvalue.data<Object>()->data;

		case Class::string:
			return lvalue.data<String>()->str < rvalue.data<String>()->str;

		case Class::regex:
			return lvalue.data<Regex>()->initializer < rvalue.data<Regex>()->initializer;

		case Class::array:
			for (auto i = lvalue.data<Array>()->values.begin(), j = rvalue.data<Array>()->values.begin();
				 i != lvalue.data<Array>()->values.end() && j != rvalue.data<Array>()->values.end(); ++i, ++j) {
				if (operator ()(array_get_item(i), array_get_item(j))) {
					return true;
				}
				if (operator ()(array_get_item(j), array_get_item(i))) {
					return false;
				}
			}
			return lvalue.data<Array>()->values.size() < rvalue.data<Array>()->values.size();

		case Class::hash:
		case Class::iterator:
		case Class::library:
		case Class::libobject:
			string ltype = type_name(lvalue);
			error("invalid use of '%s' type as hash key", ltype.c_str());
		}
		break;
	case Data::fmt_package:
	case Data::fmt_function:
		string ltype = type_name(lvalue);
		error("invalid use of '%s' type as hash key", ltype.c_str());
	}

	return false;
}
