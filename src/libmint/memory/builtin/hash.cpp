#include "memory/builtin/hash.h"
#include "memory/builtin/iterator.h"
#include "memory/functiontool.h"
#include "memory/casttool.h"
#include "ast/abstractsyntaxtree.h"
#include "ast/cursor.h"
#include "system/error.h"

#include <iterator>

using namespace mint;
using namespace std;

HashClass *HashClass::instance() {
	return GlobalData::instance()->builtin<HashClass>(Class::hash);
}

Hash::Hash() : Object(HashClass::instance()) {

}

Hash::Hash(const Hash &other) : Object(HashClass::instance()) {
	for (auto i = other.values.begin(); i != other.values.end(); ++i) {
		values.emplace(hash_key(i->first), hash_value(i->second));
	}
}

void Hash::mark() {
	if (!markedBit()) {
		Object::mark();
		for (auto &item : values) {
			item.first.data()->mark();
			item.second.data()->mark();
		}
	}
}

HashClass::HashClass() : Class("hash", Class::hash) {

	AbstractSyntaxTree *ast = AbstractSyntaxTree::instance();

	createBuiltinMember(copy_operator, ast->createBuiltinMethode(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							Reference &rvalue = load_from_stack(cursor, base);
							Reference &self = load_from_stack(cursor, base - 1);

							self.data<Hash>()->values = to_hash(cursor, rvalue);
							cursor->stack().pop_back();
						}));

	createBuiltinMember(eq_operator, ast->createBuiltinMethode(this, 2, R"""(
						def (const self, const other) {
							if typeof self == typeof other {
								if self.size() == other.size() {
									for key, value in self {
										if key not in other {
											return false
										}
										if value != other[key] {
											return false
										}
									}
									return true
								}
							}
							return false
						})"""));

	createBuiltinMember(ne_operator, ast->createBuiltinMethode(this, 2, R"""(
						def (const self, const other) {
							if typeof self == typeof other {
								if self.size() == other.size() {
									for key, value in self {
										if key not in other {
											return true
										}
										if value != other[key] {
											return true
										}
									}
									return false
								}
							}
							return true
						})"""));

	createBuiltinMember(add_operator, ast->createBuiltinMethode(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							Reference &rvalue = load_from_stack(cursor, base);
							Reference &self = load_from_stack(cursor, base - 1);
							WeakReference result = create_hash();

							for (auto &item : self.data<Hash>()->values) {
								hash_insert(result.data<Hash>(), item.first, hash_get_value(item));
							}
							for (auto &item : to_hash(cursor, rvalue)) {
								hash_insert(result.data<Hash>(), item.first, hash_get_value(item));
							}

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().emplace_back(std::forward<Reference>(result));
						}));

	createBuiltinMember(subscript_operator, ast->createBuiltinMethode(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							WeakReference &key = load_from_stack(cursor, base);
							Reference &self = load_from_stack(cursor, base - 1);
							WeakReference result = hash_get_item(self.data<Hash>(), key);

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().emplace_back(std::forward<Reference>(result));
						}));

	createBuiltinMember(subscript_move_operator, ast->createBuiltinMethode(this, 3, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							Reference &value = load_from_stack(cursor, base);
							WeakReference &key = load_from_stack(cursor, base - 1);
							Reference &self = load_from_stack(cursor, base - 2);
							WeakReference result = hash_get_item(self.data<Hash>(), key);

							result.move(value);

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().emplace_back(std::forward<Reference>(result));
						}));

	createBuiltinMember(in_operator, ast->createBuiltinMethode(this, 1, [] (Cursor *cursor) {
							cursor->stack().back() = WeakReference(Reference::const_address, iterator_init(cursor->stack().back()));
						}));

	createBuiltinMember(in_operator, ast->createBuiltinMethode(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							WeakReference &value = load_from_stack(cursor, base);
							Reference &self = load_from_stack(cursor, base - 1);
							WeakReference result = WeakReference::create<Boolean>(self.data<Hash>()->values.find(value) != self.data<Hash>()->values.end());

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().emplace_back(std::forward<Reference>(result));
						}));

	createBuiltinMember("each", ast->createBuiltinMethode(this, 2, R"""(
						def (const self, const func) {
							unpack_func = func[2]
							if defined unpack_func {
								for key, value in self {
									unpack_func(key, value)
								}
							} else {
								for item in self {
									func(item)
								}
							}
						})"""));

	createBuiltinMember(call_operator, ast->createBuiltinMethode(this, VARIADIC 2, R"""(
						def (const self, const key, ...) {
							return self[key](self, *va_args)
						})"""));

	createBuiltinMember("isEmpty", ast->createBuiltinMethode(this, 1, [] (Cursor *cursor) {
							cursor->stack().back() = WeakReference::create<Boolean>(cursor->stack().back().data<Hash>()->values.empty());
						}));

	createBuiltinMember("size", ast->createBuiltinMethode(this, 1, [] (Cursor *cursor) {
							cursor->stack().back() = WeakReference::create<Number>(static_cast<double>(cursor->stack().back().data<Hash>()->values.size()));
						}));

	createBuiltinMember("remove", ast->createBuiltinMethode(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							WeakReference &key = load_from_stack(cursor, base);
							Reference &self = load_from_stack(cursor, base - 1);

							auto it = self.data<Hash>()->values.find(key);
							if (it != self.data<Hash>()->values.end()) {
								self.data<Hash>()->values.erase(it);
							}

							cursor->stack().pop_back();
						}));

	createBuiltinMember("clear", ast->createBuiltinMethode(this, 1, [] (Cursor *cursor) {
							Reference &self = cursor->stack().back();
							if (UNLIKELY(self.flags() & Reference::const_value)) {
								error("invalid modification of constant value");
							}
							self.data<Hash>()->values.clear();
							cursor->stack().back() = WeakReference::create<None>();
						}));
}

void mint::hash_new(Cursor *cursor, size_t length) {

	auto &stack = cursor->stack();

	Hash *self(Reference::alloc<Hash>());
	self->values.reserve(length);
	self->construct();

	const auto from = std::prev(stack.end(), length * 2);
	const auto to = stack.end();
	for (auto it = from; it != to; it = std::next(it, 2)) {
		hash_insert(self, hash_key(*it), hash_value(*std::next(it)));
	}

	stack.erase(from, to);
	stack.emplace_back(Reference::const_address, self);
}

Hash::values_type::iterator mint::hash_insert(Hash *hash, const Hash::key_type &key, const Reference &value) {
	return hash->values.emplace(hash_key(key), hash_value(value)).first;
}

WeakReference mint::hash_get_item(Hash *hash, const Hash::key_type &key) {

	auto i = hash->values.find(key);

	if (i == hash->values.end()) {
		i = hash_insert(hash, key, WeakReference::create<None>());
	}

	return WeakReference::share(i->second);
}

WeakReference mint::hash_get_item(Hash *hash, Hash::key_type &key) {

	auto i = hash->values.find(key);

	if (i == hash->values.end()) {
		i = hash_insert(hash, key, WeakReference::create<None>());
	}

	return WeakReference::share(i->second);
}

WeakReference mint::hash_get_key(Hash::values_type::iterator &it) {
	return WeakReference(it->first.flags(), it->first.data());
}

WeakReference mint::hash_get_key(Hash::values_type::value_type &item) {
	return WeakReference(item.first.flags(), item.first.data());
}

WeakReference mint::hash_get_value(Hash::values_type::iterator &it) {
	return WeakReference::share(it->second);
}

WeakReference mint::hash_get_value(Hash::values_type::value_type &item) {
	return WeakReference::share(item.second);
}

Hash::key_type mint::hash_key(const Reference &key) {
	return WeakReference(Reference::const_address | Reference::const_value, key.data());
}

WeakReference mint::hash_value(const Reference &value) {

	WeakReference item_value;

	if ((value.flags() & (Reference::const_value | Reference::temporary)) == Reference::const_value) {
		item_value.copy(value);
	}
	else {
		item_value.move(value);
	}

	return item_value;
}
