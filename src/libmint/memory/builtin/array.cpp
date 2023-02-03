#include "memory/builtin/array.h"
#include "memory/builtin/iterator.h"
#include "memory/functiontool.h"
#include "memory/algorithm.hpp"
#include "memory/casttool.h"
#include "ast/abstractsyntaxtree.h"
#include "ast/cursor.h"
#include "system/error.h"

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
	if (!markedBit()) {
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

	createBuiltinMember(copy_operator, ast->createBuiltinMethode(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							Reference &other = load_from_stack(cursor, base);
							Reference &self = load_from_stack(cursor, base - 1);

							self.data<Array>()->values = to_array(other);
							cursor->stack().pop_back();
						}));

	createBuiltinMember(eq_operator, ast->createBuiltinMethode(this, 2, R"""(
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

	createBuiltinMember(ne_operator, ast->createBuiltinMethode(this, 2, R"""(
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

	createBuiltinMember(add_operator, ast->createBuiltinMethode(this, 2, [] (Cursor *cursor) {

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

	createBuiltinMember(sub_operator, ast->createBuiltinMethode(this, 2, R"""(
						def (const self, const other) {
							result = []
							for item in self {
								if item not in other {
									result << item
								}
							}
							return result
						})"""));

	createBuiltinMember(mul_operator, ast->createBuiltinMethode(this, 2, [] (Cursor *cursor) {

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

	createBuiltinMember(shift_left_operator, ast->createBuiltinMethode(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							Reference &other = load_from_stack(cursor, base);
							Reference &self = load_from_stack(cursor, base - 1);

							array_append(self.data<Array>(), other);

							cursor->stack().pop_back();
						}));

	createBuiltinMember(band_operator, ast->createBuiltinMethode(this, 2, R"""(
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

	createBuiltinMember(subscript_operator, ast->createBuiltinMethode(this, 2, [] (Cursor *cursor) {

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

	createBuiltinMember(subscript_move_operator, ast->createBuiltinMethode(this, 3, [] (Cursor *cursor) {

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

	createBuiltinMember(in_operator, ast->createBuiltinMethode(this, 1, [] (Cursor *cursor) {
							cursor->stack().back() = WeakReference(Reference::const_address, iterator_init(cursor->stack().back()));
						}));

	createBuiltinMember(in_operator, ast->createBuiltinMethode(this, 2, R"""(
						def (const self, const value) {
							for item in self {
								if item == value {
									return true
								}
							}
							return false
						})"""));

	createBuiltinMember("each", ast->createBuiltinMethode(this, 2, R"""(
						def (const self, const func) {
							for item in self {
								func(item)
							}
						})"""));

	createBuiltinMember("isEmpty", ast->createBuiltinMethode(this, 1, [] (Cursor *cursor) {
							cursor->stack().back() = WeakReference::create<Boolean>(cursor->stack().back().data<Array>()->values.empty());
						}));

	createBuiltinMember("size", ast->createBuiltinMethode(this, 1, [] (Cursor *cursor) {
							cursor->stack().back() = WeakReference::create<Number>(static_cast<double>(cursor->stack().back().data<Array>()->values.size()));
						}));

	createBuiltinMember("remove", ast->createBuiltinMethode(this, 2, [] (Cursor *cursor) {

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

	createBuiltinMember("clear", ast->createBuiltinMethode(this, 1, [] (Cursor *cursor) {
							Reference &self = cursor->stack().back();
							if (UNLIKELY(self.flags() & Reference::const_value)) {
								error("invalid modification of constant value");
							}
							self.data<Array>()->values.clear();
							cursor->stack().back() = WeakReference::create<None>();
						}));

	createBuiltinMember("contains", ast->createBuiltinMethode(this, 2, R"""(
						def (const self, const value) {
							if value in self {
								return true
							}
							return false
						})"""));

	createBuiltinMember("indexOf", ast->createBuiltinMethode(this, 2, R"""(
						def (const self, const value) {
							return self.indexOf(value, 0)
						})"""));

	createBuiltinMember("indexOf", ast->createBuiltinMethode(this, 3, R"""(
						def (const self, const value, const from) {
							for i in from...self.size() {
								if self[i] == value {
									return i
								}
							}
							return none
						})"""));

	createBuiltinMember("lastIndexOf", ast->createBuiltinMethode(this, 2, R"""(
						def (const self, const value) {
							return self.lastIndexOf(value, none)
						})"""));

	createBuiltinMember("lastIndexOf", ast->createBuiltinMethode(this, 3, R"""(
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

	createBuiltinMember("join", ast->createBuiltinMethode(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							Reference &sep = load_from_stack(cursor, base);
							Reference &self = load_from_stack(cursor, base - 1);

							WeakReference result = create_string([] (Array::values_type &values, const std::string &sep) {
								std::string join;
								for (auto it = values.begin(); it != values.end(); ++it) {
									if (it != values.begin()) {
										join += sep;
									}
									join += to_string(array_get_item(it));
								}
								return join;
							} (self.data<Array>()->values, to_string(sep)));

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().emplace_back(std::forward<Reference>(result));
						}));
}

void mint::array_new(Cursor *cursor, size_t length) {

	auto &stack = cursor->stack();

	Array *self(Reference::alloc<Array>());
	self->values.reserve(length);
	self->construct();

	const auto from = std::prev(stack.end(), length);
	const auto to = stack.end();
	for (auto it = from; it != to; ++it) {
		array_append(self, array_item(*it));
	}

	stack.erase(from, to);
	stack.emplace_back(Reference::const_address, self);
}

void mint::array_append(Array *array, Reference &item) {
	array->values.emplace_back(array_item(item));
}

void mint::array_append(Array *array, Reference &&item) {
	array->values.emplace_back(std::forward<Reference>(item));
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
