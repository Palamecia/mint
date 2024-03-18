/**
 * Copyright (c) 2024 Gauvain CHERY.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "mint/memory/builtin/hash.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/memory/functiontool.h"
#include "mint/memory/casttool.h"
#include "mint/ast/abstractsyntaxtree.h"
#include "mint/ast/cursor.h"
#include "mint/system/error.h"

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
	if (!marked_bit()) {
		Object::mark();
		for (auto &[key, value] : values) {
			key.data()->mark();
			value.data()->mark();
		}
	}
}

HashClass::HashClass() : Class("hash", Class::hash) {

	AbstractSyntaxTree *ast = AbstractSyntaxTree::instance();
	
	create_builtin_member(copy_operator, ast->create_builtin_method(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							Reference &rvalue = load_from_stack(cursor, base);
							Reference &self = load_from_stack(cursor, base - 1);

							self.data<Hash>()->values = to_hash(cursor, rvalue);
							cursor->stack().pop_back();
						}));
	
	create_builtin_member(eq_operator, ast->create_builtin_method(this, 2, R"""(
						def (const self, const other) {
							if typeof self == typeof other {
								if self.size() == other.size() {
									for let var (key, value) in self {
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
	
	create_builtin_member(ne_operator, ast->create_builtin_method(this, 2, R"""(
						def (const self, const other) {
							if typeof self == typeof other {
								if self.size() == other.size() {
									for let var (key, value) in self {
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
	
	create_builtin_member(add_operator, ast->create_builtin_method(this, 2, [] (Cursor *cursor) {

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
	
	create_builtin_member(subscript_operator, ast->create_builtin_method(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							WeakReference &key = load_from_stack(cursor, base);
							Reference &self = load_from_stack(cursor, base - 1);
							WeakReference result = hash_get_item(self.data<Hash>(), key);

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().emplace_back(std::forward<Reference>(result));
						}));
	
	create_builtin_member(subscript_move_operator, ast->create_builtin_method(this, 3, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							Reference &value = load_from_stack(cursor, base);
							WeakReference &key = load_from_stack(cursor, base - 1);
							Reference &self = load_from_stack(cursor, base - 2);
							WeakReference result = hash_get_item(self.data<Hash>(), key);

							result.move_data(value);

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().emplace_back(std::forward<Reference>(result));
						}));
	
	create_builtin_member(in_operator, ast->create_builtin_method(this, 1, [] (Cursor *cursor) {
							cursor->stack().back() = WeakReference(Reference::const_address, iterator_init(cursor->stack().back()));
						}));
	
	create_builtin_member(in_operator, ast->create_builtin_method(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							WeakReference &value = load_from_stack(cursor, base);
							Reference &self = load_from_stack(cursor, base - 1);
							WeakReference result = WeakReference::create<Boolean>(self.data<Hash>()->values.find(value) != self.data<Hash>()->values.end());

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().emplace_back(std::forward<Reference>(result));
						}));
	
	create_builtin_member("get", ast->create_builtin_method(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							WeakReference &key = load_from_stack(cursor, base);
							Reference &self = load_from_stack(cursor, base - 1);

							auto i = self.data<Hash>()->values.find(key);
							WeakReference result = i != self.data<Hash>()->values.end() ? hash_get_value(i) : WeakReference::create<None>();

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().emplace_back(std::forward<Reference>(result));
						}));
	
	create_builtin_member("get", ast->create_builtin_method(this, 3, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							Reference &default_value = load_from_stack(cursor, base);
							WeakReference &key = load_from_stack(cursor, base - 1);
							Reference &self = load_from_stack(cursor, base - 2);

							auto i = self.data<Hash>()->values.find(key);
							WeakReference result = i != self.data<Hash>()->values.end() ? hash_get_value(i) : WeakReference::share(default_value);

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().emplace_back(std::forward<Reference>(result));
						}));
	
	create_builtin_member("each", ast->create_builtin_method(this, 2, R"""(
						def (const self, const func) {
							var unpack_func = func[2]
							if defined unpack_func {
								for let var (key, value) in self {
									unpack_func(key, value)
								}
							} else {
								for let var item in self {
									func(item)
								}
							}
						})"""));
	
	create_builtin_member(call_operator, ast->create_builtin_method(this, VARIADIC 2, R"""(
						def (const self, const key, ...) {
							return self[key](self, *va_args)
						})"""));
	
	create_builtin_member("isEmpty", ast->create_builtin_method(this, 1, [] (Cursor *cursor) {
							cursor->stack().back() = WeakReference::create<Boolean>(cursor->stack().back().data<Hash>()->values.empty());
						}));
	
	create_builtin_member("size", ast->create_builtin_method(this, 1, [] (Cursor *cursor) {
							cursor->stack().back() = WeakReference::create<Number>(static_cast<double>(cursor->stack().back().data<Hash>()->values.size()));
						}));
	
	create_builtin_member("remove", ast->create_builtin_method(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							WeakReference &key = load_from_stack(cursor, base);
							Reference &self = load_from_stack(cursor, base - 1);

							auto it = self.data<Hash>()->values.find(key);
							if (it != self.data<Hash>()->values.end()) {
								self.data<Hash>()->values.erase(it);
							}

							cursor->stack().pop_back();
						}));
	
	create_builtin_member("clear", ast->create_builtin_method(this, 1, [] (Cursor *cursor) {
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

	Cursor::Call call = std::move(cursor->waiting_calls().top());
	cursor->waiting_calls().pop();

	Hash *self = call.function().data<Hash>();
	self->values.reserve(length);
	self->construct();

	const auto from = std::prev(stack.end(), length * 2);
	const auto to = stack.end();
	for (auto it = from; it != to; it = std::next(it, 2)) {
		hash_insert(self, hash_key(*it), hash_value(*std::next(it)));
	}

	stack.erase(from, to);
	stack.emplace_back(std::move(call.function()));
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
	return WeakReference::copy(it->first);
}

WeakReference mint::hash_get_key(Hash::values_type::value_type &item) {
	return WeakReference::copy(item.first);
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
		item_value.copy_data(value);
	}
	else {
		item_value.move_data(value);
	}

	return item_value;
}
