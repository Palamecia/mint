#include "memory/builtin/hash.h"
#include "memory/casttool.h"
#include "memory/memorytool.h"
#include "memory/functiontool.h"
#include "ast/abstractsyntaxtree.h"
#include "ast/cursor.h"
#include "system/error.h"

using namespace mint;
using namespace std;

HashClass *HashClass::instance() {

	static HashClass g_instance;
	return &g_instance;
}

Hash::Hash() : Object(HashClass::instance()) {

}

Hash::~Hash() {
	invalidateReferenceManager();
}

void Hash::mark() {
	if (!markedBit()) {
		Object::mark();
		for (auto &item : values) {
			item.first->data()->mark();
			item.second->data()->mark();
		}
	}
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
																		  "				for key, value in self {\n"
																		  "					if key not in other {\n"
																		  "						return false\n"
																		  "					}\n"
																		  "					if value != other[key] {\n"
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
																		  "				for key, value in self {\n"
																		  "					if key not in other {\n"
																		  "						return true\n"
																		  "					}\n"
																		  "					if value != other[key] {\n"
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

							SharedReference rvalue = move(cursor->stack().at(base));
							SharedReference self = move(cursor->stack().at(base - 1));

							Reference *result = StrongReference::create<Hash>();
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
							cursor->stack().emplace_back(move(result));
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
							cursor->stack().emplace_back(move(result));
						}));

	createBuiltinMember("in", 1, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {
							SharedReference self = move(cursor->stack().back());
							cursor->stack().back() = SharedReference::unique(StrongReference::create(iterator_init(self)));
						}));

	createBuiltinMember("in", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							SharedReference value = move(cursor->stack().at(base));
							SharedReference self = move(cursor->stack().at(base - 1));
							SharedReference result = create_boolean(self->data<Hash>()->values.find(value) != self->data<Hash>()->values.end());

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().emplace_back(move(result));
						}));

	createBuiltinMember("each", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(),
																			"	def (self, func) {\n"
																			"		unpack_func = func[2]\n"
																			"		if defined unpack_func {\n"
																			"			for key, value in self {\n"
																			"				unpack_func(key, value)\n"
																			"			}\n"
																			"		} else {\n"
																			"			for item in self {\n"
																			"				func(item)\n"
																			"			}\n"
																			"		}\n"
																			"	}\n"));

	createBuiltinMember("()", -2, AbstractSyntaxTree::createBuiltinMethode(metatype(),
																		  "	def (self, key, ...) { \n"
																		  "		return self[key](self, *va_args) \n"
																		  "	}\n"));

	createBuiltinMember("isEmpty", 1, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {
							SharedReference self = move(cursor->stack().back());
							cursor->stack().back() = create_boolean(self->data<Hash>()->values.empty());
						}));

	createBuiltinMember("size", 1, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {
							SharedReference self = move(cursor->stack().back());
							cursor->stack().back() = create_number(static_cast<double>(self->data<Hash>()->values.size()));
						}));

	createBuiltinMember("remove", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							SharedReference &rvalue = cursor->stack().at(base);
							SharedReference lvalue = move(cursor->stack().at(base - 1));

							auto it = lvalue->data<Hash>()->values.find(rvalue);
							if (it != lvalue->data<Hash>()->values.end()) {
								lvalue->data<Hash>()->values.erase(it);
							}

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().emplace_back(move(lvalue));
						}));

	createBuiltinMember("clear", 1, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {
							SharedReference self = move(cursor->stack().back());
							if (self->flags() & Reference::const_value) {
								error("invalid modification of constant value");
							}
							self->data<Hash>()->values.clear();
							cursor->stack().back() = SharedReference::unique(StrongReference::create<None>());
						}));
}
