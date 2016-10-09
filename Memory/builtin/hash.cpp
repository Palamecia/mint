#include "Memory/class.h"
#include "Memory/casttool.h"
#include "Memory/memorytool.h"
#include "AbstractSyntaxTree/abstractsyntaxtree.h"

HashClass::HashClass() : Class("hash") {

	createBuiltinMember(":=", 2, AbstractSynatxTree::createBuiltinMethode(HASH_TYPE, [] (AbstractSynatxTree *ast) {

							size_t base = get_base(ast);

							Reference &rvalue = *ast->stack().at(base);
							Reference &lvalue = *ast->stack().at(base - 1);

							((Hash *)lvalue.data())->values.clear();
							for (auto &item : to_hash(rvalue)) {
								((Hash *)lvalue.data())->values.insert({item.first.get(), item.second.get()});
							}

							ast->stack().pop_back();
						}));

	createBuiltinMember("+", 2, AbstractSynatxTree::createBuiltinMethode(HASH_TYPE, [] (AbstractSynatxTree *ast) {

							size_t base = get_base(ast);

							Reference &rvalue = *ast->stack().at(base);
							Reference &lvalue = *ast->stack().at(base - 1);

							Reference *result = Reference::create<Hash>();
							((Hash *)result->data())->construct();
							for (auto &item : ((Hash *)lvalue.data())->values) {
								((Hash *)result->data())->values.insert({item.first.get(), item.second.get()});
							}
							for (auto &item : to_hash(rvalue)) {
								((Hash *)result->data())->values.insert({item.first.get(), item.second.get()});
							}

							ast->stack().pop_back();
							ast->stack().pop_back();
							ast->stack().push_back(SharedReference::unique(result));
						}));

	createBuiltinMember("[]", 2, AbstractSynatxTree::createBuiltinMethode(HASH_TYPE, [] (AbstractSynatxTree *ast) {

							size_t base = get_base(ast);

							Reference &rvalue = *ast->stack().at(base);
							Reference &lvalue = *ast->stack().at(base - 1);

							SharedReference result = ((Hash *)lvalue.data())->values[SharedReference(&rvalue)].get();

							ast->stack().pop_back();
							ast->stack().pop_back();
							ast->stack().push_back(result);
						}));

	/// \todo register operator overloads

	createBuiltinMember("size", 1, AbstractSynatxTree::createBuiltinMethode(HASH_TYPE, [] (AbstractSynatxTree *ast) {

							Reference &value = *ast->stack().back();

							Reference *result = Reference::create<Number>();
							((Number *)result->data())->value = ((Hash *)value.data())->values.size();

							ast->stack().pop_back();
							ast->stack().push_back(SharedReference::unique(result));
						}));
}
