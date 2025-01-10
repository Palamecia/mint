/**
 * Copyright (c) 2024 Gauvain CHERY.
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

#include "mint/memory/builtin/string.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/memory/algorithm.hpp"
#include "mint/memory/casttool.h"
#include "mint/memory/functiontool.h"
#include "mint/ast/abstractsyntaxtree.h"
#include "mint/ast/cursor.h"
#include "mint/system/string.h"
#include "mint/system/utf8.h"
#include "mint/system/error.h"

#include <iterator>
#include <cstring>

using namespace mint;

inline std::size_t string_index(const std::string &str, intmax_t index);
inline std::string::const_iterator string_next(const std::string &str, size_t index);
inline void string_format(mint::Cursor *cursor, std::string &dest, const std::string &format, mint::Iterator *args);

StringClass *StringClass::instance() {
	return GlobalData::instance()->builtin<StringClass>(Class::STRING);
}

String::String() : Object(StringClass::instance()) {

}

String::String(const String &other) : Object(StringClass::instance()),
	str(other.str) {

}

String::String(const char *value) : Object(StringClass::instance()),
	str(value) {

}

String::String(const std::string &value) : Object(StringClass::instance()),
	str(value) {

}

String::String(std::string_view value) : Object(StringClass::instance()),
	str(value) {

}

StringClass::StringClass() : Class("string", Class::STRING) {

	AbstractSyntaxTree *ast = AbstractSyntaxTree::instance();
	
	create_builtin_member(COPY_OPERATOR, ast->create_builtin_method(this, 2, [] (Cursor *cursor) {

		const size_t base = get_stack_base(cursor);

		const Reference &rvalue = load_from_stack(cursor, base);
		const Reference &self = load_from_stack(cursor, base - 1);

		self.data<String>()->str = to_string(rvalue);

		cursor->stack().pop_back();
	}));
	
	create_builtin_member(REGEX_MATCH_OPERATOR, ast->create_builtin_method(this, 2, [] (Cursor *cursor) {
		
		const size_t base = get_stack_base(cursor);
		
		Reference &rvalue = load_from_stack(cursor, base);
		Reference &self = load_from_stack(cursor, base - 1);
		const bool result = regex_search(self.data<String>()->str, to_regex(rvalue));
		
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(WeakReference::create<Boolean>(result));
	}));
	
	create_builtin_member(REGEX_UNMATCH_OPERATOR, ast->create_builtin_method(this, 2, [] (Cursor *cursor) {
		
		const size_t base = get_stack_base(cursor);
		
		Reference &rvalue = load_from_stack(cursor, base);
		Reference &self = load_from_stack(cursor, base - 1);
		const bool result = !regex_search(self.data<String>()->str, to_regex(rvalue));
		
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(WeakReference::create<Boolean>(result));
	}));
	
	create_builtin_member(ADD_OPERATOR, ast->create_builtin_method(this, 2, [] (Cursor *cursor) {
		
		const size_t base = get_stack_base(cursor);
		
		const Reference &rvalue = load_from_stack(cursor, base);
		const Reference &self = load_from_stack(cursor, base - 1);
		WeakReference result = create_string(self.data<String>()->str + to_string(rvalue));
		
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(std::forward<Reference>(result));
	}));
	
	create_builtin_member(MUL_OPERATOR, ast->create_builtin_method(this, 2, [] (Cursor *cursor) {
		
		const size_t base = get_stack_base(cursor);
		
		Reference &rvalue = load_from_stack(cursor, base);
		Reference &self = load_from_stack(cursor, base - 1);
		std::string result;
		
		for (intmax_t i = 0; i < to_integer(cursor, rvalue); ++i) {
			result += self.data<String>()->str;
		}
		
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(create_string(result));
	}));
	
	create_builtin_member(MOD_OPERATOR, ast->create_builtin_method(this, 2, [] (Cursor *cursor) {
		
		const size_t base = get_stack_base(cursor);
		
		Reference &values = load_from_stack(cursor, base);
		Reference &self = load_from_stack(cursor, base - 1);
		
		std::string result;
		
		if (is_instance_of(values, Class::ITERATOR)) {
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
	
	create_builtin_member(SHIFT_LEFT_OPERATOR, ast->create_builtin_method(this, 2, [] (Cursor *cursor) {
		
		const size_t base = get_stack_base(cursor);
		
		const Reference &other = load_from_stack(cursor, base);
		const Reference &self = load_from_stack(cursor, base - 1);
		
		if (self.flags() & Reference::CONST_VALUE) {
			cursor->stack().pop_back();
			cursor->stack().back() = create_string(self.data<String>()->str + to_string(other));
		}
		else {
			self.data<String>()->str.append(to_string(other));
			cursor->stack().pop_back();
		}
	}));
	
	create_builtin_member(EQ_OPERATOR, ast->create_builtin_method(this, 2, [] (Cursor *cursor) {
		
		const size_t base = get_stack_base(cursor);
		
		const Reference &rvalue = load_from_stack(cursor, base);
		const Reference &self = load_from_stack(cursor, base - 1);
		WeakReference result = WeakReference::create<Boolean>(self.data<String>()->str == to_string(rvalue));
		
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(std::forward<Reference>(result));
	}));
	
	create_builtin_member(NE_OPERATOR, ast->create_builtin_method(this, 2, [] (Cursor *cursor) {
		
		const size_t base = get_stack_base(cursor);
		
		const Reference &rvalue = load_from_stack(cursor, base);
		const Reference &self = load_from_stack(cursor, base - 1);
		WeakReference result = WeakReference::create<Boolean>(self.data<String>()->str != to_string(rvalue));
		
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(std::forward<Reference>(result));
	}));
	
	create_builtin_member(LT_OPERATOR, ast->create_builtin_method(this, 2, [] (Cursor *cursor) {
		
		const size_t base = get_stack_base(cursor);
		
		const Reference &rvalue = load_from_stack(cursor, base);
		const Reference &self = load_from_stack(cursor, base - 1);
		WeakReference result = WeakReference::create<Boolean>(self.data<String>()->str < to_string(rvalue));
		
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(std::forward<Reference>(result));
	}));
	
	create_builtin_member(GT_OPERATOR, ast->create_builtin_method(this, 2, [] (Cursor *cursor) {
		
		const size_t base = get_stack_base(cursor);
		
		const Reference &rvalue = load_from_stack(cursor, base);
		const Reference &self = load_from_stack(cursor, base - 1);
		WeakReference result = WeakReference::create<Boolean>(self.data<String>()->str > to_string(rvalue));
		
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(std::forward<Reference>(result));
		
	}));
	
	create_builtin_member(LE_OPERATOR, ast->create_builtin_method(this, 2, [] (Cursor *cursor) {
		
		const size_t base = get_stack_base(cursor);
		
		const Reference &rvalue = load_from_stack(cursor, base);
		const Reference &self = load_from_stack(cursor, base - 1);
		WeakReference result = WeakReference::create<Boolean>(self.data<String>()->str <= to_string(rvalue));
		
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(std::forward<Reference>(result));
	}));
	
	create_builtin_member(GE_OPERATOR, ast->create_builtin_method(this, 2, [] (Cursor *cursor) {
		
		const size_t base = get_stack_base(cursor);
		
		const Reference &rvalue = load_from_stack(cursor, base);
		const Reference &self = load_from_stack(cursor, base - 1);
		WeakReference result = WeakReference::create<Boolean>(self.data<String>()->str >= to_string(rvalue));
		
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(std::forward<Reference>(result));
	}));

	create_builtin_member(NOT_OPERATOR, ast->create_builtin_method(this, 1, [] (Cursor *cursor) {
		
		const Reference &self = cursor->stack().back();
		WeakReference result = WeakReference::create<Boolean>(self.data<String>()->str.empty());
		
		cursor->stack().pop_back();
		cursor->stack().emplace_back(std::forward<Reference>(result));
	}));

	create_builtin_member(SUBSCRIPT_OPERATOR, ast->create_builtin_method(this, 2, [] (Cursor *cursor) {
		
		const size_t base = get_stack_base(cursor);
		
		Reference &index = load_from_stack(cursor, base);
		Reference &self = load_from_stack(cursor, base - 1);
		WeakReference result = WeakReference::create<String>();
		result.data<String>()->construct();
		
		if ((index.data()->format != Data::FMT_OBJECT) || (index.data<Object>()->metadata->metatype() != Class::ITERATOR)) {
			std::string &string_ref = self.data<String>()->str;
			auto offset = string_index(string_ref, to_integer(cursor, index));
			result.data<String>()->str = *(utf8iterator(string_ref.begin()) + static_cast<size_t>(offset));
		}
		else if (index.data<Iterator>()->ctx.getType() == Iterator::ctx_type::RANGE) {
			
			const std::string &string_ref = self.data<String>()->str;
			size_t begin_index = string_index(string_ref, to_integer(cursor, index.data<Iterator>()->ctx.next()));
			size_t end_index = string_index(string_ref, to_integer(cursor, index.data<Iterator>()->ctx.back()));
			
			if (begin_index > end_index) {
				std::swap(begin_index, end_index);
			}
			
			std::string::const_iterator begin = string_ref.begin() + static_cast<int>(utf8_code_point_index_to_byte_index(string_ref, begin_index));
			std::string::const_iterator end = string_ref.begin() + static_cast<int>(utf8_code_point_index_to_byte_index(string_ref, end_index));
			
			end += static_cast<int>(utf8_code_point_length(static_cast<byte_t>(*end)));
			result.data<String>()->str = std::string(begin, end);
		}
		else {
			std::string &string_ref = self.data<String>()->str;
			while (std::optional<WeakReference> &&item = iterator_next(index.data<Iterator>())) {
				result.data<String>()->str += *(utf8iterator(string_ref.begin()) + string_index(string_ref, to_integer(cursor, *item)));
			}
		}
		
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(std::forward<Reference>(result));
	}));
	
	create_builtin_member(SUBSCRIPT_MOVE_OPERATOR, ast->create_builtin_method(this, 3, [] (Cursor *cursor) {
		
		const size_t base = get_stack_base(cursor);
		
		Reference &value = load_from_stack(cursor, base);
		Reference &index = load_from_stack(cursor, base - 1);
		Reference &self = load_from_stack(cursor, base - 2);
		
		if ((index.data()->format != Data::FMT_OBJECT) || (index.data<Object>()->metadata->metatype() != Class::ITERATOR)) {
			std::string &string_ref = self.data<String>()->str;
			auto offset = string_index(string_ref, to_integer(cursor, index));
			auto utf8_index = utf8_code_point_index_to_byte_index(string_ref, offset);
			auto utf8_length = utf8_code_point_length(static_cast<byte_t>(string_ref[utf8_index]));
			string_ref.replace(utf8_index, utf8_length, to_string(value));
			
			cursor->stack().pop_back();
			cursor->stack().pop_back();
			cursor->stack().pop_back();
			cursor->stack().emplace_back(std::forward<Reference>(value));
		}
		else if (index.data<Iterator>()->ctx.getType() == Iterator::ctx_type::RANGE) {
			
			std::string &string_ref = self.data<String>()->str;
			size_t begin_index = string_index(string_ref, to_integer(cursor, index.data<Iterator>()->ctx.next()));
			size_t end_index = string_index(string_ref, to_integer(cursor, index.data<Iterator>()->ctx.back()));
			
			if (begin_index > end_index) {
				std::swap(begin_index, end_index);
			}
			
			std::string::const_iterator begin = string_next(string_ref, utf8_code_point_index_to_byte_index(string_ref, begin_index));
			std::string::const_iterator end = string_next(string_ref, utf8_code_point_index_to_byte_index(string_ref, end_index));
			
			std::advance(end, utf8_code_point_length(static_cast<byte_t>(*end)));
			string_ref.replace(begin, end, to_string(value));
			
			cursor->stack().pop_back();
			cursor->stack().pop_back();
		}
		else {
			
			size_t offset = 0;
			std::string &string_ref = self.data<String>()->str;
			
			for_each(value, [cursor, &string_ref, &offset, &index] (const Reference &ref) {
				if (!index.data<Iterator>()->ctx.empty()) {
					offset = utf8_code_point_index_to_byte_index(string_ref, string_index(string_ref, to_integer(cursor, index.data<Iterator>()->ctx.next())));
					size_t utf8_length = utf8_code_point_length(static_cast<byte_t>(string_ref.at(offset)));
					string_ref.replace(offset, utf8_length, to_string(ref));
					index.data<Iterator>()->ctx.pop();
					offset += utf8_length;
				}
				else {
					size_t length = utf8_code_point_length(static_cast<byte_t>(string_ref.at(offset)));
					string_ref.insert(offset, to_string(ref));
					offset += length;
				}
			});
			
			std::map<size_t, size_t> to_remove;
			
			while (!index.data<Iterator>()->ctx.empty()) {
				offset = utf8_code_point_index_to_byte_index(string_ref, string_index(string_ref, to_integer(cursor, index.data<Iterator>()->ctx.next())));
				size_t utf8_length = utf8_code_point_length(static_cast<byte_t>(string_ref.at(offset)));
				to_remove.insert({offset, utf8_length});
				index.data<Iterator>()->ctx.pop();
			}
			
			for (auto i = to_remove.rbegin(); i != to_remove.rend(); ++i) {
				string_ref.erase(i->first, i->second);
			}
			
			cursor->stack().pop_back();
			cursor->stack().pop_back();
		}
	}));
	
	create_builtin_member("insert", ast->create_builtin_method(this, 3, [] (Cursor *cursor) {
		
		const size_t base = get_stack_base(cursor);
		
		Reference &value = load_from_stack(cursor, base);
		Reference &index = load_from_stack(cursor, base - 1);
		Reference &self = load_from_stack(cursor, base - 2);
		
		std::string &string_ref = self.data<String>()->str;
		auto offset = string_index(string_ref, to_integer(cursor, index));
		auto utf8_index = utf8_code_point_index_to_byte_index(string_ref, offset);
		string_ref.insert(utf8_index, to_string(value));
		
		cursor->stack().pop_back();
		cursor->stack().pop_back();
	}));
	
	create_builtin_member(IN_OPERATOR, ast->create_builtin_method(this, 1, [] (Cursor *cursor) {
		cursor->stack().back() = WeakReference(Reference::CONST_ADDRESS, iterator_init(cursor->stack().back()));
	}));
	
	create_builtin_member(IN_OPERATOR, ast->create_builtin_method(this, 2, [] (Cursor *cursor) {
		
		const size_t base = get_stack_base(cursor);
		
		const Reference &value = load_from_stack(cursor, base);
		const Reference &self = load_from_stack(cursor, base - 1);
		WeakReference result = WeakReference::create<Boolean>(self.data<String>()->str.find(to_string(value)) != std::string::npos);
		
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(std::forward<Reference>(result));
	}));

	create_builtin_member("each", ast->create_builtin_method(this, 2, R"""(
						def (const self, const func) {
							for item in self {
								func(item)
							}
						})"""));
	
	create_builtin_member("isEmpty", ast->create_builtin_method(this, 1, [] (Cursor *cursor) {
		const Reference &self = cursor->stack().back();
		cursor->stack().back() = WeakReference::create<Boolean>(self.data<String>()->str.empty());
	}));
	
	create_builtin_member("size", ast->create_builtin_method(this, 1, [] (Cursor *cursor) {
		const Reference &self = cursor->stack().back();
		cursor->stack().back() = WeakReference::create<Number>(static_cast<double>(utf8_code_point_count(self.data<String>()->str)));
	}));
	
	create_builtin_member("clear", ast->create_builtin_method(this, 1, [] (Cursor *cursor) {
		const Reference &self = cursor->stack().back();
		if (UNLIKELY(self.flags() & Reference::CONST_VALUE)) {
			error("invalid modification of constant value");
		}
		self.data<String>()->str.clear();
		cursor->stack().back() = WeakReference::create<None>();
	}));
	
	create_builtin_member("substring", ast->create_builtin_method(this, 2, [] (Cursor *cursor) {

		const size_t base = get_stack_base(cursor);

		Reference &from = load_from_stack(cursor, base);
		Reference &self = load_from_stack(cursor, base - 1);

		const std::string substring = self.data<String>()->str.substr(utf8_code_point_index_to_byte_index(self.data<String>()->str, to_integer(cursor, from)));
		cursor->stack().pop_back();
		cursor->stack().back() = create_string(substring);
	}));

	create_builtin_member("substring", ast->create_builtin_method(this, 3, [] (Cursor *cursor) {

		const size_t base = get_stack_base(cursor);

		Reference &length = load_from_stack(cursor, base);
		Reference &from = load_from_stack(cursor, base - 1);
		Reference &self = load_from_stack(cursor, base - 2);

		std::string::size_type utf8_start = utf8_code_point_index_to_byte_index(self.data<String>()->str, to_integer(cursor, from));
		std::string::size_type utf8_length = length.data()->format != Data::FMT_NONE ? utf8_substring_byte_count(self.data<String>()->str, utf8_start, to_integer(cursor, length)) : std::string::npos;
		const std::string substring = self.data<String>()->str.substr(utf8_start, utf8_length);
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().back() = create_string(substring);
	}));

	create_builtin_member("replace", ast->create_builtin_method(this, 3, [] (Cursor *cursor) {

		const size_t base = get_stack_base(cursor);

		Reference &value = load_from_stack(cursor, base);
		Reference &pattern = load_from_stack(cursor, base - 1);
		Reference &self = load_from_stack(cursor, base - 2);

		std::string before = to_string(pattern);
		std::string after = to_string(value);

		if (self.flags() & Reference::CONST_VALUE) {

			std::string str = self.data<String>()->str;

			if (is_instance_of(pattern, Class::REGEX)) {
				str = regex_replace(str, to_regex(pattern), after);
			}
			else {
				size_t pos = 0;
				while ((pos = str.find(before, pos)) != std::string::npos) {
					str.replace(pos, before.size(), after);
					pos += after.size();
				}
			}

			cursor->stack().pop_back();
			cursor->stack().pop_back();
			cursor->stack().back() = create_string(str);
		}
		else {

			if (is_instance_of(pattern, Class::REGEX)) {
				self.data<String>()->str = regex_replace(self.data<String>()->str, to_regex(pattern), after);
			}
			else {
				size_t pos = 0;
				while ((pos = self.data<String>()->str.find(before, pos)) != std::string::npos) {
					self.data<String>()->str.replace(pos, before.size(), after);
					pos += after.size();
				}
			}

			cursor->stack().pop_back();
			cursor->stack().pop_back();
		}
	}));
	
	create_builtin_member("replace", ast->create_builtin_method(this, 4, [] (Cursor *cursor) {

		const size_t base = get_stack_base(cursor);

		Reference &value = load_from_stack(cursor, base);
		Reference &length = load_from_stack(cursor, base - 1);
		Reference &from = load_from_stack(cursor, base - 2);
		Reference &self = load_from_stack(cursor, base - 3);

		if (self.flags() & Reference::CONST_VALUE) {

			std::string string_copy = self.data<String>()->str;
			auto offset = string_index(string_copy, to_integer(cursor, from));
			auto utf8_index = utf8_code_point_index_to_byte_index(string_copy, offset);
			auto utf8_length = utf8_substring_byte_count(string_copy, offset, to_integer(cursor, length));
			string_copy.replace(utf8_index, utf8_length, to_string(value));

			cursor->stack().pop_back();
			cursor->stack().pop_back();
			cursor->stack().pop_back();
			cursor->stack().back() = create_string(string_copy);
		}
		else {

			std::string &string_ref = self.data<String>()->str;
			auto offset = string_index(string_ref, to_integer(cursor, from));
			auto utf8_index = utf8_code_point_index_to_byte_index(string_ref, offset);
			auto utf8_length = utf8_substring_byte_count(string_ref, offset, to_integer(cursor, length));
			string_ref.replace(utf8_index, utf8_length, to_string(value));

			cursor->stack().pop_back();
			cursor->stack().pop_back();
			cursor->stack().pop_back();
		}
	}));
	
	create_builtin_member("contains", ast->create_builtin_method(this, 2, [] (Cursor *cursor) {

		const size_t base = get_stack_base(cursor);

		Reference &other = load_from_stack(cursor, base);
		Reference &self = load_from_stack(cursor, base - 1);
		bool result;

		if (is_instance_of(other, Class::REGEX)) {
			result = regex_search(self.data<String>()->str, to_regex(other));
		}
		else {
			result = self.data<String>()->str.find(to_string(other)) != std::string::npos;
		}

		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(WeakReference::create<Boolean>(result));
	}));
	
	create_builtin_member("indexOf", ast->create_builtin_method(this, 2, [] (Cursor *cursor) {

		const size_t base = get_stack_base(cursor);

		Reference &other = load_from_stack(cursor, base);
		Reference &self = load_from_stack(cursor, base - 1);

		auto pos = std::string::npos;
		if (is_instance_of(other, Class::REGEX)) {
			std::smatch match;
			if (regex_search(self.data<String>()->str, match, to_regex(other))) {
				pos = static_cast<decltype (pos)>(match.position(0));
			}
		}
		else {
			pos = self.data<String>()->str.find(to_string(other));
		}

		WeakReference result = pos != std::string::npos
								   ? WeakReference::create<Number>(static_cast<double>(utf8_byte_index_to_code_point_index(self.data<String>()->str, pos)))
								   : WeakReference::create<None>();

		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(std::forward<Reference>(result));
	}));
	
	create_builtin_member("indexOf", ast->create_builtin_method(this, 3, [] (Cursor *cursor) {

		const size_t base = get_stack_base(cursor);

		Reference &from = load_from_stack(cursor, base);
		Reference &other = load_from_stack(cursor, base - 1);
		Reference &self = load_from_stack(cursor, base - 2);

		auto pos = std::string::npos;
		auto start = utf8_code_point_index_to_byte_index(self.data<String>()->str, static_cast<size_t>(to_number(cursor, from)));
		if (start != std::string::npos) {
			if (is_instance_of(other, Class::REGEX)) {
				std::regex expr = to_regex(other);
				auto begin = std::sregex_iterator(self.data<String>()->str.begin(), self.data<String>()->str.end(), expr);
				auto end = std::sregex_iterator();
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

		WeakReference result = pos != std::string::npos
								   ? WeakReference::create<Number>(static_cast<double>(utf8_byte_index_to_code_point_index(self.data<String>()->str, pos)))
								   : WeakReference::create<None>();

		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(std::forward<Reference>(result));
	}));
	
	create_builtin_member("lastIndexOf", ast->create_builtin_method(this, 2, [] (Cursor *cursor) {

		const size_t base = get_stack_base(cursor);

		Reference &other = load_from_stack(cursor, base);
		Reference &self = load_from_stack(cursor, base - 1);

		auto pos = std::string::npos;
		if (is_instance_of(other, Class::REGEX)) {
			std::regex expr = to_regex(other);
			auto begin = std::sregex_iterator(self.data<String>()->str.begin(), self.data<String>()->str.end(), expr);
			auto end = std::sregex_iterator();
			for (auto i = begin; i != end; ++i) {
				pos = static_cast<decltype (pos)>(i->position());
			}
		}
		else {
			pos = self.data<String>()->str.rfind(to_string(other));
		}

		WeakReference result = pos != std::string::npos
								   ? WeakReference::create<Number>(static_cast<double>(utf8_byte_index_to_code_point_index(self.data<String>()->str, pos)))
								   : WeakReference::create<None>();

		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(std::forward<Reference>(result));
	}));
	
	create_builtin_member("lastIndexOf", ast->create_builtin_method(this, 3, [] (Cursor *cursor) {

		const size_t base = get_stack_base(cursor);

		Reference &from = load_from_stack(cursor, base);
		Reference &other = load_from_stack(cursor, base - 1);
		Reference &self = load_from_stack(cursor, base - 2);

		auto pos = std::string::npos;
		auto start = utf8_code_point_index_to_byte_index(self.data<String>()->str, static_cast<size_t>(to_number(cursor, from)));
		if (start != std::string::npos) {
			if (is_instance_of(other, Class::REGEX)) {
				std::regex expr = to_regex(other);
				auto begin = std::sregex_iterator(self.data<String>()->str.begin(), self.data<String>()->str.end(), expr);
				auto end = std::sregex_iterator();
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

		WeakReference result = pos != std::string::npos
								   ? WeakReference::create<Number>(static_cast<double>(utf8_byte_index_to_code_point_index(self.data<String>()->str, pos)))
								   : WeakReference::create<None>();

		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(std::forward<Reference>(result));
	}));
	
	create_builtin_member("startsWith", ast->create_builtin_method(this, 2, [] (Cursor *cursor) {

		const size_t base = get_stack_base(cursor);

		Reference &other = load_from_stack(cursor, base);
		Reference &self = load_from_stack(cursor, base - 1);
		bool result;

		if (is_instance_of(other, Class::REGEX)) {
			std::smatch match;
			if (regex_search(self.data<String>()->str, match, to_regex(other))) {
				result = match.position(0) == 0;
			}
			else {
				result= false;
			}
		}
		else {
			result = starts_with(self.data<String>()->str, to_string(other));
		}

		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(WeakReference::create<Boolean>(result));
	}));
	
	create_builtin_member("endsWith", ast->create_builtin_method(this, 2, [] (Cursor *cursor) {

		const size_t base = get_stack_base(cursor);

		Reference &other = load_from_stack(cursor, base);
		Reference &self = load_from_stack(cursor, base - 1);
		bool result;

		if (is_instance_of(other, Class::REGEX)) {
			result = false;
			std::regex expr = to_regex(other);
			auto begin = std::sregex_iterator(self.data<String>()->str.begin(), self.data<String>()->str.end(), expr);
			auto end = std::sregex_iterator();
			for (auto i = begin; i != end; ++i) {
				if (size_t(0) + static_cast<size_t>(i->position() + i->length()) == self.data<String>()->str.size()) {
					result = true;
					break;
				}
			}
		}
		else {
			result = ends_with(self.data<String>()->str, to_string(other));
		}

		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(WeakReference::create<Boolean>(result));
	}));
	
	create_builtin_member("split", ast->create_builtin_method(this, 2, [] (Cursor *cursor) {

		const size_t base = get_stack_base(cursor);

		const Reference &sep = load_from_stack(cursor, base);
		const Reference &self = load_from_stack(cursor, base - 1);
		WeakReference result = create_array();

		std::string sep_str = to_string(sep);
		std::string self_str = self.data<String>()->str;

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
			if (!self_str.empty()) {
				array_append(result.data<Array>(), create_string(self_str.substr(from)));
			}
		}

		cursor->stack().pop_back();
		cursor->stack().pop_back();
		cursor->stack().emplace_back(std::forward<Reference>(result));
	}));
}

size_t string_index(const std::string &str, intmax_t index) {

	size_t i = (index < 0) ? static_cast<size_t>(index) + str.size() : static_cast<size_t>(index);

	if (UNLIKELY(i >= str.size())) {
		error("string index '%ld' is out of range", index);
	}

	return i;
}

std::string::const_iterator string_next(const std::string &str, size_t index) {
	return next(std::begin(str), static_cast<std::string::difference_type>(index));
}

void string_format(Cursor *cursor, std::string &dest, const std::string &format, Iterator *args) {

	for (std::string::const_iterator cptr = format.begin(); cptr != format.end(); ++cptr) {

		if ((*cptr == '%') && !args->ctx.empty()) {

			if (*(cptr + 1) == '%') {
				dest += '%';
				++cptr;
				continue;
			}

			std::optional<WeakReference> argv = iterator_next(args);
			StringFormatFlags flags = 0;
			bool handled = false;

			while (!handled && cptr != format.end()) {
				if (UNLIKELY(++cptr == format.end())) {
					error("incomplete format '%s'", format.c_str());
				}
				switch (*cptr) {
				case '-':
					flags |= STRING_LEFT;
					continue;
				case '+':
					flags |= STRING_PLUS;
					continue;
				case ' ':
					flags |= STRING_SPACE;
					continue;
				case '#':
					flags |= STRING_SPECIAL;
					continue;
				case '0':
					flags |= STRING_ZEROPAD;
					continue;
				default:
					handled = true;
					break;
				}

				int field_width = -1;
				if (isdigit(*cptr)) {
					std::string num;
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
						flags |= STRING_LEFT;
					}
				}

				int precision = -1;
				if (*cptr == '.') {
					if (UNLIKELY(++cptr == format.end())) {
						error("incomplete format '%s'", format.c_str());
					}
					if (isdigit(*cptr)) {
						std::string num;
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

				std::string s;
				int len;
				int base = 10;

				switch (*cptr) {
				case 'c':
					if (!(flags & STRING_LEFT)) while (--field_width > 0) dest += ' ';
					dest += to_char(*argv);
					while (--field_width > 0) dest += ' ';
					continue;
				case 's':
					s = to_string(*argv);
					len = (precision < 0) ? static_cast<int>(s.size()) : std::min(precision, static_cast<int>(s.size()));
					if (!(flags & STRING_LEFT)) while (len < field_width--) dest += ' ';
					dest += s.substr(0, static_cast<size_t>(len));
					while (len < field_width--) dest += ' ';
					continue;
				case 'P':
					flags |= STRING_LARGE;
					[[fallthrough]];
				case 'p':
					if (field_width == -1) {
						field_width = 2 * sizeof(void *);
						flags |= STRING_ZEROPAD;
					}
					dest += format_integer(reinterpret_cast<uintptr_t>(argv->data()), 16, field_width, precision, flags);
					continue;
				case 'A':
					flags |= STRING_LARGE;
					[[fallthrough]];
				case 'a':
					dest += format_float(to_number(cursor, *argv), 16, DECIMAL_FORMAT, field_width, precision, flags);
					continue;
				case 'B':
					flags |= STRING_LARGE;
					[[fallthrough]];
				case 'b':
					base = 2;
					break;
				case 'O':
					flags |= STRING_LARGE;
					[[fallthrough]];
				case 'o':
					base = 8;
					break;
				case 'X':
					flags |= STRING_LARGE;
					[[fallthrough]];
				case 'x':
					base = 16;
					break;
				case 'd':
				case 'i':
					flags |= STRING_SIGN;
					break;
				case 'u':
					break;
				case 'E':
					flags |= STRING_LARGE;
					[[fallthrough]];
				case 'e':
					dest += format_float(to_number(cursor, *argv), 10, SCIENTIFIC_FORMAT, field_width, precision, flags | STRING_SIGN);
					continue;
				case 'F':
					flags |= STRING_LARGE;
					[[fallthrough]];
				case 'f':
					dest += format_float(to_number(cursor, *argv), 10, DECIMAL_FORMAT, field_width, precision, flags | STRING_SIGN);
					continue;
				case 'G':
					flags |= STRING_LARGE;
					[[fallthrough]];
				case 'g':
					dest += format_float(to_number(cursor, *argv), 10, SHORTEST_FORMAT, field_width, precision, flags | STRING_SIGN);
					continue;
				default:
					dest += *cptr;
					continue;
				}

				if (flags & STRING_SIGN) {
					dest += format_integer(to_integer(cursor, *argv), base, field_width, precision, flags);
				}
				else {
					dest += format_integer(to_integer(cursor, *argv), base, field_width, precision, flags);
				}
			}
		}
		else {
			dest += *cptr;
		}
	}
}
