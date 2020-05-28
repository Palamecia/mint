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

RegexClass::RegexClass() : Class("regex", Class::regex) {

	createBuiltinMember(":=", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							SharedReference other = move(cursor->stack().at(base));
							SharedReference self = move(cursor->stack().at(base - 1));

							if ((other->data()->format == Data::fmt_object) && (other->data<Object>()->metadata->metatype() == Class::regex)) {
								self->data<Regex>()->initializer = other->data<Regex>()->initializer;
							}
							else {
								self->data<Regex>()->initializer = "/" + to_string(other) + "/";
							}
							self->data<Regex>()->expr = to_regex(other);

							cursor->stack().pop_back();
						}));

	createBuiltinMember("=~", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							SharedReference rvalue = move(cursor->stack().at(base));
							SharedReference self = move(cursor->stack().at(base - 1));

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().emplace_back(create_boolean(regex_search(to_string(rvalue), self->data<Regex>()->expr)));
						}));

	createBuiltinMember("!~", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							SharedReference rvalue = move(cursor->stack().at(base));
							SharedReference self = move(cursor->stack().at(base - 1));

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().emplace_back(create_boolean(!regex_search(to_string(rvalue), self->data<Regex>()->expr)));
						}));
}
