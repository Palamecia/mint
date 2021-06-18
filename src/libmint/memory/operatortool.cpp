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

#include <math.h>

using namespace std;
using namespace mint;

bool mint::call_overload(Cursor *cursor, const Symbol &operator_overload, int signature) {

	size_t base = get_stack_base(cursor);
	Object *object = load_from_stack(cursor, base - signature)->data<Object>();
	auto it = object->metadata->members().find(operator_overload);

	if (it != object->metadata->members().end()) {

		if (UNLIKELY(is_class(object))) {
			error("invalid use of class in an operation");
		}

		cursor->waitingCalls().emplace(SharedReference::weak(object->data[it->second->offset]));
		cursor->waitingCalls().top().setMetadata(it->second->owner);
		call_member_operator(cursor, signature);
		return true;
	}

	return false;
}

void mint::move_operator(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	SharedReference &rvalue = load_from_stack(cursor, base);
	SharedReference &lvalue = load_from_stack(cursor, base - 1);

	if (UNLIKELY((lvalue->flags() & Reference::const_address) && (lvalue->data()->format != Data::fmt_none))) {
		error("invalid modification of constant reference");
	}

	if (rvalue->flags() & Reference::const_value) {
		lvalue->copy(*rvalue);
	}
	else {
		lvalue->move(*rvalue);
	}

	cursor->stack().pop_back();
}

void mint::copy_operator(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	SharedReference &rvalue = load_from_stack(cursor, base);
	SharedReference &lvalue = load_from_stack(cursor, base - 1);

	if (UNLIKELY(lvalue->flags() & Reference::const_value)) {
		error("invalid modification of constant value");
	}

	switch (lvalue->data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(move(lvalue));
		break;
	case Data::fmt_number:
		lvalue->data<Number>()->value = to_number(cursor, rvalue);
		cursor->stack().pop_back();
		break;
	case Data::fmt_boolean:
		lvalue->data<Boolean>()->value = to_boolean(cursor, rvalue);
		cursor->stack().pop_back();
		break;
	case Data::fmt_function:
		if (UNLIKELY(rvalue->data()->format != Data::fmt_function)) {
			error("invalid conversion from '%s' to '%s'", type_name(rvalue).c_str(), type_name(lvalue).c_str());
		}
		lvalue->data<Function>()->mapping = rvalue->data<Function>()->mapping;
		cursor->stack().pop_back();
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, Symbol::CopyOperator, 1)) {
			if (UNLIKELY(rvalue->data()->format != Data::fmt_object)) {
				error("cannot convert '%s' to '%s' in assignment", type_name(rvalue).c_str(), type_name(lvalue).c_str());
			}
			if (UNLIKELY(lvalue->data<Object>()->metadata != rvalue->data<Object>()->metadata)) {
				error("cannot convert '%s' to '%s' in assignment", type_name(rvalue).c_str(), type_name(lvalue).c_str());
			}
			delete [] lvalue->data<Object>()->data;
			lvalue->data<Object>()->construct(*rvalue->data<Object>());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
		break;
	}
}

void mint::call_operator(Cursor *cursor, int signature) {

	Cursor::Call call_infos = move(cursor->waitingCalls().top());
	cursor->waitingCalls().pop();

	SharedReference result = nullptr;
	SharedReference lvalue = move(call_infos.function());
	Class *metadata = call_infos.getMetadata();
	bool member = call_infos.getFlags() & Cursor::Call::member_call;
	signature += call_infos.extraArgumentCount();

	switch (lvalue->data()->format) {
	case Data::fmt_none:
		if (LIKELY(member)) {
			if (UNLIKELY(signature)) {
				error("default constructors doesn't take %d argument(s)", signature);
			}
		}
		else {
			error("invalid use of none value as a function");
		}
		break;
	case Data::fmt_null:
		cursor->raise(move(lvalue));
		break;
	case Data::fmt_number:
		result = SharedReference::strong<Number>();
		result->copy(*lvalue);
		cursor->stack().emplace_back(move(result));
		break;
	case Data::fmt_boolean:
		result = SharedReference::strong<Boolean>();
		result->copy(*lvalue);
		cursor->stack().emplace_back(move(result));
		break;
	case Data::fmt_object:
		result = SharedReference::strong<None>();
		result->copy(*lvalue);
		cursor->stack().emplace_back(move(result));
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
		break;
	case Data::fmt_function:
		auto it = find_function_signature(cursor, lvalue->data<Function>()->mapping, signature + (member ? 1 : 0));
		if (UNLIKELY(it == lvalue->data<Function>()->mapping.end())) {
			error("called function doesn't take %d parameter(s)", signature + (member ? 1 : 0));
		}
		const Function::Handler &hanlder = it->second;
		if (cursor->call(hanlder.module, hanlder.offset, hanlder.package, metadata)) {
			if (hanlder.generator) {
				cursor->symbols().defaultResult() = StrongReference(Reference::standard, Reference::alloc<Iterator>(cursor, cursor->stack().size() - (signature + (member ? 1 : 0))));
				cursor->symbols().defaultResult().data<Iterator>()->construct();
				cursor->setExecutionMode(Cursor::interruptible);
			}
			if (hanlder.capture) {
				for (SymbolTable::strong_symbol_type item : *hanlder.capture) {
					cursor->symbols().insert(item);
				}
			}
		}
		break;
	}
}

void mint::call_member_operator(Cursor *cursor, int signature) {

	Cursor::Call call_infos = move(cursor->waitingCalls().top());
	cursor->waitingCalls().pop();

	SharedReference result = nullptr;
	SharedReference lvalue = move(call_infos.function());
	Class *metadata = call_infos.getMetadata();
	bool member = call_infos.getFlags() & Cursor::Call::member_call;
	bool global = lvalue->flags() & Reference::global;
	signature += call_infos.extraArgumentCount();

	switch (lvalue->data()->format) {
	case Data::fmt_none:
		if (LIKELY(member)) {
			if (UNLIKELY(signature)) {
				error("default constructors doesn't take %d argument(s)", signature);
			}
		}
		else {
			error("invalid use of none value as a function");
		}
		break;
	case Data::fmt_null:
		cursor->raise(move(lvalue));
		break;
	case Data::fmt_number:
		result = SharedReference::strong<Number>();
		result->copy(*lvalue);
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(result));
		break;
	case Data::fmt_boolean:
		result = SharedReference::strong<Boolean>();
		result->copy(*lvalue);
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(result));
		break;
	case Data::fmt_object:
		result = SharedReference::strong<None>();
		result->copy(*lvalue);
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(result));
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
		break;
	case Data::fmt_function:
		auto it = find_function_signature(cursor, lvalue->data<Function>()->mapping, signature + (global ? 0 : 1));
		if (UNLIKELY(it == lvalue->data<Function>()->mapping.end())) {
			error("called member doesn't take %d parameter(s)", signature + (global ? 0 : 1));
		}
		const Function::Handler &hanlder = it->second;
		if (cursor->call(hanlder.module, hanlder.offset, hanlder.package, metadata)) {
			if (hanlder.generator) {
				cursor->symbols().defaultResult() = StrongReference(Reference::standard, Reference::alloc<Iterator>(cursor, cursor->stack().size() - (signature + (global ? 0 : 1))));
				cursor->symbols().defaultResult().data<Iterator>()->construct();
				cursor->setExecutionMode(Cursor::interruptible);
			}
			if (hanlder.capture) {
				for (SymbolTable::strong_symbol_type item : *hanlder.capture) {
					cursor->symbols().insert(item);
				}
			}
		}
		break;
	}
}

void mint::add_operator(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	SharedReference &rvalue = load_from_stack(cursor, base);
	SharedReference &lvalue = load_from_stack(cursor, base - 1);
	SharedReference result = nullptr;

	switch (lvalue->data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(move(lvalue));
		break;
	case Data::fmt_number:
		result = SharedReference::strong<Number>();
		result->data<Number>()->value = lvalue->data<Number>()->value + to_number(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(result));
		break;
	case Data::fmt_boolean:
		result = SharedReference::strong<Boolean>();
		result->data<Boolean>()->value = lvalue->data<Boolean>()->value + to_boolean(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(result));
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Symbol::AddOperator, 1))) {
			error("class '%s' dosen't ovreload operator '+'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
		break;
	case Data::fmt_function:
		result = SharedReference::strong<Function>();
		if (UNLIKELY(rvalue->data()->format != Data::fmt_function)) {
			error("invalid use of operator '+' with '%s' and '%s' types", type_name(lvalue).c_str(), type_name(rvalue).c_str());
		}
		for (auto item : lvalue->data<Function>()->mapping) {
			result->data<Function>()->mapping.insert(item);
		}
		for (auto item : rvalue->data<Function>()->mapping) {
			result->data<Function>()->mapping.insert(item);
		}
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(result));
		break;
	}
}

void mint::sub_operator(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	SharedReference &rvalue = load_from_stack(cursor, base);
	SharedReference &lvalue = load_from_stack(cursor, base - 1);
	SharedReference result = nullptr;

	switch (lvalue->data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(move(lvalue));
		break;
	case Data::fmt_number:
		result = SharedReference::strong<Number>();
		result->data<Number>()->value = lvalue->data<Number>()->value - to_number(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(result));
		break;
	case Data::fmt_boolean:
		result = SharedReference::strong<Boolean>();
		result->data<Boolean>()->value = lvalue->data<Boolean>()->value - to_boolean(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(result));
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Symbol::SubOperator, 1))) {
			error("class '%s' dosen't ovreload operator '-'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '-'", type_name(lvalue).c_str());
		break;
	}
}

void mint::mul_operator(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	SharedReference &rvalue = load_from_stack(cursor, base);
	SharedReference &lvalue = load_from_stack(cursor, base - 1);
	SharedReference result = nullptr;

	switch (lvalue->data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(move(lvalue));
		break;
	case Data::fmt_number:
		result = SharedReference::strong<Number>();
		result->data<Number>()->value = lvalue->data<Number>()->value * to_number(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(result));
		break;
	case Data::fmt_boolean:
		result = SharedReference::strong<Boolean>();
		result->data<Boolean>()->value = lvalue->data<Boolean>()->value && to_boolean(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(result));
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Symbol::MulOperator, 1))) {
			error("class '%s' dosen't ovreload operator '*'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '*'", type_name(lvalue).c_str());
	}
}

void mint::div_operator(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	SharedReference &rvalue = load_from_stack(cursor, base);
	SharedReference &lvalue = load_from_stack(cursor, base - 1);
	SharedReference result = nullptr;

	switch (lvalue->data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(move(lvalue));
		break;
	case Data::fmt_number:
		result = SharedReference::strong<Number>();
		result->data<Number>()->value = lvalue->data<Number>()->value / to_number(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(result));
		break;
	case Data::fmt_boolean:
		result = SharedReference::strong<Boolean>();
		result->data<Boolean>()->value = lvalue->data<Boolean>()->value / to_boolean(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(result));
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Symbol::DivOperator, 1))) {
			error("class '%s' dosen't ovreload operator '/'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '/'", type_name(lvalue).c_str());
		break;
	}
}

void mint::pow_operator(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	SharedReference &rvalue = load_from_stack(cursor, base);
	SharedReference &lvalue = load_from_stack(cursor, base - 1);
	SharedReference result = nullptr;

	switch (lvalue->data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(move(lvalue));
		break;
	case Data::fmt_number:
		result = SharedReference::strong<Number>();
		result->data<Number>()->value = pow(lvalue->data<Number>()->value, to_number(cursor, rvalue));
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(result));
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Symbol::PowOperator, 1))) {
			error("class '%s' dosen't ovreload operator '**'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
		break;
	case Data::fmt_boolean:
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '**'", type_name(lvalue).c_str());
		break;
	}
}

void mint::mod_operator(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	SharedReference &rvalue = load_from_stack(cursor, base);
	SharedReference &lvalue = load_from_stack(cursor, base - 1);
	SharedReference result = nullptr;

	switch (lvalue->data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(move(lvalue));
		break;
	case Data::fmt_number:
		if (intmax_t divider = to_integer(cursor, rvalue)) {
			result = SharedReference::strong<Number>();
			result->data<Number>()->value = static_cast<double>(to_integer(lvalue->data<Number>()->value) % divider);
			cursor->stack().pop_back();
			cursor->stack().pop_back();
			cursor->stack().emplace_back(move(result));
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
		break;
	case Data::fmt_boolean:
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '%%'", type_name(lvalue).c_str());
		break;
	}
}

void mint::is_operator(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	SharedReference &rvalue = load_from_stack(cursor, base);
	SharedReference &lvalue = load_from_stack(cursor, base - 1);

	SharedReference result = SharedReference::strong<Boolean>();
	result->data<Boolean>()->value = lvalue->data() == rvalue->data();
	cursor->stack().pop_back();
	cursor->stack().pop_back();
	cursor->stack().emplace_back(move(result));
}

void mint::eq_operator(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	SharedReference &rvalue = load_from_stack(cursor, base);
	SharedReference &lvalue = load_from_stack(cursor, base - 1);
	SharedReference result = nullptr;

	switch (lvalue->data()->format) {
	case Data::fmt_none:
		result = SharedReference::strong<Boolean>();
		result->data<Boolean>()->value = (rvalue->data()->format == Data::fmt_none);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(result));
		break;
	case Data::fmt_null:
		result = SharedReference::strong<Boolean>();
		result->data<Boolean>()->value = (rvalue->data()->format == Data::fmt_null);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(result));
		break;
	case Data::fmt_number:
		result = SharedReference::strong<Boolean>();
		switch (rvalue->data()->format) {
		case Data::fmt_none:
		case Data::fmt_null:
			result->data<Boolean>()->value = false;
			break;
		default:
			result->data<Boolean>()->value = lvalue->data<Number>()->value == to_number(cursor, rvalue);
		}
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(result));
		break;
	case Data::fmt_boolean:
		result = SharedReference::strong<Boolean>();
		switch (rvalue->data()->format) {
		case Data::fmt_none:
		case Data::fmt_null:
			result->data<Boolean>()->value = false;
			break;
		default:
			result->data<Boolean>()->value = lvalue->data<Boolean>()->value == to_boolean(cursor, rvalue);
		}
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(result));
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, Symbol::EqOperator, 1)) {
			result = SharedReference::strong<Boolean>();
			switch (rvalue->data()->format) {
			case Data::fmt_none:
			case Data::fmt_null:
				result->data<Boolean>()->value = false;
				break;
			default:
				error("class '%s' dosen't ovreload operator '=='(1)", type_name(lvalue).c_str());
			}
			cursor->stack().pop_back();
			cursor->stack().pop_back();
			cursor->stack().emplace_back(move(result));
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '=='", type_name(lvalue).c_str());
		break;
	}
}

void mint::ne_operator(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	SharedReference &rvalue = load_from_stack(cursor, base);
	SharedReference &lvalue = load_from_stack(cursor, base - 1);
	SharedReference result = nullptr;

	switch (lvalue->data()->format) {
	case Data::fmt_none:
		result = SharedReference::strong<Boolean>();
		result->data<Boolean>()->value = (rvalue->data()->format != Data::fmt_none);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(result));
		break;
	case Data::fmt_null:
		result = SharedReference::strong<Boolean>();
		result->data<Boolean>()->value = (rvalue->data()->format != Data::fmt_null);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(result));
		break;
	case Data::fmt_number:
		result = SharedReference::strong<Boolean>();
		switch (rvalue->data()->format) {
		case Data::fmt_none:
		case Data::fmt_null:
			result->data<Boolean>()->value = true;
			break;
		default:
			result->data<Boolean>()->value = lvalue->data<Number>()->value != to_number(cursor, rvalue);
		}
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(result));
		break;
	case Data::fmt_boolean:
		result = SharedReference::strong<Boolean>();
		switch (rvalue->data()->format) {
		case Data::fmt_none:
		case Data::fmt_null:
			result->data<Boolean>()->value = true;
			break;
		default:
			result->data<Boolean>()->value = lvalue->data<Boolean>()->value != to_boolean(cursor, rvalue);
		}
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(result));
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, Symbol::NeOperator, 1)) {
			result = SharedReference::strong<Boolean>();
			switch (rvalue->data()->format) {
			case Data::fmt_none:
			case Data::fmt_null:
				result->data<Boolean>()->value = true;
				break;
			default:
				error("class '%s' dosen't ovreload operator '!='(1)", type_name(lvalue).c_str());
			}
			cursor->stack().pop_back();
			cursor->stack().pop_back();
			cursor->stack().emplace_back(move(result));
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '!='", type_name(lvalue).c_str());
		break;
	}
}

void mint::lt_operator(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	SharedReference &rvalue = load_from_stack(cursor, base);
	SharedReference &lvalue = load_from_stack(cursor, base - 1);
	SharedReference result = nullptr;

	switch (lvalue->data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(move(lvalue));
		break;
	case Data::fmt_number:
		result = SharedReference::strong<Boolean>();
		result->data<Boolean>()->value = lvalue->data<Number>()->value < to_number(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(result));
		break;
	case Data::fmt_boolean:
		result = SharedReference::strong<Boolean>();
		result->data<Boolean>()->value = lvalue->data<Boolean>()->value < to_boolean(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(result));
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Symbol::LtOperator, 1))) {
			error("class '%s' dosen't ovreload operator '<'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '<'", type_name(lvalue).c_str());
		break;
	}
}

void mint::gt_operator(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	SharedReference &rvalue = load_from_stack(cursor, base);
	SharedReference &lvalue = load_from_stack(cursor, base - 1);
	SharedReference result = nullptr;

	switch (lvalue->data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(move(lvalue));
		break;
	case Data::fmt_number:
		result = SharedReference::strong<Boolean>();
		result->data<Boolean>()->value = lvalue->data<Number>()->value > to_number(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(result));
		break;
	case Data::fmt_boolean:
		result = SharedReference::strong<Boolean>();
		result->data<Boolean>()->value = lvalue->data<Boolean>()->value > to_boolean(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(result));
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Symbol::GtOperator, 1))) {
			error("class '%s' dosen't ovreload operator '>'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '>'", type_name(lvalue).c_str());
		break;
	}
}

void mint::le_operator(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	SharedReference &rvalue = load_from_stack(cursor, base);
	SharedReference &lvalue = load_from_stack(cursor, base - 1);
	SharedReference result = nullptr;

	switch (lvalue->data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(move(lvalue));
		break;
	case Data::fmt_number:
		result = SharedReference::strong<Boolean>();
		result->data<Boolean>()->value = lvalue->data<Number>()->value <= to_number(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(result));
		break;
	case Data::fmt_boolean:
		result = SharedReference::strong<Boolean>();
		result->data<Boolean>()->value = lvalue->data<Boolean>()->value <= to_boolean(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(result));
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Symbol::LeOperator, 1))) {
			error("class '%s' dosen't ovreload operator '<='(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '<='", type_name(lvalue).c_str());
		break;
	}
}

void mint::ge_operator(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	SharedReference &rvalue = load_from_stack(cursor, base);
	SharedReference &lvalue = load_from_stack(cursor, base - 1);
	SharedReference result = nullptr;

	switch (lvalue->data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(move(lvalue));
		break;
	case Data::fmt_number:
		result = SharedReference::strong<Boolean>();
		result->data<Boolean>()->value = lvalue->data<Number>()->value >= to_number(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(result));
		break;
	case Data::fmt_boolean:
		result = SharedReference::strong<Boolean>();
		result->data<Boolean>()->value = lvalue->data<Boolean>()->value >= to_boolean(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(result));
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Symbol::GeOperator, 1))) {
			error("class '%s' dosen't ovreload operator '>='(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '>='", type_name(lvalue).c_str());
		break;
	}
}

void mint::and_operator(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	SharedReference &rvalue = load_from_stack(cursor, base);
	SharedReference &lvalue = load_from_stack(cursor, base - 1);
	SharedReference result = nullptr;

	switch (lvalue->data()->format) {
	case Data::fmt_none:
	case Data::fmt_null:
		result = create_boolean(false);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(result));
		break;
	case Data::fmt_number:
		result = SharedReference::strong<Boolean>();
		result->data<Boolean>()->value = lvalue->data<Number>()->value && to_boolean(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(result));
		break;
	case Data::fmt_boolean:
		result = SharedReference::strong<Boolean>();
		result->data<Boolean>()->value = lvalue->data<Boolean>()->value && to_boolean(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(result));
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Symbol::AndOperator, 1))) {
			error("class '%s' dosen't ovreload operator '&&'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '&&'", type_name(lvalue).c_str());
		break;
	}
}

void mint::or_operator(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	SharedReference &rvalue = load_from_stack(cursor, base);
	SharedReference &lvalue = load_from_stack(cursor, base - 1);
	SharedReference result = nullptr;

	switch (lvalue->data()->format) {
	case Data::fmt_none:
	case Data::fmt_null:
		result = create_boolean(to_boolean(cursor, rvalue));
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(result));
		break;
	case Data::fmt_number:
		result = SharedReference::strong<Boolean>();
		result->data<Boolean>()->value = lvalue->data<Number>()->value || to_boolean(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(result));
		break;
	case Data::fmt_boolean:
		result = SharedReference::strong<Boolean>();
		result->data<Boolean>()->value = lvalue->data<Boolean>()->value || to_boolean(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(result));
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Symbol::OrOperator, 1))) {
			error("class '%s' dosen't ovreload operator '||'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '||'", type_name(lvalue).c_str());
		break;
	}
}

void mint::band_operator(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	SharedReference &rvalue = load_from_stack(cursor, base);
	SharedReference &lvalue = load_from_stack(cursor, base - 1);
	SharedReference result = nullptr;

	switch (lvalue->data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(move(lvalue));
		break;
	case Data::fmt_number:
		result = SharedReference::strong<Number>();
		result->data<Number>()->value = static_cast<double>(to_integer(lvalue->data<Number>()->value) & to_integer(cursor, rvalue));
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(result));
		break;
	case Data::fmt_boolean:
		result = SharedReference::strong<Boolean>();
		result->data<Boolean>()->value = lvalue->data<Boolean>()->value & to_boolean(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(result));
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Symbol::BandOperator, 1))) {
			error("class '%s' dosen't ovreload operator '&'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '&'", type_name(lvalue).c_str());
		break;
	}
}

void mint::bor_operator(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	SharedReference &rvalue = load_from_stack(cursor, base);
	SharedReference &lvalue = load_from_stack(cursor, base - 1);
	SharedReference result = nullptr;

	switch (lvalue->data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(move(lvalue));
		break;
	case Data::fmt_number:
		result = SharedReference::strong<Number>();
		result->data<Number>()->value = static_cast<double>(to_integer(lvalue->data<Number>()->value) | to_integer(cursor, rvalue));
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(result));
		break;
	case Data::fmt_boolean:
		result = SharedReference::strong<Boolean>();
		result->data<Boolean>()->value = lvalue->data<Boolean>()->value | to_boolean(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(result));
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Symbol::BorOperator, 1))) {
			error("class '%s' dosen't ovreload operator '|'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '|'", type_name(lvalue).c_str());
		break;
	}
}

void mint::xor_operator(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	SharedReference &rvalue = load_from_stack(cursor, base);
	SharedReference &lvalue = load_from_stack(cursor, base - 1);
	SharedReference result = nullptr;

	switch (lvalue->data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(move(lvalue));
		break;
	case Data::fmt_number:
		result = SharedReference::strong<Number>();
		result->data<Number>()->value = static_cast<double>(to_integer(lvalue->data<Number>()->value) ^ to_integer(cursor, rvalue));
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(result));
		break;
	case Data::fmt_boolean:
		result = SharedReference::strong<Boolean>();
		result->data<Boolean>()->value = to_integer(lvalue->data<Number>()->value) ^ to_boolean(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(result));
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Symbol::XorOperator, 1))) {
			error("class '%s' dosen't ovreload operator '^'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '^'", type_name(lvalue).c_str());
		break;
	}
}

void mint::inc_operator(Cursor *cursor) {

	SharedReference &value = cursor->stack().back();
	SharedReference result = nullptr;

	if (value->flags() & Reference::const_value) {
		error("invalid modification of constant value");
	}

	switch (value->data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(move(value));
		break;
	case Data::fmt_number:
		result = SharedReference::strong<Number>();
		result->data<Number>()->value = value->data<Number>()->value + 1;
		value->move(*result);
		break;
	case Data::fmt_boolean:
		result = SharedReference::strong<Boolean>();
		result->data<Boolean>()->value = value->data<Boolean>()->value + 1;
		value->move(*result);
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Symbol::IncOperator, 0))) {
			error("class '%s' dosen't ovreload operator '++'(0)", type_name(value).c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '++'", type_name(value).c_str());
		break;
	}
}

void mint::dec_operator(Cursor *cursor) {

	SharedReference &value = cursor->stack().back();
	SharedReference result = nullptr;

	if (value->flags() & Reference::const_value) {
		error("invalid modification of constant value");
	}

	switch (value->data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(move(value));
		break;
	case Data::fmt_number:
		result = SharedReference::strong<Number>();
		result->data<Number>()->value = value->data<Number>()->value - 1;
		value->move(*result);
		break;
	case Data::fmt_boolean:
		result = SharedReference::strong<Boolean>();
		result->data<Boolean>()->value = value->data<Boolean>()->value - 1;
		value->move(*result);
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Symbol::DecOperator, 0))) {
			error("class '%s' dosen't ovreload operator '--'(0)", type_name(value).c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '--'", type_name(value).c_str());
		break;
	}
}

void mint::not_operator(Cursor *cursor) {

	SharedReference &value = cursor->stack().back();
	SharedReference result = SharedReference::strong<Boolean>();

	switch (value->data()->format) {
	case Data::fmt_none:
	case Data::fmt_null:
		result->data<Boolean>()->value = true;
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(result));
		break;
	case Data::fmt_number:
		result->data<Boolean>()->value = !value->data<Number>()->value;
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(result));
		break;
	case Data::fmt_boolean:
		result->data<Boolean>()->value = !value->data<Boolean>()->value;
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(result));
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Symbol::NotOperator, 0))) {
			error("class '%s' dosen't ovreload operator '!'(0)", type_name(value).c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '!'", type_name(value).c_str());
		break;
	}
}

void mint::compl_operator(Cursor *cursor) {

	SharedReference &value = cursor->stack().back();
	SharedReference result = nullptr;

	switch (value->data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(move(value));
		break;
	case Data::fmt_number:
		result = SharedReference::strong<Number>();
		result->data<Number>()->value = static_cast<double>(~(to_integer(cursor, value)));
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(result));
		break;
	case Data::fmt_boolean:
		result = SharedReference::strong<Boolean>();
		result->data<Boolean>()->value = !value->data<Boolean>()->value;
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(result));
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Symbol::ComplOperator, 0))) {
			error("class '%s' dosen't ovreload operator '~'(0)", type_name(value).c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '~'", type_name(value).c_str());
		break;
	}
}

void mint::pos_operator(Cursor *cursor) {

	SharedReference &value = cursor->stack().back();
	SharedReference result = nullptr;

	switch (value->data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(move(value));
		break;
	case Data::fmt_number:
		result = SharedReference::strong<Number>();
		result->data<Number>()->value = +(value->data<Number>()->value);
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(result));
		break;
	case Data::fmt_boolean:
		result = SharedReference::strong<Boolean>();
		result->data<Boolean>()->value = +(value->data<Boolean>()->value);
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(result));
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Symbol::AddOperator, 0))) {
			error("class '%s' dosen't ovreload operator '+'(0)", type_name(value).c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '+'", type_name(value).c_str());
		break;
	}
}

void mint::neg_operator(Cursor *cursor) {

	SharedReference &value = cursor->stack().back();
	SharedReference result = nullptr;

	switch (value->data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(move(value));
		break;
	case Data::fmt_number:
		result = SharedReference::strong<Number>();
		result->data<Number>()->value = -(value->data<Number>()->value);
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(result));
		break;
	case Data::fmt_boolean:
		result = SharedReference::strong<Boolean>();
		result->data<Boolean>()->value = -(value->data<Boolean>()->value);
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(result));
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Symbol::SubOperator, 0))) {
			error("class '%s' dosen't ovreload operator '-'(0)", type_name(value).c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '-'", type_name(value).c_str());
		break;
	}
}

void mint::shift_left_operator(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	SharedReference &rvalue = load_from_stack(cursor, base);
	SharedReference &lvalue = load_from_stack(cursor, base - 1);
	SharedReference result = nullptr;

	switch (lvalue->data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(move(lvalue));
		break;
	case Data::fmt_number:
		result = SharedReference::strong<Number>();
		result->data<Number>()->value = static_cast<double>(to_integer(lvalue->data<Number>()->value) << to_integer(cursor, rvalue));
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(result));
		break;
	case Data::fmt_boolean:
		result = SharedReference::strong<Number>();
		result->data<Number>()->value = lvalue->data<Boolean>()->value << to_integer(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(result));
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Symbol::ShiftLeftOperator, 1))) {
			error("class '%s' dosen't ovreload operator '<<'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '<<'", type_name(lvalue).c_str());
		break;
	}
}

void mint::shift_right_operator(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	SharedReference &rvalue = load_from_stack(cursor, base);
	SharedReference &lvalue = load_from_stack(cursor, base - 1);
	SharedReference result = nullptr;

	switch (lvalue->data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(move(lvalue));
		break;
	case Data::fmt_number:
		result = SharedReference::strong<Number>();
		result->data<Number>()->value = static_cast<double>(to_integer(lvalue->data<Number>()->value) >> to_integer(cursor, rvalue));
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(result));
		break;
	case Data::fmt_boolean:
		result = SharedReference::strong<Boolean>();
		result->data<Boolean>()->value = lvalue->data<Boolean>()->value >> to_integer(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(result));
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Symbol::ShiftRightOperator, 1))) {
			error("class '%s' dosen't ovreload operator '>>'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '>>'", type_name(lvalue).c_str());
		break;
	}
}

void mint::inclusive_range_operator(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	SharedReference &rvalue = load_from_stack(cursor, base);
	SharedReference &lvalue = load_from_stack(cursor, base - 1);
	SharedReference result = nullptr;

	switch (lvalue->data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(move(lvalue));
		break;
	case Data::fmt_number:
		result = Iterator::fromInclusiveRange(lvalue->data<Number>()->value, to_number(cursor, rvalue));
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(result));
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Symbol::InclusiveRangeOperator, 1))) {
			error("class '%s' dosen't ovreload operator '..'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
		break;
	case Data::fmt_boolean:
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '..'", type_name(lvalue).c_str());
		break;
	}
}

void mint::exclusive_range_operator(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	SharedReference &rvalue = load_from_stack(cursor, base);
	SharedReference &lvalue = load_from_stack(cursor, base - 1);
	SharedReference result = nullptr;

	switch (lvalue->data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(move(lvalue));
		break;
	case Data::fmt_number:
		result = Iterator::fromExclusiveRange(lvalue->data<Number>()->value, to_number(cursor, rvalue));
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(result));
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Symbol::ExclusiveRangeOperator, 1))) {
			error("class '%s' dosen't ovreload operator '...'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
		break;
	case Data::fmt_boolean:
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '...'", type_name(lvalue).c_str());
		break;
	}
}

void mint::typeof_operator(Cursor *cursor) {

	SharedReference value = move(cursor->stack().back());
	cursor->stack().back() = create_string(type_name(value));
}

void mint::membersof_operator(Cursor *cursor) {

	SharedReference &value = cursor->stack().back();
	SharedReference result = SharedReference::strong<Array>();

	switch (value->data()->format) {
	case Data::fmt_object:
		if (Object *object = value->data<Object>()) {

			Array *array = result->data<Array>();

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
		if (Package *package = value->data<Package>()) {

			Array *array = result->data<Array>();

			array->construct();
			array->values.reserve(package->data->symbols().size());

			for (auto symbol : package->data->symbols()) {
				array_append(array, create_string(symbol.first.str()));
			}
		}
		break;

	default:
		break;
	}


	cursor->stack().pop_back();
	cursor->stack().emplace_back(move(result));
}

void mint::subscript_operator(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	SharedReference &rvalue = load_from_stack(cursor, base);
	SharedReference &lvalue = load_from_stack(cursor, base - 1);
	SharedReference result = nullptr;

	switch (lvalue->data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(move(lvalue));
		break;
	case Data::fmt_number:
		result = SharedReference::strong<Number>();
		result->data<Number>()->value = static_cast<double>(to_integer(lvalue->data<Number>()->value / pow(10, to_number(cursor, rvalue))) % 10);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(result));
		break;
	case Data::fmt_boolean:
		error("invalid use of '%s' type with operator '[]'", type_name(lvalue).c_str());
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Symbol::SubscriptOperator, 1))) {
			error("class '%s' dosen't ovreload operator '[]'(1)", lvalue->data<Object>()->metadata->name().c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
		break;
	case Data::fmt_function:
		auto signature = lvalue->data<Function>()->mapping.find(static_cast<int>(to_number(cursor, rvalue)));
		if (signature != lvalue->data<Function>()->mapping.end()) {
			result = SharedReference::strong<Function>();
			result->data<Function>()->mapping.insert(*signature);
		}
		else {
			result = SharedReference::strong<None>();
		}
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(result));
		break;
	}
}

void mint::subscript_move_operator(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	SharedReference &rvalue = load_from_stack(cursor, base);
	SharedReference &kvalue = load_from_stack(cursor, base - 1);
	SharedReference &lvalue = load_from_stack(cursor, base - 2);

	if ((lvalue->flags() & Reference::const_value)) {
		error("invalid modification of constant value");
	}

	switch (lvalue->data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(move(lvalue));
		break;
	case Data::fmt_number:
		lvalue->data<Number>()->value -= (static_cast<double>(to_integer(lvalue->data<Number>()->value / pow(10, to_number(cursor, kvalue))) % 10) * pow(10, to_number(cursor, kvalue)));
		lvalue->data<Number>()->value += to_number(cursor, rvalue) * pow(10, to_number(cursor, kvalue));
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		break;
	case Data::fmt_boolean:
		error("invalid use of '%s' type with operator '[]='", type_name(lvalue).c_str());
		break;
	case Data::fmt_object:
		if (UNLIKELY(!call_overload(cursor, Symbol::SubscriptMoveOperator, 2))) {
			error("class '%s' dosen't ovreload operator '[]='(2)", lvalue->data<Object>()->metadata->name().c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '[]='", type_name(lvalue).c_str());
		break;
	}
}

void mint::regex_match(Cursor *cursor) {

	size_t base = get_stack_base(cursor);
	SharedReference &lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue->data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
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
		break;
	case Data::fmt_number:
	case Data::fmt_boolean:
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '=~'", type_name(lvalue).c_str());
		break;
	}
}

void mint::regex_unmatch(Cursor *cursor) {

	size_t base = get_stack_base(cursor);
	SharedReference &lvalue = load_from_stack(cursor, base - 1);

	switch (lvalue->data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
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
		break;
	case Data::fmt_number:
	case Data::fmt_boolean:
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '!~'", type_name(lvalue).c_str());
		break;
	}
}

void mint::find_defined_symbol(Cursor *cursor, const Symbol &symbol) {

	if (Class *desc = GlobalData::instance().getClass(symbol)) {
		Object *object = desc->makeInstance();
		object->construct();
		cursor->stack().emplace_back(SharedReference::strong(Reference::standard, object));
	}
	else {

		auto it = GlobalData::instance().symbols().find(symbol);
		if (it != GlobalData::instance().symbols().end()) {
			cursor->stack().emplace_back(SharedReference::weak(it->second));
		}
		else {

			it = cursor->symbols().find(symbol);
			if (it != cursor->symbols().end()) {
				cursor->stack().emplace_back(SharedReference::weak(it->second));
			}
			else {
				cursor->stack().emplace_back(SharedReference::strong<None>());
			}
		}
	}

}

void mint::find_defined_member(Cursor *cursor, const Symbol &symbol) {

	if (cursor->stack().back()->data()->format != Data::fmt_none) {

		SharedReference value = move(cursor->stack().back());
		cursor->stack().pop_back();

		switch (value->data()->format) {
		case Data::fmt_object:
			if (Object *object = value->data<Object>()) {

				if (Class::TypeInfo *type = object->metadata->globals().getClass(symbol)) {
					Object *object = type->description->makeInstance();
					object->construct();
					cursor->stack().emplace_back(SharedReference::strong(Reference::standard, object));
				}
				else {

					auto it_global = object->metadata->globals().members().find(symbol);
					if (it_global != object->metadata->globals().members().end()) {
						cursor->stack().emplace_back(SharedReference::weak(it_global->second->value));
					}
					else {

						auto it_member = object->metadata->members().find(symbol);
						if (it_member != object->metadata->members().end()) {
							cursor->stack().emplace_back(SharedReference::weak(object->data[it_member->second->offset]));
						}
						else {
							cursor->stack().emplace_back(SharedReference::strong<None>());
						}
					}
				}
			}
			break;
		case Data::fmt_package:
			if (Package *package = value->data<Package>()) {
				if (Class *desc = package->data->getClass(symbol)) {
					Object *object = desc->makeInstance();
					object->construct();
					cursor->stack().emplace_back(SharedReference::strong(Reference::standard, object));
				}
				else {

					auto it_package = package->data->symbols().find(symbol);
					if (it_package != package->data->symbols().end()) {
						cursor->stack().emplace_back(SharedReference::weak(it_package->second));
					}
					else {
						cursor->stack().emplace_back(SharedReference::strong<None>());
					}
				}
			}
			break;
		default:
			cursor->stack().emplace_back(SharedReference::strong<None>());
			break;
		}
	}
}

void mint::check_defined(Cursor *cursor) {

	SharedReference value = move(cursor->stack().back());
	SharedReference result = SharedReference::strong<Boolean>();

	result->data<Boolean>()->value = (value->data()->format != Data::fmt_none);

	cursor->stack().pop_back();
	cursor->stack().emplace_back(move(result));
}

void mint::find_operator(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	SharedReference &range = load_from_stack(cursor, base);
	SharedReference &value = load_from_stack(cursor, base - 1);

	switch (range->data()->format) {
	case Data::fmt_object:
		cursor->stack().emplace_back(SharedReference::weak(*value));
		if (!call_overload(cursor, Symbol::InOperator, 1)) {
			cursor->stack().pop_back();
			SharedReference result = SharedReference::strong(iterator_init(range));
			cursor->stack().back() = move(result);
		}
		break;

	default:
		SharedReference result = SharedReference::strong(iterator_init(range));
		cursor->stack().back() = move(result);
		break;
	}
}

void mint::find_init(Cursor *cursor) {

	SharedReference &range = cursor->stack().back();

	if (range->data()->format != Data::fmt_boolean) {
		SharedReference result = SharedReference::strong(iterator_init(range));
		cursor->stack().back() = move(result);
	}
}

void mint::find_next(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	SharedReference &range = load_from_stack(cursor, base);
	SharedReference &value = load_from_stack(cursor, base - 1);

	if (range->data()->format == Data::fmt_boolean) {
		cursor->stack().emplace_back(SharedReference::strong(range->data()));
	}
	else {
		Iterator *iterator = range->data<Iterator>();
		assert(iterator != nullptr);
		if (SharedReference item = iterator_next(iterator)) {
			cursor->stack().emplace_back(SharedReference::weak(*value));
			cursor->stack().emplace_back(SharedReference::weak(*item));
			eq_operator(cursor);
		}
		else {
			cursor->stack().emplace_back(create_boolean(false));
		}
	}
}

void mint::find_check(Cursor *cursor, size_t pos) {

	size_t base = get_stack_base(cursor);

	SharedReference found = move_from_stack(cursor, base);
	SharedReference &range = load_from_stack(cursor, base - 1);

	if (range->data()->format == Data::fmt_boolean) {
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(found));
		cursor->jmp(pos);
	}
	else if (to_boolean(cursor, found)) {
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(found));
		cursor->jmp(pos);
	}
	else if (range->data<Iterator>()->ctx.empty()) {
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(move(found));
		cursor->jmp(pos);
	}
	else {
		cursor->stack().pop_back();
	}
}

void mint::in_operator(Cursor *cursor) {

	SharedReference &range = cursor->stack().back();

	if (range->data()->format == Data::fmt_object) {
		call_overload(cursor, Symbol::InOperator, 0);
	}
}

void mint::range_init(Cursor *cursor) {
	cursor->stack().back() = SharedReference::strong(iterator_init(move(cursor->stack().back())));
}

void mint::range_next(Cursor *cursor) {

	SharedReference &range = cursor->stack().back();

	Iterator *iterator = range->data<Iterator>();
	assert(iterator != nullptr);
	iterator->ctx.pop_front();
}

void mint::range_check(Cursor *cursor, size_t pos) {

	size_t base = get_stack_base(cursor);

	SharedReference &range = load_from_stack(cursor, base);
	SharedReference &target = load_from_stack(cursor, base - 1);

	Iterator *iterator = range->data<Iterator>();
	assert(iterator != nullptr);

	if (SharedReference item = iterator_get(iterator)) {
		cursor->stack().emplace_back(SharedReference::weak(*target));
		cursor->stack().emplace_back(move(item));
		move_operator(cursor);
	}
	else {
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->jmp(pos);
	}
}

void mint::range_iterator_check(Cursor *cursor, size_t pos) {

	size_t base = get_stack_base(cursor);

	SharedReference &range = load_from_stack(cursor, base);
	SharedReference &target = load_from_stack(cursor, base - 1);

	Iterator *iterator = range->data<Iterator>();
	assert(iterator != nullptr);

	if (SharedReference item = iterator_get(iterator)) {
		cursor->stack().emplace_back(SharedReference::weak(*target));
		cursor->stack().emplace_back(move(item));
		copy_operator(cursor);
	}
	else {
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->jmp(pos);
	}
}

bool Hash::compare::operator ()(const Hash::key_type &lvalue, const Hash::key_type &rvalue) const {

	if (lvalue->data()->format != rvalue->data()->format) {
		return lvalue->data()->format < rvalue->data()->format;
	}

	switch (lvalue->data()->format) {
	case Data::fmt_none:
	case Data::fmt_null:
		return false;

	case Data::fmt_number:
		return lvalue->data<Number>()->value < rvalue->data<Number>()->value;

	case Data::fmt_boolean:
		return lvalue->data<Boolean>()->value < rvalue->data<Boolean>()->value;

	case Data::fmt_object:
		if (lvalue->data<Object>()->metadata->metatype() != rvalue->data<Object>()->metadata->metatype()) {
			return lvalue->data<Object>()->metadata->metatype() < rvalue->data<Object>()->metadata->metatype();
		}

		switch (lvalue->data<Object>()->metadata->metatype()) {
		case Class::object:
			if (lvalue->data<Object>()->metadata != rvalue->data<Object>()->metadata) {
				return lvalue->data<Object>()->metadata < rvalue->data<Object>()->metadata;
			}
			return lvalue->data<Object>()->data < rvalue->data<Object>()->data;

		case Class::string:
			return lvalue->data<String>()->str < rvalue->data<String>()->str;

		case Class::regex:
			return lvalue->data<Regex>()->initializer < rvalue->data<Regex>()->initializer;

		case Class::array:
			for (auto i = lvalue->data<Array>()->values.begin(), j = rvalue->data<Array>()->values.begin();
				 i != lvalue->data<Array>()->values.end() && j != rvalue->data<Array>()->values.end(); ++i, ++j) {
				if (operator ()(array_get_item(i), array_get_item(j))) {
					return true;
				}
				if (operator ()(array_get_item(j), array_get_item(i))) {
					return false;
				}
			}
			return lvalue->data<Array>()->values.size() < rvalue->data<Array>()->values.size();

		case Class::hash:
		case Class::iterator:
		case Class::library:
		case Class::libobject:
			error("invalid use of '%s' type as hash key", type_name(lvalue).c_str());
			break;
		}
		break;
	case Data::fmt_package:
	case Data::fmt_function:
		error("invalid use of '%s' type as hash key", type_name(lvalue).c_str());
		break;
	}

	return false;
}
