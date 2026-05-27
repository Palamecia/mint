/**
 * Copyright (c) 2026 Gauvain CHERY.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "mint/memory/builtin/regex.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/memory/class.h"
#include "mint/memory/data.h"
#include "mint/memory/function_tools.h"
#include "mint/memory/cast_tools.h"
#include "mint/ast/abstract_syntax_tree.h"
#include "mint/ast/cursor.h"
#include "mint/memory/reference.h"
#include "mint/system/utf8.h"
#include "mint/scheduler/scheduler.h"
#include <cstddef>
#include <regex>
#include <string>
#include <utility>

using namespace mint;

namespace {

Reference sub_match_to_iterator(Cursor& cursor, const std::string& str, const std::smatch& match, std::size_t index) {
	const auto match_str = match[index].str();
	return create_iterator_from(cursor, create_string(cursor.ast(), match_str),
	    create_unsigned_number(utf8_byte_index_to_code_point_index(str, match.position(index))),
	    create_unsigned_number(utf8_code_point_count(match_str)));
}

Reference match_to_iterator(Cursor& cursor, const std::string& str, const std::smatch& match) {

	Reference result = create_iterator(cursor.ast());

	for (std::size_t index = 0; index < match.size(); ++index) {
		iterator_yield(cursor, result.data<Iterator>(), sub_match_to_iterator(cursor, str, match, index));
	}

	return result;
}

}

RegexClass& RegexClass::instance(AbstractSyntaxTree& ast) {
	return ast.global_data().builtin<RegexClass>(Class::Metatype::regex);
}

Regex::Regex(AbstractSyntaxTree& ast) :
    Object(RegexClass::instance(ast)) {}

Regex::Regex(Regex&& other) noexcept :
    Object(other.metadata),
    initializer(std::move(other.initializer)),
    expr(std::move(other.expr)) {}

Regex::Regex(const Regex& other) :
    Object(other.metadata),
    initializer(other.initializer),
    expr(other.expr) {}

Regex& Regex::operator=(Regex&& other) noexcept {
	initializer = std::move(other.initializer);
	expr = std::move(other.expr);
	return *this;
}

Regex& Regex::operator=(const Regex& other) {
	initializer = other.initializer;
	expr = other.expr;
	return *this;
}

RegexClass::RegexClass(AbstractSyntaxTree& ast) :
    Class(ast.global_data(), "regex", Class::Metatype::regex) {

	create_builtin_member(copy_operator, ast.create_builtin_method(*this, 2, [](Cursor& cursor) {
		const auto base = get_stack_base(cursor);

		const auto& other = load_from_stack(cursor, base);
		const auto& self = load_from_stack(cursor, base - 1);

		if ((other.data().format() == Data::Format::object)
		    && (other.data<Object>().metadata.metatype() == Class::Metatype::regex)) {
			self.data<Regex>().initializer = other.data<Regex>().initializer;
		}
		else {
			self.data<Regex>().initializer = "/" + to_string(other) + "/";
		}
		self.data<Regex>().expr = to_regex(other);

		cursor.stack().pop_back();
	}));

	create_builtin_member(regex_match_operator, ast.create_builtin_method(*this, 2, [](Cursor& cursor) {
		const auto base = get_stack_base(cursor);

		const Reference& rvalue = load_from_stack(cursor, base);
		const Reference& self = load_from_stack(cursor, base - 1);
		const bool result = regex_search(to_string(rvalue), self.data<Regex>().expr);

		cursor.stack().pop_back();
		cursor.stack().pop_back();
		cursor.stack().emplace_back(create_boolean(result));
	}));

	create_builtin_member(regex_unmatch_operator, ast.create_builtin_method(*this, 2, [](Cursor& cursor) {
		const auto base = get_stack_base(cursor);

		const Reference& rvalue = load_from_stack(cursor, base);
		const Reference& self = load_from_stack(cursor, base - 1);
		const bool result = !regex_search(to_string(rvalue), self.data<Regex>().expr);

		cursor.stack().pop_back();
		cursor.stack().pop_back();
		cursor.stack().emplace_back(create_boolean(result));
	}));

	create_builtin_member("match", ast.create_builtin_method(*this, 2, [](Cursor& cursor) {
		const auto base = get_stack_base(cursor);

		const Reference& str = load_from_stack(cursor, base);
		const Reference& self = load_from_stack(cursor, base - 1);

		std::smatch match;
		const std::smatch::string_type s = to_string(str);

		if (regex_match(s, match, self.data<Regex>().expr)) {
			cursor.stack().pop_back();
			cursor.stack().pop_back();
			cursor.stack().emplace_back(match_to_iterator(cursor, s, match));
		}
		else {
			cursor.stack().pop_back();
			cursor.stack().pop_back();
			cursor.stack().emplace_back(create_none());
		}
	}));

	create_builtin_member("search", ast.create_builtin_method(*this, 2, [](Cursor& cursor) {
		const auto base = get_stack_base(cursor);

		const Reference& str = load_from_stack(cursor, base);
		const Reference& self = load_from_stack(cursor, base - 1);

		std::smatch match;
		const std::smatch::string_type s = to_string(str);

		if (regex_search(s, match, self.data<Regex>().expr)) {
			cursor.stack().pop_back();
			cursor.stack().pop_back();
			cursor.stack().emplace_back(match_to_iterator(cursor, s, match));
		}
		else {
			cursor.stack().pop_back();
			cursor.stack().pop_back();
			cursor.stack().emplace_back(create_none());
		}
	}));

	create_builtin_member("getFlags", ast.create_builtin_method(*this, 1, [](Cursor& cursor) {
		const Reference& self = cursor.stack().back();
		cursor.stack().back() = create_string(cursor.ast(),
		    self.data<Regex>().initializer.substr(self.data<Regex>().initializer.rfind('/') + 1));
	}));
}
