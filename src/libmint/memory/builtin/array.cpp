#include "memory/builtin/array.h"
#include "memory/algorithm.hpp"
#include "memory/casttool.h"
#include "memory/memorytool.h"
#include "memory/functiontool.h"
#include "ast/abstractsyntaxtree.h"
#include "ast/cursor.h"
#include "system/error.h"

#include <iterator>

using namespace std;
using namespace mint;

ArrayClass *ArrayClass::instance() {

	static ArrayClass g_instance;
	return &g_instance;
}

Array::Array() : Object(ArrayClass::instance()) {

}

Array::Array(const Array &other) : Object(ArrayClass::instance()),
	values(other.values) {

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

	createBuiltinMember(Symbol::CopyOperator, AbstractSyntaxTree::createBuiltinMethode(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							SharedReference other = move_from_stack(cursor, base);
							SharedReference &self = load_from_stack(cursor, base - 1);

							self->data<Array>()->values = to_array(other);
							cursor->stack().pop_back();
						}));

	createBuiltinMember(Symbol::EqOperator, AbstractSyntaxTree::createBuiltinMethode(this, 2,
																		  "	def (self, other) {\n"
																		  "		if typeof self == typeof other {\n"
																		  "			if self.size() == other.size() {\n"
																		  "				for i in 0...self.size() {\n"
																		  "					if self[i] != other[i] {\n"
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
																		  "				for i in 0...self.size() {\n"
																		  "					if self[i] != other[i] {\n"
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

							SharedReference other = move_from_stack(cursor, base);
							SharedReference self = move_from_stack(cursor, base - 1);
							SharedReference result = create_array();

							for (auto &value : self->data<Array>()->values) {
								result->data<Array>()->values.push_back(value);
							}
							for (auto &value : to_array(other)) {
								result->data<Array>()->values.push_back(value);
							}

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().emplace_back(move(result));
						}));

	createBuiltinMember(Symbol::SubOperator, AbstractSyntaxTree::createBuiltinMethode(this, 2,
																		  "	def (self, other) {\n"
																		  "		result = []\n"
																		  "		for item in self {\n"
																		  "			if item not in other {\n"
																		  "				result << item\n"
																		  "			}\n"
																		  "		}\n"
																		  "		return result\n"
																		  "	}\n"));

	createBuiltinMember(Symbol::MulOperator, AbstractSyntaxTree::createBuiltinMethode(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							SharedReference other = move_from_stack(cursor, base);
							SharedReference self = move_from_stack(cursor, base - 1);
							SharedReference result = create_array();

							for (intmax_t i = 0; i < to_integer(cursor, other); ++i) {
								for (auto &value : self->data<Array>()->values) {
									result->data<Array>()->values.push_back(value);
								}
							}

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().emplace_back(move(result));
						}));

	createBuiltinMember(Symbol::ShiftLeftOperator, AbstractSyntaxTree::createBuiltinMethode(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							SharedReference other = move_from_stack(cursor, base);
							SharedReference &self = load_from_stack(cursor, base - 1);

							array_append(self->data<Array>(), other);

							cursor->stack().pop_back();
						}));

	createBuiltinMember(Symbol::BandOperator, AbstractSyntaxTree::createBuiltinMethode(this, 2,
																		  "	def (self, other) {\n"
																		  "		store = {}\n"
																		  "		result = []\n"
																		  "		for item in self {\n"
																		  "			store[item] = true\n"
																		  "		}\n"
																		  "		for item in other {\n"
																		  "			if store[item] {\n"
																		  "				result << item\n"
																		  "			}\n"
																		  "		}\n"
																		  "		return result\n"
																		  "	}\n"));

	createBuiltinMember(Symbol::SubscriptOperator, AbstractSyntaxTree::createBuiltinMethode(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							SharedReference &index = load_from_stack(cursor, base);
							SharedReference &self = load_from_stack(cursor, base - 1);

							if ((index->data()->format != Data::fmt_object) || (index->data<Object>()->metadata->metatype() != Class::iterator)) {
								SharedReference &&result = array_get_item(self->data<Array>(), to_integer(cursor, index));
								cursor->stack().pop_back();
								cursor->stack().back() = move(result);
							}
							else if (index->data<Iterator>()->ctx.getType() == Iterator::ctx_type::range) {

								size_t begin_index = array_index(self->data<Array>(), to_integer(cursor, index->data<Iterator>()->ctx.front()));
								size_t end_index = array_index(self->data<Array>(), to_integer(cursor, index->data<Iterator>()->ctx.back()));

								if (begin_index > end_index) {
									swap(begin_index, end_index);
								}

								SharedReference result = create_array();

								for (size_t i = begin_index; i <= end_index; ++i) {
									result->data<Array>()->values.emplace_back(self->data<Array>()->values[i]);
								}

								cursor->stack().pop_back();
								cursor->stack().back() = move(result);
							}
							else {

								SharedReference result = create_array();

								while (SharedReference &&item = iterator_next(index->data<Iterator>())) {
									result->data<Array>()->values.emplace_back(self->data<Array>()->values[array_index(self->data<Array>(), to_integer(cursor, item))]);
								}

								cursor->stack().pop_back();
								cursor->stack().back() = move(result);
							}
						}));

	createBuiltinMember(Symbol::SubscriptMoveOperator, AbstractSyntaxTree::createBuiltinMethode(this, 3, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							SharedReference &value = load_from_stack(cursor, base);
							SharedReference &index = load_from_stack(cursor, base - 1);
							SharedReference &self = load_from_stack(cursor, base - 2);

							if ((index->data()->format != Data::fmt_object) || (index->data<Object>()->metadata->metatype() != Class::iterator)) {
								SharedReference &&result = array_get_item(self->data<Array>(), to_integer(cursor, index));
								result->move(*value);
								cursor->stack().pop_back();
								cursor->stack().pop_back();
								cursor->stack().pop_back();
								cursor->stack().emplace_back(move(result));
							}
							else if (index->data<Iterator>()->ctx.getType() == Iterator::ctx_type::range) {

								size_t begin_index = array_index(self->data<Array>(), to_integer(cursor, index->data<Iterator>()->ctx.front()));
								size_t end_index = array_index(self->data<Array>(), to_integer(cursor, index->data<Iterator>()->ctx.back()));

								if (begin_index > end_index) {
									swap(begin_index, end_index);
								}

								for_each(value, [&self, &begin_index, &end_index] (Reference &&ref) {
									if (begin_index <= end_index) {
										self->data<Array>()->values[begin_index++].move(ref);
									}
									else {
										self->data<Array>()->values.insert(array_next(self->data<Array>(), begin_index++), array_item(ref));
									}
								});

								while (begin_index <= end_index) {
									self->data<Array>()->values.erase(array_next(self->data<Array>(), end_index--));
								}

								cursor->stack().pop_back();
								cursor->stack().pop_back();
							}
							else {

								size_t offset = 0;

								for_each(value, [cursor, &self, &offset, &index] (Reference &&ref) {
									if (!index->data<Iterator>()->ctx.empty()) {
										offset = array_index(self->data<Array>(), to_integer(cursor, index->data<Iterator>()->ctx.front()));
										self->data<Array>()->values[offset++].move(ref);
										index->data<Iterator>()->ctx.pop_front();
									}
									else {
										self->data<Array>()->values.insert(array_next(self->data<Array>(), offset++), array_item(ref));
									}
								});

								std::set<size_t> to_remove;

								while (!index->data<Iterator>()->ctx.empty()) {
									to_remove.insert(array_index(self->data<Array>(), to_integer(cursor, index->data<Iterator>()->ctx.front())));
									index->data<Iterator>()->ctx.pop_front();
								}

								for (auto i = to_remove.rbegin(); i != to_remove.rend(); ++i) {
									self->data<Array>()->values.erase(array_next(self->data<Array>(), *i));
								}

								cursor->stack().pop_back();
								cursor->stack().pop_back();
							}
						}));

	createBuiltinMember(Symbol::InOperator, AbstractSyntaxTree::createBuiltinMethode(this, 1, [] (Cursor *cursor) {
							cursor->stack().back() = SharedReference::strong(iterator_init(cursor->stack().back()));
						}));

	createBuiltinMember(Symbol::InOperator, AbstractSyntaxTree::createBuiltinMethode(this, 2,
																		  "	def (self, value) {\n"
																		  "		for item in self {\n"
																		  "			if item == value {\n"
																		  "				return true\n"
																		  "			}\n"
																		  "		}\n"
																		  "		return false\n"
																		  "	}\n"));

	createBuiltinMember("each", AbstractSyntaxTree::createBuiltinMethode(this, 2,
																			"	def (self, func) {\n"
																			"		for item in self {\n"
																			"			func(item)\n"
																			"		}\n"
																			"	}\n"));

	createBuiltinMember("isEmpty", AbstractSyntaxTree::createBuiltinMethode(this, 1, [] (Cursor *cursor) {
							SharedReference self = move(cursor->stack().back());
							cursor->stack().back() = SharedReference::strong<Boolean>(self->data<Array>()->values.empty());
						}));

	createBuiltinMember("size", AbstractSyntaxTree::createBuiltinMethode(this, 1, [] (Cursor *cursor) {
							SharedReference self = move(cursor->stack().back());
							cursor->stack().back() = SharedReference::strong<Number>(static_cast<double>(self->data<Array>()->values.size()));
						}));

	createBuiltinMember("remove", AbstractSyntaxTree::createBuiltinMethode(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							SharedReference &index = load_from_stack(cursor, base);
							SharedReference &self = load_from_stack(cursor, base - 1);

							if ((index->data()->format != Data::fmt_object) || (index->data<Object>()->metadata->metatype() != Class::iterator)) {
								self->data<Array>()->values.erase(array_next(self->data<Array>(), array_index(self->data<Array>(), to_integer(cursor, index))));
							}
							else if (index->data<Iterator>()->ctx.getType() == Iterator::ctx_type::range) {

								size_t begin_index = array_index(self->data<Array>(), to_integer(cursor, index->data<Iterator>()->ctx.front()));
								size_t end_index = array_index(self->data<Array>(), to_integer(cursor, index->data<Iterator>()->ctx.back()));

								if (begin_index > end_index) {
									swap(begin_index, end_index);
								}

								self->data<Array>()->values.erase(array_next(self->data<Array>(), begin_index), array_next(self->data<Array>(), end_index + 1));
							}
							else {
								set<size_t> to_remove;

								while (!index->data<Iterator>()->ctx.empty()) {
									to_remove.insert(array_index(self->data<Array>(), to_integer(cursor, index->data<Iterator>()->ctx.front())));
									index->data<Iterator>()->ctx.pop_front();
								}

								for (auto i = to_remove.rbegin(); i != to_remove.rend(); ++i) {
									self->data<Array>()->values.erase(array_next(self->data<Array>(), *i));
								}
							}

							cursor->stack().pop_back();
						}));

	createBuiltinMember("clear", AbstractSyntaxTree::createBuiltinMethode(this, 1, [] (Cursor *cursor) {
							SharedReference self = move(cursor->stack().back());
							if (UNLIKELY(self->flags() & Reference::const_value)) {
								error("invalid modification of constant value");
							}
							self->data<Array>()->values.clear();
							cursor->stack().back() = SharedReference::strong<None>();
						}));

	createBuiltinMember("contains", AbstractSyntaxTree::createBuiltinMethode(this, 2,
																				"	def (self, value) {\n"
																				"		if value in self {\n"
																				"			return true\n"
																				"		}\n"
																				"		return false\n"
																				"	}\n"));

	createBuiltinMember("indexOf", AbstractSyntaxTree::createBuiltinMethode(this, 2,
																			   "	def (self, value) {\n"
																			   "		return self.indexOf(value, 0)\n"
																			   "	}\n"));

	createBuiltinMember("indexOf", AbstractSyntaxTree::createBuiltinMethode(this, 3,
																			   "	def (self, value, from) {\n"
																			   "		for i in from...self.size() {\n"
																			   "			if self[i] == value {\n"
																			   "				return i\n"
																			   "			}\n"
																			   "		}\n"
																			   "		return none\n"
																			   "	}\n"));

	createBuiltinMember("lastIndexOf", AbstractSyntaxTree::createBuiltinMethode(this, 2,
																				   "	def (self, value) {\n"
																				   "		return self.lastIndexOf(value, none)\n"
																				   "	}\n"));

	createBuiltinMember("lastIndexOf", AbstractSyntaxTree::createBuiltinMethode(this, 3,
																				   "	def (self, value, from) {\n"
																				   "	if not defined from {\n"
																				   "		from = self.size() - 1\n"
																				   "	}\n"
																				   "	for i in from..0 {\n"
																				   "		if self[i] == value {\n"
																				   "			return i\n"
																				   "		}\n"
																				   "	}\n"
																				   "	return none\n"
																				   "	}\n"));

	createBuiltinMember("join", AbstractSyntaxTree::createBuiltinMethode(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							SharedReference sep = move_from_stack(cursor, base);
							SharedReference self = move_from_stack(cursor, base - 1);

							SharedReference result = create_string([] (Array::values_type &values, const std::string &sep) {
								std::string join;
								for (auto it = values.begin(); it != values.end(); ++it) {
									if (it != values.begin()) {
										join += sep;
									}
									join += to_string(array_get_item(it));
								}
								return join;
							} (self->data<Array>()->values, to_string(sep)));

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().emplace_back(move(result));
						}));
}

void mint::array_append_from_stack(Cursor *cursor) {

	const size_t base = get_stack_base(cursor);

	SharedReference &value = load_from_stack(cursor, base);
	SharedReference &array = load_from_stack(cursor, base - 1);

	array_append(array->data<Array>(), value);
	cursor->stack().pop_back();
}

void mint::array_append(Array *array, const SharedReference &item) {
	array->values.emplace_back(array_item(item));
}

SharedReference mint::array_get_item(Array *array, intmax_t index) {
	return SharedReference::weak(array->values[array_index(array, index)]);
}

SharedReference mint::array_get_item(Array::values_type::iterator &it) {
	return SharedReference::weak(*it);
}

SharedReference mint::array_get_item(Array::values_type::value_type &value) {
	return SharedReference::weak(value);
}

size_t mint::array_index(const Array *array, intmax_t index) {

	size_t i = (index >= 0) ? static_cast<size_t>(index) : static_cast<size_t>(index) + array->values.size();

	if (UNLIKELY(i >= array->values.size())) {
		error("array index '%ld' is out of range", index);
	}

	return i;
}

WeakReference mint::array_item(const SharedReference &item) {

	WeakReference item_value;

	if (item->flags() & Reference::const_value) {
		item_value.copy(*item);
	}
	else {
		item_value.move(*item);
	}

	return item_value;
}

WeakReference mint::array_item(const Reference &item) {

	WeakReference item_value;

	if (item.flags() & Reference::const_value) {
		item_value.copy(item);
	}
	else {
		item_value.move(item);
	}

	return item_value;
}
