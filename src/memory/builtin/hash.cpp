#include "memory/builtin/hash.h"
#include "memory/casttool.h"
#include "memory/memorytool.h"
#include "ast/abstractsyntaxtree.h"

HashClass *HashClass::instance() {

	static HashClass *g_instance = new HashClass;

	return g_instance;
}

Hash::Hash() : Object(HashClass::instance()) {}

HashClass::HashClass() : Class("hash", Class::hash) {

	createBuiltinMember(":=", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (AbstractSyntaxTree *ast) {

							size_t base = get_base(ast);

							Reference &rvalue = *ast->stack().at(base);
							Reference &lvalue = *ast->stack().at(base - 1);

							((Hash *)lvalue.data())->values.clear();
							for (auto &item : to_hash(rvalue)) {
								hash_insert((Hash *)lvalue.data(), item.first, item.second);
							}

							ast->stack().pop_back();
						}));

	createBuiltinMember("+", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (AbstractSyntaxTree *ast) {

							size_t base = get_base(ast);

							Reference &rvalue = *ast->stack().at(base);
							Reference &lvalue = *ast->stack().at(base - 1);

							Reference *result = Reference::create<Hash>();
							((Hash *)result->data())->construct();
							for (auto &item : ((Hash *)lvalue.data())->values) {
								hash_insert((Hash *)result->data(), item.first, item.second);
							}
							for (auto &item : to_hash(rvalue)) {
								hash_insert((Hash *)result->data(), item.first, item.second);
							}

							ast->stack().pop_back();
							ast->stack().pop_back();
							ast->stack().push_back(SharedReference::unique(result));
						}));

	createBuiltinMember("[]", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (AbstractSyntaxTree *ast) {

							size_t base = get_base(ast);

							SharedReference &rvalue = ast->stack().at(base);
							Reference &lvalue = *ast->stack().at(base - 1);

							SharedReference result = hash_get_item((Hash *)lvalue.data(), rvalue);

							ast->stack().pop_back();
							ast->stack().pop_back();
							ast->stack().push_back(result);
						}));

	/// \todo register operator overloads

	createBuiltinMember("size", 1, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (AbstractSyntaxTree *ast) {

							Reference &value = *ast->stack().back();

							Reference *result = Reference::create<Number>();
							((Number *)result->data())->value = ((Hash *)value.data())->values.size();

							ast->stack().pop_back();
							ast->stack().push_back(SharedReference::unique(result));
						}));

	createBuiltinMember("erase", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (AbstractSyntaxTree *ast) {

							size_t base = get_base(ast);

							SharedReference &rvalue = ast->stack().at(base);
							SharedReference lvalue = ast->stack().at(base - 1);

							auto it = ((Hash *)lvalue->data())->values.find(rvalue);
							if (it != ((Hash *)lvalue->data())->values.end()) {
								((Hash *)lvalue->data())->values.erase(it);
							}

							ast->stack().pop_back();
							ast->stack().pop_back();
							ast->stack().push_back(lvalue);
						}));
}
