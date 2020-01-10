#include "memory/builtin/hash.h"
#include "memory/casttool.h"
#include "memory/memorytool.h"
#include "memory/functiontool.h"
#include "ast/abstractsyntaxtree.h"
#include "ast/cursor.h"

using namespace mint;

HashClass *HashClass::instance() {

	static HashClass g_instance;
	return &g_instance;
}

Hash::Hash() : Object(HashClass::instance()) {

}

Hash::~Hash() {
	invalidateReferenceManager();
}

HashClass::HashClass() : Class("hash", Class::hash) {

	createBuiltinMember(":=", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							SharedReference &rvalue = cursor->stack().at(base);
							SharedReference &self = cursor->stack().at(base - 1);

							self->data<Hash>()->values.clear();
							for (auto &item : to_hash(cursor, rvalue)) {
								hash_insert(self->data<Hash>(), item.first, item.second);
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

							SharedReference rvalue = cursor->stack().at(base);
							SharedReference self = cursor->stack().at(base - 1);

							Reference *result = Reference::create<Hash>();
							result->data<Hash>()->construct();
							for (auto &item : self->data<Hash>()->values) {
								hash_insert(result->data<Hash>(), item.first, item.second);
							}
							for (auto &item : to_hash(cursor, rvalue)) {
								hash_insert(result->data<Hash>(), item.first, item.second);
							}

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().emplace_back(SharedReference::unique(result));
						}));

	createBuiltinMember("[]", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							SharedReference &key = cursor->stack().at(base);
							SharedReference &self = cursor->stack().at(base - 1);

							SharedReference result = hash_get_item(self->data<Hash>(), key);

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().emplace_back(result);
						}));

	createBuiltinMember("[]=", 3, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							SharedReference &value = cursor->stack().at(base);
							SharedReference &key = cursor->stack().at(base - 1);
							SharedReference &self = cursor->stack().at(base - 2);

							SharedReference result = hash_get_item(self->data<Hash>(), key);
							result->move(*value);

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().emplace_back(result);
						}));

	createBuiltinMember("in", 1, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {}));

	createBuiltinMember("in", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							SharedReference value = cursor->stack().at(base);
							SharedReference self = cursor->stack().at(base - 1);
							SharedReference result = create_boolean(self->data<Hash>()->values.find(value) != self->data<Hash>()->values.end());

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().emplace_back(result);
						}));

	createBuiltinMember("()", -2, AbstractSyntaxTree::createBuiltinMethode(metatype(),
																		  "	def (self, key, ...) { \n"
																		  "		return self[key](self, *va_args) \n"
																		  "	}\n"));

	createBuiltinMember("isEmpty", 1, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {
							SharedReference self = cursor->stack().back();
							cursor->stack().back() = create_boolean(self->data<Hash>()->values.empty());
						}));

	createBuiltinMember("size", 1, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {
							SharedReference self = cursor->stack().back();
							cursor->stack().back() = create_number(self->data<Hash>()->values.size());
						}));

	createBuiltinMember("remove", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							SharedReference &rvalue = cursor->stack().at(base);
							SharedReference lvalue = cursor->stack().at(base - 1);

							auto it = lvalue->data<Hash>()->values.find(rvalue);
							if (it != lvalue->data<Hash>()->values.end()) {
								lvalue->data<Hash>()->values.erase(it);
							}

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().emplace_back(lvalue);
						}));

	createBuiltinMember("clear", 1, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							SharedReference self = cursor->stack().at(base);

							self->data<Hash>()->values.clear();

							cursor->stack().pop_back();
							cursor->stack().emplace_back(SharedReference::unique(Reference::create<None>()));
						}));
}
