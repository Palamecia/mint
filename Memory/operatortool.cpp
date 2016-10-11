#include "Memory/operatortool.h"
#include "Memory/memorytool.h"
#include "Memory/globaldata.h"
#include "Memory/object.h"
#include "Memory/class.h"
#include "AbstractSyntaxTree/abstractsyntaxtree.h"
#include "Scheduler/processor.h"
#include "System/error.h"

#include <math.h>

using namespace std;

bool call_overload(AbstractSynatxTree *ast, const string &operator_overload, int signature) {

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

void move_operator(AbstractSynatxTree *ast) {

	size_t base = get_base(ast);

	Reference &rvalue = *ast->stack().at(base);
	Reference &lvalue = *ast->stack().at(base - 1);

	if ((lvalue.data()->format == Data::fmt_function) && (rvalue.data()->format == Data::fmt_function)) {
		for (auto signature : ((Function*)rvalue.data())->mapping) {
			((Function*)lvalue.data())->mapping[signature.first] = signature.second;
		}
	}
	else if ((lvalue.flags() & Reference::const_ref) && (lvalue.data()->format != Data::fmt_none)) {
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

void copy_operator(AbstractSynatxTree *ast) {

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
	case Data::fmt_function:
		/// \todo ((Function *)lvalue.data())->mapping = to_function(ast, rvalue);
		ast->stack().pop_back();
		break;
	case Data::fmt_object:
		if (!call_overload(ast, ":=", 1)) {
			/// \todo
		}
		break;
	}
}

void call_operator(AbstractSynatxTree *ast, int signature) {

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

void call_member_operator(AbstractSynatxTree *ast, int signature) {

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

void add_operator(AbstractSynatxTree *ast) {

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
	case Data::fmt_object:
		if (!call_overload(ast, "+", 1)) {
			error("class '%s' dosen't ovreload operator '+'(1)", ((Object *)lvalue.data())->metadata->name().c_str());
		}
		break;
	case Data::fmt_function:
		result = Reference::create<Function>();
		if (rvalue.data()->format != Data::fmt_function) {
			error("invalid use of operator '+' with function and not function types");
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

void sub_operator(AbstractSynatxTree *ast) {

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
	case Data::fmt_object:
		if (!call_overload(ast, "-", 1)) {
			error("class '%s' dosen't ovreload operator '-'(1)", ((Object *)lvalue.data())->metadata->name().c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of function type with operator '-'");
		break;
	}
}

void mul_operator(AbstractSynatxTree *ast) {

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
	case Data::fmt_object:
		if (!call_overload(ast, "*", 1)) {
			error("class '%s' dosen't ovreload operator '*'(1)", ((Object *)lvalue.data())->metadata->name().c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of function type with operator '*'");
	}
}

void div_operator(AbstractSynatxTree *ast) {

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
	case Data::fmt_object:
		if (!call_overload(ast, "/", 1)) {
			error("class '%s' dosen't ovreload operator '/'(1)", ((Object *)lvalue.data())->metadata->name().c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of function type with operator '/'");
		break;
	}
}

void pow_operator(AbstractSynatxTree *ast) {

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
			error("class '%s' dosen't ovreload operator '**'(1)", ((Object *)lvalue.data())->metadata->name().c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of function type with operator '**'");
		break;
	}
}

void mod_operator(AbstractSynatxTree *ast) {

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
			error("class '%s' dosen't ovreload operator '%'(1)", ((Object *)lvalue.data())->metadata->name().c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of function type with operator '%'");
		break;
	}
}

void is_operator(AbstractSynatxTree *ast) {

	size_t base = get_base(ast);

	Reference &rvalue = *ast->stack().at(base);
	Reference &lvalue = *ast->stack().at(base - 1);

	Reference *result = Reference::create<Number>();
	((Number*)result->data())->value = lvalue.data() == rvalue.data();
	ast->stack().pop_back();
	ast->stack().pop_back();
	ast->stack().push_back(SharedReference::unique(result));
}

void eq_operator(AbstractSynatxTree *ast) {

	size_t base = get_base(ast);

	Reference &rvalue = *ast->stack().at(base);
	Reference &lvalue = *ast->stack().at(base - 1);
	Reference *result = nullptr;

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		result = Reference::create<Number>();
		((Number*)result->data())->value = (rvalue.data()->format == Data::fmt_none);
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_null:
		result = Reference::create<Number>();
		((Number*)result->data())->value = (rvalue.data()->format == Data::fmt_null);
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_number:
		result = Reference::create<Number>();
		switch (rvalue.data()->format) {
		case Data::fmt_none:
		case Data::fmt_null:
			((Number*)result->data())->value = false;
			break;
		default:
			((Number*)result->data())->value = ((Number*)lvalue.data())->value == to_number(ast, rvalue);
		}
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(ast, "==", 1)) {
			result = Reference::create<Number>();
			switch (rvalue.data()->format) {
			case Data::fmt_none:
			case Data::fmt_null:
				((Number*)result->data())->value = false;
				break;
			default:
				error("class '%s' dosen't ovreload operator '=='(1)", ((Object *)lvalue.data())->metadata->name().c_str());
			}
			ast->stack().pop_back();
			ast->stack().pop_back();
			ast->stack().push_back(SharedReference::unique(result));
		}
		break;
	case Data::fmt_function:
		error("invalid use of function type with operator '=='");
		break;
	}
}

void ne_operator(AbstractSynatxTree *ast) {

	size_t base = get_base(ast);

	Reference &rvalue = *ast->stack().at(base);
	Reference &lvalue = *ast->stack().at(base - 1);
	Reference *result = nullptr;

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		result = Reference::create<Number>();
		((Number*)result->data())->value = (rvalue.data()->format != Data::fmt_none);
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_null:
		result = Reference::create<Number>();
		((Number*)result->data())->value = (rvalue.data()->format != Data::fmt_null);
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_number:
		result = Reference::create<Number>();
		switch (rvalue.data()->format) {
		case Data::fmt_none:
		case Data::fmt_null:
			((Number*)result->data())->value = true;
			break;
		default:
			((Number*)result->data())->value = ((Number*)lvalue.data())->value != to_number(ast, rvalue);
		}
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(ast, "!=", 1)) {
			result = Reference::create<Number>();
			switch (rvalue.data()->format) {
			case Data::fmt_none:
			case Data::fmt_null:
				((Number*)result->data())->value = true;
				break;
			default:
				error("class '%s' dosen't ovreload operator '!='(1)", ((Object *)lvalue.data())->metadata->name().c_str());
			}
			ast->stack().pop_back();
			ast->stack().pop_back();
			ast->stack().push_back(SharedReference::unique(result));
		}
		break;
	case Data::fmt_function:
		error("invalid use of function type with operator '!='");
		break;
	}
}

void lt_operator(AbstractSynatxTree *ast) {

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
		((Number*)result->data())->value = ((Number*)lvalue.data())->value < to_number(ast, rvalue);
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(ast, "<", 1)) {
			error("class '%s' dosen't ovreload operator '<'(1)", ((Object *)lvalue.data())->metadata->name().c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of function type with operator '<'");
		break;
	}
}

void gt_operator(AbstractSynatxTree *ast) {

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
		((Number*)result->data())->value = ((Number*)lvalue.data())->value > to_number(ast, rvalue);
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(ast, ">", 1)) {
			error("class '%s' dosen't ovreload operator '>'(1)", ((Object *)lvalue.data())->metadata->name().c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of function type with operator '>'");
		break;
	}
}

void le_operator(AbstractSynatxTree *ast) {

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
		((Number*)result->data())->value = ((Number*)lvalue.data())->value <= to_number(ast, rvalue);
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(ast, "<=", 1)) {
			error("class '%s' dosen't ovreload operator '<='(1)", ((Object *)lvalue.data())->metadata->name().c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of function type with operator '<='");
		break;
	}
}

void ge_operator(AbstractSynatxTree *ast) {

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
		((Number*)result->data())->value = ((Number*)lvalue.data())->value >= to_number(ast, rvalue);
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(ast, ">=", 1)) {
			error("class '%s' dosen't ovreload operator '>='(1)", ((Object *)lvalue.data())->metadata->name().c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of function type with operator '>='");
		break;
	}
}

void and_operator(AbstractSynatxTree *ast) {

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
		((Number*)result->data())->value = ((Number*)lvalue.data())->value && to_number(ast, rvalue);
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(ast, "&&", 1)) {
			error("class '%s' dosen't ovreload operator '&&'(1)", ((Object *)lvalue.data())->metadata->name().c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of function type with operator '&&'");
		break;
	}
}

void or_operator(AbstractSynatxTree *ast) {

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
		((Number*)result->data())->value = ((Number*)lvalue.data())->value || to_number(ast, rvalue);
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(ast, "||", 1)) {
			error("class '%s' dosen't ovreload operator '||'(1)", ((Object *)lvalue.data())->metadata->name().c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of function type with operator '||'");
		break;
	}
}

void xor_operator(AbstractSynatxTree *ast) {

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
		((Number*)result->data())->value = (long)((Number*)lvalue.data())->value ^ (long)to_number(ast, rvalue);
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(ast, "^", 1)) {
			error("class '%s' dosen't ovreload operator '^'(1)", ((Object *)lvalue.data())->metadata->name().c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of function type with operator '^'");
		break;
	}
}

void inc_operator(AbstractSynatxTree *ast) {

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
		((Number *)result->data())->value = ((Number*)value.data())->value + 1;
		value.move(*SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(ast, "++", 0)) {
			error("class '%s' dosen't ovreload operator '++'(0)", ((Object *)value.data())->metadata->name().c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of function type with operator '++'");
		break;
	}
}

void dec_operator(AbstractSynatxTree *ast) {

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
		((Number *)result->data())->value = ((Number*)value.data())->value - 1;
		value.move(*SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(ast, "--", 0)) {
			error("class '%s' dosen't ovreload operator '--'(0)", ((Object *)value.data())->metadata->name().c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of function type with operator '--'");
		break;
	}
}

void not_operator(AbstractSynatxTree *ast) {

	Reference &value = *ast->stack().back();
	Reference *result = Reference::create<Number>();

	switch (value.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		ast->raise(&value);
		break;
	case Data::fmt_number:
		((Number*)result->data())->value = !((Number*)value.data())->value;
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(ast, "!", 0)) {
			error("class '%s' dosen't ovreload operator '!'(0)", ((Object *)value.data())->metadata->name().c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of function type with operator '!'");
		break;
	}
}

void compl_operator(AbstractSynatxTree *ast) {

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
		((Number*)result->data())->value = ~((long)((Number*)value.data())->value);
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(ast, "~", 0)) {
			error("class '%s' dosen't ovreload operator '~'(0)", ((Object *)value.data())->metadata->name().c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of function type with operator '~'");
		break;
	}
}

void pos_operator(AbstractSynatxTree *ast) {

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
		((Number*)result->data())->value = +(((Number*)value.data())->value);
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(ast, "+", 0)) {
			error("class '%s' dosen't ovreload operator '+'(0)", ((Object *)value.data())->metadata->name().c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of function type with operator '+'");
		break;
	}
}

void neg_operator(AbstractSynatxTree *ast) {

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
		((Number*)result->data())->value = -(((Number*)value.data())->value);
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(ast, "-", 0)) {
			error("class '%s' dosen't ovreload operator '-'(0)", ((Object *)value.data())->metadata->name().c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of function type with operator '-'");
		break;
	}
}

void shift_left_operator(AbstractSynatxTree *ast) {

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
		((Number*)result->data())->value = (long)((Number*)lvalue.data())->value << (long)to_number(ast, rvalue);
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(ast, "<<", 1)) {
			error("class '%s' dosen't ovreload operator '<<'(1)", ((Object *)lvalue.data())->metadata->name().c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of function type with operator '<<'");
		break;
	}
}

void shift_right_operator(AbstractSynatxTree *ast) {

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
		((Number*)result->data())->value = (long)((Number*)lvalue.data())->value >> (long)to_number(ast, rvalue);
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(ast, ">>", 1)) {
			error("class '%s' dosen't ovreload operator '>>'(1)", ((Object *)lvalue.data())->metadata->name().c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of function type with operator '>>'");
		break;
	}
}

void inclusive_range_operator(AbstractSynatxTree *ast) {

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
				((Iterator *)result->data())->ctx.push_back(SharedReference::unique(item));
			}
			else {
				((Iterator *)result->data())->ctx.push_front(SharedReference::unique(item));
			}
		}
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(ast, "..", 1)) {
			error("class '%s' dosen't ovreload operator '..'(1)", ((Object *)lvalue.data())->metadata->name().c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of function type with operator '..'");
		break;
	}
}

void exclusive_range_operator(AbstractSynatxTree *ast) {

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
				((Iterator *)result->data())->ctx.push_back(SharedReference::unique(item));
			}
			else {
				((Number *)item->data())->value = i + 1;
				((Iterator *)result->data())->ctx.push_front(SharedReference::unique(item));
			}
		}
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (!call_overload(ast, "...", 1)) {
			error("class '%s' dosen't ovreload operator '...'(1)", ((Object *)lvalue.data())->metadata->name().c_str());
		}
		break;
	case Data::fmt_function:
		error("invalid use of function type with operator '...'");
		break;
	}
}

void typeof_operator(AbstractSynatxTree *ast) {

	Reference &value = *ast->stack().back();
	Reference *result = Reference::create<String>();

	switch (value.data()->format) {
	case Data::fmt_none:
		((String *)result->data())->str = "none";
		break;
	case Data::fmt_null:
		((String *)result->data())->str = "null";
		break;
	case Data::fmt_number:
		((String *)result->data())->str = "number";
		break;
	case Data::fmt_object:
		((String *)result->data())->str = ((Object *)value.data())->metadata->name();
		break;
	case Data::fmt_function:
		((String *)result->data())->str = "function";
		break;
	}

	ast->stack().pop_back();
	ast->stack().push_back(SharedReference::unique(result));
}

void membersof_operator(AbstractSynatxTree *ast) {

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

void subscript_operator(AbstractSynatxTree *ast) {

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

void iterator_move(Reference *dest, deque<SharedReference> &iterator, AbstractSynatxTree *ast) {

	ast->stack().push_back(dest);
	ast->stack().push_back(iterator.front());
	move_operator(ast);
	ast->stack().pop_back();
}

void find_defined_symbol(AbstractSynatxTree *ast, const std::string &symbol) {

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

void find_defined_member(AbstractSynatxTree *ast, const std::string &symbol) {

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

void check_defined(AbstractSynatxTree *ast) {

	SharedReference value = ast->stack().back();
	ast->stack().pop_back();

	Reference *result = Reference::create<Number>();
	if (value->data()->format == Data::fmt_none) {
		((Number *)result->data())->value = 0;
	}
	else {
		((Number *)result->data())->value = 1;
	}

	ast->stack().push_back(SharedReference::unique(result));
}

void in_find(AbstractSynatxTree *ast) {

}

void in_init(AbstractSynatxTree *ast) {

	size_t base = get_base(ast);

	Reference &rvalue = *ast->stack().at(base);
	Reference &lvalue = *ast->stack().at(base - 1);

	Reference *result = Reference::create<Iterator>();
	Iterator *iterator = (Iterator *)result->data();
	iterator_init(iterator->ctx, rvalue);
	ast->stack().push_back(SharedReference::unique(result));

	if (!iterator->ctx.empty()) {
		iterator_move(&lvalue, iterator->ctx, ast);
	}
}

void in_next(AbstractSynatxTree *ast) {

	size_t base = get_base(ast);

	Reference &rvalue = *ast->stack().at(base);
	Reference &lvalue = *ast->stack().at(base - 2);

	Iterator *iterator = (Iterator *)rvalue.data();
	iterator->ctx.pop_front();
	if (!iterator->ctx.empty()) {
		iterator_move(&lvalue, iterator->ctx, ast);
	}
}

void in_check(AbstractSynatxTree *ast) {

	Reference &rvalue = *ast->stack().back();
	Reference *result = Reference::create<Number>();

	if (((Iterator *)rvalue.data())->ctx.empty()) {
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().pop_back();

		((Number *)result->data())->value = 0;
	}
	else {
		((Number *)result->data())->value = 1;
	}

	ast->stack().push_back(SharedReference::unique(result));
}

bool Hash::compare::operator ()(const SharedReference &a, const SharedReference &b) const {

	AbstractSynatxTree ast;
	ast.stack().push_back(SharedReference::unique(new Reference(*a)));
	ast.stack().push_back(SharedReference::unique(new Reference(*b)));

	AbstractSynatxTree::CallHandler handler = ast.getCallHandler();
	lt_operator(&ast);
	while (ast.callInProgress(handler)) {
		run_step(&ast);
	}

	return is_not_zero(ast.stack().back());
}
