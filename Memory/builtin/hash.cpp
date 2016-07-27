#include "Memory/class.h"
#include "Memory/casttool.h"
#include "Memory/memorytool.h"
#include "AbstractSyntaxTree/abstractsyntaxtree.h"

HashClass::HashClass() : Class("hash") {

	createBuiltinMember(":=", 2, AbstractSynatxTree::createBuiltinMethode(HASH_TYPE, [] (AbstractSynatxTree *ast) {

							size_t base = get_base(ast);

							Reference &rvalue = ast->stack().at(base).get();
							Reference &lvalue = ast->stack().at(base - 1).get();

							((Hash *)lvalue.data())->values.clear();
							for (auto item : to_hash(rvalue)) {
								((Hash *)lvalue.data())->values.insert({item.first, item.second});
							}

							ast->stack().pop_back();
						}));

	createBuiltinMember("+", 2, AbstractSynatxTree::createBuiltinMethode(HASH_TYPE, [] (AbstractSynatxTree *ast) {

							size_t base = get_base(ast);

							Reference &rvalue = ast->stack().at(base).get();
							Reference &lvalue = ast->stack().at(base - 1).get();

							Reference *result = Reference::create<Hash>();
							((Hash *)result->data())->construct();
							for (auto value : ((Hash *)lvalue.data())->values) {
								Reference key;
								Reference item;
								key.move(value.first);
								item.move(value.second);
								((Hash *)result->data())->values.insert({key, item});
							}
							for (auto value : to_hash(rvalue)) {
								Reference key;
								Reference item;
								key.move(value.first);
								item.move(value.second);
								((Hash *)result->data())->values.insert({key, item});
							}

							ast->stack().pop_back();
							ast->stack().pop_back();
							ast->stack().push_back(SharedReference::unique(result));
						}));

	createBuiltinMember("[]", 2, AbstractSynatxTree::createBuiltinMethode(HASH_TYPE, [] (AbstractSynatxTree *ast) {

							size_t base = get_base(ast);

							Reference &rvalue = ast->stack().at(base).get();
							Reference &lvalue = ast->stack().at(base - 1).get();

							Reference *result = &((Hash *)lvalue.data())->values[rvalue];

							ast->stack().pop_back();
							ast->stack().pop_back();
							ast->stack().push_back(result);
						}));

	/// \todo register operator overloads

	createBuiltinMember("size", 1, AbstractSynatxTree::createBuiltinMethode(HASH_TYPE, [] (AbstractSynatxTree *ast) {

							Reference &value = ast->stack().back().get();

							Reference *result = Reference::create<Number>();
							((Number *)result->data())->value = ((Hash *)value.data())->values.size();

							ast->stack().pop_back();
							ast->stack().push_back(SharedReference::unique(result));
						}));
}
