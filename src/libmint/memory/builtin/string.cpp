#include "memory/builtin/string.h"
#include "memory/builtin/iterator.h"
#include "memory/algorithm.hpp"
#include "memory/casttool.h"
#include "memory/functiontool.h"
#include "ast/abstractsyntaxtree.h"
#include "ast/cursor.h"
#include "system/utf8iterator.h"
#include "system/error.h"

#include <iterator>
#include <cstring>
#include <cmath>

using namespace std;
using namespace mint;

static constexpr const char *lower_digits = "0123456789abcdefghijklmnopqrstuvwxyz";
static constexpr const char *upper_digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static constexpr const char *inf_string = "inf";
static constexpr const char *nan_string = "nan";

enum Flag {
	string_left = 0x01,
	string_plus = 0x02,
	string_space = 0x04,
	string_special = 0x08,
	string_zeropad = 0x10,
	string_large = 0x20,
	string_sign = 0x40
};

enum DigitsFormat {
	scientific_format,
	decimal_format,
	shortest_format
};

template<typename number_t>
std::string string_integer(number_t number, int base, int size, int precision, int flags);

template<typename number_t>
std::string string_real(number_t number, int base, DigitsFormat format, int size, int precision, int flags);

inline std::size_t string_index(const std::string &str, intmax_t index);
inline std::string::iterator string_next(std::string &str, size_t index);
inline void string_format(mint::Cursor *cursor, std::string &dest, const std::string &format, mint::Iterator *args);

StringClass *StringClass::instance() {
	return GlobalData::instance()->builtin<StringClass>(Class::string);
}

String::String() : Object(StringClass::instance()) {

}

String::String(const String &other) : Object(StringClass::instance()),
	str(other.str) {

}

String::String(const string& value) : Object(StringClass::instance()),
	str(value) {

}

StringClass::StringClass() : Class("string", Class::string) {

	AbstractSyntaxTree *ast = AbstractSyntaxTree::instance();

	createBuiltinMember(copy_operator, ast->createBuiltinMethode(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							WeakReference rvalue = move_from_stack(cursor, base);
							Reference &self = load_from_stack(cursor, base - 1);

							self.data<String>()->str = to_string(rvalue);

							cursor->stack().pop_back();
						}));

	createBuiltinMember(regex_match_operator, ast->createBuiltinMethode(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							WeakReference rvalue = move_from_stack(cursor, base);
							WeakReference self = move_from_stack(cursor, base - 1);

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().emplace_back(WeakReference::create<Boolean>(regex_search(self.data<String>()->str, to_regex(rvalue))));
						}));

	createBuiltinMember(regex_unmatch_operator, ast->createBuiltinMethode(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							WeakReference rvalue = move_from_stack(cursor, base);
							WeakReference self = move_from_stack(cursor, base - 1);

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().emplace_back(WeakReference::create<Boolean>(!regex_search(self.data<String>()->str, to_regex(rvalue))));
						}));

	createBuiltinMember(add_operator, ast->createBuiltinMethode(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							WeakReference rvalue = move_from_stack(cursor, base);
							WeakReference self = move_from_stack(cursor, base - 1);

							WeakReference result = create_string(self.data<String>()->str + to_string(rvalue));

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().emplace_back(std::forward<Reference>(result));
						}));

	createBuiltinMember(mul_operator, ast->createBuiltinMethode(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							WeakReference rvalue = move_from_stack(cursor, base);
							WeakReference self = move_from_stack(cursor, base - 1);

							std::string result;

							for (intmax_t i = 0; i < to_integer(cursor, rvalue); ++i) {
								result += self.data<String>()->str;
							}

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().emplace_back(create_string(result));
						}));

	createBuiltinMember(mod_operator, ast->createBuiltinMethode(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							WeakReference values = move_from_stack(cursor, base);
							WeakReference self = move_from_stack(cursor, base - 1);

							std::string result;

							if (values.data()->format == Data::fmt_object && values.data<Object>()->metadata->metatype() == Class::iterator) {
								string_format(cursor, result, self.data<String>()->str, values.data<Iterator>());
							}
							else {
								WeakReference it = create_iterator();
								iterator_insert(it.data<Iterator>(), std::move(values));
								string_format(cursor, result, self.data<String>()->str, it.data<Iterator>());
							}

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().emplace_back(create_string(result));
						}));

	createBuiltinMember(shift_left_operator, ast->createBuiltinMethode(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							WeakReference other = move_from_stack(cursor, base);
							Reference &self = load_from_stack(cursor, base - 1);

							if (self.flags() & Reference::const_value) {
								cursor->stack().pop_back();
								cursor->stack().back() = create_string(self.data<String>()->str + to_string(other));
							}
							else {
								self.data<String>()->str.append(to_string(other));
								cursor->stack().pop_back();
							}
						}));

	createBuiltinMember(eq_operator, ast->createBuiltinMethode(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							WeakReference rvalue = move_from_stack(cursor, base);
							WeakReference self = move_from_stack(cursor, base - 1);

							WeakReference result = WeakReference::create<Boolean>();
							result.data<Boolean>()->value = self.data<String>()->str == to_string(rvalue);

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().emplace_back(std::forward<Reference>(result));
						}));

	createBuiltinMember(ne_operator, ast->createBuiltinMethode(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							WeakReference rvalue = move_from_stack(cursor, base);
							WeakReference self = move_from_stack(cursor, base - 1);

							WeakReference result = WeakReference::create<Boolean>();
							result.data<Boolean>()->value = self.data<String>()->str != to_string(rvalue);

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().emplace_back(std::forward<Reference>(result));
						}));

	createBuiltinMember(lt_operator, ast->createBuiltinMethode(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							WeakReference rvalue = move_from_stack(cursor, base);
							WeakReference self = move_from_stack(cursor, base - 1);

							WeakReference result = WeakReference::create<Boolean>();
							result.data<Boolean>()->value = self.data<String>()->str < to_string(rvalue);

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().emplace_back(std::forward<Reference>(result));
						}));

	createBuiltinMember(gt_operator, ast->createBuiltinMethode(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							WeakReference rvalue = move_from_stack(cursor, base);
							WeakReference self = move_from_stack(cursor, base - 1);

							WeakReference result = WeakReference::create<Boolean>();
							result.data<Boolean>()->value = self.data<String>()->str > to_string(rvalue);

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().emplace_back(std::forward<Reference>(result));

						}));

	createBuiltinMember(le_operator, ast->createBuiltinMethode(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							WeakReference rvalue = move_from_stack(cursor, base);
							WeakReference self = move_from_stack(cursor, base - 1);

							WeakReference result = WeakReference::create<Boolean>();
							result.data<Boolean>()->value = self.data<String>()->str <= to_string(rvalue);

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().emplace_back(std::forward<Reference>(result));
						}));

	createBuiltinMember(ge_operator, ast->createBuiltinMethode(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							WeakReference rvalue = move_from_stack(cursor, base);
							WeakReference self = move_from_stack(cursor, base - 1);

							WeakReference result = WeakReference::create<Boolean>();
							result.data<Boolean>()->value = self.data<String>()->str >= to_string(rvalue);

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().emplace_back(std::forward<Reference>(result));
						}));

	createBuiltinMember(and_operator, ast->createBuiltinMethode(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							WeakReference rvalue = move_from_stack(cursor, base);
							WeakReference self = move_from_stack(cursor, base - 1);

							WeakReference result = WeakReference::create<Boolean>();
							result.data<Boolean>()->value = self.data<String>()->str.size() && to_boolean(cursor, rvalue);

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().emplace_back(std::forward<Reference>(result));
						}));

	createBuiltinMember(or_operator, ast->createBuiltinMethode(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							WeakReference rvalue = move_from_stack(cursor, base);
							WeakReference self = move_from_stack(cursor, base - 1);

							WeakReference result = WeakReference::create<Boolean>();
							result.data<Boolean>()->value = self.data<String>()->str.size() || to_boolean(cursor, rvalue);

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().emplace_back(std::forward<Reference>(result));
						}));

	createBuiltinMember(xor_operator, ast->createBuiltinMethode(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							WeakReference rvalue = move_from_stack(cursor, base);
							WeakReference self = move_from_stack(cursor, base - 1);

							WeakReference result = WeakReference::create<Boolean>();
							result.data<Boolean>()->value = self.data<String>()->str.size() ^ static_cast<size_t>(to_boolean(cursor, rvalue));

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().emplace_back(std::forward<Reference>(result));
						}));

	createBuiltinMember(not_operator, ast->createBuiltinMethode(this, 1, [] (Cursor *cursor) {

							WeakReference self = std::move(cursor->stack().back());

							WeakReference result = WeakReference::create<Boolean>();
							result.data<Boolean>()->value = self.data<String>()->str.empty();

							cursor->stack().pop_back();
							cursor->stack().emplace_back(std::forward<Reference>(result));
						}));

	createBuiltinMember(subscript_operator, ast->createBuiltinMethode(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							WeakReference index = move_from_stack(cursor, base);
							WeakReference self = move_from_stack(cursor, base - 1);

							WeakReference result = WeakReference::create<String>();
							result.data<String>()->construct();

							if ((index.data()->format != Data::fmt_object) || (index.data<Object>()->metadata->metatype() != Class::iterator)) {
								std::string &string_ref = self.data<String>()->str;
								auto offset = string_index(string_ref, to_integer(cursor, index));
								result.data<String>()->str = *(utf8iterator(string_ref.begin()) + static_cast<size_t>(offset));
							}
							else if (index.data<Iterator>()->ctx.getType() == Iterator::ctx_type::range) {

								std::string &string_ref = self.data<String>()->str;
								size_t begin_index = string_index(string_ref, to_integer(cursor, index.data<Iterator>()->ctx.front()));
								size_t end_index = string_index(string_ref, to_integer(cursor, index.data<Iterator>()->ctx.back()));

								if (begin_index > end_index) {
									swap(begin_index, end_index);
								}

								string::iterator begin = string_ref.begin() + static_cast<int>(utf8_pos_to_byte_index(string_ref, begin_index));
								string::iterator end = string_ref.begin() + static_cast<int>(utf8_pos_to_byte_index(string_ref, end_index));

								end += static_cast<int>(utf8char_length(static_cast<byte>(*end)));
								result.data<String>()->str = std::string(begin, end);
							}
							else {
								std::string &string_ref = self.data<String>()->str;
								while (optional<WeakReference> &&item = iterator_next(index.data<Iterator>())) {
									result.data<String>()->str += *(utf8iterator(string_ref.begin()) + string_index(string_ref, to_integer(cursor, *item)));
								}
							}

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().emplace_back(std::forward<Reference>(result));
						}));

	createBuiltinMember(subscript_move_operator, ast->createBuiltinMethode(this, 3, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							WeakReference value = move_from_stack(cursor, base);
							WeakReference index = move_from_stack(cursor, base - 1);
							Reference &self = load_from_stack(cursor, base - 2);

							if ((index.data()->format != Data::fmt_object) || (index.data<Object>()->metadata->metatype() != Class::iterator)) {
								std::string &string_ref = self.data<String>()->str;
								auto offset = string_index(string_ref, to_integer(cursor, index));
								auto index = utf8_pos_to_byte_index(string_ref, offset);
								auto length = utf8char_length(static_cast<byte>(string_ref.at(index)));
								string_ref.replace(index, length, to_string(value));

								cursor->stack().pop_back();
								cursor->stack().pop_back();
								cursor->stack().pop_back();
								cursor->stack().emplace_back(std::forward<Reference>(value));
							}
							else if (index.data<Iterator>()->ctx.getType() == Iterator::ctx_type::range) {

								std::string &string_ref = self.data<String>()->str;
								size_t begin_index = string_index(string_ref, to_integer(cursor, index.data<Iterator>()->ctx.front()));
								size_t end_index = string_index(string_ref, to_integer(cursor, index.data<Iterator>()->ctx.back()));

								if (begin_index > end_index) {
									swap(begin_index, end_index);
								}

								string::iterator begin = string_next(string_ref, utf8_pos_to_byte_index(string_ref, begin_index));
								string::iterator end = string_next(string_ref, utf8_pos_to_byte_index(string_ref, end_index));

								advance(end, utf8char_length(static_cast<byte>(*end)));
								string_ref.replace(begin, end, to_string(value));

								cursor->stack().pop_back();
								cursor->stack().pop_back();
							}
							else {

								size_t offset = 0;
								std::string &string_ref = self.data<String>()->str;

								for_each(value, [cursor, &string_ref, &offset, &index] (const Reference &ref) {
									if (!index.data<Iterator>()->ctx.empty()) {
										offset = utf8_pos_to_byte_index(string_ref, string_index(string_ref, to_integer(cursor, index.data<Iterator>()->ctx.front())));
										size_t length = utf8char_length(static_cast<byte>(string_ref.at(offset)));
										string_ref.replace(offset, length, to_string(ref));
										index.data<Iterator>()->ctx.pop_front();
										offset += length;
									}
									else {
										size_t length = utf8char_length(static_cast<byte>(string_ref.at(offset)));
										string_ref.insert(offset, to_string(ref));
										offset += length;
									}
								});

								std::map<size_t, size_t> to_remove;

								while (!index.data<Iterator>()->ctx.empty()) {
									offset = utf8_pos_to_byte_index(string_ref, string_index(string_ref, to_integer(cursor, index.data<Iterator>()->ctx.front())));
									size_t length = utf8char_length(static_cast<byte>(string_ref.at(offset)));
									to_remove.insert({offset, length});
									index.data<Iterator>()->ctx.pop_front();
								}

								for (auto i = to_remove.rbegin(); i != to_remove.rend(); ++i) {
									string_ref.erase(i->first, i->second);
								}

								cursor->stack().pop_back();
								cursor->stack().pop_back();
							}
						}));

	createBuiltinMember(in_operator, ast->createBuiltinMethode(this, 1, [] (Cursor *cursor) {
							cursor->stack().back() = WeakReference(Reference::const_address, iterator_init(cursor->stack().back()));
						}));

	createBuiltinMember(in_operator, ast->createBuiltinMethode(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							WeakReference value = move_from_stack(cursor, base);
							WeakReference self = move_from_stack(cursor, base - 1);
							WeakReference result = WeakReference::create<Boolean>(self.data<String>()->str.find(to_string(value)) != string::npos);

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().emplace_back(std::forward<Reference>(result));
						}));

	createBuiltinMember("each", ast->createBuiltinMethode(this, 2,
																			"	def (const self, const func) {\n"
																			"		for item in self {\n"
																			"			func(item)\n"
																			"		}\n"
																			"	}\n"));

	createBuiltinMember("isEmpty", ast->createBuiltinMethode(this, 1, [] (Cursor *cursor) {
							WeakReference self = std::move(cursor->stack().back());
							cursor->stack().back() = WeakReference::create<Boolean>(self.data<String>()->str.empty());
						}));

	createBuiltinMember("size", ast->createBuiltinMethode(this, 1, [] (Cursor *cursor) {
							WeakReference self = std::move(cursor->stack().back());
							cursor->stack().back() = WeakReference::create<Number>(static_cast<double>(utf8length(self.data<String>()->str)));
						}));

	createBuiltinMember("clear", ast->createBuiltinMethode(this, 1, [] (Cursor *cursor) {
							WeakReference self = std::move(cursor->stack().back());
							if (UNLIKELY(self.flags() & Reference::const_value)) {
								error("invalid modification of constant value");
							}
							self.data<String>()->str.clear();
							cursor->stack().back() = WeakReference::create<None>();
						}));

	createBuiltinMember("replace", ast->createBuiltinMethode(this, 3, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							WeakReference str = move_from_stack(cursor, base);
							WeakReference pattern = move_from_stack(cursor, base - 1);
							Reference &self = load_from_stack(cursor, base - 2);

							std::string before = to_string(pattern);
							std::string after = to_string(str);

							if (self.flags() & Reference::const_value) {

								std::string str = self.data<String>()->str;

								if ((pattern.data()->format == Data::fmt_object) && (pattern.data<Object>()->metadata->metatype() == Class::regex)) {
									str = regex_replace(str, to_regex(pattern), after);
								}
								else {
									size_t pos = 0;
									while ((pos = str.find(before, pos)) != string::npos) {
										str.replace(pos, before.size(), after);
										pos += after.size();
									}
								}

								cursor->stack().pop_back();
								cursor->stack().pop_back();
								cursor->stack().back() = create_string(str);
							}
							else {

								if ((pattern.data()->format == Data::fmt_object) && (pattern.data<Object>()->metadata->metatype() == Class::regex)) {
									self.data<String>()->str = regex_replace(self.data<String>()->str, to_regex(pattern), after);
								}
								else {
									size_t pos = 0;
									while ((pos = self.data<String>()->str.find(before, pos)) != string::npos) {
										self.data<String>()->str.replace(pos, before.size(), after);
										pos += after.size();
									}
								}

								cursor->stack().pop_back();
								cursor->stack().pop_back();
							}
						}));

	createBuiltinMember("contains", ast->createBuiltinMethode(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							WeakReference other = move_from_stack(cursor, base);
							WeakReference self = move_from_stack(cursor, base - 1);

							WeakReference result = WeakReference::create<Boolean>();
							if ((other.data()->format == Data::fmt_object) && (other.data<Object>()->metadata->metatype() == Class::regex)) {
								result.data<Boolean>()->value = regex_search(self.data<String>()->str, to_regex(other));
							}
							else {
								result.data<Boolean>()->value = self.data<String>()->str.find(to_string(other)) != string::npos;
							}

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().emplace_back(std::forward<Reference>(result));
						}));

	createBuiltinMember("indexOf", ast->createBuiltinMethode(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							WeakReference other = move_from_stack(cursor, base);
							WeakReference self = move_from_stack(cursor, base - 1);

							auto pos = string::npos;
							if ((other.data()->format == Data::fmt_object) && (other.data<Object>()->metadata->metatype() == Class::regex)) {
								smatch match;
								if (regex_search(self.data<String>()->str, match, to_regex(other))) {
									pos = static_cast<decltype (pos)>(match.position(0));
								}
							}
							else {
								pos = self.data<String>()->str.find(to_string(other));
							}

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().emplace_back(pos != string::npos ?
							WeakReference::create<Number>(static_cast<double>(utf8_byte_index_to_pos(self.data<String>()->str, pos))) :
							WeakReference::create<None>());
						}));

	createBuiltinMember("indexOf", ast->createBuiltinMethode(this, 3, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							WeakReference from = move_from_stack(cursor, base);
							WeakReference other = move_from_stack(cursor, base - 1);
							WeakReference self = move_from_stack(cursor, base - 2);

							auto pos = string::npos;
							auto start = utf8_pos_to_byte_index(self.data<String>()->str, static_cast<size_t>(to_number(cursor, from)));
							if (start != string::npos) {
								if ((other.data()->format == Data::fmt_object) && (other.data<Object>()->metadata->metatype() == Class::regex)) {
									std::regex expr = to_regex(other);
									auto begin = sregex_iterator(self.data<String>()->str.begin(), self.data<String>()->str.end(), expr);
									auto end = sregex_iterator();
									for (auto i = begin; i != end; ++i) {
										if (start <= size_t(0) + static_cast<decltype (pos)>(i->position())) {
											pos = static_cast<decltype (pos)>(i->position());
											break;
										}
									}
								}
								else {
									pos = self.data<String>()->str.find(to_string(other), start);
								}
							}

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().emplace_back(pos != string::npos ?
							WeakReference::create<Number>(static_cast<double>(utf8_byte_index_to_pos(self.data<String>()->str, pos))) :
							WeakReference::create<None>());
						}));

	createBuiltinMember("lastIndexOf", ast->createBuiltinMethode(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							WeakReference other = move_from_stack(cursor, base);
							WeakReference self = move_from_stack(cursor, base - 1);

							auto pos = string::npos;
							if ((other.data()->format == Data::fmt_object) && (other.data<Object>()->metadata->metatype() == Class::regex)) {
								std::regex expr = to_regex(other);
								auto begin = sregex_iterator(self.data<String>()->str.begin(), self.data<String>()->str.end(), expr);
								auto end = sregex_iterator();
								for (auto i = begin; i != end; ++i) {
									pos = static_cast<decltype (pos)>(i->position());
								}
							}
							else {
								pos = self.data<String>()->str.rfind(to_string(other));
							}

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().emplace_back(pos != string::npos ?
							WeakReference::create<Number>(static_cast<double>(utf8_byte_index_to_pos(self.data<String>()->str, pos))) :
							WeakReference::create<None>());
						}));

	createBuiltinMember("lastIndexOf", ast->createBuiltinMethode(this, 3, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							WeakReference from = move_from_stack(cursor, base);
							WeakReference other = move_from_stack(cursor, base - 1);
							WeakReference self = move_from_stack(cursor, base - 2);

							auto pos = string::npos;
							auto start = utf8_pos_to_byte_index(self.data<String>()->str, static_cast<size_t>(to_number(cursor, from)));
							if (start != string::npos) {
								if ((other.data()->format == Data::fmt_object) && (other.data<Object>()->metadata->metatype() == Class::regex)) {
									std::regex expr = to_regex(other);
									auto begin = sregex_iterator(self.data<String>()->str.begin(), self.data<String>()->str.end(), expr);
									auto end = sregex_iterator();
									for (auto i = begin; i != end; ++i) {
										if (start >= size_t(0) + static_cast<decltype (pos)>(i->position())) {
											pos = static_cast<decltype (pos)>(i->position());
										}
									}
								}
								else {
									pos = self.data<String>()->str.rfind(to_string(other), start);
								}
							}

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().emplace_back(pos != string::npos ?
							WeakReference::create<Number>(static_cast<double>(utf8_byte_index_to_pos(self.data<String>()->str, pos))) :
							WeakReference::create<None>());
						}));

	createBuiltinMember("startsWith", ast->createBuiltinMethode(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							WeakReference other = move_from_stack(cursor, base);
							WeakReference self = move_from_stack(cursor, base - 1);

							WeakReference result = WeakReference::create<Boolean>();
							if ((other.data()->format == Data::fmt_object) && (other.data<Object>()->metadata->metatype() == Class::regex)) {
								smatch match;
								if (regex_search(self.data<String>()->str, match, to_regex(other))) {
									result.data<Boolean>()->value = match.position(0) == 0;
								}
								else {
									result.data<Boolean>()->value = false;
								}
							}
							else {
								result.data<Boolean>()->value = self.data<String>()->str.find(to_string(other)) == 0;
							}

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().emplace_back(std::forward<Reference>(result));
						}));

	createBuiltinMember("endsWith", ast->createBuiltinMethode(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							WeakReference other = move_from_stack(cursor, base);
							WeakReference self = move_from_stack(cursor, base - 1);

							WeakReference result = WeakReference::create<Boolean>();
							if ((other.data()->format == Data::fmt_object) && (other.data<Object>()->metadata->metatype() == Class::regex)) {
								result.data<Boolean>()->value = false;
								std::regex expr = to_regex(other);
								auto begin = sregex_iterator(self.data<String>()->str.begin(), self.data<String>()->str.end(), expr);
								auto end = sregex_iterator();
								for (auto i = begin; i != end; ++i) {
									if (size_t(0) + static_cast<size_t>(i->position() + i->length()) == self.data<String>()->str.size()) {
										result.data<Boolean>()->value = true;
										break;
									}
								}
							}
							else {
								std::string str = to_string(other);
								result.data<Boolean>()->value = (self.data<String>()->str.size() >= str.size()) &&
								(self.data<String>()->str.rfind(str) == self.data<String>()->str.size() - str.size());
							}

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().emplace_back(std::forward<Reference>(result));
						}));

	createBuiltinMember("split", ast->createBuiltinMethode(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							WeakReference sep = move_from_stack(cursor, base);
							WeakReference self = move_from_stack(cursor, base - 1);

							std::string sep_str = to_string(sep);
							std::string self_str = self.data<String>()->str;
							WeakReference result = create_array();

							if (sep_str.empty()) {
								for (utf8iterator i = self_str.begin(); i != self_str.end(); ++i) {
									array_append(result.data<Array>(), create_string(*i));
								}
							}
							else {
								size_t from = 0;
								size_t pos = self_str.find(sep_str);
								while (pos != std::string::npos) {
									array_append(result.data<Array>(), create_string(self_str.substr(from, pos - from)));
									pos = self_str.find(sep_str, from = pos + sep_str.size());
								}
								if (from < self_str.size()) {
									array_append(result.data<Array>(), create_string(self_str.substr(from, pos - from)));
								}
							}

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().emplace_back(std::forward<Reference>(result));
						}));

}

template<typename number_t>
string string_integer(number_t number, int base, int size, int precision, int flags) {

	string tmp;
	string result;
	const char *digits = (flags & string_large) ? upper_digits : lower_digits;

	if (flags & string_left) {
		flags &= ~string_zeropad;
	}
	if (base < 2 || base > 36) {
		return result;
	}

	char c = (flags & string_zeropad) ? '0' : ' ';
	char sign = 0;
	if (flags & string_sign) {
		if (number < 0) {
			sign = '-';
			number = -number;
			size--;
		}
		else if (flags & string_plus) {
			sign = '+';
			size--;
		}
		else if (flags & string_space) {
			sign = ' ';
			size--;
		}
	}

	if (flags & string_special) {
		if ((base == 16) || (base == 8) || (base == 2)) {
			size -= 2;
		}
	}

	if (number == 0) {
		tmp = "0";
	}
	else {
		while (number != 0) {
			tmp += digits[number % static_cast<number_t>(base)];
			number = number / static_cast<number_t>(base);
		}
	}

	if (static_cast<int>(tmp.size()) > precision) {
		precision = static_cast<int>(tmp.size());
	}
	size -= precision;
	if (!(flags & (string_zeropad + string_left))) {
		while (size-- > 0) {
			result += ' ';
		}
	}
	if (sign) {
		result += sign;
	}

	if (flags & string_special) {
		if (base == 16) {
			result += "0";
			result += digits[33];
		}
		else if (base == 8) {
			result += "0";
			result += digits[24];
		}
		else if (base == 2) {
			result += "0";
			result += digits[11];
		}
	}

	if (!(flags & string_left)) {
		while (size -- > 0) {
			result += c;
		}
	}
	while (static_cast<int>(tmp.size()) < precision--) {
		result += '0';
	}
	for (auto cptr = tmp.rbegin(); cptr != tmp.rend(); ++cptr) {
		result += *cptr;
	}
	while (size-- > 0) {
		result += ' ';
	}

	return result;
}

void force_decimal_point(string &buffer) {

	string::iterator cptr = buffer.begin();

	while (cptr != buffer.end()) {
		if (*cptr == '.') {
			return;
		}
		if ((*cptr == 'e') || (*cptr == 'E')) {
			break;
		}
		++cptr;
	}

	if (cptr != buffer.end()) {
		buffer.insert(cptr, '.');
	}
	else {
		buffer += '.';
	}
}

void crop_zeros(string &buffer) {

	string::iterator stop = buffer.end();
	string::iterator start = buffer.begin();

	while ((start != buffer.end()) && (*start != '.')) {
		start++;
	}
	if (start++ != buffer.end()) {
		while ((start != buffer.end()) && (*start != 'e') && (*start != 'E')) {
			start++;
		}
		stop = start--;
		while (*start == '0') {
			start--;
		}
		if (*start == '.') {
			start--;
		}
		buffer.erase(start + 1, stop);
	}
}

template<typename number_t>
string digits_to_string(number_t number, int base, DigitsFormat format, int precision, bool capexp, int *decpt, bool *sign) {

	string result;
	number_t fi, fj;
	const char *digits = (capexp) ? upper_digits : lower_digits;

	int r2 = 0;
	*sign = false;
	if (number < 0) {
		*sign = true;
		number = -number;
	}
	number = modf(number, &fi);

	if (fi != 0.) {
		string buffer;
		while (fi != 0.) {
			fj = modf(fi / base, &fi);
			buffer += digits[static_cast<int>((fj + .03) * base)];
			r2++;
		}
		for (auto i = buffer.rbegin(); i != buffer.rend(); ++i) {
			result += *i;
		}
	}
	else if (number > 0) {
		while ((fj = number * base) < 1) {
			number = fj;
			r2--;
		}
	}
	int pos = precision;
	if (format == decimal_format) {
		pos += r2;
	}
	*decpt = r2;
	if (pos < 0) {
		return result;
	}
	while (result.size() <= static_cast<size_t>(pos)) {
		number *= base;
		number = modf(number, &fj);
		result += digits[static_cast<int>(fj)];
	}
	int last = pos;
	result[static_cast<size_t>(pos)] += static_cast<char>(base >> 1);
	while (result[static_cast<size_t>(pos)] > digits[base - 1]) {
		result[static_cast<size_t>(pos)] = '0';
		if (pos > 0) {
			++result[static_cast<size_t>(--pos)];
		}
		else {
			result[static_cast<size_t>(pos)] = '1';
			(*decpt)++;
			if (format == decimal_format) {
				if (last > 0) {
					result[static_cast<size_t>(last)] = '0';
				}
				result.push_back('0');
				last++;
			}
		}
	}
	while(last < static_cast<int>(result.size())) {
		result.pop_back();
	}
	return result;
}

template<typename number_t>
string real_to_string(number_t number, int base, DigitsFormat format, int precision, bool capexp) {

	string result;
	int decpt = 0;
	bool sign = false;
	const char *digits = (capexp) ? upper_digits : lower_digits;

	if (isinf(number)) {
		return inf_string;
	}

	if (isnan(number)) {
		return nan_string;
	}

	if (format == shortest_format) {
		digits_to_string(number, base, scientific_format, precision, capexp, &decpt, &sign);
		int magnitude = decpt - 1;
		if ((magnitude < -4) || (magnitude > precision - 1)) {
			format = scientific_format;
			precision -= 1;
		}
		else {
			format = decimal_format;
			precision -= decpt;
		}
	}

	if (format == scientific_format) {
		string num_digits = digits_to_string(number, base, format, precision + 1, capexp, &decpt, &sign);

		if (sign) {
			result += '-';
		}
		result += num_digits.front();
		if (precision > 0) {
			result += '.';
		}
		result += string(num_digits.data() + 1, static_cast<size_t>(precision)) + (capexp ? 'E' : 'e');

		int exp = 0;

		if (decpt == 0) {
			if (number == 0.0) {
				exp = 0;
			}
			else {
				exp = -1;
			}
		}
		else {
			exp = decpt - 1;
		}

		if (exp < 0) {
			result += '-';
			exp = -exp;
		}
		else {
			result += '+';
		}

		char buffer[4];
		char *cptr = &buffer[4];
		*(--cptr) = '\0';

		while (exp && buffer < cptr) {
			*(--cptr) = digits[(exp % base)];
			exp = exp / base;
		}

		result += cptr;
	}
	else if (format == decimal_format) {
		string num_digits = digits_to_string(number, base, format, precision, capexp, &decpt, &sign);
		if (sign) {
			result += '-';
		}
		if (!num_digits.empty()) {
			if (decpt <= 0) {
				result += '0';
				result += '.';
				for (int pos = 0; pos < -decpt; pos++) {
					result += '0';
				}
				result += num_digits;
			}
			else {
				for (size_t pos = 0; pos < num_digits.size(); ++pos) {
					if (static_cast<int>(pos) == decpt) {
						result += '.';
					}
					result += num_digits[pos];
				}
			}
		}
		else {
			result += '0';
			if (precision > 0) {
				result += '.';
				for (int pos = 0; pos < precision; pos++) {
					result += '0';
				}
			}
		}
	}

	return result;
}

template<typename number_t>
string string_real(number_t number, int base, DigitsFormat format, int size, int precision, int flags) {

	string result;
	string buffer;

	if (flags & string_left) {
		flags &= ~string_zeropad;
	}

	char c = (flags & string_zeropad) ? '0' : ' ';
	char sign = 0;
	if (flags & string_sign) {
		if (number < 0.0) {
			sign = '-';
			number = -number;
			size--;
		}
		else if (flags & string_plus) {
			sign = '+';
			size--;
		}
		else if (flags & string_space) {
			sign = ' ';
			size--;
		}
	}

	if (precision < 0) {
		precision = 6;
	}
	else if ((precision == 0) && (format == shortest_format)) {
		precision = 1;
	}

	buffer = real_to_string(number, base, format, precision, flags & string_large);

	if ((flags & string_special) && (precision == 0)) {
		force_decimal_point(buffer);
	}

	if ((format == shortest_format) && !(flags & string_special)) {
		crop_zeros(buffer);
	}

	size -= static_cast<int>(buffer.size());
	if (!(flags & (string_zeropad | string_left))) {
		while (size-- > 0) {
			result += ' ';
		}
	}
	if (sign) {
		result += sign;
	}
	if (!(flags & string_left)) {
		while (size-- > 0) {
			result += c;
		}
	}
	result += buffer;
	while (size-- > 0) {
		result += ' ';
	}

	return result;
}

size_t string_index(const std::string &str, intmax_t index) {

	size_t i = (index < 0) ? static_cast<size_t>(index) + str.size() : static_cast<size_t>(index);

	if (UNLIKELY(i >= str.size())) {
		error("string index '%ld' is out of range", index);
	}

	return i;
}

string::iterator string_next(string &str, size_t index) {
	return next(begin(str), static_cast<string::difference_type>(index));
}

void string_format(Cursor *cursor, string &dest, const string &format, Iterator *args) {

	int flags = 0;

	int field_width;
	int precision;

	for (string::const_iterator cptr = format.begin(); cptr != format.end(); cptr++) {

		if ((*cptr == '%') && !args->ctx.empty()) {

			optional<WeakReference> argv = iterator_next(args);
			bool handled = false;

			while (!handled && cptr != format.end()) {
				if (UNLIKELY(++cptr == format.end())) {
					error("incomplete format '%s'", format.c_str());
				}
				switch (*cptr) {
				case '-':
					flags |= string_left;
					continue;
				case '+':
					flags |= string_plus;
					continue;
				case ' ':
					flags |= string_space;
					continue;
				case '#':
					flags |= string_special;
					continue;
				case '0':
					flags |= string_zeropad;
					continue;
				default:
					handled = true;
					break;
				}

				field_width = -1;
				if (isdigit(*cptr)) {
					string num;
					while (isdigit(*cptr)) {
						num += *cptr;
						if (UNLIKELY(++cptr == format.end())) {
							error("incomplete format '%s'", format.c_str());
						}
					}
					field_width = atoi(num.c_str());
				}
				else if (*cptr == '*') {
					if (UNLIKELY(++cptr == format.end())) {
						error("incomplete format '%s'", format.c_str());
					}
					field_width = static_cast<int>(to_integer(cursor, *argv));
					argv = iterator_next(args);
					if (field_width < 0) {
						field_width = -field_width;
						flags |= string_left;
					}
				}

				precision = -1;
				if (*cptr == '.') {
					if (UNLIKELY(++cptr == format.end())) {
						error("incomplete format '%s'", format.c_str());
					}
					if (isdigit(*cptr)) {
						string num;
						while (isdigit(*cptr)) {
							num += *cptr;
							if (UNLIKELY(++cptr == format.end())) {
								error("incomplete format '%s'", format.c_str());
							}
						}
						precision = atoi(num.c_str());
					}
					else if (*cptr == '*') {
						if (UNLIKELY(++cptr == format.end())) {
							error("incomplete format '%s'", format.c_str());
						}
						precision = static_cast<int>(to_integer(cursor, *argv));
						argv = iterator_next(args);
					}
					if (precision < 0) {
						precision = 0;
					}
				}

				string s;
				int len;
				int base = 10;

				switch (*cptr) {
				case 'c':
					if (!(flags & string_left)) while (--field_width > 0) dest += ' ';
					dest += to_char(*argv);
					while (--field_width > 0) dest += ' ';
					continue;
				case 's':
					s = to_string(*argv);
					len = (precision < 0) ? static_cast<int>(s.size()) : min(precision, static_cast<int>(s.size()));
					if (!(flags & string_left)) while (len < field_width--) dest += ' ';
					dest += s.substr(0, static_cast<size_t>(len));
					while (len < field_width--) dest += ' ';
					continue;
				case 'P':
					flags |= string_large;
					fall_through;
				case 'p':
					if (field_width == -1) {
						field_width = 2 * sizeof(void *);
						flags |= string_zeropad;
					}
#ifdef OS_WINDOWS
				{
					void *ptr = argv->data();
					dest += string_integer(*reinterpret_cast<uintptr_t *>(&ptr), 16, field_width, precision, flags);
				}
#else
					dest += string_integer(reinterpret_cast<uintptr_t>(argv->data()), 16, field_width, precision, flags);
#endif
					continue;
				case 'A':
					flags |= string_large;
					fall_through;
				case 'a':
					dest += string_real(to_number(cursor, *argv), 16, decimal_format, field_width, precision, flags);
					continue;
				case 'B':
					flags |= string_large;
					fall_through;
				case 'b':
					base = 2;
					break;
				case 'O':
					flags |= string_large;
					fall_through;
				case 'o':
					base = 8;
					break;
				case 'X':
					flags |= string_large;
					fall_through;
				case 'x':
					base = 16;
					break;
				case 'd':
				case 'i':
					flags |= string_sign;
					break;
				case 'u':
					break;
				case 'E':
					flags |= string_large;
					fall_through;
				case 'e':
					dest += string_real(to_number(cursor, *argv), 10, scientific_format, field_width, precision, flags | string_sign);
					continue;
				case 'F':
					flags |= string_large;
					fall_through;
				case 'f':
					dest += string_real(to_number(cursor, *argv), 10, decimal_format, field_width, precision, flags | string_sign);
					continue;
				case 'G':
					flags |= string_large;
					fall_through;
				case 'g':
					dest += string_real(to_number(cursor, *argv), 10, shortest_format, field_width, precision, flags | string_sign);
					continue;
				default:
					dest += *cptr;
					continue;
				}

				if (flags & string_sign) {
					dest += string_integer(to_integer(cursor, *argv), base, field_width, precision, flags);
				}
				else {
					dest += string_integer(to_integer(cursor, *argv), base, field_width, precision, flags);
				}
			}
		}
		else {
			dest += *cptr;
		}
	}
}

string mint::to_string(intmax_t value) {
	return string_integer(value, 10, -1, -1, 0);
}

string mint::to_string(double value) {
	return string_real(value, 10, shortest_format, -1, -1, 0);
}

string mint::to_string(void *value) {
	char buffer[(sizeof(void *) * 2) + 3];
	sprintf(buffer, "0x%0*" PRIXPTR,
			static_cast<int>(sizeof(void *) * 2),
			reinterpret_cast<uintptr_t>(value));
	return buffer;
}
