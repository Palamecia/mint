#include "memory/builtin/hash.h"
#include "memory/casttool.h"
#include "memory/memorytool.h"
#include "ast/abstractsyntaxtree.h"
#include "ast/cursor.h"

using namespace mint;

HashClass *HashClass::instance() {

	static HashClass g_instance;
	return &g_instance;
}

Hash::Hash() : Object(HashClass::instance()) {}

HashClass::HashClass() : Class("hash", Class::hash) {

	createBuiltinMember(":=", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							Reference &rvalue = *cursor->stack().at(base);
							Reference &self = *cursor->stack().at(base - 1);

							self.data<Hash>()->values.clear();
							for (auto &item : to_hash(cursor, rvalue)) {
								hash_insert(self.data<Hash>(), item.first, item.second);
							}

							cursor->stack().pop_back();
						}));

	createBuiltinMember("==", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(),
																		  "	def (self, other) {\n"
																		  "		if typeof self == typeof other {\n"
																		  "			if self.size() == other.size() {\n"
																		  "				for item in self {\n"
																		  "					if item not in other {\n"
																		  "						return false\n"
																		  "					}\n"
																		  "					if self[item] != other[item] {\n"
																		  "						return false\n"
																		  "					}\n"
																		  "				}\n"
																		  "				return true\n"
																		  "			}\n"
																		  "		}\n"
																		  "		return false\n"
																		  "	}\n"));

	createBuiltinMember("!=", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(),
																		  "	def (self, other) {\n"
																		  "		if typeof self == typeof other {\n"
																		  "			if self.size() == other.size() {\n"
																		  "				for item in self {\n"
																		  "					if item not in other {\n"
																		  "						return true\n"
																		  "					}\n"
																		  "					if self[item] != other[item] {\n"
																		  "						return true\n"
																		  "					}\n"
																		  "				}\n"
																		  "				return false\n"
																		  "			}\n"
																		  "		}\n"
																		  "		return true\n"
																		  "	}\n"));

	createBuiltinMember("+", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							Reference &rvalue = *cursor->stack().at(base);
							Reference &self = *cursor->stack().at(base - 1);

							Reference *result = Reference::create<Hash>();
							result->data<Hash>()->construct();
							for (auto &item : self.data<Hash>()->values) {
								hash_insert(result->data<Hash>(), item.first, item.second);
							}
							for (auto &item : to_hash(cursor, rvalue)) {
								hash_insert(result->data<Hash>(), item.first, item.second);
							}

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().push_back(SharedReference::unique(result));
						}));

	createBuiltinMember("[]", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							SharedReference &rvalue = cursor->stack().at(base);
							Reference &lvalue = *cursor->stack().at(base - 1);

							SharedReference result = hash_get_item(lvalue.data<Hash>(), rvalue);

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().push_back(result);
						}));

	createBuiltinMember("size", 1, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							Reference &self = *cursor->stack().back();

							Reference *result = Reference::create<Number>();
							result->data<Number>()->value = self.data<Hash>()->values.size();

							cursor->stack().pop_back();
							cursor->stack().push_back(SharedReference::unique(result));
						}));

	createBuiltinMember("erase", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							SharedReference &rvalue = cursor->stack().at(base);
							SharedReference lvalue = cursor->stack().at(base - 1);

							auto it = lvalue->data<Hash>()->values.find(rvalue);
							if (it != lvalue->data<Hash>()->values.end()) {
								lvalue->data<Hash>()->values.erase(it);
							}

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().push_back(lvalue);
						}));

	createBuiltinMember("clear", 1, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							SharedReference self = cursor->stack().at(base);

							self->data<Hash>()->values.clear();

							cursor->stack().pop_back();
							cursor->stack().push_back(SharedReference::unique(Reference::create<None>()));
						}));
}
