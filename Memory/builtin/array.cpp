#include "Memory/class.h"
#include "Memory/casttool.h"
#include "Memory/memorytool.h"
#include "AbstractSyntaxTree/abstractsyntaxtree.h"

using namespace std;

ArrayClass::ArrayClass() : Class("array") {

	createBuiltinMember(":=", 2, AbstractSynatxTree::createBuiltinMethode(ARRAY_TYPE, [] (AbstractSynatxTree *ast) {

							size_t base = get_base(ast);

							Reference &rvalue = ast->stack().at(base).get();
							Reference &lvalue = ast->stack().at(base - 1).get();

							((Array *)lvalue.data())->values.clear();
							for (auto &item : to_array(rvalue)) {
								((Array *)lvalue.data())->values.push_back(unique_ptr<Reference>(new Reference(*item)));
							}

							ast->stack().pop_back();
						}));

	createBuiltinMember("+", 2, AbstractSynatxTree::createBuiltinMethode(ARRAY_TYPE, [] (AbstractSynatxTree *ast) {

							size_t base = get_base(ast);

							Reference &rvalue = ast->stack().at(base).get();
							Reference &lvalue = ast->stack().at(base - 1).get();

							Reference *result = Reference::create<Array>();
							((Array *)result->data())->construct();
							for (auto &value : ((Array *)lvalue.data())->values) {
								((Array *)result->data())->values.push_back(unique_ptr<Reference>(new Reference(*value)));
							}
							for (auto &value : to_array(rvalue)) {
								((Array *)result->data())->values.push_back(unique_ptr<Reference>(new Reference(*value)));
							}

							ast->stack().pop_back();
							ast->stack().pop_back();
							ast->stack().push_back(SharedReference::unique(result));
						}));

	createBuiltinMember("[]", 2, AbstractSynatxTree::createBuiltinMethode(ARRAY_TYPE, [] (AbstractSynatxTree *ast) {

							size_t base = get_base(ast);

							Reference &rvalue = ast->stack().at(base).get();
							Reference &lvalue = ast->stack().at(base - 1).get();

							Reference *result = ((Array *)lvalue.data())->values[to_number(ast, rvalue)].get();

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
