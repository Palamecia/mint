#include "memory/operatortool.h"
#include "memory/memorytool.h"
#include "memory/globaldata.h"
#include "memory/builtin/string.h"
#include "ast/cursor.h"
#include "scheduler/scheduler.h"
#include "scheduler/processor.h"
#include "system/error.h"

#include <math.h>

using namespace std;

bool call_overload(Cursor *cursor, const string &operator_overload, int signature) {

	size_t base = get_base(cursor);
	Object *object = (Object *)cursor->stack().at(base - signature)->data();
	auto it = object->metadata->members().find(operator_overload);

	if (it == object->metadata->members().end()) {
		return false;
	}

	cursor->waitingCalls().push(&object->data[it->second->offset]);
	call_member_operator(cursor, signature);
	return true;
}

void move_operator(Cursor *cursor) {

	size_t base = get_base(cursor);

	Reference &rvalue = *cursor->stack().at(base);
	Reference &lvalue = *cursor->stack().at(base - 1);

	if ((lvalue.flags() & Reference::const_ref) && (lvalue.data()->format != Data::fmt_none)) {
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

void copy_operator(Cursor *cursor) {

	size_t base = get_base(cursor);

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
		((Number *)lvalue.data())->value = to_number(cursor, rvalue);
		cursor->stack().pop_back();
		break;
	case Data::fmt_boolean:
		((Boolean *)lvalue.data())->value = to_boolean(cursor, rvalue);
		cursor->stack().pop_back();
		break;
	case Data::fmt_function:
		if (rvalue.data()->format != Data::fmt_function) {
			error("invalid conversion from '%s' to '%s'", type_name(rvalue).c_str(), type_name(lvalue).c_str());
		}
		((Function *)lvalue.data())->mapping = ((Function *)rvalue.data())->mapping;
		cursor->stack().pop_back();
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, ":=", 1)) {
			if (rvalue.data()->format != Data::fmt_object) {
				error("cannot convert '%s' to '%s' in assignment", type_name(rvalue).c_str(), type_name(lvalue).c_str());
			}
			if (((Object *)lvalue.data())->metadata != ((Object *)rvalue.data())->metadata) {
				error("cannot convert '%s' to '%s' in assignment", type_name(rvalue).c_str(), type_name(lvalue).c_str());
			}
			delete [] ((Object *)lvalue.data())->data;
			((Object *)lvalue.data())->construct(*((Object *)rvalue.data()));
		}
		break;
	}
}

void call_operator(Cursor *cursor, int signature) {

	Reference *result = nullptr;
	Reference lvalue = cursor->waitingCalls().top().function();
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
		result = Reference::create<Data>();
		result->copy(lvalue);
		cursor->stack().push_back(SharedReference::unique(result));
	case Data::fmt_function:
		auto it = find_function_signature(cursor, ((Function*)lvalue.data())->mapping, signature + (member ? 1 : 0));
		if (it == ((Function*)lvalue.data())->mapping.end()) {
			error("called function doesn't take %d parameter(s)", signature + (member ? 1 : 0));
		}
		const Function::Handler &hanlder = it->second;
		if (cursor->call(hanlder.module, hanlder.offset)) {
			if (member) {
				Object *object = (Object *)cursor->stack().at(get_base(cursor) - signature)->data();
				cursor->symbols().metadata = object->metadata;
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

void call_member_operator(Cursor *cursor, int signature) {

	size_t base = get_base(cursor);

	Reference *result = nullptr;
	Reference &object = *cursor->stack().at(base - signature);
	Reference lvalue = cursor->waitingCalls().top().function();
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
		result = Reference::create<Data>();
		result->copy(lvalue);
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_function:
		auto it = find_function_signature(cursor, ((Function*)lvalue.data())->mapping, signature + (global ? 0 : 1));
		if (it == ((Function*)lvalue.data())->mapping.end()) {
			error("called member doesn't take %d parameter(s)", signature + (global ? 0 : 1));
		}
		const Function::Handler &hanlder = it->second;
		if (cursor->call(hanlder.module, hanlder.offset)) {
			cursor->symbols().metadata = ((Object *)object.data())->metadata;
			if (hanlder.capture) {
				for (auto item : *hanlder.capture) {
					cursor->symbols().insert(item);
				}
			}
		}
		break;
	}

	if (global) {
		cursor->stack().erase(cursor->stack().begin() + (base - signature));
	}
}

void add_operator(Cursor *cursor) {

	size_t base = get_base(cursor);

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
		((Number*)result->data())->value = ((Number*)lvalue.data())->value + to_number(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		((Boolean *)result->data())->value = ((Boolean *)lvalue.data())->value + to_boolean(cursor, rvalue);
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
		for (auto item : ((Function *)lvalue.data())->mapping) {
			((Function *)result->data())->mapping.insert(item);
		}
		for (auto item : ((Function *)rvalue.data())->mapping) {
			((Function *)result->data())->mapping.insert(item);
		}
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	}
}

void sub_operator(Cursor *cursor) {

	size_t base = get_base(cursor);

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
		((Number*)result->data())->value = ((Number*)lvalue.data())->value - to_number(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		((Boolean *)result->data())->value = ((Boolean *)lvalue.data())->value - to_boolean(cursor, rvalue);
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

void mul_operator(Cursor *cursor) {

	size_t base = get_base(cursor);

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
		((Number*)result->data())->value = ((Number*)lvalue.data())->value * to_number(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		((Boolean *)result->data())->value = ((Boolean *)lvalue.data())->value * to_boolean(cursor, rvalue);
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

void div_operator(Cursor *cursor) {

	size_t base = get_base(cursor);

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
		((Number*)result->data())->value = ((Number*)lvalue.data())->value / to_number(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		((Boolean *)result->data())->value = ((Boolean *)lvalue.data())->value / to_boolean(cursor, rvalue);
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

void pow_operator(Cursor *cursor) {

	size_t base = get_base(cursor);

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
		((Number*)result->data())->value = pow(((Number*)lvalue.data())->value, to_number(cursor, rvalue));
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

void mod_operator(Cursor *cursor) {

	size_t base = get_base(cursor);

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
			((Number*)result->data())->value = (long)((Number*)lvalue.data())->value % divider;
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

void is_operator(Cursor *cursor) {

	size_t base = get_base(cursor);

	Reference &rvalue = *cursor->stack().at(base);
	Reference &lvalue = *cursor->stack().at(base - 1);

	Reference *result = Reference::create<Boolean>();
	((Boolean *)result->data())->value = lvalue.data() == rvalue.data();
	cursor->stack().pop_back();
	cursor->stack().pop_back();
	cursor->stack().push_back(SharedReference::unique(result));
}

void eq_operator(Cursor *cursor) {

	size_t base = get_base(cursor);

	Reference &rvalue = *cursor->stack().at(base);
	Reference &lvalue = *cursor->stack().at(base - 1);
	Reference *result = nullptr;

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		result = Reference::create<Boolean>();
		((Boolean *)result->data())->value = (rvalue.data()->format == Data::fmt_none);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_null:
		result = Reference::create<Boolean>();
		((Boolean *)result->data())->value = (rvalue.data()->format == Data::fmt_null);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_number:
		result = Reference::create<Boolean>();
		switch (rvalue.data()->format) {
		case Data::fmt_none:
		case Data::fmt_null:
			((Boolean *)result->data())->value = false;
			break;
		default:
			((Boolean *)result->data())->value = ((Number *)lvalue.data())->value == to_number(cursor, rvalue);
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
			((Boolean *)result->data())->value = false;
			break;
		default:
			((Boolean *)result->data())->value = ((Boolean *)lvalue.data())->value == to_boolean(cursor, rvalue);
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
				((Boolean *)result->data())->value = false;
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

void ne_operator(Cursor *cursor) {

	size_t base = get_base(cursor);

	Reference &rvalue = *cursor->stack().at(base);
	Reference &lvalue = *cursor->stack().at(base - 1);
	Reference *result = nullptr;

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		result = Reference::create<Boolean>();
		((Boolean *)result->data())->value = (rvalue.data()->format != Data::fmt_none);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_null:
		result = Reference::create<Boolean>();
		((Boolean *)result->data())->value = (rvalue.data()->format != Data::fmt_null);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_number:
		result = Reference::create<Boolean>();
		switch (rvalue.data()->format) {
		case Data::fmt_none:
		case Data::fmt_null:
			((Boolean *)result->data())->value = true;
			break;
		default:
			((Boolean *)result->data())->value = ((Number*)lvalue.data())->value != to_number(cursor, rvalue);
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
			((Boolean *)result->data())->value = true;
			break;
		default:
			((Boolean *)result->data())->value = ((Boolean *)lvalue.data())->value != to_boolean(cursor, rvalue);
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
				((Boolean *)result->data())->value = true;
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

void lt_operator(Cursor *cursor) {

	size_t base = get_base(cursor);

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
		((Boolean *)result->data())->value = ((Number *)lvalue.data())->value < to_number(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		((Boolean *)result->data())->value = ((Boolean *)lvalue.data())->value < to_boolean(cursor, rvalue);
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

void gt_operator(Cursor *cursor) {

	size_t base = get_base(cursor);

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
		((Boolean *)result->data())->value = ((Number *)lvalue.data())->value > to_number(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		((Boolean *)result->data())->value = ((Boolean *)lvalue.data())->value > to_boolean(cursor, rvalue);
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

void le_operator(Cursor *cursor) {

	size_t base = get_base(cursor);

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
		((Boolean *)result->data())->value = ((Number *)lvalue.data())->value <= to_number(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		((Boolean *)result->data())->value = ((Boolean *)lvalue.data())->value <= to_boolean(cursor, rvalue);
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

void ge_operator(Cursor *cursor) {

	size_t base = get_base(cursor);

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
		((Boolean *)result->data())->value = ((Number *)lvalue.data())->value >= to_number(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		((Boolean *)result->data())->value = ((Boolean *)lvalue.data())->value >= to_boolean(cursor, rvalue);
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

void and_operator(Cursor *cursor) {

	size_t base = get_base(cursor);

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
		((Boolean *)result->data())->value = ((Number *)lvalue.data())->value && to_number(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		((Boolean *)result->data())->value = ((Boolean *)lvalue.data())->value && to_boolean(cursor, rvalue);
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

void or_operator(Cursor *cursor) {

	size_t base = get_base(cursor);

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
		((Boolean *)result->data())->value = ((Number *)lvalue.data())->value || to_number(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		((Boolean *)result->data())->value = ((Boolean *)lvalue.data())->value || to_boolean(cursor, rvalue);
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

void band_operator(Cursor *cursor) {

	size_t base = get_base(cursor);

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
		((Number *)result->data())->value = (long)((Number *)lvalue.data())->value & (long)to_number(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		((Boolean *)result->data())->value = ((Boolean *)lvalue.data())->value & to_boolean(cursor, rvalue);
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

void bor_operator(Cursor *cursor) {

	size_t base = get_base(cursor);

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
		((Number *)result->data())->value = (long)((Number *)lvalue.data())->value | (long)to_number(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		((Boolean *)result->data())->value = ((Boolean *)lvalue.data())->value | to_boolean(cursor, rvalue);
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

void xor_operator(Cursor *cursor) {

	size_t base = get_base(cursor);

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
		((Number *)result->data())->value = (long)((Number *)lvalue.data())->value ^ (long)to_number(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		((Boolean *)result->data())->value = (long)((Boolean *)lvalue.data())->value ^ to_boolean(cursor, rvalue);
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

void inc_operator(Cursor *cursor) {

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
		((Number *)result->data())->value = ((Number *)value.data())->value + 1;
		value.move(*SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		((Boolean *)result->data())->value = ((Boolean *)value.data())->value + 1;
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

void dec_operator(Cursor *cursor) {

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
		((Number *)result->data())->value = ((Number *)value.data())->value - 1;
		value.move(*SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		((Boolean *)result->data())->value = ((Boolean *)value.data())->value - 1;
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

void not_operator(Cursor *cursor) {

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
		((Boolean *)result->data())->value = !((Number *)value.data())->value;
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		((Boolean *)result->data())->value = !((Boolean *)value.data())->value;
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

void compl_operator(Cursor *cursor) {

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
		((Number *)result->data())->value = ~((long)((Number *)value.data())->value);
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		((Boolean *)result->data())->value = ~(((Boolean *)value.data())->value);
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

void pos_operator(Cursor *cursor) {

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
		((Number *)result->data())->value = +(((Number *)value.data())->value);
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		((Boolean *)result->data())->value = +(((Boolean *)value.data())->value);
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

void neg_operator(Cursor *cursor) {

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
		((Number *)result->data())->value = -(((Number *)value.data())->value);
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		((Boolean *)result->data())->value = -(((Boolean *)value.data())->value);
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

void shift_left_operator(Cursor *cursor) {

	size_t base = get_base(cursor);

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
		((Number *)result->data())->value = (long)((Number *)lvalue.data())->value << (long)to_number(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		((Boolean *)result->data())->value = ((Boolean *)lvalue.data())->value << (long)to_number(cursor, rvalue);
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

void shift_right_operator(Cursor *cursor) {

	size_t base = get_base(cursor);

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
		((Number *)result->data())->value = (long)((Number *)lvalue.data())->value >> (long)to_number(cursor, rvalue);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		((Boolean *)result->data())->value = ((Boolean *)lvalue.data())->value >> (long)to_number(cursor, rvalue);
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

void inclusive_range_operator(Cursor *cursor) {

	size_t base = get_base(cursor);

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
		((Object *)result->data())->construct();
		for (double begin = ((Number *)lvalue.data())->value, end = to_number(cursor, rvalue), i = min(begin, end); i <= max(begin, end); ++i) {
			Reference *item = Reference::create<Number>();
			((Number *)item->data())->value = i;
			if (begin < end) {
				iterator_insert((Iterator *)result->data(), SharedReference::unique(item));
			}
			else {
				iterator_add((Iterator *)result->data(), SharedReference::unique(item));
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

void exclusive_range_operator(Cursor *cursor) {

	size_t base = get_base(cursor);

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
		((Object *)result->data())->construct();
		for (double begin = ((Number *)lvalue.data())->value, end = to_number(cursor, rvalue), i = min(begin, end); i < max(begin, end); ++i) {
			Reference *item = Reference::create<Number>();
			if (begin < end) {
				((Number *)item->data())->value = i;
				iterator_insert((Iterator *)result->data(), SharedReference::unique(item));
			}
			else {
				((Number *)item->data())->value = i + 1;
				iterator_add((Iterator *)result->data(), SharedReference::unique(item));
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

void typeof_operator(Cursor *cursor) {

	Reference &value = *cursor->stack().back();
	Reference *result = Reference::create<String>();

	((String *)result->data())->str = type_name(value);

	cursor->stack().pop_back();
	cursor->stack().push_back(SharedReference::unique(result));
}

void membersof_operator(Cursor *cursor) {

	Reference &value = *cursor->stack().back();
	Reference *result = Reference::create<Array>();

	if (value.data()->format == Data::fmt_object) {

		Object *object = (Object *)value.data();
		Array *array = (Array *)result->data();

		array->values.reserve(object->metadata->members().size());

		for (auto member : object->metadata->members()) {

			if ((member.second->value.flags() & Reference::user_hiden) && (object->metadata != cursor->symbols().metadata)) {
				continue;
			}

			if ((member.second->value.flags() & Reference::child_hiden) && (member.second->owner != cursor->symbols().metadata)) {
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

void subscript_operator(Cursor *cursor) {

	size_t base = get_base(cursor);

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
		((Number*)result->data())->value = ((long)(((Number*)lvalue.data())->value / pow(10, to_number(cursor, rvalue))) % 10);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		error("invalid use of '%s' type with operator '[]'", type_name(lvalue).c_str());
		break;
	case Data::fmt_object:
		if (!call_overload(cursor, "[]", 1)) {
			error("class '%s' dosen't ovreload operator '[]'(1)", ((Object *)lvalue.data())->metadata->name().c_str());
		}
		break;
	case Data::fmt_function:
		auto signature = ((Function *)lvalue.data())->mapping.find(to_number(cursor, rvalue));
		if (signature != ((Function *)lvalue.data())->mapping.end()) {
			result = Reference::create<Function>();
			((Function *)result->data())->mapping.insert(*signature);
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

void iterator_move(Iterator *iterator, Reference *dest, Cursor *cursor) {

	if (!iterator->ctx.empty()) {
		cursor->stack().push_back(dest);
		cursor->stack().push_back(iterator->ctx.front());
		move_operator(cursor);
		cursor->stack().pop_back();
	}
}

void find_defined_symbol(Cursor *cursor, const std::string &symbol) {

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

void find_defined_member(Cursor *cursor, const std::string &symbol) {

	if (cursor->stack().back()->data()->format != Data::fmt_none) {

		SharedReference value = cursor->stack().back();
		cursor->stack().pop_back();

		if (value->data()->format == Data::fmt_object) {

			Object *object = (Object *)value->data();

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

void check_defined(Cursor *cursor) {

	SharedReference value = cursor->stack().back();
	Reference *result = Reference::create<Boolean>();

	((Boolean *)result->data())->value = (value->data()->format != Data::fmt_none);

	cursor->stack().pop_back();
	cursor->stack().push_back(SharedReference::unique(result));
}

void in_find(Cursor *cursor) {

	size_t base = get_base(cursor);

	Reference &rvalue = *cursor->stack().at(base);
	Reference &lvalue = *cursor->stack().at(base - 1);
	Reference *result = Reference::create<Boolean>();

	if (rvalue.data()->format == Data::fmt_object && ((Object *)rvalue.data())->metadata->metatype() == Class::hash) {
		((Boolean *)result->data())->value = ((Hash *)rvalue.data())->values.find(&lvalue) != ((Hash *)rvalue.data())->values.end();
	}
	else if (rvalue.data()->format == Data::fmt_object && ((Object *)rvalue.data())->metadata->metatype() == Class::string) {
		((Boolean *)result->data())->value = ((String *)rvalue.data())->str.find(to_string(lvalue)) != string::npos;
	}
	else {
		Iterator *iterator = Reference::alloc<Iterator>();
		iterator_init(iterator, rvalue);
		for (SharedReference &item : iterator->ctx) {
			cursor->stack().push_back(&lvalue);
			cursor->stack().push_back(item);
			eq_operator(cursor);
			if ((((Boolean *)result->data())->value = to_boolean(cursor, *cursor->stack().back()))) {
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

void in_init(Cursor *cursor) {

	size_t base = get_base(cursor);

	Reference &rvalue = *cursor->stack().at(base);
	Reference &lvalue = *cursor->stack().at(base - 1);
	Reference *result = Reference::create<Iterator>();

	Iterator *iterator = (Iterator *)result->data();
	iterator_init(iterator, rvalue);
	cursor->stack().push_back(SharedReference::unique(result));
	iterator_move(iterator, &lvalue, cursor);
}

void in_next(Cursor *cursor) {

	size_t base = get_base(cursor);

	Reference &rvalue = *cursor->stack().at(base);
	Reference &lvalue = *cursor->stack().at(base - 2);

	Iterator *iterator = (Iterator *)rvalue.data();
	iterator->ctx.pop_front();
	iterator_move(iterator, &lvalue, cursor);
}

void in_check(Cursor *cursor) {

	Reference &rvalue = *cursor->stack().back();
	Reference *result = Reference::create<Boolean>();

	Iterator *iterator = (Iterator *)rvalue.data();
	if (iterator->ctx.empty()) {
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().pop_back();

		((Boolean *)result->data())->value = false;
	}
	else {
		((Boolean *)result->data())->value = true;
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
				return ((Number *)lvalue->data())->value < to_number(cursor, *rvalue);

			case Data::fmt_boolean:
				return ((Boolean *)lvalue->data())->value < to_boolean(cursor, *rvalue);

			case Data::fmt_object:
				switch (((Object *)lvalue->data())->metadata->metatype()) {
				case Class::object:
				case Class::hash:
				case Class::array:
				case Class::iterator:
				case Class::library:
				case Class::libobject:
					error("invalid use of '%s' type as hash key", type_name(*lvalue).c_str());
					break;

				case Class::string:
					return ((String *)lvalue->data())->str < to_string(*rvalue);
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
