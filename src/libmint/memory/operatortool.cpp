#include "memory/operatortool.h"
#include "memory/memorytool.h"
#include "memory/globaldata.h"
#include "memory/builtin/string.h"
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

	cursor->waitingCalls().push(&object->data[it->second->offset]);
	cursor->waitingCalls().top().setMetadata(it->second->owner);
	call_member_operator(cursor, signature);
	return true;
}

void mint::move_operator(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	Reference &rvalue = *cursor->stack().at(base);
	Reference &lvalue = *cursor->stack().at(base - 1);

	if ((lvalue.flags() & Reference::const_address) && (lvalue.data()->format != Data::fmt_none)) {
		error("invalid modification of constant reference");
	}

	if (rvalue.flags() & Reference::const_value) {
		lvalue.copy(rvalue);
	}
	else {
		lvalue.move(rvalue);
	}

	cursor->stack().pop_back();
}

void mint::copy_operator(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	Reference &rvalue = *cursor->stack().at(base);
	Reference &lvalue = *cursor->stack().at(base - 1);

	if (lvalue.flags() & Reference::const_value) {
		error("invalid modification of constant value");
	}

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(&lvalue);
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
		if (rvalue.data()->format != Data::fmt_function) {
			error("invalid conversion from '%s' to '%s'", type_name(rvalue).c_str(), type_name(lvalue).c_str());
		}
		lvalue.data<Function>()->mapping = rvalue.data<Function>()->mapping;
		cursor->stack().pop_back();
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, ":=", 1)) {
			if (rvalue.data()->format != Data::fmt_object) {
				error("cannot convert '%s' to '%s' in assignment", type_name(rvalue).c_str(), type_name(lvalue).c_str());
			}
			if (lvalue.data<Object>()->metadata != rvalue.data<Object>()->metadata) {
				error("cannot convert '%s' to '%s' in assignment", type_name(rvalue).c_str(), type_name(lvalue).c_str());
			}
			delete [] lvalue.data<Object>()->data;
			lvalue.data<Object>()->construct(*rvalue.data<Object>());
		}
		break;
	}
}

void mint::call_operator(Cursor *cursor, int signature) {

	Reference *result = nullptr;
	Reference lvalue = cursor->waitingCalls().top().function();
	Class *metadata = cursor->waitingCalls().top().getMetadata();
	bool member = cursor->waitingCalls().top().isMember();
	cursor->waitingCalls().pop();

	switch (lvalue.data()->format) {
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
		cursor->raise(&lvalue);
		break;
	case Data::fmt_number:
		result = Reference::create<Number>();
		result->copy(lvalue);
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		result->copy(lvalue);
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		result = Reference::create<None>();
		result->copy(lvalue);
		cursor->stack().push_back(SharedReference::unique(result));
	case Data::fmt_function:
		auto it = find_function_signature(cursor, lvalue.data<Function>()->mapping, signature + (member ? 1 : 0));
		if (it == lvalue.data<Function>()->mapping.end()) {
			error("called function doesn't take %d parameter(s)", signature + (member ? 1 : 0));
		}
		const Function::Handler &hanlder = it->second;
		if (cursor->call(hanlder.module, hanlder.offset, metadata)) {
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

	Reference *result = nullptr;
	Reference lvalue = cursor->waitingCalls().top().function();
	Class *metadata = cursor->waitingCalls().top().getMetadata();
	bool member = cursor->waitingCalls().top().isMember();
	bool global = lvalue.flags() & Reference::global;
	cursor->waitingCalls().pop();

	switch (lvalue.data()->format) {
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
		cursor->raise(&lvalue);
		break;
	case Data::fmt_number:
		result = Reference::create<Number>();
		result->copy(lvalue);
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		result->copy(lvalue);
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		result = Reference::create<None>();
		result->copy(lvalue);
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_function:
		auto it = find_function_signature(cursor, lvalue.data<Function>()->mapping, signature + (global ? 0 : 1));
		if (it == lvalue.data<Function>()->mapping.end()) {
			error("called member doesn't take %d parameter(s)", signature + (global ? 0 : 1));
		}
		const Function::Handler &hanlder = it->second;
		if (cursor->call(hanlder.module, hanlder.offset, metadata)) {
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

	Reference &rvalue = *cursor->stack().at(base);
	Reference &lvalue = *cursor->stack().at(base - 1);
	Reference *result = nullptr;

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(&lvalue);
		break;
	case Data::fmt_number:
		result = Reference::create<Number>();
		result->data<Number>()->value = lvalue.data<Number>()->value + to_number(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		result->data<Boolean>()->value = lvalue.data<Boolean>()->value + to_boolean(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, "+", 1)) {
			error("class '%s' dosen't ovreload operator '+'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_function:
		result = Reference::create<Function>();
		if (rvalue.data()->format != Data::fmt_function) {
			error("invalid use of operator '+' with '%s' and '%s' types", type_name(lvalue).c_str(), type_name(rvalue).c_str());
		}
		for (auto item : lvalue.data<Function>()->mapping) {
			result->data<Function>()->mapping.insert(item);
		}
		for (auto item : rvalue.data<Function>()->mapping) {
			result->data<Function>()->mapping.insert(item);
		}
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	}
}

void mint::sub_operator(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	Reference &rvalue = *cursor->stack().at(base);
	Reference &lvalue = *cursor->stack().at(base - 1);
	Reference *result = nullptr;

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(&lvalue);
		break;
	case Data::fmt_number:
		result = Reference::create<Number>();
		result->data<Number>()->value = lvalue.data<Number>()->value - to_number(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		result->data<Boolean>()->value = lvalue.data<Boolean>()->value - to_boolean(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, "-", 1)) {
			error("class '%s' dosen't ovreload operator '-'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '-'", type_name(lvalue).c_str());
		break;
	}
}

void mint::mul_operator(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	Reference &rvalue = *cursor->stack().at(base);
	Reference &lvalue = *cursor->stack().at(base - 1);
	Reference *result = nullptr;

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(&lvalue);
		break;
	case Data::fmt_number:
		result = Reference::create<Number>();
		result->data<Number>()->value = lvalue.data<Number>()->value * to_number(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		result->data<Boolean>()->value = lvalue.data<Boolean>()->value && to_boolean(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, "*", 1)) {
			error("class '%s' dosen't ovreload operator '*'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '*'", type_name(lvalue).c_str());
	}
}

void mint::div_operator(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	Reference &rvalue = *cursor->stack().at(base);
	Reference &lvalue = *cursor->stack().at(base - 1);
	Reference *result = nullptr;

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(&lvalue);
		break;
	case Data::fmt_number:
		result = Reference::create<Number>();
		result->data<Number>()->value = lvalue.data<Number>()->value / to_number(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		result->data<Boolean>()->value = lvalue.data<Boolean>()->value / to_boolean(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, "/", 1)) {
			error("class '%s' dosen't ovreload operator '/'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '/'", type_name(lvalue).c_str());
		break;
	}
}

void mint::pow_operator(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	Reference &rvalue = *cursor->stack().at(base);
	Reference &lvalue = *cursor->stack().at(base - 1);
	Reference *result = nullptr;

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(&lvalue);
		break;
	case Data::fmt_number:
		result = Reference::create<Number>();
		result->data<Number>()->value = pow(lvalue.data<Number>()->value, to_number(cursor, rvalue));
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, "**", 1)) {
			error("class '%s' dosen't ovreload operator '**'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_boolean:
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '**'", type_name(lvalue).c_str());
		break;
	}
}

void mint::mod_operator(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	Reference &rvalue = *cursor->stack().at(base);
	Reference &lvalue = *cursor->stack().at(base - 1);
	Reference *result = nullptr;

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(&lvalue);
		break;
	case Data::fmt_number:
		if (long divider = to_number(cursor, rvalue)) {
			result = Reference::create<Number>();
			result->data<Number>()->value = (long)lvalue.data<Number>()->value % divider;
			cursor->stack().pop_back();
			cursor->stack().pop_back();
			cursor->stack().push_back(SharedReference::unique(result));
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
	case Data::fmt_boolean:
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '%%'", type_name(lvalue).c_str());
		break;
	}
}

void mint::is_operator(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	Reference &rvalue = *cursor->stack().at(base);
	Reference &lvalue = *cursor->stack().at(base - 1);

	Reference *result = Reference::create<Boolean>();
	result->data<Boolean>()->value = lvalue.data() == rvalue.data();
	cursor->stack().pop_back();
	cursor->stack().pop_back();
	cursor->stack().push_back(SharedReference::unique(result));
}

void mint::eq_operator(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	Reference &rvalue = *cursor->stack().at(base);
	Reference &lvalue = *cursor->stack().at(base - 1);
	Reference *result = nullptr;

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		result = Reference::create<Boolean>();
		result->data<Boolean>()->value = (rvalue.data()->format == Data::fmt_none);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_null:
		result = Reference::create<Boolean>();
		result->data<Boolean>()->value = (rvalue.data()->format == Data::fmt_null);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_number:
		result = Reference::create<Boolean>();
		switch (rvalue.data()->format) {
		case Data::fmt_none:
		case Data::fmt_null:
			result->data<Boolean>()->value = false;
			break;
		default:
			result->data<Boolean>()->value = lvalue.data<Number>()->value == to_number(cursor, rvalue);
		}
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		switch (rvalue.data()->format) {
		case Data::fmt_none:
		case Data::fmt_null:
			result->data<Boolean>()->value = false;
			break;
		default:
			result->data<Boolean>()->value = lvalue.data<Boolean>()->value == to_boolean(cursor, rvalue);
		}
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, "==", 1)) {
			result = Reference::create<Boolean>();
			switch (rvalue.data()->format) {
			case Data::fmt_none:
			case Data::fmt_null:
				result->data<Boolean>()->value = false;
				break;
			default:
				error("class '%s' dosen't ovreload operator '=='(1)", type_name(lvalue).c_str());
			}
			cursor->stack().pop_back();
			cursor->stack().pop_back();
			cursor->stack().push_back(SharedReference::unique(result));
		}
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '=='", type_name(lvalue).c_str());
		break;
	}
}

void mint::ne_operator(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	Reference &rvalue = *cursor->stack().at(base);
	Reference &lvalue = *cursor->stack().at(base - 1);
	Reference *result = nullptr;

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		result = Reference::create<Boolean>();
		result->data<Boolean>()->value = (rvalue.data()->format != Data::fmt_none);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_null:
		result = Reference::create<Boolean>();
		result->data<Boolean>()->value = (rvalue.data()->format != Data::fmt_null);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_number:
		result = Reference::create<Boolean>();
		switch (rvalue.data()->format) {
		case Data::fmt_none:
		case Data::fmt_null:
			result->data<Boolean>()->value = true;
			break;
		default:
			result->data<Boolean>()->value = lvalue.data<Number>()->value != to_number(cursor, rvalue);
		}
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		switch (rvalue.data()->format) {
		case Data::fmt_none:
		case Data::fmt_null:
			result->data<Boolean>()->value = true;
			break;
		default:
			result->data<Boolean>()->value = lvalue.data<Boolean>()->value != to_boolean(cursor, rvalue);
		}
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, "!=", 1)) {
			result = Reference::create<Boolean>();
			switch (rvalue.data()->format) {
			case Data::fmt_none:
			case Data::fmt_null:
				result->data<Boolean>()->value = true;
				break;
			default:
				error("class '%s' dosen't ovreload operator '!='(1)", type_name(lvalue).c_str());
			}
			cursor->stack().pop_back();
			cursor->stack().pop_back();
			cursor->stack().push_back(SharedReference::unique(result));
		}
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '!='", type_name(lvalue).c_str());
		break;
	}
}

void mint::lt_operator(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	Reference &rvalue = *cursor->stack().at(base);
	Reference &lvalue = *cursor->stack().at(base - 1);
	Reference *result = nullptr;

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(&lvalue);
		break;
	case Data::fmt_number:
		result = Reference::create<Boolean>();
		result->data<Boolean>()->value = lvalue.data<Number>()->value < to_number(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		result->data<Boolean>()->value = lvalue.data<Boolean>()->value < to_boolean(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, "<", 1)) {
			error("class '%s' dosen't ovreload operator '<'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '<'", type_name(lvalue).c_str());
		break;
	}
}

void mint::gt_operator(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	Reference &rvalue = *cursor->stack().at(base);
	Reference &lvalue = *cursor->stack().at(base - 1);
	Reference *result = nullptr;

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(&lvalue);
		break;
	case Data::fmt_number:
		result = Reference::create<Boolean>();
		result->data<Boolean>()->value = lvalue.data<Number>()->value > to_number(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		result->data<Boolean>()->value = lvalue.data<Boolean>()->value > to_boolean(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, ">", 1)) {
			error("class '%s' dosen't ovreload operator '>'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '>'", type_name(lvalue).c_str());
		break;
	}
}

void mint::le_operator(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	Reference &rvalue = *cursor->stack().at(base);
	Reference &lvalue = *cursor->stack().at(base - 1);
	Reference *result = nullptr;

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(&lvalue);
		break;
	case Data::fmt_number:
		result = Reference::create<Boolean>();
		result->data<Boolean>()->value = lvalue.data<Number>()->value <= to_number(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		result->data<Boolean>()->value = lvalue.data<Boolean>()->value <= to_boolean(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, "<=", 1)) {
			error("class '%s' dosen't ovreload operator '<='(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '<='", type_name(lvalue).c_str());
		break;
	}
}

void mint::ge_operator(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	Reference &rvalue = *cursor->stack().at(base);
	Reference &lvalue = *cursor->stack().at(base - 1);
	Reference *result = nullptr;

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(&lvalue);
		break;
	case Data::fmt_number:
		result = Reference::create<Boolean>();
		result->data<Boolean>()->value = lvalue.data<Number>()->value >= to_number(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		result->data<Boolean>()->value = lvalue.data<Boolean>()->value >= to_boolean(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, ">=", 1)) {
			error("class '%s' dosen't ovreload operator '>='(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '>='", type_name(lvalue).c_str());
		break;
	}
}

void mint::and_operator(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	Reference &rvalue = *cursor->stack().at(base);
	Reference &lvalue = *cursor->stack().at(base - 1);
	Reference *result = nullptr;

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(&lvalue);
		break;
	case Data::fmt_number:
		result = Reference::create<Boolean>();
		result->data<Boolean>()->value = lvalue.data<Number>()->value && to_number(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		result->data<Boolean>()->value = lvalue.data<Boolean>()->value && to_boolean(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, "&&", 1)) {
			error("class '%s' dosen't ovreload operator '&&'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '&&'", type_name(lvalue).c_str());
		break;
	}
}

void mint::or_operator(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	Reference &rvalue = *cursor->stack().at(base);
	Reference &lvalue = *cursor->stack().at(base - 1);
	Reference *result = nullptr;

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(&lvalue);
		break;
	case Data::fmt_number:
		result = Reference::create<Boolean>();
		result->data<Boolean>()->value = lvalue.data<Number>()->value || to_number(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		result->data<Boolean>()->value = lvalue.data<Boolean>()->value || to_boolean(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, "||", 1)) {
			error("class '%s' dosen't ovreload operator '||'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '||'", type_name(lvalue).c_str());
		break;
	}
}

void mint::band_operator(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	Reference &rvalue = *cursor->stack().at(base);
	Reference &lvalue = *cursor->stack().at(base - 1);
	Reference *result = nullptr;

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(&lvalue);
		break;
	case Data::fmt_number:
		result = Reference::create<Number>();
		result->data<Number>()->value = (long)lvalue.data<Number>()->value & (long)to_number(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		result->data<Boolean>()->value = lvalue.data<Boolean>()->value & to_boolean(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, "&", 1)) {
			error("class '%s' dosen't ovreload operator '&'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '&'", type_name(lvalue).c_str());
		break;
	}
}

void mint::bor_operator(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	Reference &rvalue = *cursor->stack().at(base);
	Reference &lvalue = *cursor->stack().at(base - 1);
	Reference *result = nullptr;

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(&lvalue);
		break;
	case Data::fmt_number:
		result = Reference::create<Number>();
		result->data<Number>()->value = (long)lvalue.data<Number>()->value | (long)to_number(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		result->data<Boolean>()->value = lvalue.data<Boolean>()->value | to_boolean(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, "|", 1)) {
			error("class '%s' dosen't ovreload operator '|'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '|'", type_name(lvalue).c_str());
		break;
	}
}

void mint::xor_operator(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	Reference &rvalue = *cursor->stack().at(base);
	Reference &lvalue = *cursor->stack().at(base - 1);
	Reference *result = nullptr;

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(&lvalue);
		break;
	case Data::fmt_number:
		result = Reference::create<Number>();
		result->data<Number>()->value = (long)lvalue.data<Number>()->value ^ (long)to_number(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		result->data<Boolean>()->value = (long)lvalue.data<Boolean>()->value ^ to_boolean(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, "^", 1)) {
			error("class '%s' dosen't ovreload operator '^'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '^'", type_name(lvalue).c_str());
		break;
	}
}

void mint::inc_operator(Cursor *cursor) {

	Reference &value = *cursor->stack().back();
	Reference *result;

	if (value.flags() & Reference::const_value) {
		error("invalid modification of constant value");
	}

	switch (value.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(&value);
		break;
	case Data::fmt_number:
		result = Reference::create<Number>();
		result->data<Number>()->value = value.data<Number>()->value + 1;
		value.move(*SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		result->data<Boolean>()->value = value.data<Boolean>()->value + 1;
		value.move(*SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, "++", 0)) {
			error("class '%s' dosen't ovreload operator '++'(0)", type_name(value).c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '++'", type_name(value).c_str());
		break;
	}
}

void mint::dec_operator(Cursor *cursor) {

	Reference &value = *cursor->stack().back();
	Reference *result;

	if (value.flags() & Reference::const_value) {
		error("invalid modification of constant value");
	}

	switch (value.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(&value);
		break;
	case Data::fmt_number:
		result = Reference::create<Number>();
		result->data<Number>()->value = value.data<Number>()->value - 1;
		value.move(*SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		result->data<Boolean>()->value = value.data<Boolean>()->value - 1;
		value.move(*SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, "--", 0)) {
			error("class '%s' dosen't ovreload operator '--'(0)", type_name(value).c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '--'", type_name(value).c_str());
		break;
	}
}

void mint::not_operator(Cursor *cursor) {

	Reference &value = *cursor->stack().back();
	Reference *result = Reference::create<Boolean>();

	switch (value.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(&value);
		break;
	case Data::fmt_number:
		result->data<Boolean>()->value = !value.data<Number>()->value;
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result->data<Boolean>()->value = !value.data<Boolean>()->value;
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, "!", 0)) {
			error("class '%s' dosen't ovreload operator '!'(0)", type_name(value).c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '!'", type_name(value).c_str());
		break;
	}
}

void mint::compl_operator(Cursor *cursor) {

	Reference &value = *cursor->stack().back();
	Reference *result;

	switch (value.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(&value);
		break;
	case Data::fmt_number:
		result = Reference::create<Number>();
		result->data<Number>()->value = ~((long)value.data<Number>()->value);
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		result->data<Boolean>()->value = !value.data<Boolean>()->value;
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, "~", 0)) {
			error("class '%s' dosen't ovreload operator '~'(0)", type_name(value).c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '~'", type_name(value).c_str());
		break;
	}
}

void mint::pos_operator(Cursor *cursor) {

	Reference &value = *cursor->stack().back();
	Reference *result;

	switch (value.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(&value);
		break;
	case Data::fmt_number:
		result = Reference::create<Number>();
		result->data<Number>()->value = +(value.data<Number>()->value);
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		result->data<Boolean>()->value = +(value.data<Boolean>()->value);
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, "+", 0)) {
			error("class '%s' dosen't ovreload operator '+'(0)", type_name(value).c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '+'", type_name(value).c_str());
		break;
	}
}

void mint::neg_operator(Cursor *cursor) {

	Reference &value = *cursor->stack().back();
	Reference *result;

	switch (value.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(&value);
		break;
	case Data::fmt_number:
		result = Reference::create<Number>();
		result->data<Number>()->value = -(value.data<Number>()->value);
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		result->data<Boolean>()->value = -(value.data<Boolean>()->value);
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, "-", 0)) {
			error("class '%s' dosen't ovreload operator '-'(0)", type_name(value).c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '-'", type_name(value).c_str());
		break;
	}
}

void mint::shift_left_operator(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	Reference &rvalue = *cursor->stack().at(base);
	Reference &lvalue = *cursor->stack().at(base - 1);
	Reference *result = nullptr;

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(&lvalue);
		break;
	case Data::fmt_number:
		result = Reference::create<Number>();
		result->data<Number>()->value = (long)lvalue.data<Number>()->value << (long)to_number(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Number>();
		result->data<Number>()->value = lvalue.data<Boolean>()->value << (long)to_number(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, "<<", 1)) {
			error("class '%s' dosen't ovreload operator '<<'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '<<'", type_name(lvalue).c_str());
		break;
	}
}

void mint::shift_right_operator(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	Reference &rvalue = *cursor->stack().at(base);
	Reference &lvalue = *cursor->stack().at(base - 1);
	Reference *result = nullptr;

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(&lvalue);
		break;
	case Data::fmt_number:
		result = Reference::create<Number>();
		result->data<Number>()->value = (long)lvalue.data<Number>()->value >> (long)to_number(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		result->data<Boolean>()->value = lvalue.data<Boolean>()->value >> (long)to_number(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, ">>", 1)) {
			error("class '%s' dosen't ovreload operator '>>'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '>>'", type_name(lvalue).c_str());
		break;
	}
}

void mint::inclusive_range_operator(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	Reference &rvalue = *cursor->stack().at(base);
	Reference &lvalue = *cursor->stack().at(base - 1);
	Reference *result = nullptr;

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(&lvalue);
		break;
	case Data::fmt_number:
		result = Reference::create<Iterator>();
		result->data<Object>()->construct();
		for (double begin = lvalue.data<Number>()->value, end = to_number(cursor, rvalue), i = min(begin, end); i <= max(begin, end); ++i) {
			Reference *item = Reference::create<Number>();
			item->data<Number>()->value = i;
			if (begin < end) {
				iterator_insert(result->data<Iterator>(), SharedReference::unique(item));
			}
			else {
				iterator_add(result->data<Iterator>(), SharedReference::unique(item));
			}
		}
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, "..", 1)) {
			error("class '%s' dosen't ovreload operator '..'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_boolean:
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '..'", type_name(lvalue).c_str());
		break;
	}
}

void mint::exclusive_range_operator(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	Reference &rvalue = *cursor->stack().at(base);
	Reference &lvalue = *cursor->stack().at(base - 1);
	Reference *result = nullptr;

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(&lvalue);
		break;
	case Data::fmt_number:
		result = Reference::create<Iterator>();
		result->data<Object>()->construct();
		for (double begin = lvalue.data<Number>()->value, end = to_number(cursor, rvalue), i = min(begin, end); i < max(begin, end); ++i) {
			Reference *item = Reference::create<Number>();
			if (begin < end) {
				item->data<Number>()->value = i;
				iterator_insert(result->data<Iterator>(), SharedReference::unique(item));
			}
			else {
				item->data<Number>()->value = i + 1;
				iterator_add(result->data<Iterator>(), SharedReference::unique(item));
			}
		}
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, "...", 1)) {
			error("class '%s' dosen't ovreload operator '...'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_boolean:
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '...'", type_name(lvalue).c_str());
		break;
	}
}

void mint::typeof_operator(Cursor *cursor) {

	Reference &value = *cursor->stack().back();
	Reference *result = Reference::create<String>();

	result->data<String>()->str = type_name(value);

	cursor->stack().pop_back();
	cursor->stack().push_back(SharedReference::unique(result));
}

void mint::membersof_operator(Cursor *cursor) {

	Reference &value = *cursor->stack().back();
	Reference *result = Reference::create<Array>();

	if (value.data()->format == Data::fmt_object) {

		Object *object = value.data<Object>();
		Array *array = result->data<Array>();

		array->construct();
		array->values.reserve(object->metadata->members().size());

		for (auto member : object->metadata->members()) {

			if ((member.second->value.flags() & Reference::user_hiden) && (object->metadata != cursor->symbols().getMetadata())) {
				continue;
			}

			if ((member.second->value.flags() & Reference::child_hiden) && (member.second->owner != cursor->symbols().getMetadata())) {
				continue;
			}

			String *name = Reference::alloc<String>();
			name->construct();
			name->str = member.first;
			array_append(array, SharedReference::unique(new Reference(Reference::standard, name)));
		}
	}

	cursor->stack().pop_back();
	cursor->stack().push_back(SharedReference::unique(result));
}

void mint::subscript_operator(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	Reference &rvalue = *cursor->stack().at(base);
	Reference &lvalue = *cursor->stack().at(base - 1);
	Reference *result = nullptr;

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(&lvalue);
		break;
	case Data::fmt_number:
		result = Reference::create<Number>();
		result->data<Number>()->value = ((long)(lvalue.data<Number>()->value / pow(10, to_number(cursor, rvalue))) % 10);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		error("invalid use of '%s' type with operator '[]'", type_name(lvalue).c_str());
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, "[]", 1)) {
			error("class '%s' dosen't ovreload operator '[]'(1)", lvalue.data<Object>()->metadata->name().c_str());
		}
		break;
	case Data::fmt_function:
		auto signature = lvalue.data<Function>()->mapping.find(to_number(cursor, rvalue));
		if (signature != lvalue.data<Function>()->mapping.end()) {
			result = Reference::create<Function>();
			result->data<Function>()->mapping.insert(*signature);
		}
		else {
			result = Reference::create<None>();
		}
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	}
}

void mint::regex_match(Cursor *cursor) {

	size_t base = get_stack_base(cursor);
	Reference &lvalue = *cursor->stack().at(base - 1);

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(&lvalue);
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, "=~", 1)) {
			error("class '%s' dosen't ovreload operator '=~'(1)", type_name(lvalue).c_str());
		}
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
	Reference &lvalue = *cursor->stack().at(base - 1);

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		cursor->raise(&lvalue);
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, "!~", 1)) {
			error("class '%s' dosen't ovreload operator '!~'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_number:
	case Data::fmt_boolean:
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '!~'", type_name(lvalue).c_str());
		break;
	}
}

void iterator_move(Iterator *iterator, Reference *dest, Cursor *cursor) {

	if (!iterator->ctx.empty()) {
		cursor->stack().push_back(dest);
		cursor->stack().push_back(iterator->ctx.front());
		move_operator(cursor);
		cursor->stack().pop_back();
	}
}

void mint::find_defined_symbol(Cursor *cursor, const string &symbol) {

	if (Class *desc = GlobalData::instance().getClass(symbol)) {
		Object *object = desc->makeInstance();
		object->construct();
		cursor->stack().push_back(SharedReference::unique(new Reference(Reference::standard, object)));
	}
	else {

		auto it = GlobalData::instance().symbols().find(symbol);
		if (it != GlobalData::instance().symbols().end()) {
			cursor->stack().push_back(&it->second);
		}
		else {

			it = cursor->symbols().find(symbol);
			if (it != cursor->symbols().end()) {
				cursor->stack().push_back(&it->second);
			}
			else {
				cursor->stack().push_back(SharedReference::unique(Reference::create<None>()));
			}
		}
	}

}

void mint::find_defined_member(Cursor *cursor, const string &symbol) {

	if (cursor->stack().back()->data()->format != Data::fmt_none) {

		SharedReference value = cursor->stack().back();
		cursor->stack().pop_back();

		if (value->data()->format == Data::fmt_object) {

			Object *object = value->data<Object>();

			if (Class *desc = object->metadata->globals().getClass(symbol)) {
				Object *object = desc->makeInstance();
				object->construct();
				cursor->stack().push_back(SharedReference::unique(new Reference(Reference::standard, object)));
			}
			else {

				auto it_global = object->metadata->globals().members().find(symbol);
				if (it_global != object->metadata->globals().members().end()) {
					cursor->stack().push_back(&it_global->second->value);
				}
				else {

					auto it_member = object->metadata->members().find(symbol);
					if (it_member != object->metadata->members().end()) {
						cursor->stack().push_back(&object->data[it_member->second->offset]);
					}
					else {
						cursor->stack().push_back(SharedReference::unique(Reference::create<None>()));
					}
				}
			}
		}
		else {
			cursor->stack().push_back(SharedReference::unique(Reference::create<None>()));
		}
	}
}

void mint::check_defined(Cursor *cursor) {

	SharedReference value = cursor->stack().back();
	Reference *result = Reference::create<Boolean>();

	result->data<Boolean>()->value = (value->data()->format != Data::fmt_none);

	cursor->stack().pop_back();
	cursor->stack().push_back(SharedReference::unique(result));
}

void mint::in_find(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	Reference &rvalue = *cursor->stack().at(base);
	Reference &lvalue = *cursor->stack().at(base - 1);
	Reference *result = Reference::create<Boolean>();

	if (rvalue.data()->format == Data::fmt_object && rvalue.data<Object>()->metadata->metatype() == Class::hash) {
		result->data<Boolean>()->value = rvalue.data<Hash>()->values.find(&lvalue) != rvalue.data<Hash>()->values.end();
	}
	else if (rvalue.data()->format == Data::fmt_object && rvalue.data<Object>()->metadata->metatype() == Class::string) {
		result->data<Boolean>()->value = rvalue.data<String>()->str.find(to_string(lvalue)) != string::npos;
	}
	else {
		Iterator *iterator = Reference::alloc<Iterator>();
		iterator_init(iterator, rvalue);
		for (SharedReference &item : iterator->ctx) {
			cursor->stack().push_back(&lvalue);
			cursor->stack().push_back(item);
			eq_operator(cursor);
			if ((result->data<Boolean>()->value = to_boolean(cursor, *cursor->stack().back()))) {
				cursor->stack().pop_back();
				break;
			}
			else {
				cursor->stack().pop_back();
			}
		}
	}

	cursor->stack().pop_back();
	cursor->stack().pop_back();
	cursor->stack().push_back(SharedReference::unique(result));
}

void mint::in_init(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	Reference &rvalue = *cursor->stack().at(base);
	Reference &lvalue = *cursor->stack().at(base - 1);
	Reference *result = Reference::create<Iterator>();

	Iterator *iterator = result->data<Iterator>();
	iterator_init(iterator, rvalue);
	cursor->stack().push_back(SharedReference::unique(result));
	iterator_move(iterator, &lvalue, cursor);
}

void mint::in_next(Cursor *cursor) {

	size_t base = get_stack_base(cursor);

	Reference &rvalue = *cursor->stack().at(base);
	Reference &lvalue = *cursor->stack().at(base - 2);

	Iterator *iterator = rvalue.data<Iterator>();
	assert(iterator != nullptr);
	iterator->ctx.pop_front();
	iterator_move(iterator, &lvalue, cursor);
}

void mint::in_check(Cursor *cursor) {

	Reference &rvalue = *cursor->stack().back();
	Reference *result = Reference::create<Boolean>();

	Iterator *iterator = rvalue.data<Iterator>();
	assert(iterator != nullptr);
	if (iterator->ctx.empty()) {
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().pop_back();

		result->data<Boolean>()->value = false;
	}
	else {
		result->data<Boolean>()->value = true;
	}

	cursor->stack().push_back(SharedReference::unique(result));
}

bool Hash::compare::operator ()(const Hash::key_type &lvalue, const Hash::key_type &rvalue) const {

	if (Process *process = Scheduler::instance()->currentProcess()) {

		if (Cursor *cursor = process->cursor()) {

			switch (lvalue->data()->format) {
			case Data::fmt_none:
				return rvalue->data()->format != Data::fmt_none;

			case Data::fmt_null:
				return rvalue->data()->format != Data::fmt_null;

			case Data::fmt_number:
				return lvalue->data<Number>()->value < to_number(cursor, *rvalue);

			case Data::fmt_boolean:
				return lvalue->data<Boolean>()->value < to_boolean(cursor, *rvalue);

			case Data::fmt_object:
				switch (lvalue->data<Object>()->metadata->metatype()) {
				case Class::object:
				case Class::regex:
				case Class::hash:
				case Class::array:
				case Class::iterator:
				case Class::library:
				case Class::libobject:
					error("invalid use of '%s' type as hash key", type_name(*lvalue).c_str());
					break;

				case Class::string:
					return lvalue->data<String>()->str < to_string(*rvalue);
				}
				break;

			case Data::fmt_function:
				error("invalid use of '%s' type as hash key", type_name(*lvalue).c_str());
				break;
			}
		}
	}

	return false;
}
