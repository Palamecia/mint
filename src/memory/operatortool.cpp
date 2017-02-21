#include "memory/operatortool.h"
#include "memory/memorytool.h"
#include "memory/globaldata.h"
#include "memory/builtin/string.h"
#include "ast/abstractsyntaxtree.h"
#include "scheduler/processor.h"
#include "system/error.h"

#include <math.h>

using namespace std;

bool call_overload(AbstractSyntaxTree *ast, const string &operator_overload, int signature) {

	size_t base = get_base(ast);
	Object *object = (Object *)ast->stack().at(base - signature)->data();
	auto it = object->metadata->members().find(operator_overload);

	if (it == object->metadata->members().end()) {
		return false;
	}

	ast->waitingCalls().push(&object->data[it->second->offset]);
	call_member_operator(ast, signature);
	return true;
}

void move_operator(AbstractSyntaxTree *ast) {

	size_t base = get_base(ast);

	Reference &rvalue = *ast->stack().at(base);
	Reference &lvalue = *ast->stack().at(base - 1);

	if ((lvalue.flags() & Reference::const_ref) && (lvalue.data()->format != Data::fmt_none)) {
		error("invalid modification of constant reference");
	}
	else if (rvalue.flags() & Reference::const_value) {
		lvalue.copy(rvalue);
	}
	else {
		lvalue.move(rvalue);
	}

	ast->stack().pop_back();
}

void copy_operator(AbstractSyntaxTree *ast) {

	size_t base = get_base(ast);

	Reference &rvalue = *ast->stack().at(base);
	Reference &lvalue = *ast->stack().at(base - 1);

	if (lvalue.flags() & Reference::const_value) {
		error("invalid modification of constant value");
	}

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		ast->raise(&lvalue);
		break;
	case Data::fmt_number:
		((Number *)lvalue.data())->value = to_number(ast, rvalue);
		ast->stack().pop_back();
		break;
	case Data::fmt_boolean:
		((Boolean *)lvalue.data())->value = to_boolean(ast, rvalue);
		ast->stack().pop_back();
		break;
	case Data::fmt_function:
		if (rvalue.data()->format != Data::fmt_function) {
			error("invalid conversion from '%s' to '%s'", type_name(rvalue).c_str(), type_name(lvalue).c_str());
		}
		((Function *)lvalue.data())->mapping = ((Function *)rvalue.data())->mapping;
		ast->stack().pop_back();
		break;
	case Data::fmt_object:
		if (!call_overload(ast, ":=", 1)) {
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

void call_operator(AbstractSyntaxTree *ast, int signature) {

	Reference *result = nullptr;
	Reference lvalue = ast->waitingCalls().top().function();
	bool member = ast->waitingCalls().top().isMember();
	ast->waitingCalls().pop();

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
		ast->raise(&lvalue);
		break;
	case Data::fmt_number:
		result = Reference::create<Number>();
		result->copy(lvalue);
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		result->copy(lvalue);
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		result = Reference::create<Data>();
		result->copy(lvalue);
		ast->stack().push_back(SharedReference::unique(result));
	case Data::fmt_function:
		auto it = find_function_signature(ast, ((Function*)lvalue.data())->mapping, signature + (member ? 1 : 0));
		if (it == ((Function*)lvalue.data())->mapping.end()) {
			error("called function doesn't take %d parameter(s)", signature + (member ? 1 : 0));
		}
		if (ast->call(it->second.first, it->second.second)) {
			if (member) {
				Object *object = (Object *)ast->stack().at(get_base(ast) - signature)->data();
				ast->symbols().metadata = object->metadata;
			}
		}
		break;
	}
}

void call_member_operator(AbstractSyntaxTree *ast, int signature) {

	size_t base = get_base(ast);

	Reference *result = nullptr;
	Reference &object = *ast->stack().at(base - signature);
	Reference lvalue = ast->waitingCalls().top().function();
	bool member = ast->waitingCalls().top().isMember();
	bool global = lvalue.flags() & Reference::global;
	ast->waitingCalls().pop();

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
		ast->raise(&lvalue);
		break;
	case Data::fmt_number:
		result = Reference::create<Number>();
		result->copy(lvalue);
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		result->copy(lvalue);
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		result = Reference::create<Data>();
		result->copy(lvalue);
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_function:
		auto it = find_function_signature(ast, ((Function*)lvalue.data())->mapping, signature + (global ? 0 : 1));
		if (it == ((Function*)lvalue.data())->mapping.end()) {
			error("called member doesn't take %d parameter(s)", signature + (global ? 0 : 1));
		}
		if (ast->call(it->second.first, it->second.second)) {
			ast->symbols().metadata = ((Object *)object.data())->metadata;
		}
		break;
	}

	if (global) {
		ast->stack().erase(ast->stack().begin() + (base - signature));
	}
}

void add_operator(AbstractSyntaxTree *ast) {

	size_t base = get_base(ast);

	Reference &rvalue = *ast->stack().at(base);
	Reference &lvalue = *ast->stack().at(base - 1);
	Reference *result = nullptr;

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		ast->raise(&lvalue);
		break;
	case Data::fmt_number:
		result = Reference::create<Number>();
		((Number*)result->data())->value = ((Number*)lvalue.data())->value + to_number(ast, rvalue);
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		((Boolean *)result->data())->value = ((Boolean *)lvalue.data())->value + to_boolean(ast, rvalue);
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(ast, "+", 1)) {
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
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	}
}

void sub_operator(AbstractSyntaxTree *ast) {

	size_t base = get_base(ast);

	Reference &rvalue = *ast->stack().at(base);
	Reference &lvalue = *ast->stack().at(base - 1);
	Reference *result = nullptr;

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		ast->raise(&lvalue);
		break;
	case Data::fmt_number:
		result = Reference::create<Number>();
		((Number*)result->data())->value = ((Number*)lvalue.data())->value - to_number(ast, rvalue);
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		((Boolean *)result->data())->value = ((Boolean *)lvalue.data())->value - to_boolean(ast, rvalue);
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(ast, "-", 1)) {
			error("class '%s' dosen't ovreload operator '-'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '-'", type_name(lvalue).c_str());
		break;
	}
}

void mul_operator(AbstractSyntaxTree *ast) {

	size_t base = get_base(ast);

	Reference &rvalue = *ast->stack().at(base);
	Reference &lvalue = *ast->stack().at(base - 1);
	Reference *result = nullptr;

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		ast->raise(&lvalue);
		break;
	case Data::fmt_number:
		result = Reference::create<Number>();
		((Number*)result->data())->value = ((Number*)lvalue.data())->value * to_number(ast, rvalue);
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		((Boolean *)result->data())->value = ((Boolean *)lvalue.data())->value * to_boolean(ast, rvalue);
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(ast, "*", 1)) {
			error("class '%s' dosen't ovreload operator '*'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '*'", type_name(lvalue).c_str());
	}
}

void div_operator(AbstractSyntaxTree *ast) {

	size_t base = get_base(ast);

	Reference &rvalue = *ast->stack().at(base);
	Reference &lvalue = *ast->stack().at(base - 1);
	Reference *result = nullptr;

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		ast->raise(&lvalue);
		break;
	case Data::fmt_number:
		result = Reference::create<Number>();
		((Number*)result->data())->value = ((Number*)lvalue.data())->value / to_number(ast, rvalue);
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		((Boolean *)result->data())->value = ((Boolean *)lvalue.data())->value / to_boolean(ast, rvalue);
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(ast, "/", 1)) {
			error("class '%s' dosen't ovreload operator '/'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '/'", type_name(lvalue).c_str());
		break;
	}
}

void pow_operator(AbstractSyntaxTree *ast) {

	size_t base = get_base(ast);

	Reference &rvalue = *ast->stack().at(base);
	Reference &lvalue = *ast->stack().at(base - 1);
	Reference *result = nullptr;

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		ast->raise(&lvalue);
		break;
	case Data::fmt_number:
		result = Reference::create<Number>();
		((Number*)result->data())->value = pow(((Number*)lvalue.data())->value, to_number(ast, rvalue));
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(ast, "**", 1)) {
			error("class '%s' dosen't ovreload operator '**'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_boolean:
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '**'", type_name(lvalue).c_str());
		break;
	}
}

void mod_operator(AbstractSyntaxTree *ast) {

	size_t base = get_base(ast);

	Reference &rvalue = *ast->stack().at(base);
	Reference &lvalue = *ast->stack().at(base - 1);
	Reference *result = nullptr;

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		ast->raise(&lvalue);
		break;
	case Data::fmt_number:
		result = Reference::create<Number>();
		((Number*)result->data())->value = (long)((Number*)lvalue.data())->value % (long)to_number(ast, rvalue);
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(ast, "%", 1)) {
			error("class '%s' dosen't ovreload operator '%%'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_boolean:
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '%%'", type_name(lvalue).c_str());
		break;
	}
}

void is_operator(AbstractSyntaxTree *ast) {

	size_t base = get_base(ast);

	Reference &rvalue = *ast->stack().at(base);
	Reference &lvalue = *ast->stack().at(base - 1);

	Reference *result = Reference::create<Boolean>();
	((Boolean *)result->data())->value = lvalue.data() == rvalue.data();
	ast->stack().pop_back();
	ast->stack().pop_back();
	ast->stack().push_back(SharedReference::unique(result));
}

void eq_operator(AbstractSyntaxTree *ast) {

	size_t base = get_base(ast);

	Reference &rvalue = *ast->stack().at(base);
	Reference &lvalue = *ast->stack().at(base - 1);
	Reference *result = nullptr;

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		result = Reference::create<Boolean>();
		((Boolean *)result->data())->value = (rvalue.data()->format == Data::fmt_none);
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_null:
		result = Reference::create<Boolean>();
		((Boolean *)result->data())->value = (rvalue.data()->format == Data::fmt_null);
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_number:
		result = Reference::create<Boolean>();
		switch (rvalue.data()->format) {
		case Data::fmt_none:
		case Data::fmt_null:
			((Boolean *)result->data())->value = false;
			break;
		default:
			((Boolean *)result->data())->value = ((Number *)lvalue.data())->value == to_number(ast, rvalue);
		}
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		switch (rvalue.data()->format) {
		case Data::fmt_none:
		case Data::fmt_null:
			((Boolean *)result->data())->value = false;
			break;
		default:
			((Boolean *)result->data())->value = ((Boolean *)lvalue.data())->value == to_boolean(ast, rvalue);
		}
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(ast, "==", 1)) {
			result = Reference::create<Boolean>();
			switch (rvalue.data()->format) {
			case Data::fmt_none:
			case Data::fmt_null:
				((Boolean *)result->data())->value = false;
				break;
			default:
				error("class '%s' dosen't ovreload operator '=='(1)", type_name(lvalue).c_str());
			}
			ast->stack().pop_back();
			ast->stack().pop_back();
			ast->stack().push_back(SharedReference::unique(result));
		}
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '=='", type_name(lvalue).c_str());
		break;
	}
}

void ne_operator(AbstractSyntaxTree *ast) {

	size_t base = get_base(ast);

	Reference &rvalue = *ast->stack().at(base);
	Reference &lvalue = *ast->stack().at(base - 1);
	Reference *result = nullptr;

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		result = Reference::create<Boolean>();
		((Boolean *)result->data())->value = (rvalue.data()->format != Data::fmt_none);
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_null:
		result = Reference::create<Boolean>();
		((Boolean *)result->data())->value = (rvalue.data()->format != Data::fmt_null);
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_number:
		result = Reference::create<Boolean>();
		switch (rvalue.data()->format) {
		case Data::fmt_none:
		case Data::fmt_null:
			((Boolean *)result->data())->value = true;
			break;
		default:
			((Boolean *)result->data())->value = ((Number*)lvalue.data())->value != to_number(ast, rvalue);
		}
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		switch (rvalue.data()->format) {
		case Data::fmt_none:
		case Data::fmt_null:
			((Boolean *)result->data())->value = true;
			break;
		default:
			((Boolean *)result->data())->value = ((Boolean *)lvalue.data())->value != to_boolean(ast, rvalue);
		}
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(ast, "!=", 1)) {
			result = Reference::create<Boolean>();
			switch (rvalue.data()->format) {
			case Data::fmt_none:
			case Data::fmt_null:
				((Boolean *)result->data())->value = true;
				break;
			default:
				error("class '%s' dosen't ovreload operator '!='(1)", type_name(lvalue).c_str());
			}
			ast->stack().pop_back();
			ast->stack().pop_back();
			ast->stack().push_back(SharedReference::unique(result));
		}
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '!='", type_name(lvalue).c_str());
		break;
	}
}

void lt_operator(AbstractSyntaxTree *ast) {

	size_t base = get_base(ast);

	Reference &rvalue = *ast->stack().at(base);
	Reference &lvalue = *ast->stack().at(base - 1);
	Reference *result = nullptr;

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		ast->raise(&lvalue);
		break;
	case Data::fmt_number:
		result = Reference::create<Boolean>();
		((Boolean *)result->data())->value = ((Number *)lvalue.data())->value < to_number(ast, rvalue);
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		((Boolean *)result->data())->value = ((Boolean *)lvalue.data())->value < to_boolean(ast, rvalue);
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(ast, "<", 1)) {
			error("class '%s' dosen't ovreload operator '<'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '<'", type_name(lvalue).c_str());
		break;
	}
}

void gt_operator(AbstractSyntaxTree *ast) {

	size_t base = get_base(ast);

	Reference &rvalue = *ast->stack().at(base);
	Reference &lvalue = *ast->stack().at(base - 1);
	Reference *result = nullptr;

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		ast->raise(&lvalue);
		break;
	case Data::fmt_number:
		result = Reference::create<Boolean>();
		((Boolean *)result->data())->value = ((Number *)lvalue.data())->value > to_number(ast, rvalue);
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		((Boolean *)result->data())->value = ((Boolean *)lvalue.data())->value > to_boolean(ast, rvalue);
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(ast, ">", 1)) {
			error("class '%s' dosen't ovreload operator '>'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '>'", type_name(lvalue).c_str());
		break;
	}
}

void le_operator(AbstractSyntaxTree *ast) {

	size_t base = get_base(ast);

	Reference &rvalue = *ast->stack().at(base);
	Reference &lvalue = *ast->stack().at(base - 1);
	Reference *result = nullptr;

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		ast->raise(&lvalue);
		break;
	case Data::fmt_number:
		result = Reference::create<Number>();
		((Boolean *)result->data())->value = ((Number *)lvalue.data())->value <= to_number(ast, rvalue);
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		((Boolean *)result->data())->value = ((Boolean *)lvalue.data())->value <= to_boolean(ast, rvalue);
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(ast, "<=", 1)) {
			error("class '%s' dosen't ovreload operator '<='(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '<='", type_name(lvalue).c_str());
		break;
	}
}

void ge_operator(AbstractSyntaxTree *ast) {

	size_t base = get_base(ast);

	Reference &rvalue = *ast->stack().at(base);
	Reference &lvalue = *ast->stack().at(base - 1);
	Reference *result = nullptr;

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		ast->raise(&lvalue);
		break;
	case Data::fmt_number:
		result = Reference::create<Boolean>();
		((Boolean *)result->data())->value = ((Number *)lvalue.data())->value >= to_number(ast, rvalue);
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		((Boolean *)result->data())->value = ((Boolean *)lvalue.data())->value >= to_boolean(ast, rvalue);
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(ast, ">=", 1)) {
			error("class '%s' dosen't ovreload operator '>='(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '>='", type_name(lvalue).c_str());
		break;
	}
}

void and_operator(AbstractSyntaxTree *ast) {

	size_t base = get_base(ast);

	Reference &rvalue = *ast->stack().at(base);
	Reference &lvalue = *ast->stack().at(base - 1);
	Reference *result = nullptr;

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		ast->raise(&lvalue);
		break;
	case Data::fmt_number:
		result = Reference::create<Boolean>();
		((Boolean *)result->data())->value = ((Number *)lvalue.data())->value && to_number(ast, rvalue);
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		((Boolean *)result->data())->value = ((Boolean *)lvalue.data())->value && to_boolean(ast, rvalue);
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(ast, "&&", 1)) {
			error("class '%s' dosen't ovreload operator '&&'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '&&'", type_name(lvalue).c_str());
		break;
	}
}

void or_operator(AbstractSyntaxTree *ast) {

	size_t base = get_base(ast);

	Reference &rvalue = *ast->stack().at(base);
	Reference &lvalue = *ast->stack().at(base - 1);
	Reference *result = nullptr;

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		ast->raise(&lvalue);
		break;
	case Data::fmt_number:
		result = Reference::create<Boolean>();
		((Boolean *)result->data())->value = ((Number *)lvalue.data())->value || to_number(ast, rvalue);
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		((Boolean *)result->data())->value = ((Boolean *)lvalue.data())->value || to_boolean(ast, rvalue);
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(ast, "||", 1)) {
			error("class '%s' dosen't ovreload operator '||'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '||'", type_name(lvalue).c_str());
		break;
	}
}

void band_operator(AbstractSyntaxTree *ast) {

	size_t base = get_base(ast);

	Reference &rvalue = *ast->stack().at(base);
	Reference &lvalue = *ast->stack().at(base - 1);
	Reference *result = nullptr;

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		ast->raise(&lvalue);
		break;
	case Data::fmt_number:
		result = Reference::create<Number>();
		((Number *)result->data())->value = (long)((Number *)lvalue.data())->value & (long)to_number(ast, rvalue);
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		((Boolean *)result->data())->value = ((Boolean *)lvalue.data())->value & to_boolean(ast, rvalue);
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(ast, "&", 1)) {
			error("class '%s' dosen't ovreload operator '&'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '&'", type_name(lvalue).c_str());
		break;
	}
}

void bor_operator(AbstractSyntaxTree *ast) {

	size_t base = get_base(ast);

	Reference &rvalue = *ast->stack().at(base);
	Reference &lvalue = *ast->stack().at(base - 1);
	Reference *result = nullptr;

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		ast->raise(&lvalue);
		break;
	case Data::fmt_number:
		result = Reference::create<Number>();
		((Number *)result->data())->value = (long)((Number *)lvalue.data())->value | (long)to_number(ast, rvalue);
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		((Boolean *)result->data())->value = ((Boolean *)lvalue.data())->value | to_boolean(ast, rvalue);
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(ast, "|", 1)) {
			error("class '%s' dosen't ovreload operator '|'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '|'", type_name(lvalue).c_str());
		break;
	}
}

void xor_operator(AbstractSyntaxTree *ast) {

	size_t base = get_base(ast);

	Reference &rvalue = *ast->stack().at(base);
	Reference &lvalue = *ast->stack().at(base - 1);
	Reference *result = nullptr;

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		ast->raise(&lvalue);
		break;
	case Data::fmt_number:
		result = Reference::create<Number>();
		((Number *)result->data())->value = (long)((Number *)lvalue.data())->value ^ (long)to_number(ast, rvalue);
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		((Boolean *)result->data())->value = (long)((Boolean *)lvalue.data())->value ^ to_boolean(ast, rvalue);
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(ast, "^", 1)) {
			error("class '%s' dosen't ovreload operator '^'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '^'", type_name(lvalue).c_str());
		break;
	}
}

void inc_operator(AbstractSyntaxTree *ast) {

	Reference &value = *ast->stack().back();
	Reference *result;

	switch (value.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		ast->raise(&value);
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
		if (!call_overload(ast, "++", 0)) {
			error("class '%s' dosen't ovreload operator '++'(0)", type_name(value).c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '++'", type_name(value).c_str());
		break;
	}
}

void dec_operator(AbstractSyntaxTree *ast) {

	Reference &value = *ast->stack().back();
	Reference *result;

	switch (value.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		ast->raise(&value);
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
		if (!call_overload(ast, "--", 0)) {
			error("class '%s' dosen't ovreload operator '--'(0)", type_name(value).c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '--'", type_name(value).c_str());
		break;
	}
}

void not_operator(AbstractSyntaxTree *ast) {

	Reference &value = *ast->stack().back();
	Reference *result = Reference::create<Boolean>();

	switch (value.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		ast->raise(&value);
		break;
	case Data::fmt_number:
		((Boolean *)result->data())->value = !((Number *)value.data())->value;
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		((Boolean *)result->data())->value = !((Boolean *)value.data())->value;
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(ast, "!", 0)) {
			error("class '%s' dosen't ovreload operator '!'(0)", type_name(value).c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '!'", type_name(value).c_str());
		break;
	}
}

void compl_operator(AbstractSyntaxTree *ast) {

	Reference &value = *ast->stack().back();
	Reference *result;

	switch (value.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		ast->raise(&value);
		break;
	case Data::fmt_number:
		result = Reference::create<Number>();
		((Number *)result->data())->value = ~((long)((Number *)value.data())->value);
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		((Boolean *)result->data())->value = ~(((Boolean *)value.data())->value);
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(ast, "~", 0)) {
			error("class '%s' dosen't ovreload operator '~'(0)", type_name(value).c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '~'", type_name(value).c_str());
		break;
	}
}

void pos_operator(AbstractSyntaxTree *ast) {

	Reference &value = *ast->stack().back();
	Reference *result;

	switch (value.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		ast->raise(&value);
		break;
	case Data::fmt_number:
		result = Reference::create<Number>();
		((Number *)result->data())->value = +(((Number *)value.data())->value);
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		((Boolean *)result->data())->value = +(((Boolean *)value.data())->value);
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(ast, "+", 0)) {
			error("class '%s' dosen't ovreload operator '+'(0)", type_name(value).c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '+'", type_name(value).c_str());
		break;
	}
}

void neg_operator(AbstractSyntaxTree *ast) {

	Reference &value = *ast->stack().back();
	Reference *result;

	switch (value.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		ast->raise(&value);
		break;
	case Data::fmt_number:
		result = Reference::create<Number>();
		((Number *)result->data())->value = -(((Number *)value.data())->value);
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		((Boolean *)result->data())->value = -(((Boolean *)value.data())->value);
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(ast, "-", 0)) {
			error("class '%s' dosen't ovreload operator '-'(0)", type_name(value).c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '-'", type_name(value).c_str());
		break;
	}
}

void shift_left_operator(AbstractSyntaxTree *ast) {

	size_t base = get_base(ast);

	Reference &rvalue = *ast->stack().at(base);
	Reference &lvalue = *ast->stack().at(base - 1);
	Reference *result = nullptr;

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		ast->raise(&lvalue);
		break;
	case Data::fmt_number:
		result = Reference::create<Number>();
		((Number *)result->data())->value = (long)((Number *)lvalue.data())->value << (long)to_number(ast, rvalue);
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		((Boolean *)result->data())->value = ((Boolean *)lvalue.data())->value << (long)to_number(ast, rvalue);
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(ast, "<<", 1)) {
			error("class '%s' dosen't ovreload operator '<<'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '<<'", type_name(lvalue).c_str());
		break;
	}
}

void shift_right_operator(AbstractSyntaxTree *ast) {

	size_t base = get_base(ast);

	Reference &rvalue = *ast->stack().at(base);
	Reference &lvalue = *ast->stack().at(base - 1);
	Reference *result = nullptr;

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		ast->raise(&lvalue);
		break;
	case Data::fmt_number:
		result = Reference::create<Number>();
		((Number *)result->data())->value = (long)((Number *)lvalue.data())->value >> (long)to_number(ast, rvalue);
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		result = Reference::create<Boolean>();
		((Boolean *)result->data())->value = ((Boolean *)lvalue.data())->value >> (long)to_number(ast, rvalue);
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(ast, ">>", 1)) {
			error("class '%s' dosen't ovreload operator '>>'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '>>'", type_name(lvalue).c_str());
		break;
	}
}

void inclusive_range_operator(AbstractSyntaxTree *ast) {

	size_t base = get_base(ast);

	Reference &rvalue = *ast->stack().at(base);
	Reference &lvalue = *ast->stack().at(base - 1);
	Reference *result = nullptr;

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		ast->raise(&lvalue);
		break;
	case Data::fmt_number:
		result = Reference::create<Iterator>();
		for (double begin = ((Number *)lvalue.data())->value, end = to_number(ast, rvalue), i = min(begin, end); i <= max(begin, end); ++i) {
			Reference *item = Reference::create<Number>();
			((Number *)item->data())->value = i;
			if (begin < end) {
				iterator_insert((Iterator *)result->data(), SharedReference::unique(item));
			}
			else {
				iterator_add((Iterator *)result->data(), SharedReference::unique(item));
			}
		}
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(ast, "..", 1)) {
			error("class '%s' dosen't ovreload operator '..'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_boolean:
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '..'", type_name(lvalue).c_str());
		break;
	}
}

void exclusive_range_operator(AbstractSyntaxTree *ast) {

	size_t base = get_base(ast);

	Reference &rvalue = *ast->stack().at(base);
	Reference &lvalue = *ast->stack().at(base - 1);
	Reference *result = nullptr;

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		ast->raise(&lvalue);
		break;
	case Data::fmt_number:
		result = Reference::create<Iterator>();
		for (double begin = ((Number *)lvalue.data())->value, end = to_number(ast, rvalue), i = min(begin, end); i < max(begin, end); ++i) {
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
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(ast, "...", 1)) {
			error("class '%s' dosen't ovreload operator '...'(1)", type_name(lvalue).c_str());
		}
		break;
	case Data::fmt_boolean:
	case Data::fmt_function:
		error("invalid use of '%s' type with operator '...'", type_name(lvalue).c_str());
		break;
	}
}

void typeof_operator(AbstractSyntaxTree *ast) {

	Reference &value = *ast->stack().back();
	Reference *result = Reference::create<String>();

	((String *)result->data())->str = type_name(value);

	ast->stack().pop_back();
	ast->stack().push_back(SharedReference::unique(result));
}

void membersof_operator(AbstractSyntaxTree *ast) {

	Reference &value = *ast->stack().back();
	Reference *result = Reference::create<Array>();

	if (value.data()->format == Data::fmt_object) {

		Object *object = (Object *)value.data();
		Array *array = (Array *)result->data();

		array->values.reserve(object->metadata->members().size());

		for (auto member : object->metadata->members()) {

			if ((member.second->value.flags() & Reference::user_hiden) && (object->metadata != ast->symbols().metadata)) {
				continue;
			}

			if ((member.second->value.flags() & Reference::child_hiden) && (member.second->owner != ast->symbols().metadata)) {
				continue;
			}

			String *name = Reference::alloc<String>();
			name->construct();
			name->str = member.first;
			array_append(array, SharedReference::unique(new Reference(Reference::standard, name)));
		}
	}

	ast->stack().pop_back();
	ast->stack().push_back(SharedReference::unique(result));
}

void subscript_operator(AbstractSyntaxTree *ast) {

	size_t base = get_base(ast);

	Reference &rvalue = *ast->stack().at(base);
	Reference &lvalue = *ast->stack().at(base - 1);
	Reference *result = nullptr;

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		ast->raise(&lvalue);
		break;
	case Data::fmt_number:
		result = Reference::create<Number>();
		((Number*)result->data())->value = ((long)(((Number*)lvalue.data())->value / pow(10, to_number(ast, rvalue))) % 10);
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_boolean:
		error("invalid use of '%s' type with operator '[]'", type_name(lvalue).c_str());
		break;
	case Data::fmt_object:
		if (!call_overload(ast, "[]", 1)) {
			error("class '%s' dosen't ovreload operator '[]'(1)", ((Object *)lvalue.data())->metadata->name().c_str());
		}
		break;
	case Data::fmt_function:
		auto signature = ((Function *)lvalue.data())->mapping.find(to_number(ast, rvalue));
		if (signature != ((Function *)lvalue.data())->mapping.end()) {
			result = Reference::create<Function>();
			((Function *)result->data())->mapping.insert(*signature);
		}
		else {
			result = Reference::create<None>();
		}
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	}
}

void iterator_move(Iterator *iterator, Reference *dest, AbstractSyntaxTree *ast) {

	if (!iterator->ctx.empty()) {
		ast->stack().push_back(dest);
		ast->stack().push_back(iterator->ctx.front());
		move_operator(ast);
		ast->stack().pop_back();
	}
}

void find_defined_symbol(AbstractSyntaxTree *ast, const std::string &symbol) {

	if (Class *desc = GlobalData::instance().getClass(symbol)) {
		Object *object = desc->makeInstance();
		object->construct();
		ast->stack().push_back(SharedReference::unique(new Reference(Reference::standard, object)));
	}
	else {

		auto it = GlobalData::instance().symbols().find(symbol);
		if (it != GlobalData::instance().symbols().end()) {
			ast->stack().push_back(&it->second);
		}
		else {

			it = ast->symbols().find(symbol);
			if (it != ast->symbols().end()) {
				ast->stack().push_back(&it->second);
			}
			else {
				ast->stack().push_back(SharedReference::unique(Reference::create<None>()));
			}
		}
	}

}

void find_defined_member(AbstractSyntaxTree *ast, const std::string &symbol) {

	if (ast->stack().back()->data()->format != Data::fmt_none) {

		SharedReference value = ast->stack().back();
		ast->stack().pop_back();

		if (value->data()->format == Data::fmt_object) {

			Object *object = (Object *)value->data();

			if (Class *desc = object->metadata->globals().getClass(symbol)) {
				Object *object = desc->makeInstance();
				object->construct();
				ast->stack().push_back(SharedReference::unique(new Reference(Reference::standard, object)));
			}
			else {

				auto it_global = object->metadata->globals().members().find(symbol);
				if (it_global != object->metadata->globals().members().end()) {
					ast->stack().push_back(&it_global->second->value);
				}
				else {

					auto it_member = object->metadata->members().find(symbol);
					if (it_member != object->metadata->members().end()) {
						ast->stack().push_back(&object->data[it_member->second->offset]);
					}
					else {
						ast->stack().push_back(SharedReference::unique(Reference::create<None>()));
					}
				}
			}
		}
		else {
			ast->stack().push_back(SharedReference::unique(Reference::create<None>()));
		}
	}
}

void check_defined(AbstractSyntaxTree *ast) {

	SharedReference value = ast->stack().back();
	Reference *result = Reference::create<Boolean>();

	((Boolean *)result->data())->value = (value->data()->format != Data::fmt_none);

	ast->stack().pop_back();
	ast->stack().push_back(SharedReference::unique(result));
}

void in_find(AbstractSyntaxTree *ast) {

	size_t base = get_base(ast);

	Reference &rvalue = *ast->stack().at(base);
	Reference &lvalue = *ast->stack().at(base - 1);
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
			ast->stack().push_back(&lvalue);
			ast->stack().push_back(item);
			eq_operator(ast);
			if ((((Boolean *)result->data())->value = to_boolean(ast, *ast->stack().back()))) {
				ast->stack().pop_back();
				break;
			}
			else {
				ast->stack().pop_back();
			}
		}
	}

	ast->stack().pop_back();
	ast->stack().pop_back();
	ast->stack().push_back(SharedReference::unique(result));
}

void in_init(AbstractSyntaxTree *ast) {

	size_t base = get_base(ast);

	Reference &rvalue = *ast->stack().at(base);
	Reference &lvalue = *ast->stack().at(base - 1);
	Reference *result = Reference::create<Iterator>();

	Iterator *iterator = (Iterator *)result->data();
	iterator_init(iterator, rvalue);
	ast->stack().push_back(SharedReference::unique(result));
	iterator_move(iterator, &lvalue, ast);
}

void in_next(AbstractSyntaxTree *ast) {

	size_t base = get_base(ast);

	Reference &rvalue = *ast->stack().at(base);
	Reference &lvalue = *ast->stack().at(base - 2);

	Iterator *iterator = (Iterator *)rvalue.data();
	iterator->ctx.pop_front();
	iterator_move(iterator, &lvalue, ast);
}

void in_check(AbstractSyntaxTree *ast) {

	Reference &rvalue = *ast->stack().back();
	Reference *result = Reference::create<Boolean>();

	Iterator *iterator = (Iterator *)rvalue.data();
	if (iterator->ctx.empty()) {
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().pop_back();

		((Boolean *)result->data())->value = false;
	}
	else {
		((Boolean *)result->data())->value = true;
	}

	ast->stack().push_back(SharedReference::unique(result));
}

bool Hash::compare::operator ()(const SharedReference &a, const SharedReference &b) const {

	AbstractSyntaxTree ast;

	ast.stack().push_back(SharedReference::unique(new Reference(*a)));
	ast.stack().push_back(SharedReference::unique(new Reference(*b)));

	AbstractSyntaxTree::CallHandler handler = ast.getCallHandler();
	lt_operator(&ast);
	while (ast.callInProgress(handler)) {
		run_step(&ast);
	}

	return to_boolean(&ast, *ast.stack().back());
}
