#include "memory/builtin/hash.h"
#include "memory/casttool.h"
#include "memory/memorytool.h"
#include "memory/functiontool.h"
#include "ast/abstractsyntaxtree.h"
#include "ast/cursor.h"
#include "system/error.h"

#include <iterator>

using namespace mint;
using namespace std;

HashClass *HashClass::instance() {

	static HashClass g_instance;
	return &g_instance;
}

Hash::Hash() : Object(HashClass::instance()) {

}

Hash::Hash(const Hash &other) : Object(HashClass::instance()) {
	for (auto i = other.values.begin(); i != other.values.end(); ++i) {
		values.emplace(hash_key(i->first), i->second);
	}
}

void Hash::mark() {
	if (!markedBit()) {
		Object::mark();
		for (auto &item : values) {
			item.first->data()->mark();
			item.second.data()->mark();
		}
	}
}

HashClass::HashClass() : Class("hash", Class::hash) {

	createBuiltinMember(Symbol::CopyOperator, AbstractSyntaxTree::createBuiltinMethode(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							SharedReference &rvalue = load_from_stack(cursor, base);
							SharedReference &self = load_from_stack(cursor, base - 1);

							self->data<Hash>()->values = to_hash(cursor, rvalue);
							cursor->stack().pop_back();
						}));

	createBuiltinMember(Symbol::EqOperator, AbstractSyntaxTree::createBuiltinMethode(this, 2,
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

	createBuiltinMember(Symbol::NeOperator, AbstractSyntaxTree::createBuiltinMethode(this, 2,
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

	createBuiltinMember(Symbol::AddOperator, AbstractSyntaxTree::createBuiltinMethode(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							SharedReference rvalue = move_from_stack(cursor, base);
							SharedReference self = move_from_stack(cursor, base - 1);

							SharedReference result = create_hash();
							for (auto &item : self->data<Hash>()->values) {
								hash_insert(result->data<Hash>(), item.first, hash_get_value(item));
							}
							for (auto &item : to_hash(cursor, rvalue)) {
								hash_insert(result->data<Hash>(), item.first, hash_get_value(item));
							}

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().emplace_back(move(result));
						}));

	createBuiltinMember(Symbol::SubscriptOperator, AbstractSyntaxTree::createBuiltinMethode(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							SharedReference &key = load_from_stack(cursor, base);
							SharedReference &self = load_from_stack(cursor, base - 1);

							SharedReference result = hash_get_item(self->data<Hash>(), key);

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().emplace_back(move(result));
						}));

	createBuiltinMember(Symbol::SubscriptMoveOperator, AbstractSyntaxTree::createBuiltinMethode(this, 3, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							SharedReference &value = load_from_stack(cursor, base);
							SharedReference &key = load_from_stack(cursor, base - 1);
							SharedReference &self = load_from_stack(cursor, base - 2);

							SharedReference result = hash_get_item(self->data<Hash>(), key);
							result->move(*value);

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().emplace_back(move(result));
						}));

	createBuiltinMember(Symbol::InOperator, AbstractSyntaxTree::createBuiltinMethode(this, 1, [] (Cursor *cursor) {
							cursor->stack().back() = SharedReference::strong(iterator_init(cursor->stack().back()));
						}));

	createBuiltinMember(Symbol::InOperator, AbstractSyntaxTree::createBuiltinMethode(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							SharedReference value = move_from_stack(cursor, base);
							SharedReference self = move_from_stack(cursor, base - 1);
							SharedReference result = SharedReference::strong<Boolean>(self->data<Hash>()->values.find(value) != self->data<Hash>()->values.end());

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().emplace_back(move(result));
						}));

	createBuiltinMember("each", AbstractSyntaxTree::createBuiltinMethode(this, 2,
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

	createBuiltinMember(Symbol::CallOperator, AbstractSyntaxTree::createBuiltinMethode(this, VARIADIC 2,
																		  "	def (self, key, ...) { \n"
																		  "		return self[key](self, *va_args) \n"
																		  "	}\n"));

	createBuiltinMember("isEmpty", AbstractSyntaxTree::createBuiltinMethode(this, 1, [] (Cursor *cursor) {
							SharedReference self = move(cursor->stack().back());
							cursor->stack().back() = SharedReference::strong<Boolean>(self->data<Hash>()->values.empty());
						}));

	createBuiltinMember("size", AbstractSyntaxTree::createBuiltinMethode(this, 1, [] (Cursor *cursor) {
							SharedReference self = move(cursor->stack().back());
							cursor->stack().back() = SharedReference::strong<Number>(static_cast<double>(self->data<Hash>()->values.size()));
						}));

	createBuiltinMember("remove", AbstractSyntaxTree::createBuiltinMethode(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							SharedReference key = move_from_stack(cursor, base);
							SharedReference &self = load_from_stack(cursor, base - 1);

							auto it = self->data<Hash>()->values.find(key);
							if (it != self->data<Hash>()->values.end()) {
								self->data<Hash>()->values.erase(it);
							}

							cursor->stack().pop_back();
						}));

	createBuiltinMember("clear", AbstractSyntaxTree::createBuiltinMethode(this, 1, [] (Cursor *cursor) {
							SharedReference self = move(cursor->stack().back());
							if (UNLIKELY(self->flags() & Reference::const_value)) {
								error("invalid modification of constant value");
							}
							self->data<Hash>()->values.clear();
							cursor->stack().back() = SharedReference::strong<None>();
						}));
}

void mint::hash_insert_from_stack(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);

	SharedReference &value = load_from_stack(cursor, base);
	SharedReference &key = load_from_stack(cursor, base - 1);
	SharedReference &hash = load_from_stack(cursor, base - 2);

	hash_insert(hash->data<Hash>(), key, value);
	cursor->stack().pop_back();
	cursor->stack().pop_back();
}

Hash::values_type::iterator mint::hash_insert(Hash *hash, const Hash::key_type &key, const SharedReference &value) {
	return hash->values.emplace(hash_key(key), hash_value(value)).first;
}

SharedReference mint::hash_get_item(Hash *hash, const Hash::key_type &key) {

	auto i = hash->values.find(key);

	if (i == hash->values.end()) {
		i = hash_insert(hash, key, SharedReference::strong<None>());
	}

	return SharedReference::weak(i->second);
}

SharedReference mint::hash_get_key(Hash::values_type::iterator &it) {
	return SharedReference::weak(*it->first);
}

SharedReference mint::hash_get_key(Hash::values_type::value_type &item) {
	return SharedReference::weak(*item.first);
}

SharedReference mint::hash_get_value(Hash::values_type::iterator &it) {
	return SharedReference::weak(it->second);
}

SharedReference mint::hash_get_value(Hash::values_type::value_type &item) {
	return SharedReference::weak(item.second);
}

Hash::key_type mint::hash_key(const SharedReference &key) {
	return SharedReference::strong(Reference::const_address | Reference::const_value, key->data());
}

WeakReference mint::hash_value(const SharedReference &value) {

	WeakReference item_value;

	if (value->flags() & Reference::const_value) {
		item_value.copy(*value);
	}
	else {
		item_value.move(*value);
	}

	return item_value;
}

WeakReference mint::hash_value(const Reference &value) {

	WeakReference item_value;

	if (value.flags() & Reference::const_value) {
		item_value.copy(value);
	}
	else {
		item_value.move(value);
	}

	return item_value;
}
