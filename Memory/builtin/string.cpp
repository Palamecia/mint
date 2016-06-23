#include "Memory/class.h"
#include "Memory/object.h"
#include "Memory/casttool.h"
#include "Memory/memorytool.h"
#include "AbstractSyntaxTree/abstractsyntaxtree.h"

using namespace std;

void string_format(AbstractSynatxTree *ast, string &dest, const string &format, const vector<Reference *> args) {

	size_t argn = 0;
	for (string::const_iterator cptr = format.begin(); cptr != format.end(); cptr++) {

		if ((*cptr == '%') && (argn < args.size())) {

			Reference *argv = args[argn++];

			/// \todo other formats
			switch (*++cptr) {
			case 'd':
				dest += to_string((long)to_number(ast, *argv));
				break;
			case 's':
				dest += to_string(*argv);
				break;
			}

		}
		else {
			dest += *cptr;
		}
	}
}

StringClass::StringClass() : Class("string") {

	createBuiltinMember("+", 2, AbstractSynatxTree::createBuiltinMethode(STRING_TYPE, [] (AbstractSynatxTree *ast) {

							size_t base = get_base(ast);

							Reference &rvalue = ast->stack().at(base).get();
							Reference &lvalue = ast->stack().at(base - 1).get();

							Reference *result = Reference::create<String>();
							((String *)result->data())->construct();
							((String *)result->data())->str = ((String *)lvalue.data())->str + to_string(rvalue);

							ast->stack().pop_back();
							ast->stack().pop_back();
							ast->stack().push_back(SharedReference::unique(result));
						}));

	createBuiltinMember("%", 2, AbstractSynatxTree::createBuiltinMethode(STRING_TYPE, [] (AbstractSynatxTree *ast) {

							size_t base = get_base(ast);

							Reference &rvalue = ast->stack().at(base).get();
							Reference &lvalue = ast->stack().at(base - 1).get();

							Reference *result = Reference::create<String>();
							((String *)result->data())->construct();
							string_format(ast, ((String *)result->data())->str, ((String *)lvalue.data())->str, to_array(rvalue));

							ast->stack().pop_back();
							ast->stack().pop_back();
							ast->stack().push_back(SharedReference::unique(result));
						}));

	createBuiltinMember("==", 2, AbstractSynatxTree::createBuiltinMethode(STRING_TYPE, [] (AbstractSynatxTree *ast) {

							size_t base = get_base(ast);

							Reference &rvalue = ast->stack().at(base).get();
							Reference &lvalue = ast->stack().at(base - 1).get();

							Reference *result = Reference::create<Number>();
							((Number *)result->data())->value = ((String *)lvalue.data())->str == to_string(rvalue);

							ast->stack().pop_back();
							ast->stack().pop_back();
							ast->stack().push_back(SharedReference::unique(result));
						}));

	createBuiltinMember("!=", 2, AbstractSynatxTree::createBuiltinMethode(STRING_TYPE, [] (AbstractSynatxTree *ast) {

							size_t base = get_base(ast);

							Reference &rvalue = ast->stack().at(base).get();
							Reference &lvalue = ast->stack().at(base - 1).get();

							Reference *result = Reference::create<Number>();
							((Number *)result->data())->value = ((String *)lvalue.data())->str != to_string(rvalue);

							ast->stack().pop_back();
							ast->stack().pop_back();
							ast->stack().push_back(SharedReference::unique(result));
						}));

	createBuiltinMember("<", 2, AbstractSynatxTree::createBuiltinMethode(STRING_TYPE, [] (AbstractSynatxTree *ast) {

							size_t base = get_base(ast);

							Reference &rvalue = ast->stack().at(base).get();
							Reference &lvalue = ast->stack().at(base - 1).get();

							Reference *result = Reference::create<Number>();
							((Number *)result->data())->value = ((String *)lvalue.data())->str < to_string(rvalue);

							ast->stack().pop_back();
							ast->stack().pop_back();
							ast->stack().push_back(SharedReference::unique(result));
						}));

	createBuiltinMember(">", 2, AbstractSynatxTree::createBuiltinMethode(STRING_TYPE, [] (AbstractSynatxTree *ast) {

							size_t base = get_base(ast);

							Reference &rvalue = ast->stack().at(base).get();
							Reference &lvalue = ast->stack().at(base - 1).get();

							Reference *result = Reference::create<Number>();
							((Number *)result->data())->value = ((String *)lvalue.data())->str > to_string(rvalue);

							ast->stack().pop_back();
							ast->stack().pop_back();
							ast->stack().push_back(SharedReference::unique(result));

						}));

	createBuiltinMember("<=", 2, AbstractSynatxTree::createBuiltinMethode(STRING_TYPE, [] (AbstractSynatxTree *ast) {

							size_t base = get_base(ast);

							Reference &rvalue = ast->stack().at(base).get();
							Reference &lvalue = ast->stack().at(base - 1).get();

							Reference *result = Reference::create<Number>();
							((Number *)result->data())->value = ((String *)lvalue.data())->str <= to_string(rvalue);

							ast->stack().pop_back();
							ast->stack().pop_back();
							ast->stack().push_back(SharedReference::unique(result));
						}));

	createBuiltinMember(">=", 2, AbstractSynatxTree::createBuiltinMethode(STRING_TYPE, [] (AbstractSynatxTree *ast) {

							size_t base = get_base(ast);

							Reference &rvalue = ast->stack().at(base).get();
							Reference &lvalue = ast->stack().at(base - 1).get();

							Reference *result = Reference::create<Number>();
							((Number *)result->data())->value = ((String *)lvalue.data())->str >= to_string(rvalue);

							ast->stack().pop_back();
							ast->stack().pop_back();
							ast->stack().push_back(SharedReference::unique(result));
						}));

	createBuiltinMember("&&", 2, AbstractSynatxTree::createBuiltinMethode(STRING_TYPE, [] (AbstractSynatxTree *ast) {

							size_t base = get_base(ast);

							Reference &rvalue = ast->stack().at(base).get();
							Reference &lvalue = ast->stack().at(base - 1).get();

							Reference *result = Reference::create<Number>();
							((Number *)result->data())->value = ((String *)lvalue.data())->str.size() && to_number(ast, rvalue);

							ast->stack().pop_back();
							ast->stack().pop_back();
							ast->stack().push_back(SharedReference::unique(result));
						}));

	createBuiltinMember("||", 2, AbstractSynatxTree::createBuiltinMethode(STRING_TYPE, [] (AbstractSynatxTree *ast) {

							size_t base = get_base(ast);

							Reference &rvalue = ast->stack().at(base).get();
							Reference &lvalue = ast->stack().at(base - 1).get();

							Reference *result = Reference::create<Number>();
							((Number *)result->data())->value = ((String *)lvalue.data())->str.size() || to_number(ast, rvalue);

							ast->stack().pop_back();
							ast->stack().pop_back();
							ast->stack().push_back(SharedReference::unique(result));
						}));

	createBuiltinMember("^", 2, AbstractSynatxTree::createBuiltinMethode(STRING_TYPE, [] (AbstractSynatxTree *ast) {

							size_t base = get_base(ast);

							Reference &rvalue = ast->stack().at(base).get();
							Reference &lvalue = ast->stack().at(base - 1).get();

							Reference *result = Reference::create<Number>();
							((Number *)result->data())->value = ((String *)lvalue.data())->str.size() ^ (size_t)to_number(ast, rvalue);

							ast->stack().pop_back();
							ast->stack().pop_back();
							ast->stack().push_back(SharedReference::unique(result));
						}));

	createBuiltinMember("!", 1, AbstractSynatxTree::createBuiltinMethode(STRING_TYPE, [] (AbstractSynatxTree *ast) {

							Reference &value = ast->stack().back().get();

							Reference *result = Reference::create<Number>();
							((Number *)result->data())->value = ((String *)value.data())->str.empty();

							ast->stack().pop_back();
							ast->stack().push_back(SharedReference::unique(result));
						}));

	createBuiltinMember("[]", 2, AbstractSynatxTree::createBuiltinMethode(STRING_TYPE, [] (AbstractSynatxTree *ast) {
							/// \todo iterator on utf-8 char
						}));

	/// \todo register operator overloads

	createBuiltinMember("size", 1, AbstractSynatxTree::createBuiltinMethode(STRING_TYPE, [] (AbstractSynatxTree *ast) {
							/// \todo iterator on utf-8 char

							Reference &value = ast->stack().back().get();

							Reference *result = Reference::create<Number>();
							((Number *)result->data())->value = ((String *)value.data())->str.size();

							ast->stack().pop_back();
							ast->stack().push_back(SharedReference::unique(result));
						}));
}
