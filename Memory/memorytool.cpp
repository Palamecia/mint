#include "Memory/memorytool.h"
#include "Memory/globaldata.h"
#include "Memory/casttool.h"
#include "Memory/class.h"
#include "Memory/object.h"
#include "AbstractSyntaxTree/abstractsyntaxtree.h"
#include "System/error.h"

using namespace std;

bool is_not_zero(const Reference &ref) {
	switch (ref.data()->format) {
	case Data::fmt_null:
	case Data::fmt_none:
		return false;
	case Data::fmt_number:
		return ((Number*)ref.data())->data;
	default:
		break;
	}
	return true;
}

Printer *toPrinter(const Reference &ref) {

	switch (ref.data()->format) {
	case Data::fmt_number:
		return new Printer((int)((Number*)ref.data())->data);
	case Data::fmt_object:
		if (((Object *)ref.data())->metadata == StringClass::instance()) {
			return new Printer(((String *)ref.data())->str.c_str());
		}
	default:
		break;
	}

	/// \todo error
	return nullptr;
}

void print(Printer *printer, const Reference &ref) {

	if (printer) {
		switch (ref.data()->format) {
		case Data::fmt_null:
		case Data::fmt_none:
			printer->printNull();
			break;
		case Data::fmt_number:
			printer->print(((Number*)ref.data())->data);
			break;
		case Data::fmt_object:
			if (((Object *)ref.data())->metadata == StringClass::instance()) {
				printer->print(((String *)ref.data())->str.c_str());
			}
			else {
				printer->print(ref.data());
			}
		default:
			break;
		}
	}
}

void init_parameter(AbstractSynatxTree *ast, const std::string &symbol) {
	ast->symbols()[symbol].move(ast->stack().back());
	ast->stack().pop_back();
}

Reference *get_symbol_reference(SymbolTable *symbols, const std::string &symbol) {

	auto it = GlobalData::instance().symbols().find(symbol);
	if (it != GlobalData::instance().symbols().end()) {
		return &it->second;
	}

	return &(*symbols)[symbol];
}

Reference *get_object_member(AbstractSynatxTree *ast, const std::string &member) {

	Reference &lvalue = ast->stack().back().get();
	Object *object = (Object *)lvalue.data();

	/// \todo find first in global members

	Reference *result = &object->data[object->metadata->members()[member].offset];

	if (result->flags() & Reference::user_hiden) {
		if (object->metadata != ast->symbols().metadata) {
			/// \todo error
		}
	}
	else if (result->flags() & Reference::child_hiden) {
		/// \todo ...
	}

	return result;
}

void reduce_member(AbstractSynatxTree *ast) {

	Reference member = ast->stack().back();
	ast->stack().pop_back();
	ast->stack().pop_back();

	Reference *result = Reference::create<Data>();
	result->clone(member);
	ast->stack().push_back(SharedReference::unique(result));
}

string var_symbol(AbstractSynatxTree *ast) {

	Reference var = ast->stack().back();
	ast->stack().pop_back();

	return to_string(var);
}

void create_symbol(AbstractSynatxTree *ast, const std::string &symbol, Reference::Flags flags) {

	auto result = ast->symbols().insert({symbol, Reference(flags)});

	if (!result.second) {
		error("symbol %s was already defined in this context", symbol.c_str());
	}
	ast->stack().push_back(&result.first->second);
}

void create_global_symbol(AbstractSynatxTree *ast, const std::string &symbol, Reference::Flags flags) {

}

void array_insert(AbstractSynatxTree *ast) {

	Reference value = ast->stack().back();
	ast->stack().pop_back();
	Reference &array = ast->stack().back().get();

	((Array *)array.data())->values.push_back(value);
}

void hash_insert(AbstractSynatxTree *ast) {

	Reference value = ast->stack().back();
	ast->stack().pop_back();
	Reference key = ast->stack().back();
	ast->stack().pop_back();
	Reference &hash = ast->stack().back().get();

	((Hash *)hash.data())->values.insert({key, value});
}
