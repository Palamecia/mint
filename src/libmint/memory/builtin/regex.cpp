#include "memory/builtin/regex.h"
#include "memory/functiontool.h"
#include "memory/casttool.h"
#include "ast/abstractsyntaxtree.h"
#include "ast/cursor.h"

using namespace mint;
using namespace std;

RegexClass *RegexClass::instance() {

	static RegexClass g_instance;
	return &g_instance;
}

Regex::Regex() : Object(RegexClass::instance()) {}

RegexClass::RegexClass() : Class("string", Class::regex) {

	createBuiltinMember(":=", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							Reference &rvalue = *cursor->stack().at(base);
							Reference &self = *cursor->stack().at(base - 1);

							self.data<Regex>()->expr = to_regex(rvalue);

							cursor->stack().pop_back();
						}));

	createBuiltinMember("=~", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							Reference &rvalue = *cursor->stack().at(base);
							Reference &self = *cursor->stack().at(base - 1);

							smatch match;
							std::string str = to_string(rvalue);
							if (regex_match(str, match, self.data<Regex>()->expr)) {
								Reference *result = Reference::create<Iterator>();
								result->data<Iterator>()->construct();
								for (auto res : match) {
									iterator_insert(result->data<Iterator>(), create_string(res));
								}
								cursor->stack().pop_back();
								cursor->stack().pop_back();
								cursor->stack().push_back(SharedReference::unique(result));
							}
							else {
								cursor->stack().pop_back();
								cursor->stack().pop_back();
								cursor->stack().push_back(SharedReference::unique(Reference::create<Null>()));
							}
						}));

	createBuiltinMember("!~", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							Reference &rvalue = *cursor->stack().at(base);
							Reference &self = *cursor->stack().at(base - 1);

							smatch match;
							std::string str = to_string(rvalue);
							bool res = regex_match(str, match, self.data<Regex>()->expr);
							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().push_back(create_boolean(!res));
						}));
}
