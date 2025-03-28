/**
 * Copyright (c) 2025 Gauvain CHERY.
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
#include "mint/memory/functiontool.h"
#include "mint/memory/casttool.h"
#include "mint/ast/abstractsyntaxtree.h"
#include "mint/ast/cursor.h"
#include "mint/system/utf8.h"
#include <algorithm>

using namespace mint;

namespace {

WeakReference sub_match_to_iterator(const std::string &str, const std::smatch &match, size_t index) {

	WeakReference item = WeakReference::create<Iterator>();
	std::string match_str = match[index].str();

	iterator_yield(item.data<Iterator>(), create_string(match_str));
	iterator_yield(item.data<Iterator>(), WeakReference::create<Number>(static_cast<double>(
											  utf8_byte_index_to_code_point_index(str, match.position(index)))));
	iterator_yield(item.data<Iterator>(),
				   WeakReference::create<Number>(static_cast<double>(utf8_code_point_count(match_str))));

	item.data<Iterator>()->construct();
	return item;
}

WeakReference match_to_iterator(const std::string &str, const std::smatch &match) {

	WeakReference result = WeakReference::create<Iterator>();

	for (size_t index = 0; index < match.size(); ++index) {
		iterator_yield(result.data<Iterator>(), sub_match_to_iterator(str, match, index));
	}

	result.data<Iterator>()->construct();
	return result;
}

}

RegexClass *RegexClass::instance() {
	return GlobalData::instance()->builtin<RegexClass>(Class::REGEX);
}

Regex::Regex() :
	Object(RegexClass::instance()) {}

Regex::Regex(Regex &&other) noexcept :
	Object(RegexClass::instance()),
	initializer(std::move(other.initializer)),
	expr(std::move(other.expr)) {}

Regex::Regex(const Regex &other) :
	Object(RegexClass::instance()),
	initializer(other.initializer),
	expr(other.expr) {}

Regex &Regex::operator=(Regex &&other) noexcept {
	initializer = std::move(other.initializer);
	expr = std::move(other.expr);
	return *this;
}

Regex &Regex::operator=(const Regex &other) {
	initializer = other.initializer;
	expr = other.expr;
	return *this;
}

RegexClass::RegexClass() :
	Class("regex", Class::REGEX) {

	AbstractSyntaxTree *ast = AbstractSyntaxTree::instance();

	create_builtin_member(COPY_OPERATOR, ast->create_builtin_method(this, 2, [](Cursor *cursor) {
		const size_t base = get_stack_base(cursor);

		Reference &other = load_from_stack(cursor, base);
		Reference &self = load_from_stack(cursor, base - 1);

		if ((other.data()->format == Data::FMT_OBJECT) && (other.data<Object>()->metadata->metatype() == Class::REGEX)) {
			self.data<Regex>()->initializer = other.data<Regex>()->initializer;
		}
		else {
			self.data<Regex>()->initializer = "/" + to_string(other) + "/";
		}
		self.data<Regex>()->expr = to_regex(other);

		cursor->stack().pop_back();
	}));

	create_builtin_member(REGEX_MATCH_OPERATOR, ast->create_builtin_method(this, 2, [](Cursor *cursor) {
		const size_t base = get_stack_base(cursor);

		const Reference &rvalue = load_from_stack(cursor, base);
		const Reference &self = load_from_stack(cursor, base - 1);
		const bool result = regex_search(to_string(rvalue), self.data<Regex>()->expr);

		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(WeakReference::create<Boolean>(result));
	}));

	create_builtin_member(REGEX_UNMATCH_OPERATOR, ast->create_builtin_method(this, 2, [](Cursor *cursor) {
		const size_t base = get_stack_base(cursor);

		const Reference &rvalue = load_from_stack(cursor, base);
		const Reference &self = load_from_stack(cursor, base - 1);
		const bool result = !regex_search(to_string(rvalue), self.data<Regex>()->expr);

		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(WeakReference::create<Boolean>(result));
	}));

	create_builtin_member("match", ast->create_builtin_method(this, 2, [](Cursor *cursor) {
		const size_t base = get_stack_base(cursor);

		const Reference &str = load_from_stack(cursor, base);
		const Reference &self = load_from_stack(cursor, base - 1);

		std::smatch match;
		std::smatch::string_type s = to_string(str);

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

	create_builtin_member("search", ast->create_builtin_method(this, 2, [](Cursor *cursor) {
		const size_t base = get_stack_base(cursor);

		const Reference &str = load_from_stack(cursor, base);
		const Reference &self = load_from_stack(cursor, base - 1);

		std::smatch match;
		std::smatch::string_type s = to_string(str);

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

	create_builtin_member("getFlags", ast->create_builtin_method(this, 1, [](Cursor *cursor) {
		const Reference &self = cursor->stack().back();
		cursor->stack().back() = create_string(
			self.data<Regex>()->initializer.substr(self.data<Regex>()->initializer.rfind('/') + 1));
	}));
}
