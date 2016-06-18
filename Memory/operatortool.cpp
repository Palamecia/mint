#include "Memory/operatortool.h"
#include "Memory/memorytool.h"
#include "Memory/object.h"
#include "Memory/class.h"
#include "AbstractSyntaxTree/abstractsyntaxtree.h"
#include "System/error.h"

#include <math.h>

using namespace std;

void call_overload(AbstractSynatxTree *ast, const string &operator_overload, int format) {

	size_t base = get_base(ast);
	Object *object = (Object *)ast->stack().at(base - format).get().data();
	auto it = object->metadata->members().find(operator_overload);

	if (it == object->metadata->members().end()) {
		error("class '%s' dosen't ovreload operator '%s'(%d)", object->metadata->name().c_str(), operator_overload.c_str(), format);
	}

	ast->waitingCalls().push(&object->data[it->second->offset]);
	call_member_operator(ast, format);
}

void move_operator(AbstractSynatxTree *ast) {

	size_t base = get_base(ast);

	Reference &rvalue = ast->stack().at(base).get();
	Reference &lvalue = ast->stack().at(base - 1).get();

	if ((lvalue.data()->format == Data::fmt_function) && (rvalue.data()->format == Data::fmt_function)) {
		for (auto form : ((Function*)rvalue.data())->mapping) {
			((Function*)lvalue.data())->mapping[form.first] = form.second;
		}
	}
	else if (rvalue.flags() & Reference::const_value) {
		lvalue.copy(rvalue);
	}
	else if (lvalue.flags() & Reference::const_ref) {
		/// \todo error
	}
	else {
		lvalue.move(rvalue);
	}

	ast->stack().pop_back();
}

void copy_operator(AbstractSynatxTree *ast) {

	Reference rvalue = ast->stack().back();
	ast->stack().pop_back();
	Reference &lvalue = ast->stack().back().get();

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		ast->raise(&lvalue);
		break;
	case Data::fmt_number:
	case Data::fmt_function:
	case Data::fmt_hash:
	case Data::fmt_array:
		lvalue.copy(rvalue);
		break;
	case Data::fmt_object:
		break;
	}
}

void call_operator(AbstractSynatxTree *ast, int format) {

	Reference *result = nullptr;
	Reference lvalue = ast->waitingCalls().top().get();
	bool is_member = ast->waitingCalls().top().isMember();
	ast->waitingCalls().pop();

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		if (is_member) {
			if (format) {
				/// \todo error wrong format
			}
		}
		else {
			error("invalid use of none value in an operation");
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
	case Data::fmt_hash:
		result = Reference::create<Hash>();
		result->copy(lvalue);
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_array:
		result = Reference::create<Array>();
		result->copy(lvalue);
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_function:
		auto it = ((Function*)lvalue.data())->mapping.find(format + (is_member ? 1 : 0));
		if (it == ((Function*)lvalue.data())->mapping.end()) {
			/// \todo error wrong format
			break;
		}
		ast->call(it->second.first, it->second.second);
		if (is_member) {
			size_t base = ast->stack().size() - 1;
			ast->symbols().metadata = ((Object *)ast->stack().at(base - format).get().data())->metadata;
		}
		break;
	}
}

void call_member_operator(AbstractSynatxTree *ast, int format) {

	size_t base = get_base(ast);

	Reference *result = nullptr;
	Reference &object = ast->stack().at(base - format).get();
	Reference lvalue = ast->waitingCalls().top().get();
	ast->waitingCalls().pop();

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
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
	case Data::fmt_hash:
		result = Reference::create<Hash>();
		result->copy(lvalue);
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_array:
		result = Reference::create<Array>();
		result->copy(lvalue);
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_function:
		auto it = ((Function*)lvalue.data())->mapping.find(format + 1);
		if (it == ((Function*)lvalue.data())->mapping.end()) {
			/// \todo error wrong format
			break;
		}
		ast->call(it->second.first, it->second.second);
		ast->symbols().metadata = ((Object *)object.data())->metadata;
		break;
	}
}

void add_operator(AbstractSynatxTree *ast) {

	size_t base = get_base(ast);

	Reference &rvalue = ast->stack().at(base).get();
	Reference &lvalue = ast->stack().at(base - 1).get();
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
		if (((Object *)lvalue.data())->metadata == StringClass::instance()) {
			result = Reference::create<String>();
			((String *)result->data())->str = ((String *)lvalue.data())->str + to_string(rvalue);
			ast->stack().pop_back();
			ast->stack().pop_back();
			ast->stack().push_back(SharedReference::unique(result));
		}
		else {
			call_overload(ast, "+", 1);
		}
		/// \todo other types
		break;
	case Data::fmt_function:
		/// \todo error
		break;
	/*case Data::fmt_hash:
		result = Reference::create<Hash>();
		((Hash *)result->data())->values = ((Hash *)lvalue.data())->values + to_hash(rvalue);
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_array:
		result = Reference::create<Array>();
		((Array *)result->data())->values = ((Array *)lvalue.data())->values + to_array(rvalue);
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;*/
	}
}

void sub_operator(AbstractSynatxTree *ast) {

	size_t base = get_base(ast);

	Reference &rvalue = ast->stack().at(base).get();
	Reference &lvalue = ast->stack().at(base - 1).get();
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
		call_overload(ast, "-", 1);
		break;
	case Data::fmt_function:
	case Data::fmt_hash:
		break;
	}
}

void mul_operator(AbstractSynatxTree *ast) {

	size_t base = get_base(ast);

	Reference &rvalue = ast->stack().at(base).get();
	Reference &lvalue = ast->stack().at(base - 1).get();
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
		call_overload(ast, "*", 1);
		break;
	case Data::fmt_function:
	case Data::fmt_hash:
		break;
	}
}

void div_operator(AbstractSynatxTree *ast) {

	size_t base = get_base(ast);

	Reference &rvalue = ast->stack().at(base).get();
	Reference &lvalue = ast->stack().at(base - 1).get();
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
		call_overload(ast, "/", 1);
		break;
	case Data::fmt_function:
	case Data::fmt_hash:
		break;
	}
}

void pow_operator(AbstractSynatxTree *ast) {

	size_t base = get_base(ast);

	Reference &rvalue = ast->stack().at(base).get();
	Reference &lvalue = ast->stack().at(base - 1).get();
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
		call_overload(ast, "**", 1);
		break;
	case Data::fmt_function:
	case Data::fmt_hash:
		break;
	}
}

void mod_operator(AbstractSynatxTree *ast) {

	size_t base = get_base(ast);

	Reference &rvalue = ast->stack().at(base).get();
	Reference &lvalue = ast->stack().at(base - 1).get();
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
		call_overload(ast, "%", 1);
		break;
	case Data::fmt_function:
	case Data::fmt_hash:
		break;
	}
}

void is_operator(AbstractSynatxTree *ast) {

	size_t base = get_base(ast);

	Reference &rvalue = ast->stack().at(base).get();
	Reference &lvalue = ast->stack().at(base - 1).get();

	Reference *result = Reference::create<Number>();
	((Number*)result->data())->value = lvalue.data() == rvalue.data();
	ast->stack().pop_back();
	ast->stack().pop_back();
	ast->stack().push_back(SharedReference::unique(result));
}

void eq_operator(AbstractSynatxTree *ast) {

	size_t base = get_base(ast);

	Reference &rvalue = ast->stack().at(base).get();
	Reference &lvalue = ast->stack().at(base - 1).get();
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
		((Number*)result->data())->value = ((Number*)lvalue.data())->value == to_number(ast, rvalue);
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (((Object *)lvalue.data())->metadata == StringClass::instance()) {
			result = Reference::create<Number>();
			((Number *)result->data())->value = ((String *)lvalue.data())->str == to_string(rvalue);
			ast->stack().pop_back();
			ast->stack().pop_back();
			ast->stack().push_back(SharedReference::unique(result));
		}
		else {
			call_overload(ast, "==", 1);
		}
		break;
	case Data::fmt_function:
	case Data::fmt_hash:
		break;
	}
}

void ne_operator(AbstractSynatxTree *ast) {

	size_t base = get_base(ast);

	Reference &rvalue = ast->stack().at(base).get();
	Reference &lvalue = ast->stack().at(base - 1).get();
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
		((Number*)result->data())->value = ((Number*)lvalue.data())->value != to_number(ast, rvalue);
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (((Object *)lvalue.data())->metadata == StringClass::instance()) {
			result = Reference::create<Number>();
			((Number *)result->data())->value = ((String *)lvalue.data())->str != to_string(rvalue);
			ast->stack().pop_back();
			ast->stack().pop_back();
			ast->stack().push_back(SharedReference::unique(result));
		}
		else {
			call_overload(ast, "!=", 1);
		}
		break;
	case Data::fmt_function:
	case Data::fmt_hash:
		break;
	}
}

void lt_operator(AbstractSynatxTree *ast) {

	size_t base = get_base(ast);

	Reference &rvalue = ast->stack().at(base).get();
	Reference &lvalue = ast->stack().at(base - 1).get();
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
		if (((Object *)lvalue.data())->metadata == StringClass::instance()) {
			result = Reference::create<Number>();
			((Number *)result->data())->value = ((String *)lvalue.data())->str < to_string(rvalue);
			ast->stack().pop_back();
			ast->stack().pop_back();
			ast->stack().push_back(SharedReference::unique(result));
		}
		else {
			call_overload(ast, "<", 1);
		}
		break;
	case Data::fmt_function:
	case Data::fmt_hash:
		break;
	}
}

void gt_operator(AbstractSynatxTree *ast) {

	size_t base = get_base(ast);

	Reference &rvalue = ast->stack().at(base).get();
	Reference &lvalue = ast->stack().at(base - 1).get();
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
		if (((Object *)lvalue.data())->metadata == StringClass::instance()) {
			result = Reference::create<Number>();
			((Number *)result->data())->value = ((String *)lvalue.data())->str > to_string(rvalue);
			ast->stack().pop_back();
			ast->stack().pop_back();
			ast->stack().push_back(SharedReference::unique(result));
		}
		else {
			call_overload(ast, ">", 1);
		}
		break;
	case Data::fmt_function:
	case Data::fmt_hash:
		break;
	}
}

void le_operator(AbstractSynatxTree *ast) {

	size_t base = get_base(ast);

	Reference &rvalue = ast->stack().at(base).get();
	Reference &lvalue = ast->stack().at(base - 1).get();
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
		if (((Object *)lvalue.data())->metadata == StringClass::instance()) {
			result = Reference::create<Number>();
			((Number *)result->data())->value = ((String *)lvalue.data())->str <= to_string(rvalue);
			ast->stack().pop_back();
			ast->stack().pop_back();
			ast->stack().push_back(SharedReference::unique(result));
		}
		else {
			call_overload(ast, "<=", 1);
		}
		break;
	case Data::fmt_function:
	case Data::fmt_hash:
		break;
	}
}

void ge_operator(AbstractSynatxTree *ast) {

	size_t base = get_base(ast);

	Reference &rvalue = ast->stack().at(base).get();
	Reference &lvalue = ast->stack().at(base - 1).get();
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
		if (((Object *)lvalue.data())->metadata == StringClass::instance()) {
			result = Reference::create<Number>();
			((Number *)result->data())->value = ((String *)lvalue.data())->str >= to_string(rvalue);
			ast->stack().pop_back();
			ast->stack().pop_back();
			ast->stack().push_back(SharedReference::unique(result));
		}
		else {
			call_overload(ast, ">=", 1);
		}
		break;
	case Data::fmt_function:
	case Data::fmt_hash:
		break;
	}
}

void and_operator(AbstractSynatxTree *ast) {

	size_t base = get_base(ast);

	Reference &rvalue = ast->stack().at(base).get();
	Reference &lvalue = ast->stack().at(base - 1).get();
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
		if (((Object *)lvalue.data())->metadata == StringClass::instance()) {
			result = Reference::create<Number>();
			((Number *)result->data())->value = ((String *)lvalue.data())->str.size() && to_number(ast, rvalue);
			ast->stack().pop_back();
			ast->stack().pop_back();
			ast->stack().push_back(SharedReference::unique(result));
		}
		else {
			call_overload(ast, "&&", 1);
		}
		break;
	case Data::fmt_function:
	case Data::fmt_hash:
		break;
	}
}

void or_operator(AbstractSynatxTree *ast) {

	size_t base = get_base(ast);

	Reference &rvalue = ast->stack().at(base).get();
	Reference &lvalue = ast->stack().at(base - 1).get();
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
		if (((Object *)lvalue.data())->metadata == StringClass::instance()) {
			result = Reference::create<Number>();
			((Number *)result->data())->value = ((String *)lvalue.data())->str.size() || to_number(ast, rvalue);
			ast->stack().pop_back();
			ast->stack().pop_back();
			ast->stack().push_back(SharedReference::unique(result));
		}
		else {
			call_overload(ast, "||", 1);
		}
		break;
	case Data::fmt_function:
	case Data::fmt_hash:
		break;
	}
}

void xor_operator(AbstractSynatxTree *ast) {

	size_t base = get_base(ast);

	Reference &rvalue = ast->stack().at(base).get();
	Reference &lvalue = ast->stack().at(base - 1).get();
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
		if (((Object *)lvalue.data())->metadata == StringClass::instance()) {
			result = Reference::create<Number>();
			((Number *)result->data())->value = ((String *)lvalue.data())->str.size() ^ (size_t)to_number(ast, rvalue);
			ast->stack().pop_back();
			ast->stack().pop_back();
			ast->stack().push_back(SharedReference::unique(result));
		}
		else {
			call_overload(ast, "^", 1);
		}
		break;
	case Data::fmt_function:
	case Data::fmt_hash:
		break;
	}
}

void inc_operator(AbstractSynatxTree *ast) {

	Reference &value = ast->stack().back().get();
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
		value.move(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		call_overload(ast, "++", 0);
		break;
	/*case Data::fmt_function:
	case Data::fmt_hash:
	case Data::fmt_array:*/
		break;
	}
}

void dec_operator(AbstractSynatxTree *ast) {

	Reference &value = ast->stack().back().get();
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
		value.move(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		call_overload(ast, "--", 0);
		break;
	/*case Data::fmt_function:
	case Data::fmt_hash:
	case Data::fmt_array:*/
		break;
	}
}

void not_operator(AbstractSynatxTree *ast) {

	Reference &value = ast->stack().back().get();
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
		if (((Object *)value.data())->metadata == StringClass::instance()) {
			((Number *)result->data())->value = ((String *)value.data())->str.empty();
			ast->stack().pop_back();
			ast->stack().push_back(SharedReference::unique(result));
		}
		else {
			call_overload(ast, "!", 0);
		}
		break;
	case Data::fmt_function:
	case Data::fmt_hash:
		break;
	}
}

void compl_operator(AbstractSynatxTree *ast) {

	Reference &value = ast->stack().back().get();
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
		call_overload(ast, "~", 0);
		break;
	case Data::fmt_function:
	case Data::fmt_hash:
		break;
	}
}

void shift_left_operator(AbstractSynatxTree *ast) {
	/// \todo
}

void shift_right_operator(AbstractSynatxTree *ast) {
	/// \todo
}

void typeof_operator(AbstractSynatxTree *ast) {

	Reference &value = ast->stack().back().get();
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
	case Data::fmt_hash:
		((String *)result->data())->str = "hash";
		break;
	case Data::fmt_array:
		((String *)result->data())->str = "array";
		break;
	}

	ast->stack().pop_back();
	ast->stack().push_back(SharedReference::unique(result));
}

void membersof_operator(AbstractSynatxTree *ast) {

	Reference &value = ast->stack().back().get();
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

			array->values.push_back(new Reference(Reference::standard, Reference::alloc<String>()));
			((String *)array->values.back()->data())->str = member.first;
		}
	}
	else {
		/// \todo error
	}

	ast->stack().pop_back();
	ast->stack().push_back(SharedReference::unique(result));
}

void subscript_operator(AbstractSynatxTree *ast) {

	size_t base = get_base(ast);

	Reference &rvalue = ast->stack().at(base).get();
	Reference &lvalue = ast->stack().at(base - 1).get();
	Reference *result = nullptr;

	switch (lvalue.data()->format) {
	case Data::fmt_none:
		error("invalid use of none value in an operation");
		break;
	case Data::fmt_null:
		ast->raise(&lvalue);
		break;
	case Data::fmt_number:
	case Data::fmt_function:
		break;
	case Data::fmt_hash:
		result = &((Hash *)lvalue.data())->values[rvalue];
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(result);
		break;
	case Data::fmt_array:
		result = ((Array *)lvalue.data())->values[to_number(ast, rvalue)];
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().push_back(result);
		break;
	case Data::fmt_object:
		call_overload(ast, "[]", 1);
		break;
	}
}

void iterator_move(Reference *dest, queue<SharedReference> &iterator, AbstractSynatxTree *ast) {

	ast->stack().push_back(dest);
	ast->stack().push_back(iterator.front());
	move_operator(ast);
	ast->stack().pop_back();
}

void in_find(AbstractSynatxTree *ast) {

}

void in_init(AbstractSynatxTree *ast) {

	size_t base = get_base(ast);

	Reference &rvalue = ast->stack().at(base).get();
	Reference &lvalue = ast->stack().at(base - 1).get();

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

	Reference &rvalue = ast->stack().at(base).get();
	Reference &lvalue = ast->stack().at(base - 2).get();

	Iterator *iterator = (Iterator *)rvalue.data();
	iterator->ctx.pop();
	if (!iterator->ctx.empty()) {
		iterator_move(&lvalue, iterator->ctx, ast);
	}
}

void in_check(AbstractSynatxTree *ast) {

	Reference &rvalue = ast->stack().back().get();
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
