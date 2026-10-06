/**
 * Copyright (c) 2026 Gauvain CHÉRY.
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
#include "mint/compiler/descriptions.h"
#include "mint/compiler/type_annotation.h"
#include "mint/memory/class.h"
#include "mint/memory/reference.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/memory/function_tools.h"
#include "mint/memory/cast_tools.h"
#include "mint/program/module.h"
#include "mint/program/program.h"
#include "mint/program/cursor.h"
#include "mint/system/error.h"
#include "mint/scheduler/scheduler.h"

#include <cstddef>
#include <iterator>
#include <utility>
#include <vector>

using namespace mint;

namespace {

ClassDescription make_description() {
	auto description = ClassDescription(Module::invalid_id, "hash");
	description.create_method(get_operator_symbol(Class::copy_operator), BuiltinTypeKind::hash)
	    .add_parameter({.name = "self", .type = BuiltinTypeKind::hash})
	    .add_parameter({.name = "other"})
	    .set_required_parameter_count(2);
	description.create_method(get_operator_symbol(Class::eq_operator), PrimitiveTypeKind::boolean)
	    .add_parameter({.name = "self", .type = BuiltinTypeKind::hash})
	    .add_parameter({.name = "other"})
	    .set_required_parameter_count(2);
	description.create_method(get_operator_symbol(Class::ne_operator), PrimitiveTypeKind::boolean)
	    .add_parameter({.name = "self", .type = BuiltinTypeKind::hash})
	    .add_parameter({.name = "other"})
	    .set_required_parameter_count(2);
	description.create_method(get_operator_symbol(Class::add_operator), BuiltinTypeKind::hash)
	    .add_parameter({.name = "self", .type = BuiltinTypeKind::hash})
	    .add_parameter({.name = "other"})
	    .set_required_parameter_count(2);
	description.create_method(get_operator_symbol(Class::subscript_operator), {})
	    .add_parameter({.name = "self", .type = BuiltinTypeKind::hash})
	    .add_parameter({.name = "key"})
	    .set_required_parameter_count(2);
	description.create_method(get_operator_symbol(Class::subscript_move_operator), {})
	    .add_parameter({.name = "self", .type = BuiltinTypeKind::hash})
	    .add_parameter({.name = "key"})
	    .add_parameter({.name = "value"})
	    .set_required_parameter_count(3);
	description.create_method(get_operator_symbol(Class::in_operator), BuiltinTypeKind::iterator)
	    .add_parameter({.name = "self", .type = BuiltinTypeKind::hash})
	    .set_required_parameter_count(1);
	description.create_method(get_operator_symbol(Class::in_operator), PrimitiveTypeKind::boolean)
	    .add_parameter({.name = "self", .type = BuiltinTypeKind::hash})
	    .add_parameter({.name = "key"})
	    .set_required_parameter_count(2);
	description.create_method("get", {})
	    .add_parameter({.name = "self", .type = BuiltinTypeKind::hash})
	    .add_parameter({.name = "key"})
	    .set_required_parameter_count(2);
	description.create_method("get", {})
	    .add_parameter({.name = "self", .type = BuiltinTypeKind::hash})
	    .add_parameter({.name = "key"})
	    .add_parameter({.name = "defaultValue"})
	    .set_required_parameter_count(3);
	description.create_method("each", PrimitiveTypeKind::none)
	    .add_parameter({.name = "self", .type = BuiltinTypeKind::hash})
	    .add_parameter({.name = "func"})
	    .set_required_parameter_count(2);
	description.create_method(get_operator_symbol(Class::call_operator), {})
	    .add_parameter({.name = "self", .type = BuiltinTypeKind::hash})
	    .add_parameter({.name = "key"})
	    .set_required_parameter_count(2)
	    .set_variadic(true);
	description.create_method("isEmpty", PrimitiveTypeKind::boolean)
	    .add_parameter({.name = "self", .type = BuiltinTypeKind::hash})
	    .set_required_parameter_count(1);
	description.create_method("size", PrimitiveTypeKind::number)
	    .add_parameter({.name = "self", .type = BuiltinTypeKind::hash})
	    .set_required_parameter_count(1);
	description.create_method("remove", BuiltinTypeKind::hash)
	    .add_parameter({.name = "self", .type = BuiltinTypeKind::hash})
	    .add_parameter({.name = "key"})
	    .set_required_parameter_count(2);
	description.create_method("clear", PrimitiveTypeKind::none)
	    .add_parameter({.name = "self", .type = BuiltinTypeKind::hash})
	    .set_required_parameter_count(1);
	return description;
}

}

ClassDescription& mint::HashClass::description() {
	static auto g_description = make_description();
	return g_description;
}

HashClass& HashClass::instance(Program& program) {
	return program.global_data().builtin<HashClass>(Class::Metatype::hash);
}

Hash::Hash(Program& program) :
    Object(HashClass::instance(program)) {}

Hash::Hash(Hash&& other) noexcept :
    Object(other.metadata),
    values(std::move(other.values)) {}

Hash::Hash(const Hash& other) :
    Object(other.metadata) {
	values.reserve(other.values.size());
	for (const auto& value : other.values) {
		values.emplace(hash_key(value.first), hash_value(value.second));
	}
}

Hash& Hash::operator=(Hash&& other) noexcept {
	std::swap(values, other.values);
	return *this;
}

Hash& Hash::operator=(const Hash& other) {
	values.clear();
	values.reserve(other.values.size());
	for (const auto& value : other.values) {
		values.emplace(hash_key(value.first), hash_value(value.second));
	}
	return *this;
}

void Hash::mark() {
	if (marked_bit()) {
		return;
	}
	Object::mark();
	for (auto& [key, value] : values) {
		key.data().mark();
		value.data().mark();
	}
}

HashClass::HashClass(Program& program) :
    Class(program.global_data(), description(), Class::Metatype::hash) {

	create_builtin_member(copy_operator, program.create_builtin_method(*this, 2, [](Cursor& cursor) {
		const auto base = get_stack_base(cursor);

		const auto& rvalue = load_from_stack(cursor, base);
		const auto& self = load_from_stack(cursor, base - 1);

		self.data<Hash>().values = to_hash(rvalue);
		cursor.stack().pop_back();
	}));

	create_builtin_member(eq_operator, program.create_builtin_method(*this, R"""(
		def (const self, const other) {
			if typeof self == typeof other {
				if self.size() == other.size() {
					for let (key, value) in self {
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

	create_builtin_member(ne_operator, program.create_builtin_method(*this, R"""(
		def (const self, const other) {
			if typeof self == typeof other {
				if self.size() == other.size() {
					for let (key, value) in self {
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

	create_builtin_member(add_operator, program.create_builtin_method(*this, 2, [](Cursor& cursor) {
		const auto base = get_stack_base(cursor);

		const auto& rvalue = load_from_stack(cursor, base);
		const auto& self = load_from_stack(cursor, base - 1);
		Reference result = create_hash(cursor.program());

		for (const auto& item : self.data<Hash>().values) {
			hash_insert(result.data<Hash>(), item.first, hash_get_value(item));
		}
		for (const auto& item : to_hash(rvalue)) {
			hash_insert(result.data<Hash>(), item.first, hash_get_value(item));
		}

		cursor.stack().pop_back();
		cursor.stack().pop_back();
		cursor.stack().emplace_back(std::forward<Reference>(result));
	}));

	create_builtin_member(subscript_operator, program.create_builtin_method(*this, 2, [](Cursor& cursor) {
		const auto base = get_stack_base(cursor);

		const auto& key = load_from_stack(cursor, base);
		const auto& self = load_from_stack(cursor, base - 1);
		Reference result = hash_get_item(self.data<Hash>(), key);

		cursor.stack().pop_back();
		cursor.stack().pop_back();
		cursor.stack().emplace_back(std::forward<Reference>(result));
	}));

	create_builtin_member(subscript_move_operator, program.create_builtin_method(*this, 3, [](Cursor& cursor) {
		const auto base = get_stack_base(cursor);

		const auto& value = load_from_stack(cursor, base);
		const auto& key = load_from_stack(cursor, base - 1);
		const auto& self = load_from_stack(cursor, base - 2);
		Reference result = hash_get_item(self.data<Hash>(), key);

		result.move_data(value);

		cursor.stack().pop_back();
		cursor.stack().pop_back();
		cursor.stack().pop_back();
		cursor.stack().emplace_back(std::forward<Reference>(result));
	}));

	create_builtin_member(in_operator, program.create_builtin_method(*this, 1, [](Cursor& cursor) {
		cursor.stack().back() = create_iterator_over(cursor, cursor.stack().back());
	}));

	create_builtin_member(in_operator, program.create_builtin_method(*this, 2, [](Cursor& cursor) {
		const auto base = get_stack_base(cursor);

		const auto& value = load_from_stack(cursor, base);
		const auto& self = load_from_stack(cursor, base - 1);
		Reference result = create_boolean(self.data<Hash>().values.contains(value));

		cursor.stack().pop_back();
		cursor.stack().pop_back();
		cursor.stack().emplace_back(std::forward<Reference>(result));
	}));

	create_builtin_member("get", program.create_builtin_method(*this, 2, [](Cursor& cursor) {
		const auto base = get_stack_base(cursor);

		const auto& key = load_from_stack(cursor, base);
		const auto& self = load_from_stack(cursor, base - 1);

		const auto it = self.data<Hash>().values.find(key);
		Reference result = it != self.data<Hash>().values.end() ? hash_get_value(it) : create_none();

		cursor.stack().pop_back();
		cursor.stack().pop_back();
		cursor.stack().emplace_back(std::forward<Reference>(result));
	}));

	create_builtin_member("get", program.create_builtin_method(*this, 3, [](Cursor& cursor) {
		const auto base = get_stack_base(cursor);

		const auto& default_value = load_from_stack(cursor, base);
		const auto& key = load_from_stack(cursor, base - 1);
		const auto& self = load_from_stack(cursor, base - 2);

		const auto it = self.data<Hash>().values.find(key);
		Reference result = it != self.data<Hash>().values.end() ? hash_get_value(it) : Reference(default_value);

		cursor.stack().pop_back();
		cursor.stack().pop_back();
		cursor.stack().pop_back();
		cursor.stack().emplace_back(std::forward<Reference>(result));
	}));

	create_builtin_member("each", program.create_builtin_method(*this, R"""(
		def (const self, const func) {
			var unpack_func = func[2]
			if defined unpack_func {
				for let (key, value) in self {
					unpack_func(key, value)
				}
			} else {
				for let item in self {
					func(item)
				}
			}
		})"""));

	create_builtin_member(call_operator, program.create_builtin_method(*this, R"""(
		def (const self, const key, ...) {
			return self[key](self, *va_args)
		})"""));

	create_builtin_member("isEmpty", program.create_builtin_method(*this, 1, [](Cursor& cursor) {
		cursor.stack().back() = create_boolean(cursor.stack().back().data<Hash>().values.empty());
	}));

	create_builtin_member("size", program.create_builtin_method(*this, 1, [](Cursor& cursor) {
		cursor.stack().back() = create_unsigned_number(cursor.stack().back().data<Hash>().values.size());
	}));

	create_builtin_member("remove", program.create_builtin_method(*this, 2, [](Cursor& cursor) {
		const auto base = get_stack_base(cursor);

		const auto& key = load_from_stack(cursor, base);
		const auto& self = load_from_stack(cursor, base - 1);

		const auto it = self.data<Hash>().values.find(key);
		if (it != self.data<Hash>().values.end()) {
			self.data<Hash>().values.erase(it);
		}

		cursor.stack().pop_back();
	}));

	create_builtin_member("clear", program.create_builtin_method(*this, 1, [](Cursor& cursor) {
		const Reference& self = cursor.stack().back();
		if (self.flags() & Reference::const_value) [[unlikely]] {
			error("invalid modification of constant value");
		}
		self.data<Hash>().values.clear();
		cursor.stack().back() = create_none();
	}));
}

void mint::hash_new(Cursor& cursor, std::size_t length) {

	auto& stack = cursor.stack();

	Cursor::Call call = std::move(cursor.waiting_calls().top());
	cursor.waiting_calls().pop();

	auto& self = call.function().data<Hash>();
	self.values.reserve(length);
	self.construct();

	const auto from = std::prev(stack.end(), static_cast<std::vector<Reference>::difference_type>(length * 2));
	const auto to = stack.end();
	for (auto it = from; it != to; it = std::next(it, 2)) {
		hash_insert(self, hash_key(*it), hash_value(*std::next(it)));
	}

	stack.erase(from, to);
	stack.emplace_back(std::move(call.function()));
}

Hash::values_type::iterator mint::hash_insert(Hash& hash, const Hash::key_type& key, const Reference& value) {
	return hash.values.emplace(hash_key(key), hash_value(value)).first;
}

Reference mint::hash_get_item(Hash& hash, const Hash::key_type& key) {
	auto i = hash.values.find(key);
	if (i == hash.values.end()) {
		i = hash_insert(hash, key, create_none());
	}
	return i->second;
}

Reference mint::hash_get_key(const Hash::values_type::iterator& it) {
	return {it->first};
}

Reference mint::hash_get_key(const Hash::values_type::value_type& item) {
	return {item.first};
}

Reference mint::hash_get_value(const Hash::values_type::iterator& it) {
	return it->second;
}

Reference mint::hash_get_value(const Hash::values_type::value_type& item) {
	return item.second;
}

Hash::key_type mint::hash_key(const Reference& key) {
	return {Reference::const_address | Reference::const_value, key.data()};
}

Reference mint::hash_value(const Reference& value) {
	Reference item_value;
	if ((value.flags() & (Reference::const_value | Reference::temporary)) == Reference::const_value) {
		item_value.copy_data(value);
	}
	else {
		item_value.move_data(value);
	}
	return item_value;
}
