#include "memorytool.h"
#include "globaldata.h"
#include "class.h"
#include "object.h"
#include "AbstractSyntaxTree/abstractsyntaxtree.h"

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
	return new Printer;
}

void print(Printer *printer, const Reference &ref) {

	if (printer) {
		switch (ref.data()->format) {
		case Data::fmt_null:
		case Data::fmt_none:
			printf("%s", nullptr);
			break;
		case Data::fmt_number:
			printf("%g", ((Number*)ref.data())->data);
			break;
		case Data::fmt_object:
			if (((Object *)ref.data())->metadata == StringClass::instance()) {
				printf("%s", ((String *)ref.data())->str.c_str());
			}
			else {
				printf("%p", ref.data());
			}
		default:
			break;
		}
	}
}

Reference *get_symbol_reference(SymbolTable *symbols, const std::string &symbol) {

	auto it = GlobalData::instance().symbols().find(symbol);
	if (it != GlobalData::instance().symbols().end()) {
		return &it->second;
	}

	return &(*symbols)[symbol];
}

Reference *get_object_member(Object *object, const std::string &member) {

	/// \todo find first in global members

	return &object->data[object->metadata->members()[member].offset];
}

void create_symbol(AbstractSynatxTree *ast, const std::string &symbol, Reference::Flags flags) {

	auto result = ast->symbols().insert({symbol, Reference(flags)});

	if (!result.second) {
		/// \todo error
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
