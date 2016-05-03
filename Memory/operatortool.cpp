#include "Memory/operatortool.h"
#include "Memory/memorytool.h"
#include "Memory/object.h"
#include "Memory/class.h"
#include "AbstractSyntaxTree/abstractsyntaxtree.h"

#include <math.h>

using namespace std;

void move_operator(AbstractSynatxTree *ast) {

	Reference rvalue = ast->stack().back();
	ast->stack().pop_back();
	Reference &lvalue = ast->stack().back().get();

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
}

void copy_operator(AbstractSynatxTree *ast) {

	Reference rvalue = ast->stack().back();
	ast->stack().pop_back();
	Reference &lvalue = ast->stack().back().get();

	switch (lvalue.data()->format) {
	case Data::fmt_null:
	case Data::fmt_none:
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

	Reference lvalue = ast->waitingCalls().top();
	ast->waitingCalls().pop();

	switch (lvalue.data()->format) {
	case Data::fmt_null:
	case Data::fmt_none:
	case Data::fmt_number:
	case Data::fmt_object:
	case Data::fmt_hash:
		/// \todo push const clone
		break;
	case Data::fmt_function:
		auto it = ((Function*)lvalue.data())->mapping.find(format);
		if (it == ((Function*)lvalue.data())->mapping.end()) {
			break;
		}
		ast->call(it->second.first, it->second.second);
		break;
	}
}

void call_member_operator(AbstractSynatxTree *ast, int format) {

	size_t base = ast->stack().size() - 1;

	Reference &object = ast->stack().at(base - format).get();
	Reference lvalue = ast->waitingCalls().top();
	ast->waitingCalls().pop();

	switch (lvalue.data()->format) {
	case Data::fmt_null:
	case Data::fmt_none:
	case Data::fmt_number:
	case Data::fmt_object:
	case Data::fmt_hash:
		/// \todo push const clone
		break;
	case Data::fmt_function:
		auto it = ((Function*)lvalue.data())->mapping.find(format + 1);
		if (it == ((Function*)lvalue.data())->mapping.end()) {
			break;
		}
		ast->call(it->second.first, it->second.second);
		ast->symbols().metadata = ((Object *)object.data())->metadata;
		break;
	}
}

void add_operator(AbstractSynatxTree *ast) {

	Reference rvalue = ast->stack().back();
	ast->stack().pop_back();
	Reference lvalue = ast->stack().back();
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
		if (((Object *)lvalue.data())->metadata == StringClass::instance()) {
			result = Reference::create<String>();
			((String *)result->data())->str = ((String *)lvalue.data())->str + to_string(rvalue);
			ast->stack().push_back(SharedReference::unique(result));
		}
		break;
	case Data::fmt_function:
	case Data::fmt_hash:
		break;
	}
}

void sub_operator(AbstractSynatxTree *ast) {

	Reference rvalue = ast->stack().back();
	ast->stack().pop_back();
	Reference lvalue = ast->stack().back();
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
	Reference lvalue = ast->stack().back();
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
	Reference lvalue = ast->stack().back();
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

void pow_operator(AbstractSynatxTree *ast) {

	Reference rvalue = ast->stack().back();
	ast->stack().pop_back();
	Reference lvalue = ast->stack().back();
	ast->stack().pop_back();
	Reference *result;

	switch (lvalue.data()->format) {
	case Data::fmt_null:
		break;
	case Data::fmt_none:
		break;
	case Data::fmt_number:
		result = Reference::create<Number>();
		((Number*)result->data())->data = pow(((Number*)lvalue.data())->data, to_number(rvalue));
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
	Reference lvalue = ast->stack().back();
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

void is_operator(AbstractSynatxTree *ast) {

	Reference rvalue = ast->stack().back();
	ast->stack().pop_back();
	Reference lvalue = ast->stack().back();
	ast->stack().pop_back();

	Reference *result = Reference::create<Number>();
	((Number*)result->data())->data = lvalue.data() == rvalue.data();
	ast->stack().push_back(SharedReference::unique(result));
}

void eq_operator(AbstractSynatxTree *ast) {

	Reference rvalue = ast->stack().back();
	ast->stack().pop_back();
	Reference lvalue = ast->stack().back();
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
		if (((Object *)lvalue.data())->metadata == StringClass::instance()) {
			result = Reference::create<Number>();
			((Number *)result->data())->data = ((String *)lvalue.data())->str == to_string(rvalue);
			ast->stack().push_back(SharedReference::unique(result));
		}
		break;
	case Data::fmt_function:
	case Data::fmt_hash:
		break;
	}
}

void ne_operator(AbstractSynatxTree *ast) {

	Reference rvalue = ast->stack().back();
	ast->stack().pop_back();
	Reference lvalue = ast->stack().back();
	ast->stack().pop_back();
	Reference *result;

	switch (lvalue.data()->format) {
	case Data::fmt_null:
		break;
	case Data::fmt_none:
		break;
	case Data::fmt_number:
		result = Reference::create<Number>();
		((Number*)result->data())->data = ((Number*)lvalue.data())->data != to_number(rvalue);
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (((Object *)lvalue.data())->metadata == StringClass::instance()) {
			result = Reference::create<Number>();
			((Number *)result->data())->data = ((String *)lvalue.data())->str != to_string(rvalue);
			ast->stack().push_back(SharedReference::unique(result));
		}
		break;
	case Data::fmt_function:
	case Data::fmt_hash:
		break;
	}
}

void lt_operator(AbstractSynatxTree *ast) {

	Reference rvalue = ast->stack().back();
	ast->stack().pop_back();
	Reference lvalue = ast->stack().back();
	ast->stack().pop_back();
	Reference *result;

	switch (lvalue.data()->format) {
	case Data::fmt_null:
		break;
	case Data::fmt_none:
		break;
	case Data::fmt_number:
		result = Reference::create<Number>();
		((Number*)result->data())->data = ((Number*)lvalue.data())->data < to_number(rvalue);
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (((Object *)lvalue.data())->metadata == StringClass::instance()) {
			result = Reference::create<Number>();
			((Number *)result->data())->data = ((String *)lvalue.data())->str < to_string(rvalue);
			ast->stack().push_back(SharedReference::unique(result));
		}
		break;
	case Data::fmt_function:
	case Data::fmt_hash:
		break;
	}
}

void gt_operator(AbstractSynatxTree *ast) {

	Reference rvalue = ast->stack().back();
	ast->stack().pop_back();
	Reference lvalue = ast->stack().back();
	ast->stack().pop_back();
	Reference *result;

	switch (lvalue.data()->format) {
	case Data::fmt_null:
		break;
	case Data::fmt_none:
		break;
	case Data::fmt_number:
		result = Reference::create<Number>();
		((Number*)result->data())->data = ((Number*)lvalue.data())->data > to_number(rvalue);
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (((Object *)lvalue.data())->metadata == StringClass::instance()) {
			result = Reference::create<Number>();
			((Number *)result->data())->data = ((String *)lvalue.data())->str > to_string(rvalue);
			ast->stack().push_back(SharedReference::unique(result));
		}
		break;
	case Data::fmt_function:
	case Data::fmt_hash:
		break;
	}
}

void le_operator(AbstractSynatxTree *ast) {

	Reference rvalue = ast->stack().back();
	ast->stack().pop_back();
	Reference lvalue = ast->stack().back();
	ast->stack().pop_back();
	Reference *result;

	switch (lvalue.data()->format) {
	case Data::fmt_null:
		break;
	case Data::fmt_none:
		break;
	case Data::fmt_number:
		result = Reference::create<Number>();
		((Number*)result->data())->data = ((Number*)lvalue.data())->data <= to_number(rvalue);
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (((Object *)lvalue.data())->metadata == StringClass::instance()) {
			result = Reference::create<Number>();
			((Number *)result->data())->data = ((String *)lvalue.data())->str <= to_string(rvalue);
			ast->stack().push_back(SharedReference::unique(result));
		}
		break;
	case Data::fmt_function:
	case Data::fmt_hash:
		break;
	}
}

void ge_operator(AbstractSynatxTree *ast) {

	Reference rvalue = ast->stack().back();
	ast->stack().pop_back();
	Reference lvalue = ast->stack().back();
	ast->stack().pop_back();
	Reference *result;

	switch (lvalue.data()->format) {
	case Data::fmt_null:
		break;
	case Data::fmt_none:
		break;
	case Data::fmt_number:
		result = Reference::create<Number>();
		((Number*)result->data())->data = ((Number*)lvalue.data())->data >= to_number(rvalue);
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (((Object *)lvalue.data())->metadata == StringClass::instance()) {
			result = Reference::create<Number>();
			((Number *)result->data())->data = ((String *)lvalue.data())->str >= to_string(rvalue);
			ast->stack().push_back(SharedReference::unique(result));
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
	case Data::fmt_null:
	case Data::fmt_none:
		break;
	case Data::fmt_number:
		result = Reference::create<Number>();
		((Number *)result->data())->data = ((Number*)value.data())->data + 1;
		value.move(SharedReference::unique(result));
		break;
	case Data::fmt_object:
	case Data::fmt_function:
	case Data::fmt_hash:
	case Data::fmt_array:
		break;
	}
}

void dec_operator(AbstractSynatxTree *ast) {

	Reference &value = ast->stack().back().get();
	Reference *result;

	switch (value.data()->format) {
	case Data::fmt_null:
	case Data::fmt_none:
		break;
	case Data::fmt_number:
		result = Reference::create<Number>();
		((Number *)result->data())->data = ((Number*)value.data())->data - 1;
		value.move(SharedReference::unique(result));
		break;
	case Data::fmt_object:
	case Data::fmt_function:
	case Data::fmt_hash:
	case Data::fmt_array:
		break;
	}
}

void not_operator(AbstractSynatxTree *ast) {

	Reference value = ast->stack().back();
	ast->stack().pop_back();
	Reference *result = Reference::create<Number>();

	switch (value.data()->format) {
	case Data::fmt_null:
		break;
	case Data::fmt_none:
		break;
	case Data::fmt_number:
		((Number*)result->data())->data = !((Number*)value.data())->data;
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
		if (((Object *)value.data())->metadata == StringClass::instance()) {
			((Number *)result->data())->data = ((String *)value.data())->str.empty();
			ast->stack().push_back(SharedReference::unique(result));
		}
		break;
	case Data::fmt_function:
	case Data::fmt_hash:
		break;
	}
}

void inv_operator(AbstractSynatxTree *ast) {

	Reference value = ast->stack().back();
	ast->stack().pop_back();
	Reference *result;

	switch (value.data()->format) {
	case Data::fmt_null:
		break;
	case Data::fmt_none:
		break;
	case Data::fmt_number:
		result = Reference::create<Number>();
		((Number*)result->data())->data = ~((int)((Number*)value.data())->data);
		ast->stack().push_back(SharedReference::unique(result));
		break;
	case Data::fmt_object:
	case Data::fmt_function:
	case Data::fmt_hash:
		break;
	}
}

void shift_left_operator(AbstractSynatxTree *ast) {

}

void shift_right_operator(AbstractSynatxTree *ast) {

}

void membersof_operator(AbstractSynatxTree *ast) {

	Reference value = ast->stack().back();
	ast->stack().pop_back();
	Reference *result = Reference::create<Array>();

	if (value.data()->format == Data::fmt_object) {

		for (auto member : ((Object *)value.data())->metadata->members()) {
			Reference *entry = Reference::create<String>();
			((String *)entry->data())->str = member.first;
			((Array *)result->data())->values.push_back(*entry);
			delete entry;
		}
	}
	else {
		/// \todo error
	}

	ast->stack().push_back(SharedReference::unique(result));
}

void subscript_operator(AbstractSynatxTree *ast) {

	Reference rvalue = ast->stack().back();
	ast->stack().pop_back();
	Reference lvalue = ast->stack().back();
	ast->stack().pop_back();

	switch (lvalue.data()->format) {
	case Data::fmt_null:
	case Data::fmt_none:
	case Data::fmt_number:
	case Data::fmt_function:
		break;
	case Data::fmt_hash:
		ast->stack().push_back(&((Hash *)lvalue.data())->values[rvalue]);
		break;
	case Data::fmt_array:
		ast->stack().push_back(&((Array *)lvalue.data())->values[to_number(rvalue)]);
		break;
	case Data::fmt_object:
		break;
	}
}

void iterator_move(queue<SharedReference> &iterator, Reference &ref, AbstractSynatxTree *ast) {

	ast->stack().push_back(&ref);
	ast->stack().push_back(iterator.front());
	move_operator(ast);
	ast->stack().pop_back();
}

void in_find(AbstractSynatxTree *ast) {

}

void in_init(AbstractSynatxTree *ast) {

	size_t base = ast->stack().size() - 1;
	Reference &rvalue = ast->stack().back().get();
	Reference &lvalue = ast->stack().at(base - 1).get();

	Reference *iterator = Reference::create<Iterator>();
	iterator_init(((Iterator *)iterator->data())->ctx, rvalue);
	ast->stack().push_back(SharedReference::unique(iterator));

	if (!((Iterator *)iterator->data())->ctx.empty()) {
		iterator_move(((Iterator *)iterator->data())->ctx, lvalue, ast);
	}
}

void in_next(AbstractSynatxTree *ast) {

	size_t base = ast->stack().size() - 1;
	Reference &rvalue = ast->stack().at(base).get();
	Reference &lvalue = ast->stack().at(base - 2).get();

	Iterator *iterator = (Iterator *)rvalue.data();
	iterator->ctx.pop();
	if (!iterator->ctx.empty()) {
		iterator_move(iterator->ctx, lvalue, ast);
	}
}

void in_check(AbstractSynatxTree *ast) {

	Reference &rvalue = ast->stack().back().get();
	Reference *result = Reference::create<Number>();

	if (((Iterator *)rvalue.data())->ctx.empty()) {
		ast->stack().pop_back();
		ast->stack().pop_back();
		ast->stack().pop_back();

		((Number *)result->data())->data = 0;
	}
	else {
		((Number *)result->data())->data = 1;
	}

	ast->stack().push_back(SharedReference::unique(result));
}
