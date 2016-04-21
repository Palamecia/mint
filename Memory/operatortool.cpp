#include "operatortool.h"
#include "object.h"
#include "AbstractSyntaxTree/abstractsyntaxtree.h"

void move_operator(AbstractSynatxTree *ast) {

	Reference rvalue = ast->stack().back();
	ast->stack().pop_back();
	Reference &lvalue = ast->stack().back().get();
	ast->stack().pop_back();

	if ((lvalue.data()->format == Data::fmt_function) &&
			(rvalue.data()->format == Data::fmt_function)) {
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
	ast->stack().push_back(&lvalue);
}

void copy_operator(AbstractSynatxTree *ast) {

	Reference rvalue = ast->stack().back();
	ast->stack().pop_back();
	Reference &lvalue = ast->stack().back().get();
	ast->stack().pop_back();

	switch (lvalue.data()->format) {
	case Data::fmt_null:
	case Data::fmt_none:
	case Data::fmt_number:
	case Data::fmt_function:
	case Data::fmt_hash:
		lvalue.copy(rvalue);
		ast->stack().push_back(SharedReference(&lvalue));
		break;
	case Data::fmt_object:
		break;
	}
}

void call_operator(AbstractSynatxTree *ast) {

	Reference &lvalue = ast->stack().back().get();
	ast->stack().pop_back();

	switch (lvalue.data()->format) {
	case Data::fmt_null:
	case Data::fmt_none:
	case Data::fmt_number:
	case Data::fmt_object:
	case Data::fmt_hash:
		/// \todo push const clone
		break;
	case Data::fmt_function:
		auto it = ((Function*)lvalue.data())->mapping.find(ast->next().parameter);
		if (it == ((Function*)lvalue.data())->mapping.end()) {
			break;
		}
		ast->call(it->second.first, it->second.second);
		break;
	}
}

void add_operator(AbstractSynatxTree *ast) {

	Reference rvalue = ast->stack().back();
	ast->stack().pop_back();
	Reference &lvalue = ast->stack().back().get();
	ast->stack().pop_back();
	Reference *result;

	switch (lvalue.data()->format) {
	case Data::fmt_null:
		break;
	case Data::fmt_none:
		break;
	case Data::fmt_number:
		result = Reference::create<Number>();
		((Number*)result->data())->data = ((Number*)lvalue.data())->data + to_number(rvalue);
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
	case Data::fmt_function:
	case Data::fmt_hash:
		break;
	}
}

void sub_operator(AbstractSynatxTree *ast) {

	Reference rvalue = ast->stack().back();
	ast->stack().pop_back();
	Reference &lvalue = ast->stack().back().get();
	ast->stack().pop_back();
	Reference *result;

	switch (lvalue.data()->format) {
	case Data::fmt_null:
		break;
	case Data::fmt_none:
		break;
	case Data::fmt_number:
		result = Reference::create<Number>();
		((Number*)result->data())->data = ((Number*)lvalue.data())->data - to_number(rvalue);
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
	case Data::fmt_function:
	case Data::fmt_hash:
		break;
	}
}

void mul_operator(AbstractSynatxTree *ast) {

	Reference rvalue = ast->stack().back();
	ast->stack().pop_back();
	Reference &lvalue = ast->stack().back().get();
	ast->stack().pop_back();
	Reference *result;

	switch (lvalue.data()->format) {
	case Data::fmt_null:
		break;
	case Data::fmt_none:
		break;
	case Data::fmt_number:
		result = Reference::create<Number>();
		((Number*)result->data())->data = ((Number*)lvalue.data())->data * to_number(rvalue);
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
	case Data::fmt_function:
	case Data::fmt_hash:
		break;
	}
}

void div_operator(AbstractSynatxTree *ast) {

	Reference rvalue = ast->stack().back();
	ast->stack().pop_back();
	Reference &lvalue = ast->stack().back().get();
	ast->stack().pop_back();
	Reference *result;

	switch (lvalue.data()->format) {
	case Data::fmt_null:
		break;
	case Data::fmt_none:
		break;
	case Data::fmt_number:
		result = Reference::create<Number>();
		((Number*)result->data())->data = ((Number*)lvalue.data())->data / to_number(rvalue);
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
	case Data::fmt_function:
	case Data::fmt_hash:
		break;
	}
}

void mod_operator(AbstractSynatxTree *ast) {

	Reference rvalue = ast->stack().back();
	ast->stack().pop_back();
	Reference &lvalue = ast->stack().back().get();
	ast->stack().pop_back();
	Reference *result;

	switch (lvalue.data()->format) {
	case Data::fmt_null:
		break;
	case Data::fmt_none:
		break;
	case Data::fmt_number:
		result = Reference::create<Number>();
		((Number*)result->data())->data = (int)((Number*)lvalue.data())->data % (int)to_number(rvalue);
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
	case Data::fmt_function:
	case Data::fmt_hash:
		break;
	}
}

void eq_operator(AbstractSynatxTree *ast) {

	Reference rvalue = ast->stack().back();
	ast->stack().pop_back();
	Reference &lvalue = ast->stack().back().get();
	ast->stack().pop_back();
	Reference *result;

	switch (lvalue.data()->format) {
	case Data::fmt_null:
		break;
	case Data::fmt_none:
		break;
	case Data::fmt_number:
		result = Reference::create<Number>();
		((Number*)result->data())->data = ((Number*)lvalue.data())->data == to_number(rvalue);
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
	case Data::fmt_function:
	case Data::fmt_hash:
		break;
	}
}
