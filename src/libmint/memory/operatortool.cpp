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

bool mint::call_overload(Cursor *cursor, const string &operator_overload, int signature) {

	size_t base = get_stack_base(cursor);
	Object *object = cursor->stack().at(base - signature)->data<Object>();
	auto it = object->metadata->members().find(operator_overload);

	if (it == object->metadata->members().end()) {
		return false;
	}

	assert(is_object(object));

	cursor->waitingCalls().push(SharedReference::linked(object->referenceManager(), object->data + it->second->offset));
	cursor->waitingCalls().top().setMetadata(it->second->owner);
	call_member_operator(cursor, signature);
	return true;
}

void mint::move_operator(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	SharedReference &rvalue = cursor->stack().at(base);
	SharedReference &lvalue = cursor->stack().at(base - 1);

	if ((lvalue->flags() & Reference::const_address) && (lvalue->data()->format != Data::fmt_none)) {
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

	SharedReference &rvalue = cursor->stack().at(base);
	SharedReference &lvalue = cursor->stack().at(base - 1);

	if (lvalue->flags() & Reference::const_value) {
		error("invalid modification of constant value");
	}

	switch (lvalue->data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(lvalue);
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
		if (rvalue->data()->format != Data::fmt_function) {
			error("invalid conversion from '%s' to '%s'", type_name(rvalue).c_str(), type_name(lvalue).c_str());
		}
		lvalue->data<Function>()->mapping = rvalue->data<Function>()->mapping;
		cursor->stack().pop_back();
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, ":=", 1)) {
			if (rvalue->data()->format != Data::fmt_object) {
				error("cannot convert '%s' to '%s' in assignment", type_name(rvalue).c_str(), type_name(lvalue).c_str());
			}
			if (lvalue->data<Object>()->metadata != rvalue->data<Object>()->metadata) {
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
	SharedReference lvalue = call_infos.function();
	Class *metadata = call_infos.getMetadata();
	bool member = call_infos.isMember();
	signature += call_infos.extraArgumentCount();

	switch (lvalue->data()->format) {
	case Data::fmt_none:
		if (member) {
			if (signature) {
				error("default constructors doesn't take %d argument(s)", signature);
			}
		}
		else {
			error("invalid use of none value as a function");
		}
		break;
	case Data::fmt_null:
		cursor->raise(lvalue);
		break;
	case Data::fmt_number:
		result = SharedReference::unique(Reference::create<Number>());
		result->copy(*lvalue);
		cursor->stack().emplace_back(result);
		break;
	case Data::fmt_boolean:
		result = SharedReference::unique(Reference::create<Boolean>());
		result->copy(*lvalue);
		cursor->stack().emplace_back(result);
		break;
	case Data::fmt_object:
		result = SharedReference::unique(Reference::create<None>());
		result->copy(*lvalue);
		cursor->stack().emplace_back(result);
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
		break;
	case Data::fmt_function:
		auto it = find_function_signature(cursor, lvalue->data<Function>()->mapping, signature + (member ? 1 : 0));
		if (it == lvalue->data<Function>()->mapping.end()) {
			error("called function doesn't take %d parameter(s)", signature + (member ? 1 : 0));
		}
		const Function::Handler &hanlder = it->second;
		if (cursor->call(hanlder.module, hanlder.offset, hanlder.package, metadata)) {
			if (hanlder.generator) {
				cursor->symbols().defaultResult() = Reference(Reference::standard, Reference::alloc<Iterator>(cursor, cursor->stack().size() - (signature + (member ? 1 : 0))));
				cursor->symbols().defaultResult().data<Iterator>()->construct();
				cursor->setExecutionMode(Cursor::interruptible);
			}
			if (hanlder.capture) {
				for (auto item : *hanlder.capture) {
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
	SharedReference lvalue = call_infos.function();
	Class *metadata = call_infos.getMetadata();
	bool member = call_infos.isMember();
	bool global = lvalue->flags() & Reference::global;
	signature += call_infos.extraArgumentCount();

	switch (lvalue->data()->format) {
	case Data::fmt_none:
		if (member) {
			if (signature) {
				error("default constructors doesn't take %d argument(s)", signature);
			}
		}
		else {
			error("invalid use of none value as a function");
		}
		break;
	case Data::fmt_null:
		cursor->raise(lvalue);
		break;
	case Data::fmt_number:
		result = SharedReference::unique(Reference::create<Number>());
		result->copy(*lvalue);
		cursor->stack().pop_back();
		cursor->stack().emplace_back(result);
		break;
	case Data::fmt_boolean:
		result = SharedReference::unique(Reference::create<Boolean>());
		result->copy(*lvalue);
		cursor->stack().pop_back();
		cursor->stack().emplace_back(result);
		break;
	case Data::fmt_object:
		result = SharedReference::unique(Reference::create<None>());
		result->copy(*lvalue);
		cursor->stack().pop_back();
		cursor->stack().emplace_back(result);
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
		break;
	case Data::fmt_function:
		auto it = find_function_signature(cursor, lvalue->data<Function>()->mapping, signature + (global ? 0 : 1));
		if (it == lvalue->data<Function>()->mapping.end()) {
			error("called member doesn't take %d parameter(s)", signature + (global ? 0 : 1));
		}
		const Function::Handler &hanlder = it->second;
		if (cursor->call(hanlder.module, hanlder.offset, hanlder.package, metadata)) {
			if (hanlder.generator) {
				cursor->symbols().defaultResult() = Reference(Reference::standard, Reference::alloc<Iterator>(cursor, cursor->stack().size() - (signature + (global ? 0 : 1))));
				cursor->symbols().defaultResult().data<Iterator>()->construct();
				cursor->setExecutionMode(Cursor::interruptible);
			}
			if (hanlder.capture) {
				for (auto item : *hanlder.capture) {
					cursor->symbols().insert(item);
				}
			}
		}
		break;
	}
}

void mint::add_operator(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	SharedReference &rvalue = cursor->stack().at(base);
	SharedReference &lvalue = cursor->stack().at(base - 1);
	SharedReference result = nullptr;

	switch (lvalue->data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(lvalue);
		break;
	case Data::fmt_number:
		result = SharedReference::unique(Reference::create<Number>());
		result->data<Number>()->value = lvalue->data<Number>()->value + to_number(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(result);
		break;
	case Data::fmt_boolean:
		result = SharedReference::unique(Reference::create<Boolean>());
		result->data<Boolean>()->value = lvalue->data<Boolean>()->value + to_boolean(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(result);
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, "+", 1)) {
			error("class '%s' dosen't ovreload operator '+'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
		break;
	case Data::fmt_function:
		result = SharedReference::unique(Reference::create<Function>());
		if (rvalue->data()->format != Data::fmt_function) {
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
		cursor->stack().emplace_back(result);
		break;
	}
}

void mint::sub_operator(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	SharedReference &rvalue = cursor->stack().at(base);
	SharedReference &lvalue = cursor->stack().at(base - 1);
	SharedReference result = nullptr;

	switch (lvalue->data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(lvalue);
		break;
	case Data::fmt_number:
		result = SharedReference::unique(Reference::create<Number>());
		result->data<Number>()->value = lvalue->data<Number>()->value - to_number(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(result);
		break;
	case Data::fmt_boolean:
		result = SharedReference::unique(Reference::create<Boolean>());
		result->data<Boolean>()->value = lvalue->data<Boolean>()->value - to_boolean(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(result);
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, "-", 1)) {
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

	SharedReference &rvalue = cursor->stack().at(base);
	SharedReference &lvalue = cursor->stack().at(base - 1);
	SharedReference result = nullptr;

	switch (lvalue->data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(lvalue);
		break;
	case Data::fmt_number:
		result = SharedReference::unique(Reference::create<Number>());
		result->data<Number>()->value = lvalue->data<Number>()->value * to_number(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(result);
		break;
	case Data::fmt_boolean:
		result = SharedReference::unique(Reference::create<Boolean>());
		result->data<Boolean>()->value = lvalue->data<Boolean>()->value && to_boolean(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(result);
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, "*", 1)) {
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

	SharedReference &rvalue = cursor->stack().at(base);
	SharedReference &lvalue = cursor->stack().at(base - 1);
	SharedReference result = nullptr;

	switch (lvalue->data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(lvalue);
		break;
	case Data::fmt_number:
		result = SharedReference::unique(Reference::create<Number>());
		result->data<Number>()->value = lvalue->data<Number>()->value / to_number(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(result);
		break;
	case Data::fmt_boolean:
		result = SharedReference::unique(Reference::create<Boolean>());
		result->data<Boolean>()->value = lvalue->data<Boolean>()->value / to_boolean(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(result);
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, "/", 1)) {
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

	SharedReference &rvalue = cursor->stack().at(base);
	SharedReference &lvalue = cursor->stack().at(base - 1);
	SharedReference result = nullptr;

	switch (lvalue->data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(lvalue);
		break;
	case Data::fmt_number:
		result = SharedReference::unique(Reference::create<Number>());
		result->data<Number>()->value = pow(lvalue->data<Number>()->value, to_number(cursor, rvalue));
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(result);
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, "**", 1)) {
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

	SharedReference &rvalue = cursor->stack().at(base);
	SharedReference &lvalue = cursor->stack().at(base - 1);
	SharedReference result = nullptr;

	switch (lvalue->data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(lvalue);
		break;
	case Data::fmt_number:
		if (long divider = to_number(cursor, rvalue)) {
			result = SharedReference::unique(Reference::create<Number>());
			result->data<Number>()->value = (long)lvalue->data<Number>()->value % divider;
			cursor->stack().pop_back();
			cursor->stack().pop_back();
			cursor->stack().emplace_back(result);
		}
		else {
			error("modulo by zero");
		}
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, "%", 1)) {
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

	SharedReference &rvalue = cursor->stack().at(base);
	SharedReference &lvalue = cursor->stack().at(base - 1);

	SharedReference result = SharedReference::unique(Reference::create<Boolean>());
	result->data<Boolean>()->value = lvalue->data() == rvalue->data();
	cursor->stack().pop_back();
	cursor->stack().pop_back();
	cursor->stack().emplace_back(result);
}

void mint::eq_operator(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	SharedReference &rvalue = cursor->stack().at(base);
	SharedReference &lvalue = cursor->stack().at(base - 1);
	SharedReference result = nullptr;

	switch (lvalue->data()->format) {
	case Data::fmt_none:
		result = SharedReference::unique(Reference::create<Boolean>());
		result->data<Boolean>()->value = (rvalue->data()->format == Data::fmt_none);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(result);
		break;
	case Data::fmt_null:
		result = SharedReference::unique(Reference::create<Boolean>());
		result->data<Boolean>()->value = (rvalue->data()->format == Data::fmt_null);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(result);
		break;
	case Data::fmt_number:
		result = SharedReference::unique(Reference::create<Boolean>());
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
		cursor->stack().emplace_back(result);
		break;
	case Data::fmt_boolean:
		result = SharedReference::unique(Reference::create<Boolean>());
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
		cursor->stack().emplace_back(result);
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, "==", 1)) {
			result = SharedReference::unique(Reference::create<Boolean>());
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
			cursor->stack().emplace_back(result);
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

	SharedReference &rvalue = cursor->stack().at(base);
	SharedReference &lvalue = cursor->stack().at(base - 1);
	SharedReference result = nullptr;

	switch (lvalue->data()->format) {
	case Data::fmt_none:
		result = SharedReference::unique(Reference::create<Boolean>());
		result->data<Boolean>()->value = (rvalue->data()->format != Data::fmt_none);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(result);
		break;
	case Data::fmt_null:
		result = SharedReference::unique(Reference::create<Boolean>());
		result->data<Boolean>()->value = (rvalue->data()->format != Data::fmt_null);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(result);
		break;
	case Data::fmt_number:
		result = SharedReference::unique(Reference::create<Boolean>());
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
		cursor->stack().emplace_back(result);
		break;
	case Data::fmt_boolean:
		result = SharedReference::unique(Reference::create<Boolean>());
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
		cursor->stack().emplace_back(result);
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, "!=", 1)) {
			result = SharedReference::unique(Reference::create<Boolean>());
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
			cursor->stack().emplace_back(result);
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

	SharedReference &rvalue = cursor->stack().at(base);
	SharedReference &lvalue = cursor->stack().at(base - 1);
	SharedReference result = nullptr;

	switch (lvalue->data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(lvalue);
		break;
	case Data::fmt_number:
		result = SharedReference::unique(Reference::create<Boolean>());
		result->data<Boolean>()->value = lvalue->data<Number>()->value < to_number(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(result);
		break;
	case Data::fmt_boolean:
		result = SharedReference::unique(Reference::create<Boolean>());
		result->data<Boolean>()->value = lvalue->data<Boolean>()->value < to_boolean(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(result);
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, "<", 1)) {
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

	SharedReference &rvalue = cursor->stack().at(base);
	SharedReference &lvalue = cursor->stack().at(base - 1);
	SharedReference result = nullptr;

	switch (lvalue->data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(lvalue);
		break;
	case Data::fmt_number:
		result = SharedReference::unique(Reference::create<Boolean>());
		result->data<Boolean>()->value = lvalue->data<Number>()->value > to_number(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(result);
		break;
	case Data::fmt_boolean:
		result = SharedReference::unique(Reference::create<Boolean>());
		result->data<Boolean>()->value = lvalue->data<Boolean>()->value > to_boolean(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(result);
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, ">", 1)) {
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

	SharedReference &rvalue = cursor->stack().at(base);
	SharedReference &lvalue = cursor->stack().at(base - 1);
	SharedReference result = nullptr;

	switch (lvalue->data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(lvalue);
		break;
	case Data::fmt_number:
		result = SharedReference::unique(Reference::create<Boolean>());
		result->data<Boolean>()->value = lvalue->data<Number>()->value <= to_number(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(result);
		break;
	case Data::fmt_boolean:
		result = SharedReference::unique(Reference::create<Boolean>());
		result->data<Boolean>()->value = lvalue->data<Boolean>()->value <= to_boolean(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(result);
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, "<=", 1)) {
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

	SharedReference &rvalue = cursor->stack().at(base);
	SharedReference &lvalue = cursor->stack().at(base - 1);
	SharedReference result = nullptr;

	switch (lvalue->data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(lvalue);
		break;
	case Data::fmt_number:
		result = SharedReference::unique(Reference::create<Boolean>());
		result->data<Boolean>()->value = lvalue->data<Number>()->value >= to_number(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(result);
		break;
	case Data::fmt_boolean:
		result = SharedReference::unique(Reference::create<Boolean>());
		result->data<Boolean>()->value = lvalue->data<Boolean>()->value >= to_boolean(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(result);
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, ">=", 1)) {
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

	SharedReference &rvalue = cursor->stack().at(base);
	SharedReference &lvalue = cursor->stack().at(base - 1);
	SharedReference result = nullptr;

	switch (lvalue->data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(lvalue);
		break;
	case Data::fmt_number:
		result = SharedReference::unique(Reference::create<Boolean>());
		result->data<Boolean>()->value = lvalue->data<Number>()->value && to_number(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(result);
		break;
	case Data::fmt_boolean:
		result = SharedReference::unique(Reference::create<Boolean>());
		result->data<Boolean>()->value = lvalue->data<Boolean>()->value && to_boolean(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(result);
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, "&&", 1)) {
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

	SharedReference &rvalue = cursor->stack().at(base);
	SharedReference &lvalue = cursor->stack().at(base - 1);
	SharedReference result = nullptr;

	switch (lvalue->data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(lvalue);
		break;
	case Data::fmt_number:
		result = SharedReference::unique(Reference::create<Boolean>());
		result->data<Boolean>()->value = lvalue->data<Number>()->value || to_number(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(result);
		break;
	case Data::fmt_boolean:
		result = SharedReference::unique(Reference::create<Boolean>());
		result->data<Boolean>()->value = lvalue->data<Boolean>()->value || to_boolean(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(result);
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, "||", 1)) {
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

	SharedReference &rvalue = cursor->stack().at(base);
	SharedReference &lvalue = cursor->stack().at(base - 1);
	SharedReference result = nullptr;

	switch (lvalue->data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(lvalue);
		break;
	case Data::fmt_number:
		result = SharedReference::unique(Reference::create<Number>());
		result->data<Number>()->value = (long)lvalue->data<Number>()->value & (long)to_number(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(result);
		break;
	case Data::fmt_boolean:
		result = SharedReference::unique(Reference::create<Boolean>());
		result->data<Boolean>()->value = lvalue->data<Boolean>()->value & to_boolean(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(result);
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, "&", 1)) {
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

	SharedReference &rvalue = cursor->stack().at(base);
	SharedReference &lvalue = cursor->stack().at(base - 1);
	SharedReference result = nullptr;

	switch (lvalue->data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(lvalue);
		break;
	case Data::fmt_number:
		result = SharedReference::unique(Reference::create<Number>());
		result->data<Number>()->value = (long)lvalue->data<Number>()->value | (long)to_number(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(result);
		break;
	case Data::fmt_boolean:
		result = SharedReference::unique(Reference::create<Boolean>());
		result->data<Boolean>()->value = lvalue->data<Boolean>()->value | to_boolean(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(result);
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, "|", 1)) {
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

	SharedReference &rvalue = cursor->stack().at(base);
	SharedReference &lvalue = cursor->stack().at(base - 1);
	SharedReference result = nullptr;

	switch (lvalue->data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(lvalue);
		break;
	case Data::fmt_number:
		result = SharedReference::unique(Reference::create<Number>());
		result->data<Number>()->value = (long)lvalue->data<Number>()->value ^ (long)to_number(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(result);
		break;
	case Data::fmt_boolean:
		result = SharedReference::unique(Reference::create<Boolean>());
		result->data<Boolean>()->value = (long)lvalue->data<Boolean>()->value ^ to_boolean(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(result);
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, "^", 1)) {
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
		cursor->raise(value);
		break;
	case Data::fmt_number:
		result = SharedReference::unique(Reference::create<Number>());
		result->data<Number>()->value = value->data<Number>()->value + 1;
		value->move(*result);
		break;
	case Data::fmt_boolean:
		result = SharedReference::unique(Reference::create<Boolean>());
		result->data<Boolean>()->value = value->data<Boolean>()->value + 1;
		value->move(*result);
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, "++", 0)) {
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
		cursor->raise(value);
		break;
	case Data::fmt_number:
		result = SharedReference::unique(Reference::create<Number>());
		result->data<Number>()->value = value->data<Number>()->value - 1;
		value->move(*result);
		break;
	case Data::fmt_boolean:
		result = SharedReference::unique(Reference::create<Boolean>());
		result->data<Boolean>()->value = value->data<Boolean>()->value - 1;
		value->move(*result);
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, "--", 0)) {
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
	SharedReference result = SharedReference::unique(Reference::create<Boolean>());

	switch (value->data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(value);
		break;
	case Data::fmt_number:
		result->data<Boolean>()->value = !value->data<Number>()->value;
		cursor->stack().pop_back();
		cursor->stack().emplace_back(result);
		break;
	case Data::fmt_boolean:
		result->data<Boolean>()->value = !value->data<Boolean>()->value;
		cursor->stack().pop_back();
		cursor->stack().emplace_back(result);
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, "!", 0)) {
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
		cursor->raise(value);
		break;
	case Data::fmt_number:
		result = SharedReference::unique(Reference::create<Number>());
		result->data<Number>()->value = ~((long)value->data<Number>()->value);
		cursor->stack().pop_back();
		cursor->stack().emplace_back(result);
		break;
	case Data::fmt_boolean:
		result = SharedReference::unique(Reference::create<Boolean>());
		result->data<Boolean>()->value = !value->data<Boolean>()->value;
		cursor->stack().pop_back();
		cursor->stack().emplace_back(result);
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, "~", 0)) {
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
		cursor->raise(value);
		break;
	case Data::fmt_number:
		result = SharedReference::unique(Reference::create<Number>());
		result->data<Number>()->value = +(value->data<Number>()->value);
		cursor->stack().pop_back();
		cursor->stack().emplace_back(result);
		break;
	case Data::fmt_boolean:
		result = SharedReference::unique(Reference::create<Boolean>());
		result->data<Boolean>()->value = +(value->data<Boolean>()->value);
		cursor->stack().pop_back();
		cursor->stack().emplace_back(result);
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, "+", 0)) {
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
		cursor->raise(value);
		break;
	case Data::fmt_number:
		result = SharedReference::unique(Reference::create<Number>());
		result->data<Number>()->value = -(value->data<Number>()->value);
		cursor->stack().pop_back();
		cursor->stack().emplace_back(result);
		break;
	case Data::fmt_boolean:
		result = SharedReference::unique(Reference::create<Boolean>());
		result->data<Boolean>()->value = -(value->data<Boolean>()->value);
		cursor->stack().pop_back();
		cursor->stack().emplace_back(result);
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, "-", 0)) {
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

	SharedReference &rvalue = cursor->stack().at(base);
	SharedReference &lvalue = cursor->stack().at(base - 1);
	SharedReference result = nullptr;

	switch (lvalue->data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(lvalue);
		break;
	case Data::fmt_number:
		result = SharedReference::unique(Reference::create<Number>());
		result->data<Number>()->value = (long)lvalue->data<Number>()->value << (long)to_number(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(result);
		break;
	case Data::fmt_boolean:
		result = SharedReference::unique(Reference::create<Number>());
		result->data<Number>()->value = lvalue->data<Boolean>()->value << (long)to_number(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(result);
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, "<<", 1)) {
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

	SharedReference &rvalue = cursor->stack().at(base);
	SharedReference &lvalue = cursor->stack().at(base - 1);
	SharedReference result = nullptr;

	switch (lvalue->data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(lvalue);
		break;
	case Data::fmt_number:
		result = SharedReference::unique(Reference::create<Number>());
		result->data<Number>()->value = (long)lvalue->data<Number>()->value >> (long)to_number(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(result);
		break;
	case Data::fmt_boolean:
		result = SharedReference::unique(Reference::create<Boolean>());
		result->data<Boolean>()->value = lvalue->data<Boolean>()->value >> (long)to_number(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(result);
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, ">>", 1)) {
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

	SharedReference &rvalue = cursor->stack().at(base);
	SharedReference &lvalue = cursor->stack().at(base - 1);
	SharedReference result = nullptr;

	switch (lvalue->data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(lvalue);
		break;
	case Data::fmt_number:
		result = SharedReference::unique(Iterator::fromInclusiveRange(lvalue->data<Number>()->value, to_number(cursor, rvalue)));
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(result);
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, "..", 1)) {
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

	SharedReference &rvalue = cursor->stack().at(base);
	SharedReference &lvalue = cursor->stack().at(base - 1);
	SharedReference result = nullptr;

	switch (lvalue->data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(lvalue);
		break;
	case Data::fmt_number:
		result = SharedReference::unique(Iterator::fromExclusiveRange(lvalue->data<Number>()->value, to_number(cursor, rvalue)));
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(result);
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, "...", 1)) {
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

	SharedReference value = cursor->stack().back();
	cursor->stack().pop_back();
	cursor->stack().emplace_back(create_string(type_name(value)));
}

void mint::membersof_operator(Cursor *cursor) {

	SharedReference &value = cursor->stack().back();
	SharedReference result = SharedReference::unique(Reference::create<Array>());

	switch (value->data()->format) {
	case Data::fmt_object:
		if (Object *object = value->data<Object>()) {

			Array *array = result->data<Array>();

			array->construct();
			array->values.reserve(object->metadata->members().size());

			for (auto member : object->metadata->members()) {

				if ((member.second->value.flags() & Reference::protected_visibility) && (!member.second->owner->isBaseOrSame(cursor->symbols().getMetadata()))) {
					continue;
				}

				if ((member.second->value.flags() & Reference::private_visibility) && (member.second->owner != cursor->symbols().getMetadata())) {
					continue;
				}

				if ((member.second->value.flags() & Reference::package_visibility) && (member.second->owner->getPackage() != cursor->symbols().getPackage())) {
					continue;
				}

				array_append(array, create_string(member.first));
			}
		}
		break;

	case Data::fmt_package:
		if (Package *package = value->data<Package>()) {

			Array *array = result->data<Array>();

			array->construct();
			array->values.reserve(package->data->symbols().size());

			for (auto symbol : package->data->symbols()) {
				array_append(array, create_string(symbol.first));
			}
		}
		break;

	default:
		break;
	}


	cursor->stack().pop_back();
	cursor->stack().emplace_back(result);
}

void mint::subscript_operator(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	SharedReference &rvalue = cursor->stack().at(base);
	SharedReference &lvalue = cursor->stack().at(base - 1);
	SharedReference result = nullptr;

	switch (lvalue->data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(lvalue);
		break;
	case Data::fmt_number:
		result = SharedReference::unique(Reference::create<Number>());
		result->data<Number>()->value = ((long)(lvalue->data<Number>()->value / pow(10, to_number(cursor, rvalue))) % 10);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(result);
		break;
	case Data::fmt_boolean:
		error("invalid use of '%s' type with operator '[]'", type_name(lvalue).c_str());
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, "[]", 1)) {
			error("class '%s' dosen't ovreload operator '[]'(1)", lvalue->data<Object>()->metadata->name().c_str());
		}
		break;
	case Data::fmt_package:
		error("invalid use of package in an operation");
		break;
	case Data::fmt_function:
		auto signature = lvalue->data<Function>()->mapping.find(to_number(cursor, rvalue));
		if (signature != lvalue->data<Function>()->mapping.end()) {
			result = SharedReference::unique(Reference::create<Function>());
			result->data<Function>()->mapping.insert(*signature);
		}
		else {
			result = SharedReference::unique(Reference::create<None>());
		}
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(result);
		break;
	}
}

void mint::subscript_move_operator(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	SharedReference &rvalue = cursor->stack().at(base);
	SharedReference &kvalue = cursor->stack().at(base - 1);
	SharedReference &lvalue = cursor->stack().at(base - 2);

	if ((lvalue->flags() & Reference::const_value)) {
		error("invalid modification of constant value");
	}

	switch (lvalue->data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(lvalue);
		break;
	case Data::fmt_number:
		lvalue->data<Number>()->value -= ((long)(lvalue->data<Number>()->value / pow(10, to_number(cursor, kvalue))) % 10) * pow(10, to_number(cursor, kvalue));
		lvalue->data<Number>()->value += to_number(cursor, rvalue) * pow(10, to_number(cursor, kvalue));
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		break;
	case Data::fmt_boolean:
		error("invalid use of '%s' type with operator '[]='", type_name(lvalue).c_str());
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, "[]=", 2)) {
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
	SharedReference &lvalue = cursor->stack().at(base - 1);

	switch (lvalue->data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(lvalue);
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, "=~", 1)) {
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
	SharedReference &lvalue = cursor->stack().at(base - 1);

	switch (lvalue->data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(lvalue);
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, "!~", 1)) {
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

void mint::find_defined_symbol(Cursor *cursor, const string &symbol) {

	if (Class *desc = GlobalData::instance().getClass(symbol)) {
		Object *object = desc->makeInstance();
		object->construct();
		cursor->stack().emplace_back(SharedReference::unique(new Reference(Reference::standard, object)));
	}
	else {

		auto it = GlobalData::instance().symbols().find(symbol);
		if (it != GlobalData::instance().symbols().end()) {
			cursor->stack().emplace_back(SharedReference::linked(GlobalData::instance().symbols().referenceManager(), &it->second));
		}
		else {

			it = cursor->symbols().find(symbol);
			if (it != cursor->symbols().end()) {
				cursor->stack().emplace_back(SharedReference::linked(cursor->symbols().referenceManager(), &it->second));
			}
			else {
				cursor->stack().emplace_back(SharedReference::unique(Reference::create<None>()));
			}
		}
	}

}

void mint::find_defined_member(Cursor *cursor, const string &symbol) {

	if (cursor->stack().back()->data()->format != Data::fmt_none) {

		SharedReference value = cursor->stack().back();
		cursor->stack().pop_back();

		switch (value->data()->format) {
		case Data::fmt_object:
			if (Object *object = value->data<Object>()) {

				if (Class::TypeInfo *type = object->metadata->globals().getClass(symbol)) {
					Object *object = type->description->makeInstance();
					object->construct();
					cursor->stack().emplace_back(SharedReference::unique(new Reference(Reference::standard, object)));
				}
				else {

					auto it_global = object->metadata->globals().members().find(symbol);
					if (it_global != object->metadata->globals().members().end()) {
						cursor->stack().emplace_back(SharedReference::unsafe(&it_global->second->value));
					}
					else {

						auto it_member = object->metadata->members().find(symbol);
						if (it_member != object->metadata->members().end()) {
							cursor->stack().emplace_back(SharedReference::linked(object->referenceManager(), object->data + it_member->second->offset));
						}
						else {
							cursor->stack().emplace_back(SharedReference::unique(Reference::create<None>()));
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
					cursor->stack().emplace_back(SharedReference::unique(new Reference(Reference::standard, object)));
				}
				else {

					auto it_package = package->data->symbols().find(symbol);
					if (it_package != package->data->symbols().end()) {
						cursor->stack().emplace_back(SharedReference::linked(package->data->symbols().referenceManager(), &it_package->second));
					}
					else {
						cursor->stack().emplace_back(SharedReference::unique(Reference::create<None>()));
					}
				}
			}
			break;
		default:
			cursor->stack().emplace_back(SharedReference::unique(Reference::create<None>()));
			break;
		}
	}
}

void mint::check_defined(Cursor *cursor) {

	SharedReference value = cursor->stack().back();
	SharedReference result = SharedReference::unique(Reference::create<Boolean>());

	result->data<Boolean>()->value = (value->data()->format != Data::fmt_none);

	cursor->stack().pop_back();
	cursor->stack().emplace_back(result);
}

void mint::find_operator(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	SharedReference &range = cursor->stack().at(base);
	SharedReference &value = cursor->stack().at(base - 1);

	switch (range->data()->format) {
	case Data::fmt_object:
		cursor->stack().emplace_back(SharedReference::unsafe(value.get()));
		if (!call_overload(cursor, "in", 1)) {
			cursor->stack().pop_back();
			SharedReference result = SharedReference::unique(Reference::create(iterator_init(range)));
			cursor->stack().back() = result;
		}
		break;

	default:
		SharedReference result = SharedReference::unique(Reference::create(iterator_init(range)));
		cursor->stack().back() = result;
		break;
	}
}

void mint::find_init(Cursor *cursor) {

	SharedReference &range = cursor->stack().back();

	if (range->data()->format != Data::fmt_boolean) {
		SharedReference result = SharedReference::unique(Reference::create(iterator_init(range)));
		cursor->stack().back() = result;
	}
}

void mint::find_next(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	SharedReference &range = cursor->stack().at(base);
	SharedReference &value = cursor->stack().at(base - 1);

	if (range->data()->format == Data::fmt_boolean) {
		cursor->stack().emplace_back(SharedReference::unique(Reference::create(range->data())));
	}
	else {
		Iterator *iterator = range->data<Iterator>();
		assert(iterator != nullptr);
		if (SharedReference item = iterator_next(iterator)) {
			cursor->stack().emplace_back(value);
			cursor->stack().emplace_back(item);
			eq_operator(cursor);
		}
		else {
			cursor->stack().emplace_back(create_boolean(false));
		}
	}
}

void mint::find_check(Cursor *cursor, size_t pos) {

	size_t base = get_stack_base(cursor);

	SharedReference found = cursor->stack().at(base);
	SharedReference &range = cursor->stack().at(base - 1);

	if (range->data()->format == Data::fmt_boolean) {
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(found);
		cursor->jmp(pos);
	}
	else if (range->data<Iterator>()->ctx.empty()) {
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(found);
		cursor->jmp(pos);
	}
	else {
		cursor->stack().pop_back();
	}
}

void mint::in_operator(Cursor *cursor) {

	SharedReference &range = cursor->stack().back();

	if (range->data()->format == Data::fmt_object) {
		call_overload(cursor, "in", 0);
	}
}

void mint::range_init(Cursor *cursor) {

	SharedReference range = cursor->stack().back();
	SharedReference result = SharedReference::unique(Reference::create(iterator_init(range)));
	cursor->stack().back() = result;
}

void mint::range_next(Cursor *cursor) {

	SharedReference &range = cursor->stack().back();

	Iterator *iterator = range->data<Iterator>();
	assert(iterator != nullptr);
	iterator->ctx.pop_front();
}

void mint::range_check(Cursor *cursor, size_t pos) {

	size_t base = get_stack_base(cursor);

	SharedReference &range = cursor->stack().at(base);
	SharedReference &target = cursor->stack().at(base - 1);

	Iterator *iterator = range->data<Iterator>();
	assert(iterator != nullptr);

	if (iterator->ctx.empty()) {
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->jmp(pos);
	}
	else {
		cursor->stack().emplace_back(target);
		cursor->stack().emplace_back(iterator_get(iterator));
		move_operator(cursor);
	}
}

void mint::range_iterator_check(Cursor *cursor, size_t pos) {

	size_t base = get_stack_base(cursor);

	SharedReference &range = cursor->stack().at(base);
	SharedReference &target = cursor->stack().at(base - 1);

	Iterator *iterator = range->data<Iterator>();
	assert(iterator != nullptr);

	if (iterator->ctx.empty()) {
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->jmp(pos);
	}
	else {
		cursor->stack().emplace_back(SharedReference::unsafe(target.get()));
		cursor->stack().emplace_back(iterator_get(iterator));
		copy_operator(cursor);
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
				if (operator ()(*i, *j)) {
					return true;
				}
				if (operator ()(*j, *i)) {
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
