#include "memory/builtin/regex.h"
#include "memory/functiontool.h"
#include "memory/casttool.h"
#include "ast/abstractsyntaxtree.h"
#include "ast/cursor.h"
#include "system/utf8iterator.h"

using namespace mint;
using namespace std;

RegexClass *RegexClass::instance() {

	static RegexClass g_instance;
	return &g_instance;
}

Regex::Regex() : Object(RegexClass::instance()) {}

SharedReference match_to_iterator(const string &str, const smatch &match);
SharedReference sub_match_to_iterator(const string &str, const smatch &match, size_t index);

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

	createBuiltinMember("match", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							SharedReference str = move(cursor->stack().at(base));
							SharedReference self = move(cursor->stack().at(base - 1));

							smatch match;
							smatch::string_type s = to_string(str);

							if (regex_match(s, match, self->data<Regex>()->expr)) {
								cursor->stack().pop_back();
								cursor->stack().pop_back();
								cursor->stack().emplace_back(match_to_iterator(s, match));
							}
							else {
								cursor->stack().pop_back();
								cursor->stack().pop_back();
								cursor->stack().emplace_back(SharedReference::unique(StrongReference::create<None>()));
							}
						}));

	createBuiltinMember("search", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							SharedReference str = move(cursor->stack().at(base));
							SharedReference self = move(cursor->stack().at(base - 1));

							smatch match;
							smatch::string_type s = to_string(str);

							if (regex_search(s, match, self->data<Regex>()->expr)) {
								cursor->stack().pop_back();
								cursor->stack().pop_back();
								cursor->stack().emplace_back(match_to_iterator(s, match));
							}
							else {
								cursor->stack().pop_back();
								cursor->stack().pop_back();
								cursor->stack().emplace_back(SharedReference::unique(StrongReference::create<None>()));
							}
						}));

	createBuiltinMember("getFlags", 1, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {
							SharedReference self = move(cursor->stack().back());
							cursor->stack().back() = create_string(self->data<Regex>()->initializer.substr(self->data<Regex>()->initializer.rfind('/') + 1));
						}));
}


SharedReference match_to_iterator(const std::string &str, const smatch &match) {

	SharedReference result = SharedReference::unique(StrongReference::create<Iterator>());

	for (size_t index = 0; index < match.size(); ++index) {
		iterator_insert(result->data<Iterator>(), sub_match_to_iterator(str, match, index));
	}

	result->data<Iterator>()->construct();
	return result;
}

SharedReference sub_match_to_iterator(const string &str, const smatch &match, size_t index) {

	SharedReference item = SharedReference::unique(StrongReference::create<Iterator>());
	std::string match_str = match[index].str();

	iterator_insert(item->data<Iterator>(), create_string(match_str));
	iterator_insert(item->data<Iterator>(), create_number(utf8_byte_index_to_pos(str, match.position(index))));
	iterator_insert(item->data<Iterator>(), create_number(utf8length(match_str)));

	item->data<Iterator>()->construct();
	return item;
}

