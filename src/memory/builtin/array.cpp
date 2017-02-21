#include "memory/builtin/array.h"
#include "memory/casttool.h"
#include "memory/memorytool.h"
#include "ast/abstractsyntaxtree.h"
#include "system/error.h"

using namespace std;

ArrayClass *ArrayClass::instance() {

	static ArrayClass *g_instance = new ArrayClass;

	return g_instance;
}

Array::Array() : Object(ArrayClass::instance()) {}

ArrayClass::ArrayClass() : Class("array", Class::array) {

	createBuiltinMember(":=", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (AbstractSyntaxTree *ast) {

							size_t base = get_base(ast);

							Reference &rvalue = *ast->stack().at(base);
							Reference &lvalue = *ast->stack().at(base - 1);

							((Array *)lvalue.data())->values.clear();
							for (auto &item : to_array(rvalue)) {
								array_append((Array *)lvalue.data(), item);
							}

							ast->stack().pop_back();
						}));

	createBuiltinMember("+", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (AbstractSyntaxTree *ast) {

							size_t base = get_base(ast);

							Reference &rvalue = *ast->stack().at(base);
							Reference &lvalue = *ast->stack().at(base - 1);

							Reference *result = Reference::create<Array>();
							((Array *)result->data())->construct();
							for (auto &value : ((Array *)lvalue.data())->values) {
								array_append((Array *)result->data(), value);
							}
							for (auto &value : to_array(rvalue)) {
								array_append((Array *)result->data(), value);
							}

							ast->stack().pop_back();
							ast->stack().pop_back();
							ast->stack().push_back(SharedReference::unique(result));
						}));

	createBuiltinMember("[]", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (AbstractSyntaxTree *ast) {

							size_t base = get_base(ast);

							Reference &rvalue = *ast->stack().at(base);
							Reference &lvalue = *ast->stack().at(base - 1);

							SharedReference result = array_get_item((Array *)lvalue.data(), to_number(ast, rvalue));

							ast->stack().pop_back();
							ast->stack().pop_back();
							ast->stack().push_back(result);
						}));

	/// \todo register operator overloads

	createBuiltinMember("size", 1, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (AbstractSyntaxTree *ast) {

							Reference &value = *ast->stack().back();

							Reference *result = Reference::create<Number>();
							((Number *)result->data())->value = ((Array *)value.data())->values.size();

							ast->stack().pop_back();
							ast->stack().push_back(SharedReference::unique(result));
						}));

	createBuiltinMember("erase", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (AbstractSyntaxTree *ast) {

							size_t base = get_base(ast);

							SharedReference &rvalue = ast->stack().at(base);
							SharedReference lvalue = ast->stack().at(base - 1);

							Array *array = (Array *)lvalue->data();
							array->values.erase(array->values.begin() + array_index(array, to_number(ast, *rvalue)));

							ast->stack().pop_back();
							ast->stack().pop_back();
							ast->stack().push_back(lvalue);
						}));
}
