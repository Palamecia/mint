#include "memory/operatortool.h"
#include "memory/memorytool.h"
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
			cursor->raise(move(function));
			break;
		case Data::fmt_number:
			if (signature == 0) {
				WeakReference result = WeakReference::create<Number>();
				result.copy(function);
				cursor->stack().back() = move(result);
			}
			else {
				error("number copy doesn't take %d argument(s)", signature);
			}
			break;
		case Data::fmt_boolean:
			if (signature == 0) {
				WeakReference result = WeakReference::create<Boolean>();
				result.copy(function);
				cursor->stack().back() = move(result);
			}
			else {
				error("boolean copy doesn't take %d argument(s)", signature);
			}
			break;
		case Data::fmt_object:
			if (signature == 0) {
				WeakReference result = WeakReference::create<None>();
				result.copy(function);
				cursor->stack().back() = move(result);
			}
			else {
				error("object copy doesn't take %d argument(s)", signature);
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

	if ((rvalue.flags() & Reference::const_value) && !(rvalue.flags() & Reference::temporary)) {
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
		cursor->raise(move(lvalue));
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
			error("invalid conversion from '%s' to '%s'", type_name(rvalue).c_str(), type_name(lvalue).c_str());
		}
		lvalue.data<Function>()->mapping = rvalue.data<Function>()->mapping;
		cursor->stack().pop_back();
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, Symbol::CopyOperator, 1)) {
			if (UNLIKELY(rvalue.data()->format != Data::fmt_object)) {
				error("cannot convert '%s' to '%s' in assignment", type_name(rvalue).c_str(), type_name(lvalue).c_str());
			}
			if (UNLIKELY(lvalue.data<Object>()->metadata != rvalue.data<Object>()->metadata)) {
				error("cannot convert '%s' to '%s' in assignment", type_name(rvalue).c_str(), type_name(lvalue).c_str());
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
		cursor->raise(move(function));
		break;
	case Data::fmt_number:
		if (signature == 0) {
			WeakReference result = WeakReference::create<Number>();
			result.copy(function);
			cursor->stack().emplace_back(move(result));
		}
		else {
			error("number copy doesn't take %d argument(s)", signature);
		}
		break;
	case Data::fmt_boolean:
		if (signature == 0) {
			WeakReference result = WeakReference::create<Boolean>();
			result.copy(function);
			cursor->stack().emplace_back(move(result));
		}
		else {
			error("boolean copy doesn't take %d argument(s)", signature);
		}
		break;
	case Data::fmt_object:
		if (signature == 0) {
			WeakReference result = WeakReference::create<None>();
			result.copy(function);
			cursor->stack().emplace_back(move(result));
		}
		else {
			error("object copy doesn't take %d argument(s)", signature);
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
		cursor->raise(move(function));
		break;
	case Data::fmt_number:
		if (signature == 0) {
			WeakReference result = WeakReference::create<Number>();
			result.copy(function);
			cursor->stack().back() = move(result);
		}
		else {
			error("number copy doesn't take %d argument(s)", signature);
		}
		break;
	case Data::fmt_boolean:
		if (signature == 0) {
			WeakReference result = WeakReference::create<Boolean>();
			result.copy(function);
			cursor->stack().back() = move(result);
		}
		else {
			error("boolean copy doesn't take %d argument(s)", signature);
		}
		break;
	case Data::fmt_object:
		if (signature == 0) {
			WeakReference result = WeakReference::create<None>();
			result.copy(function);
			cursor->stack().back() = move(result);
		}
		else {
			error("object copy doesn't take %d argument(s)", signature);
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
		cursor->raise(move(lvalue));
		break;
	case Data::fmt_number:
	{
		Reference &&result = WeakReference::create<Number>(lvalue.data<Number>()->value + to_number(cursor, rvalue));
		cursor->stack().pop_back();
		cursor->stack().back() = move(result);
	}
		break;
	case Data::fmt_boolean:
	{
		Reference &&result = WeakReference::create<Boolean>(lvalue.data<Boolean>()->value + to_boolean(cursor, rvalue));
		cursor->stack().pop_back();
		cursor->stack().back() = move(result);
	}
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Symbol::AddOperator, 1))) {
			error("class '%s' dosen't ovreload operator '+'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	case Data::fmt_function:
	{
		Reference &&result = WeakReference::create<Function>();
		if (UNLIKELY(rvalue.data()->format != Data::fmt_function)) {
			error("invalid use of operator '+' with '%s' and '%s' types", type_name(lvalue).c_str(), type_name(rvalue).c_str());
		}
		for (auto item : lvalue.data<Function>()->mapping) {
			result.data<Function>()->mapping.insert(item);
		}
		for (auto item : rvalue.data<Function>()->mapping) {
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
		cursor->raise(move(lvalue));
		break;
	case Data::fmt_number:
	{
		Reference &&result = WeakReference::create<Number>(lvalue.data<Number>()->value - to_number(cursor, rvalue));
		cursor->stack().pop_back();
		cursor->stack().back() = move(result);
	}
		break;
	case Data::fmt_boolean:
	{
		Reference &&result = WeakReference::create<Boolean>(lvalue.data<Boolean>()->value - to_boolean(cursor, rvalue));
		cursor->stack().pop_back();
		cursor->stack().back() = move(result);
	}
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Symbol::SubOperator, 1))) {
			error("class '%s' dosen't ovreload operator '-'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '-'", type_name(lvalue).c_str());
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
		cursor->raise(move(lvalue));
		break;
	case Data::fmt_number:
	{
		Reference &&result = WeakReference::create<Number>(lvalue.data<Number>()->value * to_number(cursor, rvalue));
		cursor->stack().pop_back();
		cursor->stack().back() = move(result);
	}
		break;
	case Data::fmt_boolean:
	{
		Reference &&result = WeakReference::create<Boolean>(lvalue.data<Boolean>()->value && to_boolean(cursor, rvalue));
		cursor->stack().pop_back();
		cursor->stack().back() = move(result);
	}
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Symbol::MulOperator, 1))) {
			error("class '%s' dosen't ovreload operator '*'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '*'", type_name(lvalue).c_str());
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
		cursor->raise(move(lvalue));
		break;
	case Data::fmt_number:
	{
		Reference &&result = WeakReference::create<Number>(lvalue.data<Number>()->value / to_number(cursor, rvalue));
		cursor->stack().pop_back();
		cursor->stack().back() = move(result);
	}
		break;
	case Data::fmt_boolean:
	{
		Reference &&result = WeakReference::create<Boolean>(lvalue.data<Boolean>()->value / to_boolean(cursor, rvalue));
		cursor->stack().pop_back();
		cursor->stack().back() = move(result);
	}
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Symbol::DivOperator, 1))) {
			error("class '%s' dosen't ovreload operator '/'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '/'", type_name(lvalue).c_str());
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
		cursor->raise(move(lvalue));
		break;
	case Data::fmt_number:
	{
		Reference &&result = WeakReference::create<Number>(pow(lvalue.data<Number>()->value, to_number(cursor, rvalue)));
		cursor->stack().pop_back();
		cursor->stack().back() = move(result);
	}
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Symbol::PowOperator, 1))) {
			error("class '%s' dosen't ovreload operator '**'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	case Data::fmt_boolean:
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '**'", type_name(lvalue).c_str());
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
		cursor->raise(move(lvalue));
		break;
	case Data::fmt_number:
		if (intmax_t divider = to_integer(cursor, rvalue)) {
			Reference &&result = WeakReference::create<Number>(static_cast<double>(to_integer(lvalue.data<Number>()->value) % divider));
			cursor->stack().pop_back();
			cursor->stack().back() = move(result);
		}
		else {
			error("modulo by zero");
		}
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Symbol::ModOperator, 1))) {
			error("class '%s' dosen't ovreload operator '%%'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	case Data::fmt_boolean:
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '%%'", type_name(lvalue).c_str());
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
			Reference &&result = WeakReference::create<Boolean>(false);cursor->stack().pop_back();
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
		if (!call_overload(cursor, Symbol::EqOperator, 1)) {
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
				error("class '%s' dosen't ovreload operator '=='(1)", type_name(lvalue).c_str());
			}
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '=='", type_name(lvalue).c_str());
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
		if (!call_overload(cursor, Symbol::NeOperator, 1)) {
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
				error("class '%s' dosen't ovreload operator '!='(1)", type_name(lvalue).c_str());
			}
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '!='", type_name(lvalue).c_str());
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
		cursor->raise(move(lvalue));
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
		if (UNLIKELY(!call_overload(cursor, Symbol::LtOperator, 1))) {
			error("class '%s' dosen't ovreload operator '<'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '<'", type_name(lvalue).c_str());
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
		cursor->raise(move(lvalue));
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
		if (UNLIKELY(!call_overload(cursor, Symbol::GtOperator, 1))) {
			error("class '%s' dosen't ovreload operator '>'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '>'", type_name(lvalue).c_str());
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
		cursor->raise(move(lvalue));
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
		if (UNLIKELY(!call_overload(cursor, Symbol::LeOperator, 1))) {
			error("class '%s' dosen't ovreload operator '<='(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '<='", type_name(lvalue).c_str());
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
		cursor->raise(move(lvalue));
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
		if (UNLIKELY(!call_overload(cursor, Symbol::GeOperator, 1))) {
			error("class '%s' dosen't ovreload operator '>='(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '>='", type_name(lvalue).c_str());
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
		if (!call_overload(cursor, Symbol::AndOperator, 1)) {
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
			if (value.data<Object>()->metadata->members().find(Symbol::OrOperator) == value.data<Object>()->metadata->members().end()) {
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
		if (UNLIKELY(!call_overload(cursor, Symbol::OrOperator, 1))) {
			error("class '%s' dosen't ovreload operator '||'(1)", type_name(lvalue).c_str());
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
		cursor->raise(move(lvalue));
		break;
	case Data::fmt_number:
	{
		Reference &&result = WeakReference::create<Number>(static_cast<double>(to_integer(lvalue.data<Number>()->value) & to_integer(cursor, rvalue)));
		cursor->stack().pop_back();
		cursor->stack().back() = move(result);
	}
		break;
	case Data::fmt_boolean:
	{
		Reference &&result = WeakReference::create<Boolean>();
		result.data<Boolean>()->value = lvalue.data<Boolean>()->value & to_boolean(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().back() = move(result);
	}
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Symbol::BandOperator, 1))) {
			error("class '%s' dosen't ovreload operator '&'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '&'", type_name(lvalue).c_str());
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
		cursor->raise(move(lvalue));
		break;
	case Data::fmt_number:
	{
		Reference &&result = WeakReference::create<Number>(static_cast<double>(to_integer(lvalue.data<Number>()->value) | to_integer(cursor, rvalue)));
		cursor->stack().pop_back();
		cursor->stack().back() = move(result);
	}
		break;
	case Data::fmt_boolean:
	{
		Reference &&result = WeakReference::create<Boolean>(lvalue.data<Boolean>()->value | to_boolean(cursor, rvalue));
		cursor->stack().pop_back();
		cursor->stack().back() = move(result);
	}
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Symbol::BorOperator, 1))) {
			error("class '%s' dosen't ovreload operator '|'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '|'", type_name(lvalue).c_str());
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
		cursor->raise(move(lvalue));
		break;
	case Data::fmt_number:
	{
		Reference &&result = WeakReference::create<Number>(static_cast<double>(to_integer(lvalue.data<Number>()->value) ^ to_integer(cursor, rvalue)));
		cursor->stack().pop_back();
		cursor->stack().back() = move(result);
	}
		break;
	case Data::fmt_boolean:
	{
		Reference &&result = WeakReference::create<Boolean>(to_integer(lvalue.data<Number>()->value) ^ to_boolean(cursor, rvalue));
		cursor->stack().pop_back();
		cursor->stack().back() = move(result);
	}
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Symbol::XorOperator, 1))) {
			error("class '%s' dosen't ovreload operator '^'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '^'", type_name(lvalue).c_str());
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
		cursor->raise(move(value));
		break;
	case Data::fmt_number:
		value.move(WeakReference(Reference::const_address | Reference::const_value, Reference::alloc<Number>(value.data<Number>()->value + 1)));
		break;
	case Data::fmt_boolean:
		value.move(WeakReference(Reference::const_address | Reference::const_value, Reference::alloc<Boolean>(value.data<Boolean>()->value + 1)));
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Symbol::IncOperator, 0))) {
			error("class '%s' dosen't ovreload operator '++'(0)", type_name(value).c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '++'", type_name(value).c_str());
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
		cursor->raise(move(value));
		break;
	case Data::fmt_number:
		value.move(WeakReference(Reference::const_address | Reference::const_value, Reference::alloc<Number>(value.data<Number>()->value - 1)));
		break;
	case Data::fmt_boolean:
		value.move(WeakReference(Reference::const_address | Reference::const_value, Reference::alloc<Boolean>(value.data<Boolean>()->value - 1)));
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Symbol::DecOperator, 0))) {
			error("class '%s' dosen't ovreload operator '--'(0)", type_name(value).c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '--'", type_name(value).c_str());
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
		if (UNLIKELY(!call_overload(cursor, Symbol::NotOperator, 0))) {
			error("class '%s' dosen't ovreload operator '!'(0)", type_name(value).c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '!'", type_name(value).c_str());
	}
}

void mint::compl_operator(Cursor *cursor) {

	Reference &value = cursor->stack().back();

	switch (value.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
	case Data::fmt_null:
		cursor->raise(move(value));
		break;
	case Data::fmt_number:
		cursor->stack().back() = WeakReference::create<Number>(static_cast<double>(~(to_integer(cursor, value))));
		break;
	case Data::fmt_boolean:
		cursor->stack().back() = WeakReference::create<Boolean>(!value.data<Boolean>()->value);
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Symbol::ComplOperator, 0))) {
			error("class '%s' dosen't ovreload operator '~'(0)", type_name(value).c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '~'", type_name(value).c_str());
	}
}

void mint::pos_operator(Cursor *cursor) {

	Reference &value = cursor->stack().back();

	switch (value.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
	case Data::fmt_null:
		cursor->raise(move(value));
		break;
	case Data::fmt_number:
		cursor->stack().back() = WeakReference::create<Number>(+(value.data<Number>()->value));
		break;
	case Data::fmt_boolean:
		cursor->stack().back() = WeakReference::create<Boolean>(+(value.data<Boolean>()->value));
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Symbol::AddOperator, 0))) {
			error("class '%s' dosen't ovreload operator '+'(0)", type_name(value).c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '+'", type_name(value).c_str());
	}
}

void mint::neg_operator(Cursor *cursor) {

	Reference &value = cursor->stack().back();

	switch (value.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
	case Data::fmt_null:
		cursor->raise(move(value));
		break;
	case Data::fmt_number:
		cursor->stack().back() = WeakReference::create<Number>(-(value.data<Number>()->value));
		break;
	case Data::fmt_boolean:
		cursor->stack().back() = WeakReference::create<Boolean>(-(value.data<Boolean>()->value));
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Symbol::SubOperator, 0))) {
			error("class '%s' dosen't ovreload operator '-'(0)", type_name(value).c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '-'", type_name(value).c_str());
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
		cursor->raise(move(lvalue));
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
		if (UNLIKELY(!call_overload(cursor, Symbol::ShiftLeftOperator, 1))) {
			error("class '%s' dosen't ovreload operator '<<'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '<<'", type_name(lvalue).c_str());
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
		cursor->raise(move(lvalue));
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
		if (UNLIKELY(!call_overload(cursor, Symbol::ShiftRightOperator, 1))) {
			error("class '%s' dosen't ovreload operator '>>'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '>>'", type_name(lvalue).c_str());
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
		cursor->raise(move(lvalue));
		break;
	case Data::fmt_number:
	{
		Reference &&result = Iterator::fromInclusiveRange(lvalue.data<Number>()->value, to_number(cursor, rvalue));
		cursor->stack().pop_back();
		cursor->stack().back() = move(result);
	}
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Symbol::InclusiveRangeOperator, 1))) {
			error("class '%s' dosen't ovreload operator '..'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	case Data::fmt_boolean:
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '..'", type_name(lvalue).c_str());
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
		cursor->raise(move(lvalue));
		break;
	case Data::fmt_number:
	{
		Reference &&result = Iterator::fromExclusiveRange(lvalue.data<Number>()->value, to_number(cursor, rvalue));
		cursor->stack().pop_back();
		cursor->stack().back() = move(result);
	}
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Symbol::ExclusiveRangeOperator, 1))) {
			error("class '%s' dosen't ovreload operator '...'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	case Data::fmt_boolean:
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '...'", type_name(lvalue).c_str());
	}
}

void mint::typeof_operator(Cursor *cursor) {
	cursor->stack().back() = create_string(type_name(move(cursor->stack().back())));
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

			for (auto member : object->metadata->members()) {

				if ((member.second->value.flags() & Reference::protected_visibility) && (!is_protected_accessible(member.second->owner, cursor->symbols().getMetadata()))) {
					continue;
				}

				if ((member.second->value.flags() & Reference::private_visibility) && (member.second->owner != cursor->symbols().getMetadata())) {
					continue;
				}

				if ((member.second->value.flags() & Reference::package_visibility) && (member.second->owner->getPackage() != cursor->symbols().getPackage())) {
					continue;
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
		cursor->raise(move(lvalue));
		break;
	case Data::fmt_number:
	{
		WeakReference result = WeakReference::create<Number>();
		result.data<Number>()->value = static_cast<double>(to_integer(lvalue.data<Number>()->value / pow(10, to_number(cursor, rvalue))) % 10);
		cursor->stack().pop_back();
		cursor->stack().back() = move(result);
	}
		break;
	case Data::fmt_boolean:
		error("invalid use of '%s' type with operator '[]'", type_name(lvalue).c_str());
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Symbol::SubscriptOperator, 1))) {
			error("class '%s' dosen't ovreload operator '[]'(1)", lvalue.data<Object>()->metadata->name().c_str());
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
		cursor->raise(move(lvalue));
		break;
	case Data::fmt_number:
		lvalue.data<Number>()->value -= (static_cast<double>(to_integer(lvalue.data<Number>()->value / pow(10, to_number(cursor, kvalue))) % 10) * pow(10, to_number(cursor, kvalue)));
		lvalue.data<Number>()->value += to_number(cursor, rvalue) * pow(10, to_number(cursor, kvalue));
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		break;
	case Data::fmt_boolean:
		error("invalid use of '%s' type with operator '[]='", type_name(lvalue).c_str());
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Symbol::SubscriptMoveOperator, 2))) {
			error("class '%s' dosen't ovreload operator '[]='(2)", lvalue.data<Object>()->metadata->name().c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '[]='", type_name(lvalue).c_str());
	}
}

void mint::regex_match(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);
	Reference &lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
	case Data::fmt_null:
		cursor->raise(move(lvalue));
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Symbol::RegexMatchOperator, 1))) {
			error("class '%s' dosen't ovreload operator '=~'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	case Data::fmt_number:
	case Data::fmt_boolean:
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '=~'", type_name(lvalue).c_str());
	}
}

void mint::regex_unmatch(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);
	Reference &lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
	case Data::fmt_null:
		cursor->raise(move(lvalue));
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Symbol::RegexUnmatchOperator, 1))) {
			error("class '%s' dosen't ovreload operator '!~'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
	case Data::fmt_number:
	case Data::fmt_boolean:
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '!~'", type_name(lvalue).c_str());
	}
}

void mint::find_defined_symbol(Cursor *cursor, const Symbol &symbol) {

	auto it_local = cursor->symbols().find(symbol);
	if (it_local != cursor->symbols().end()) {
		cursor->stack().emplace_back(WeakReference::share(it_local->second));
		return;
	}

	auto it_global = GlobalData::instance().symbols().find(symbol);
	if (it_global != GlobalData::instance().symbols().end()) {
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

				auto it_global = object->metadata->globals().members().find(symbol);
				if (it_global != object->metadata->globals().members().end()) {
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
		if (!call_overload(cursor, Symbol::InOperator, 1)) {
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
		call_overload(cursor, Symbol::InOperator, 0);
	}
}

void mint::range_init(Cursor *cursor) {

	Reference &range = cursor->stack().back();

	if (range.data()->format != Data::fmt_object || range.data<Object>()->metadata->metatype() != Class::iterator) {
		cursor->stack().back() = WeakReference::create(iterator_init(move(range)));
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
		cursor->stack().emplace_back(WeakReference::share(target));
		cursor->stack().emplace_back(move(*item));
		move_operator(cursor);
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

	if (optional<WeakReference> &&item = iterator_get(range.data<Iterator>())) {
		cursor->stack().emplace_back(WeakReference::share(target));
		cursor->stack().emplace_back(move(*item));
		copy_operator(cursor);
	}
	else {
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->jmp(pos);
	}
}

bool Hash::compare::operator ()(const Hash::key_type &lvalue, const Hash::key_type &rvalue) const {

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
			error("invalid use of '%s' type as hash key", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_package:
	case Data::fmt_function:
		error("invalid use of '%s' type as hash key", type_name(lvalue).c_str());
	}

	return false;
}
