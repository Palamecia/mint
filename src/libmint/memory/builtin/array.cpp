#include "memory/builtin/array.h"
#include "memory/builtin/string.h"
#include "memory/casttool.h"
#include "memory/memorytool.h"
#include "memory/functiontool.h"
#include "ast/abstractsyntaxtree.h"
#include "ast/cursor.h"
#include "system/error.h"

using namespace std;
using namespace mint;

ArrayClass *ArrayClass::instance() {

	static ArrayClass g_instance;
	return &g_instance;
}

Array::Array() : Object(ArrayClass::instance()) {

}

Array::~Array() {
	invalidateReferenceManager();
}

void Array::mark() {
	if (!markedBit()) {
		Object::mark();
		for (SharedReference &item : values) {
			item->data()->mark();
		}
	}
}

ArrayClass::ArrayClass() : Class("array", Class::array) {

	createBuiltinMember(":=", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							SharedReference other = move(cursor->stack().at(base));
							SharedReference &self = cursor->stack().at(base - 1);

							self->data<Array>()->values.clear();
							for (auto &item : to_array(other)) {
								array_append(self->data<Array>(), item);
							}

							cursor->stack().pop_back();
						}));

	createBuiltinMember("==", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(),
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

	createBuiltinMember("!=", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(),
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

	createBuiltinMember("+", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							SharedReference other = move(cursor->stack().at(base));
							SharedReference self = move(cursor->stack().at(base - 1));
							Reference *result = StrongReference::create<Array>();

							result->data<Array>()->construct();
							for (auto &value : self->data<Array>()->values) {
								array_append(result->data<Array>(), value);
							}
							for (auto &value : to_array(other)) {
								array_append(result->data<Array>(), value);
							}

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().emplace_back(SharedReference::unique(result));
						}));

	createBuiltinMember("-", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(),
																		  "	def (self, other) {\n"
																		  "		result = []\n"
																		  "		for item in self {\n"
																		  "			if item not in other {\n"
																		  "				result << item\n"
																		  "			}\n"
																		  "		}\n"
																		  "		return result\n"
																		  "	}\n"));

	createBuiltinMember("*", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							SharedReference other = move(cursor->stack().at(base));
							SharedReference self = move( cursor->stack().at(base - 1));
							Reference *result = StrongReference::create<Array>();

							result->data<Array>()->construct();
							for (uintmax_t i = 0; i < static_cast<uintmax_t>(to_number(cursor, other)); ++i) {
								for (auto &value : self->data<Array>()->values) {
									array_append(result->data<Array>(), value);
								}
							}

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().emplace_back(SharedReference::unique(result));
						}));

	createBuiltinMember("<<", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							SharedReference other = move(cursor->stack().at(base));
							SharedReference &self = cursor->stack().at(base - 1);

							array_append(self->data<Array>(), other);

							cursor->stack().pop_back();
						}));

	createBuiltinMember("&", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(),
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

	createBuiltinMember("[]", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							SharedReference index = move(cursor->stack().at(base));
							SharedReference self = move(cursor->stack().at(base - 1));

							if ((index->data()->format == Data::fmt_object) && (index->data<Object>()->metadata->metatype() == Class::iterator)) {

								Reference *result = StrongReference::create<Array>();
								result->data<Array>()->construct();

								while (SharedReference item = iterator_next(index->data<Iterator>())) {
									array_append(result->data<Array>(), array_get_item(self->data<Array>(), static_cast<intmax_t>(to_number(cursor, item))));
								}

								cursor->stack().pop_back();
								cursor->stack().pop_back();
								cursor->stack().emplace_back(SharedReference::unique(result));
							}
							else {
								SharedReference result = array_get_item(self->data<Array>(), static_cast<intmax_t>(to_number(cursor, index)));

								cursor->stack().pop_back();
								cursor->stack().pop_back();
								cursor->stack().emplace_back(move(result));
							}
						}));

	createBuiltinMember("[]=", 3, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							SharedReference value = move(cursor->stack().at(base));
							SharedReference index = move(cursor->stack().at(base - 1));
							SharedReference &self = cursor->stack().at(base - 2);

							if ((index->data()->format == Data::fmt_object) && (index->data<Object>()->metadata->metatype() == Class::iterator)) {

								size_t offset = 0;

								SharedReference values = SharedReference::unique(StrongReference::create(iterator_init(value)));

								while (SharedReference item = iterator_next(index->data<Iterator>())) {
									offset = array_index(self->data<Array>(), static_cast<intmax_t>(to_number(cursor, item)));
									if (SharedReference other = iterator_next(values->data<Iterator>())) {
										self->data<Array>()->values[offset] = array_item(other);
									}
									else {
										self->data<Array>()->values.erase(self->data<Array>()->values.begin() + static_cast<Array::values_type::difference_type>(offset));
									}
								}

								while (SharedReference other = iterator_next(values->data<Iterator>())) {
									self->data<Array>()->values.insert(self->data<Array>()->values.begin() + static_cast<Array::values_type::difference_type>(offset++), array_item(other));
								}

								cursor->stack().pop_back();
								cursor->stack().pop_back();
							}
							else {
								SharedReference result = array_get_item(self->data<Array>(), static_cast<intmax_t>(to_number(cursor, index)));
								result->move(*value);

								cursor->stack().pop_back();
								cursor->stack().pop_back();
								cursor->stack().pop_back();
								cursor->stack().emplace_back(move(result));
							}
						}));

	createBuiltinMember("in", 1, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {
							SharedReference self = move(cursor->stack().back());
							cursor->stack().back() = SharedReference::unique(StrongReference::create(iterator_init(self)));
						}));

	createBuiltinMember("in", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(),
																		  "	def (self, value) {\n"
																		  "		for item in self {\n"
																		  "			if item == value {\n"
																		  "				return true\n"
																		  "			}\n"
																		  "		}\n"
																		  "		return false\n"
																		  "	}\n"));

	createBuiltinMember("each", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(),
																			"	def (self, func) {\n"
																			"		for item in self {\n"
																			"			func(item)\n"
																			"		}\n"
																			"	}\n"));

	createBuiltinMember("isEmpty", 1, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {
							SharedReference self = move(cursor->stack().back());
							cursor->stack().back() = create_boolean(self->data<Array>()->values.empty());
						}));

	createBuiltinMember("size", 1, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {
							SharedReference self = move(cursor->stack().back());
							cursor->stack().back() = create_number(static_cast<double>(self->data<Array>()->values.size()));
						}));

	createBuiltinMember("remove", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							SharedReference index = move(cursor->stack().at(base));
							SharedReference &self = cursor->stack().at(base - 1);

							Array *array = self->data<Array>();
							if ((index->data()->format == Data::fmt_object) && (index->data<Object>()->metadata->metatype() == Class::iterator)) {
								set<size_t> indexes;
								while (SharedReference item = iterator_next(index->data<Iterator>())) {
									indexes.insert(array_index(array, static_cast<intmax_t>(to_number(cursor, item))));
								}
								for (auto i = indexes.rbegin(); i != indexes.rend(); ++i) {
									array->values.erase(array->values.begin() + static_cast<Array::values_type::difference_type>(*i));
								}
							}
							else {
								array->values.erase(array->values.begin() + static_cast<Array::values_type::difference_type>(array_index(array, static_cast<intmax_t>(to_number(cursor, index)))));
							}

							cursor->stack().pop_back();
						}));

	createBuiltinMember("clear", 1, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {
							SharedReference self = move(cursor->stack().back());
							if (self->flags() & Reference::const_value) {
								error("invalid modification of constant value");
							}
							self->data<Array>()->values.clear();
							cursor->stack().back() = SharedReference::unique(StrongReference::create<None>());
						}));

	createBuiltinMember("contains", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(),
																				"	def (self, value) {\n"
																				"		if value in self {\n"
																				"			return true\n"
																				"		}\n"
																				"		return false\n"
																				"	}\n"));

	createBuiltinMember("indexOf", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(),
																			   "	def (self, value) {\n"
																			   "		return self.indexOf(value, 0)\n"
																			   "	}\n"));

	createBuiltinMember("indexOf", 3, AbstractSyntaxTree::createBuiltinMethode(metatype(),
																			   "	def (self, value, from) {\n"
																			   "		for i in from...self.size() {\n"
																			   "			if self[i] == value {\n"
																			   "				return i\n"
																			   "			}\n"
																			   "		}\n"
																			   "		return none\n"
																			   "	}\n"));

	createBuiltinMember("lastIndexOf", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(),
																				   "	def (self, value) {\n"
																				   "		return self.lastIndexOf(value, none)\n"
																				   "	}\n"));

	createBuiltinMember("lastIndexOf", 3, AbstractSyntaxTree::createBuiltinMethode(metatype(),
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

	createBuiltinMember("join", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							SharedReference sep = move(cursor->stack().at(base));
							SharedReference self = move(cursor->stack().at(base - 1));

							Reference *result = StrongReference::create<String>();
							result->data<String>()->str = [] (Array::values_type &values, const std::string &sep) {
								std::string join;
								for (auto it = values.begin(); it != values.end(); ++it) {
									if (it != values.begin()) {
										join += sep;
									}
									join += to_string(*it);
								}
								return join;
							} (self->data<Array>()->values, sep->data<String>()->str);

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().emplace_back(SharedReference::unique(result));
						}));
}
