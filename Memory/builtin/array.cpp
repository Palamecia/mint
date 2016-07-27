#include "Memory/class.h"
#include "Memory/casttool.h"
#include "Memory/memorytool.h"
#include "AbstractSyntaxTree/abstractsyntaxtree.h"

ArrayClass::ArrayClass() : Class("array") {

	createBuiltinMember(":=", 2, AbstractSynatxTree::createBuiltinMethode(ARRAY_TYPE, [] (AbstractSynatxTree *ast) {

							size_t base = get_base(ast);

							Reference &rvalue = ast->stack().at(base).get();
							Reference &lvalue = ast->stack().at(base - 1).get();

							((Array *)lvalue.data())->values = to_array(rvalue);

							ast->stack().pop_back();
						}));

	createBuiltinMember("+", 2, AbstractSynatxTree::createBuiltinMethode(ARRAY_TYPE, [] (AbstractSynatxTree *ast) {

							size_t base = get_base(ast);

							Reference &rvalue = ast->stack().at(base).get();
							Reference &lvalue = ast->stack().at(base - 1).get();

							Reference *result = Reference::create<Array>();
							((Array *)result->data())->construct();
							for (Reference *value : ((Array *)lvalue.data())->values) {
								Reference *copy = new Reference();
								copy->move(*value);
								((Array *)result->data())->values.push_back(copy);
							}
							for (Reference *value : to_array(rvalue)) {
								Reference *copy = new Reference();
								copy->move(*value);
								((Array *)result->data())->values.push_back(copy);
							}

							ast->stack().pop_back();
							ast->stack().pop_back();
							ast->stack().push_back(SharedReference::unique(result));
						}));

	createBuiltinMember("[]", 2, AbstractSynatxTree::createBuiltinMethode(ARRAY_TYPE, [] (AbstractSynatxTree *ast) {

							size_t base = get_base(ast);

							Reference &rvalue = ast->stack().at(base).get();
							Reference &lvalue = ast->stack().at(base - 1).get();

							Reference *result = ((Array *)lvalue.data())->values[to_number(ast, rvalue)];

							ast->stack().pop_back();
							ast->stack().pop_back();
							ast->stack().push_back(result);
						}));

	/// \todo register operator overloads

	createBuiltinMember("size", 1, AbstractSynatxTree::createBuiltinMethode(ARRAY_TYPE, [] (AbstractSynatxTree *ast) {

							Reference &value = ast->stack().back().get();

							Reference *result = Reference::create<Number>();
							((Number *)result->data())->value = ((Array *)value.data())->values.size();

							ast->stack().pop_back();
							ast->stack().push_back(SharedReference::unique(result));
						}));
}
