#include "memory/builtin/array.h"
#include "memory/casttool.h"
#include "memory/memorytool.h"
#include "ast/abstractsyntaxtree.h"
#include "system/error.h"

using namespace std;

ArrayClass *ArrayClass::instance() {

	static ArrayClass *g_instance = new ArrayClass;

	return g_instance;
}

Array::Array() : Object(ArrayClass::instance()) {}

ArrayClass::ArrayClass() : Class("array", Class::array) {

	createBuiltinMember(":=", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (AbstractSyntaxTree *ast) {

							size_t base = get_base(ast);

							Reference &other = *ast->stack().at(base);
							Reference &self = *ast->stack().at(base - 1);

							((Array *)self.data())->values.clear();
							for (auto &item : to_array(other)) {
								array_append((Array *)self.data(), item);
							}

							ast->stack().pop_back();
						}));

	createBuiltinMember("+", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (AbstractSyntaxTree *ast) {

							size_t base = get_base(ast);

							Reference &other = *ast->stack().at(base);
							Reference &self = *ast->stack().at(base - 1);
							Reference *result = Reference::create<Array>();

							((Array *)result->data())->construct();
							for (auto &value : ((Array *)self.data())->values) {
								array_append((Array *)result->data(), value);
							}
							for (auto &value : to_array(other)) {
								array_append((Array *)result->data(), value);
							}

							ast->stack().pop_back();
							ast->stack().pop_back();
							ast->stack().push_back(SharedReference::unique(result));
						}));

	createBuiltinMember("[]", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (AbstractSyntaxTree *ast) {

							size_t base = get_base(ast);

							Reference &index = *ast->stack().at(base);
							Reference &self = *ast->stack().at(base - 1);

							SharedReference result = array_get_item((Array *)self.data(), to_number(ast, index));

							ast->stack().pop_back();
							ast->stack().pop_back();
							ast->stack().push_back(result);
						}));

	/// \todo register operator overloads

	createBuiltinMember("size", 1, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (AbstractSyntaxTree *ast) {

							Reference &self = *ast->stack().back();

							Reference *result = Reference::create<Number>();
							((Number *)result->data())->value = ((Array *)self.data())->values.size();

							ast->stack().pop_back();
							ast->stack().push_back(SharedReference::unique(result));
						}));

	createBuiltinMember("erase", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (AbstractSyntaxTree *ast) {

							size_t base = get_base(ast);

							SharedReference &value = ast->stack().at(base);
							SharedReference self = ast->stack().at(base - 1);

							Array *array = (Array *)self->data();
							array->values.erase(array->values.begin() + array_index(array, to_number(ast, *value)));

							ast->stack().pop_back();
							ast->stack().pop_back();
							ast->stack().push_back(self);
						}));
}
