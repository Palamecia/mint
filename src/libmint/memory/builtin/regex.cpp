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

Regex::Regex() : Object(RegexClass::instance()) {

}

Regex::Regex(const Regex &other) : Object(RegexClass::instance()),
	initializer(other.initializer),
	expr(other.expr) {

}

WeakReference match_to_iterator(const string &str, const smatch &match);
WeakReference sub_match_to_iterator(const string &str, const smatch &match, size_t index);

RegexClass::RegexClass() : Class("regex", Class::regex) {

	createBuiltinMember(copy_operator, AbstractSyntaxTree::createBuiltinMethode(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							WeakReference other = move_from_stack(cursor, base);
							Reference &self = load_from_stack(cursor, base - 1);

							if ((other.data()->format == Data::fmt_object) && (other.data<Object>()->metadata->metatype() == Class::regex)) {
								self.data<Regex>()->initializer = other.data<Regex>()->initializer;
							}
							else {
								self.data<Regex>()->initializer = "/" + to_string(other) + "/";
							}
							self.data<Regex>()->expr = to_regex(other);

							cursor->stack().pop_back();
						}));

	createBuiltinMember(regex_match_operator, AbstractSyntaxTree::createBuiltinMethode(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							WeakReference rvalue = move_from_stack(cursor, base);
							WeakReference self = move_from_stack(cursor, base - 1);

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().emplace_back(WeakReference::create<Boolean>(regex_search(to_string(rvalue), self.data<Regex>()->expr)));
						}));

	createBuiltinMember(regex_unmatch_operator, AbstractSyntaxTree::createBuiltinMethode(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							WeakReference rvalue = move_from_stack(cursor, base);
							WeakReference self = move_from_stack(cursor, base - 1);

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().emplace_back(WeakReference::create<Boolean>(!regex_search(to_string(rvalue), self.data<Regex>()->expr)));
						}));

	createBuiltinMember("match", AbstractSyntaxTree::createBuiltinMethode(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							WeakReference str = move_from_stack(cursor, base);
							WeakReference self = move_from_stack(cursor, base - 1);

							smatch match;
							smatch::string_type s = to_string(str);

							if (regex_match(s, match, self.data<Regex>()->expr)) {
								cursor->stack().pop_back();
								cursor->stack().pop_back();
								cursor->stack().emplace_back(match_to_iterator(s, match));
							}
							else {
								cursor->stack().pop_back();
								cursor->stack().pop_back();
								cursor->stack().emplace_back(WeakReference::create<None>());
							}
						}));

	createBuiltinMember("search", AbstractSyntaxTree::createBuiltinMethode(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							WeakReference str = move_from_stack(cursor, base);
							WeakReference self = move_from_stack(cursor, base - 1);

							smatch match;
							smatch::string_type s = to_string(str);

							if (regex_search(s, match, self.data<Regex>()->expr)) {
								cursor->stack().pop_back();
								cursor->stack().pop_back();
								cursor->stack().emplace_back(match_to_iterator(s, match));
							}
							else {
								cursor->stack().pop_back();
								cursor->stack().pop_back();
								cursor->stack().emplace_back(WeakReference::create<None>());
							}
						}));

	createBuiltinMember("getFlags", AbstractSyntaxTree::createBuiltinMethode(this, 1, [] (Cursor *cursor) {
							WeakReference self = move(cursor->stack().back());
							cursor->stack().back() = create_string(self.data<Regex>()->initializer.substr(self.data<Regex>()->initializer.rfind('/') + 1));
						}));
}


WeakReference match_to_iterator(const std::string &str, const smatch &match) {

	WeakReference result = WeakReference::create<Iterator>();

	for (size_t index = 0; index < match.size(); ++index) {
		iterator_insert(result.data<Iterator>(), sub_match_to_iterator(str, match, index));
	}

	result.data<Iterator>()->construct();
	return result;
}

WeakReference sub_match_to_iterator(const string &str, const smatch &match, size_t index) {

	WeakReference item = WeakReference::create<Iterator>();
	std::string match_str = match[index].str();

	iterator_insert(item.data<Iterator>(), create_string(match_str));
	iterator_insert(item.data<Iterator>(), WeakReference::create<Number>(static_cast<double>(utf8_byte_index_to_pos(str, match.position(index)))));
	iterator_insert(item.data<Iterator>(), WeakReference::create<Number>(static_cast<double>(utf8length(match_str))));

	item.data<Iterator>()->construct();
	return item;
}

