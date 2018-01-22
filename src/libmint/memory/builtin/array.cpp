#include "memory/builtin/array.h"
#include "memory/casttool.h"
#include "memory/memorytool.h"
#include "ast/abstractsyntaxtree.h"
#include "ast/cursor.h"
#include "system/error.h"

using namespace std;
using namespace mint;

ArrayClass *ArrayClass::instance() {

	static ArrayClass g_instance;
	return &g_instance;
}

Array::Array() : Object(ArrayClass::instance()) {}

ArrayClass::ArrayClass() : Class("array", Class::array) {

	createBuiltinMember(":=", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							Reference &other = *cursor->stack().at(base);
							Reference &self = *cursor->stack().at(base - 1);

							self.data<Array>()->values.clear();
							for (auto &item : to_array(other)) {
								array_append(self.data<Array>(), item);
							}

							cursor->stack().pop_back();
						}));

	createBuiltinMember("+", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							Reference &other = *cursor->stack().at(base);
							Reference &self = *cursor->stack().at(base - 1);
							Reference *result = Reference::create<Array>();

							result->data<Array>()->construct();
							for (auto &value : self.data<Array>()->values) {
								array_append(result->data<Array>(), value);
							}
							for (auto &value : to_array(other)) {
								array_append(result->data<Array>(), value);
							}

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().push_back(SharedReference::unique(result));
						}));

	createBuiltinMember("[]", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							Reference &index = *cursor->stack().at(base);
							Reference &self = *cursor->stack().at(base - 1);

							SharedReference result = array_get_item(self.data<Array>(), to_number(cursor, index));

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().push_back(result);
						}));

	/// \todo register operator overloads

	createBuiltinMember("size", 1, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							Reference &self = *cursor->stack().back();

							Reference *result = Reference::create<Number>();
							result->data<Number>()->value = self.data<Array>()->values.size();

							cursor->stack().pop_back();
							cursor->stack().push_back(SharedReference::unique(result));
						}));

	createBuiltinMember("erase", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							SharedReference &value = cursor->stack().at(base);
							SharedReference self = cursor->stack().at(base - 1);

							Array *array = self->data<Array>();
							array->values.erase(array->values.begin() + array_index(array, to_number(cursor, *value)));

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().push_back(self);
						}));
}
