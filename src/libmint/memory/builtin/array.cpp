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

#include "mint/memory/builtin/array.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/memory/functiontool.h"
#include "mint/memory/algorithm.hpp"
#include "mint/memory/casttool.h"
#include "mint/ast/abstractsyntaxtree.h"
#include "mint/ast/cursor.h"
#include "mint/system/string.h"
#include "mint/system/error.h"

#include <iterator>

using namespace std;
using namespace mint;

ArrayClass *ArrayClass::instance() {
	return GlobalData::instance()->builtin<ArrayClass>(Class::array);
}

Array::Array() : Object(ArrayClass::instance()) {

}

Array::Array(const Array &other) : Object(ArrayClass::instance()) {
	for (auto i = other.values.begin(); i != other.values.end(); ++i) {
		values.emplace_back(array_item(*i));
	}
}

void Array::mark() {
	if (!marked_bit()) {
		Object::mark();
		for (values_type::value_type &item : values) {
			item.data()->mark();
		}
	}
}

inline Array::values_type::iterator array_next(Array *array, size_t index) {
	return next(begin(array->values), static_cast<Array::values_type::difference_type>(index));
}

ArrayClass::ArrayClass() : Class("array", Class::array) {

	AbstractSyntaxTree *ast = AbstractSyntaxTree::instance();
	
	create_builtin_member(copy_operator, ast->create_builtin_methode(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							Reference &other = load_from_stack(cursor, base);
							Reference &self = load_from_stack(cursor, base - 1);

							self.data<Array>()->values = to_array(other);
							cursor->stack().pop_back();
						}));
	
	create_builtin_member(eq_operator, ast->create_builtin_methode(this, 2, R"""(
						def (const self, const other) {
							if typeof self == typeof other {
								if self.size() == other.size() {
									for i in 0...self.size() {
										if self[i] != other[i] {
											return false
										}
									}
									return true
								}
							}
							return false
						})"""));
	
	create_builtin_member(ne_operator, ast->create_builtin_methode(this, 2, R"""(
						def (const self, const other) {
							if typeof self == typeof other {
								if self.size() == other.size() {
									for i in 0...self.size() {
										if self[i] != other[i] {
											return true
										}
									}
									return false
								}
							}
							return true
						})"""));
	
	create_builtin_member(add_operator, ast->create_builtin_methode(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							Reference &other = load_from_stack(cursor, base);
							Reference &self = load_from_stack(cursor, base - 1);
							WeakReference result = create_array();

							for (auto &value : self.data<Array>()->values) {
								result.data<Array>()->values.push_back(array_get_item(value));
							}
							for (auto &value : to_array(other)) {
								result.data<Array>()->values.push_back(array_get_item(value));
							}

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().emplace_back(std::forward<Reference>(result));
						}));
	
	create_builtin_member(sub_operator, ast->create_builtin_methode(this, 2, R"""(
						def (const self, const other) {
							result = []
							for item in self {
								if item not in other {
									result << item
								}
							}
							return result
						})"""));
	
	create_builtin_member(mul_operator, ast->create_builtin_methode(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							Reference &other = load_from_stack(cursor, base);
							Reference &self = load_from_stack(cursor, base - 1);
							WeakReference result = create_array();

							for (intmax_t i = 0; i < to_integer(cursor, other); ++i) {
								for (auto &value : self.data<Array>()->values) {
									result.data<Array>()->values.push_back(array_get_item(value));
								}
							}

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().emplace_back(std::forward<Reference>(result));
						}));
	
	create_builtin_member(shift_left_operator, ast->create_builtin_methode(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							Reference &other = load_from_stack(cursor, base);
							Reference &self = load_from_stack(cursor, base - 1);

							array_append(self.data<Array>(), other);

							cursor->stack().pop_back();
						}));
	
	create_builtin_member(band_operator, ast->create_builtin_methode(this, 2, R"""(
						def (const self, const other) {
							store = {}
							result = []
							for item in self {
								store[item] = true
							}
							for item in other {
								if store[item] {
									result << item
								}
							}
							return result
						})"""));
	
	create_builtin_member(subscript_operator, ast->create_builtin_methode(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							Reference &index = load_from_stack(cursor, base);
							Reference &self = load_from_stack(cursor, base - 1);

							if ((index.data()->format != Data::fmt_object) || (index.data<Object>()->metadata->metatype() != Class::iterator)) {
								intmax_t index_value = to_integer(cursor, index);
								cursor->stack().pop_back();
								cursor->stack().back() = array_get_item(self.data<Array>(), index_value);
							}
							else if (index.data<Iterator>()->ctx.getType() == Iterator::ctx_type::range) {

								size_t begin_index = array_index(self.data<Array>(), to_integer(cursor, index.data<Iterator>()->ctx.next()));
								size_t end_index = array_index(self.data<Array>(), to_integer(cursor, index.data<Iterator>()->ctx.back()));

								if (begin_index > end_index) {
									swap(begin_index, end_index);
								}

								WeakReference result = create_array();

								for (size_t i = begin_index; i <= end_index; ++i) {
									result.data<Array>()->values.emplace_back(array_get_item(self.data<Array>()->values[i]));
								}

								cursor->stack().pop_back();
								cursor->stack().back() = std::move(result);
							}
							else {

								WeakReference result = create_array();

								while (optional<WeakReference> &&item = iterator_next(index.data<Iterator>())) {
									result.data<Array>()->values.emplace_back(array_get_item(self.data<Array>()->values[array_index(self.data<Array>(), to_integer(cursor, *item))]));
								}

								cursor->stack().pop_back();
								cursor->stack().back() = std::move(result);
							}
						}));
	
	create_builtin_member(subscript_move_operator, ast->create_builtin_methode(this, 3, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							Reference &value = load_from_stack(cursor, base);
							Reference &index = load_from_stack(cursor, base - 1);
							Reference &self = load_from_stack(cursor, base - 2);

							if ((index.data()->format != Data::fmt_object) || (index.data<Object>()->metadata->metatype() != Class::iterator)) {
								Reference &&result = array_get_item(self.data<Array>(), to_integer(cursor, index));
								result.move(value);
								cursor->stack().pop_back();
								cursor->stack().pop_back();
								cursor->stack().pop_back();
								cursor->stack().emplace_back(std::forward<Reference>(result));
							}
							else if (index.data<Iterator>()->ctx.getType() == Iterator::ctx_type::range) {

								size_t begin_index = array_index(self.data<Array>(), to_integer(cursor, index.data<Iterator>()->ctx.next()));
								size_t end_index = array_index(self.data<Array>(), to_integer(cursor, index.data<Iterator>()->ctx.back()));

								if (begin_index > end_index) {
									swap(begin_index, end_index);
								}

								for_each(value, [&self, &begin_index, &end_index] (const Reference &ref) {
									if (begin_index <= end_index) {
										self.data<Array>()->values[begin_index++].move(ref);
									}
									else {
										self.data<Array>()->values.insert(array_next(self.data<Array>(), begin_index++), array_item(ref));
									}
								});

								while (begin_index <= end_index) {
									self.data<Array>()->values.erase(array_next(self.data<Array>(), end_index--));
								}

								cursor->stack().pop_back();
								cursor->stack().pop_back();
							}
							else {

								size_t offset = 0;

								for_each(value, [cursor, &self, &offset, &index] (const Reference &ref) {
									if (!index.data<Iterator>()->ctx.empty()) {
										offset = array_index(self.data<Array>(), to_integer(cursor, index.data<Iterator>()->ctx.next()));
										self.data<Array>()->values[offset++].move(ref);
										index.data<Iterator>()->ctx.pop();
									}
									else {
										self.data<Array>()->values.insert(array_next(self.data<Array>(), offset++), array_item(ref));
									}
								});

								std::set<size_t> to_remove;

								while (!index.data<Iterator>()->ctx.empty()) {
									to_remove.insert(array_index(self.data<Array>(), to_integer(cursor, index.data<Iterator>()->ctx.next())));
									index.data<Iterator>()->ctx.pop();
								}

								for (auto i = to_remove.rbegin(); i != to_remove.rend(); ++i) {
									self.data<Array>()->values.erase(array_next(self.data<Array>(), *i));
								}

								cursor->stack().pop_back();
								cursor->stack().pop_back();
							}
						}));
	
	create_builtin_member("insert", ast->create_builtin_methode(this, 3, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							Reference &value = load_from_stack(cursor, base);
							Reference &index = load_from_stack(cursor, base - 1);
							Reference &self = load_from_stack(cursor, base - 2);

							array_insert(self.data<Array>(), to_integer(cursor, index), value);
							cursor->stack().pop_back();
							cursor->stack().pop_back();
						}));
	
	create_builtin_member(in_operator, ast->create_builtin_methode(this, 1, [] (Cursor *cursor) {
							cursor->stack().back() = WeakReference(Reference::const_address, iterator_init(cursor->stack().back()));
						}));
	
	create_builtin_member(in_operator, ast->create_builtin_methode(this, 2, R"""(
						def (const self, const value) {
							for item in self {
								if item == value {
									return true
								}
							}
							return false
						})"""));
	
	create_builtin_member("each", ast->create_builtin_methode(this, 2, R"""(
						def (const self, const func) {
							for item in self {
								func(item)
							}
						})"""));
	
	create_builtin_member("isEmpty", ast->create_builtin_methode(this, 1, [] (Cursor *cursor) {
							cursor->stack().back() = WeakReference::create<Boolean>(cursor->stack().back().data<Array>()->values.empty());
						}));
	
	create_builtin_member("size", ast->create_builtin_methode(this, 1, [] (Cursor *cursor) {
							cursor->stack().back() = WeakReference::create<Number>(static_cast<double>(cursor->stack().back().data<Array>()->values.size()));
						}));
	
	create_builtin_member("remove", ast->create_builtin_methode(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							Reference &index = load_from_stack(cursor, base);
							Reference &self = load_from_stack(cursor, base - 1);

							if ((index.data()->format != Data::fmt_object) || (index.data<Object>()->metadata->metatype() != Class::iterator)) {
								self.data<Array>()->values.erase(array_next(self.data<Array>(), array_index(self.data<Array>(), to_integer(cursor, index))));
							}
							else if (index.data<Iterator>()->ctx.getType() == Iterator::ctx_type::range) {

								size_t begin_index = array_index(self.data<Array>(), to_integer(cursor, index.data<Iterator>()->ctx.next()));
								size_t end_index = array_index(self.data<Array>(), to_integer(cursor, index.data<Iterator>()->ctx.back()));

								if (begin_index > end_index) {
									swap(begin_index, end_index);
								}

								self.data<Array>()->values.erase(array_next(self.data<Array>(), begin_index), array_next(self.data<Array>(), end_index + 1));
							}
							else {
								set<size_t> to_remove;

								while (!index.data<Iterator>()->ctx.empty()) {
									to_remove.insert(array_index(self.data<Array>(), to_integer(cursor, index.data<Iterator>()->ctx.next())));
									index.data<Iterator>()->ctx.pop();
								}

								for (auto i = to_remove.rbegin(); i != to_remove.rend(); ++i) {
									self.data<Array>()->values.erase(array_next(self.data<Array>(), *i));
								}
							}

							cursor->stack().pop_back();
						}));
	
	create_builtin_member("clear", ast->create_builtin_methode(this, 1, [] (Cursor *cursor) {
							Reference &self = cursor->stack().back();
							if (UNLIKELY(self.flags() & Reference::const_value)) {
								error("invalid modification of constant value");
							}
							self.data<Array>()->values.clear();
							cursor->stack().back() = WeakReference::create<None>();
						}));
	
	create_builtin_member("contains", ast->create_builtin_methode(this, 2, R"""(
						def (const self, const value) {
							if value in self {
								return true
							}
							return false
						})"""));
	
	create_builtin_member("indexOf", ast->create_builtin_methode(this, 2, R"""(
						def (const self, const value) {
							return self.indexOf(value, 0)
						})"""));
	
	create_builtin_member("indexOf", ast->create_builtin_methode(this, 3, R"""(
						def (const self, const value, const from) {
							for i in from...self.size() {
								if self[i] == value {
									return i
								}
							}
							return none
						})"""));
	
	create_builtin_member("lastIndexOf", ast->create_builtin_methode(this, 2, R"""(
						def (const self, const value) {
							return self.lastIndexOf(value, none)
						})"""));
	
	create_builtin_member("lastIndexOf", ast->create_builtin_methode(this, 3, R"""(
						def (const self, const value, const from) {
							if not defined from {
								from = self.size() - 1
							}
							for i in from..0 {
								if self[i] == value {
									return i
								}
							}
							return none
						})"""));
	
	create_builtin_member("join", ast->create_builtin_methode(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							Reference &sep = load_from_stack(cursor, base);
							Reference &self = load_from_stack(cursor, base - 1);

							WeakReference result = create_string(mint::join(self.data<Array>()->values, to_string(sep), [](auto it) {
								return to_string(array_get_item(it));
							}));

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().emplace_back(std::forward<Reference>(result));
						}));
}

void mint::array_new(Cursor *cursor, size_t length) {

	auto &stack = cursor->stack();

	Cursor::Call call = std::move(cursor->waiting_calls().top());
	cursor->waiting_calls().pop();

	Array *self = call.function().data<Array>();
	self->values.reserve(length + call.extra_argument_count());
	self->construct();

	const auto from = std::prev(stack.end(), length + call.extra_argument_count());
	const auto to = stack.end();
	for (auto it = from; it != to; ++it) {
		array_append(self, array_item(*it));
	}

	stack.erase(from, to);
	stack.emplace_back(std::move(call.function()));
}

void mint::array_append(Array *array, Reference &item) {
	array->values.emplace_back(array_item(item));
}

void mint::array_append(Array *array, Reference &&item) {
	array->values.emplace_back(std::forward<Reference>(item));
}

WeakReference mint::array_insert(Array *array, intmax_t index, Reference &item) {
	return WeakReference::share(*array->values.emplace(std::next(array->values.begin(), index), array_item(item)));
}

WeakReference mint::array_insert(Array *array, intmax_t index, Reference &&item) {
	return WeakReference::share(*array->values.emplace(std::next(array->values.begin(), index), std::forward<Reference>(item)));
}

WeakReference mint::array_get_item(Array *array, intmax_t index) {
	return WeakReference::share(array->values[array_index(array, index)]);
}

WeakReference mint::array_get_item(Array::values_type::iterator &it) {
	return WeakReference::share(*it);
}

WeakReference mint::array_get_item(Array::values_type::value_type &value) {
	return WeakReference::share(value);
}

size_t mint::array_index(const Array *array, intmax_t index) {

	size_t i = (index >= 0) ? static_cast<size_t>(index) : static_cast<size_t>(index) + array->values.size();

	if (UNLIKELY(i >= array->values.size())) {
		error("array index '%ld' is out of range", index);
	}

	return i;
}

WeakReference mint::array_item(const Reference &item) {

	WeakReference item_value;

	if ((item.flags() & (Reference::const_value | Reference::temporary)) == Reference::const_value) {
		item_value.copy(item);
	}
	else {
		item_value.move(item);
	}

	return item_value;
}
