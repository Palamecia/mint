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

#include "mint/memory/builtin/string.h"
#include "mint/config.h"
#include "mint/memory/builtin/array.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/memory/algorithm.h"
#include "mint/memory/casttool.h"
#include "mint/memory/class.h"
#include "mint/memory/data.h"
#include "mint/memory/functiontool.h"
#include "mint/ast/abstractsyntaxtree.h"
#include "mint/ast/cursor.h"
#include "mint/memory/memorytool.h"
#include "mint/memory/reference.h"
#include "mint/system/string.h"
#include "mint/system/utf8.h"
#include "mint/system/error.h"
#include "mint/scheduler/scheduler.h"

#include <algorithm>
#include <bit>
#include <cassert>
#include <cctype>
#include <cstdint>
#include <iterator>
#include <cstring>
#include <map>
#include <optional>
#include <ranges>
#include <regex>
#include <string>
#include <string_view>
#include <utility>

using namespace mint;

namespace {

std::size_t string_index(const std::string& str, intmax_t index) {

	const auto i = (index < 0) ? static_cast<std::size_t>(index) + str.size() : static_cast<std::size_t>(index);

	if (i >= str.size()) [[unlikely]] {
		error("string index '{}' is out of range", index);
	}

	return i;
}

std::string::const_iterator string_next(const std::string& str, std::size_t index) {
	return next(std::begin(str), static_cast<std::string::difference_type>(index));
}

void string_format(Cursor& cursor, std::string& dest, const std::string& format, Iterator& args) {

	for (std::string::const_iterator cptr = format.begin(); cptr != format.end(); ++cptr) {

		if ((*cptr == '%') && !args.ctx.empty()) {

			if (*(cptr + 1) == '%') {
				dest += '%';
				++cptr;
				continue;
			}

			std::optional<WeakReference> argv = iterator_next(cursor, args);
			StringFormatFlags flags = 0;
			bool handled = false;

			while (!handled && cptr != format.end()) {
				if (++cptr == format.end()) [[unlikely]] {
					error("incomplete format '{}'", format);
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

				std::intmax_t field_width = -1;
				if (isdigit(*cptr)) {
					std::string num;
					while (isdigit(*cptr)) {
						num += *cptr;
						if (++cptr == format.end()) [[unlikely]] {
							error("incomplete format '{}'", format);
						}
					}
					field_width = std::stoll(num);
				}
				else if (*cptr == '*') {
					if (++cptr == format.end()) [[unlikely]] {
						error("incomplete format '{}'", format);
					}
					field_width = to_signed_integer(cursor, *argv);
					argv = iterator_next(cursor, args);
					if (field_width < 0) {
						field_width = -field_width;
						flags |= string_left;
					}
				}

				std::intmax_t precision = -1;
				if (*cptr == '.') {
					if (++cptr == format.end()) [[unlikely]] {
						error("incomplete format '{}'", format);
					}
					if (isdigit(*cptr)) {
						precision = 0;
						while (isdigit(*cptr)) {
							precision = (precision * decimal_base) + (*cptr - '0');
							if (++cptr == format.end()) [[unlikely]] {
								error("incomplete format '{}'", format);
							}
						}
					}
					else if (*cptr == '*') {
						if (++cptr == format.end()) [[unlikely]] {
							error("incomplete format '{}'", format);
						}
						precision = to_signed_integer(cursor, *argv);
						argv = iterator_next(cursor, args);
					}
					precision = std::max(precision, std::intmax_t {0});
				}

				std::string s;
				std::intmax_t len = 0;
				NumberBase base = decimal_base;

				switch (*cptr) {
				case 'c':
					if (!(flags & string_left)) {
						while (--field_width > 0) {
							dest += ' ';
						}
					}
					dest += to_char(*argv);
					while (--field_width > 0) {
						dest += ' ';
					}
					continue;
				case 's':
					s = to_string(*argv);
					len = (precision < 0) ? static_cast<std::intmax_t>(s.size())
					                      : std::min(precision, static_cast<std::intmax_t>(s.size()));
					if (!(flags & string_left)) {
						while (len < field_width--) {
							dest += ' ';
						}
					}
					dest += s.substr(0, static_cast<std::size_t>(len));
					while (len < field_width--) {
						dest += ' ';
					}
					continue;
				case 'P':
					flags |= string_large;
					[[fallthrough]];
				case 'p':
					if (field_width == -1) {
						field_width = 2 * sizeof(void*);
						flags |= string_zeropad;
					}
					dest += format_integer(std::bit_cast<std::uintptr_t>(&argv->data()), hexadecimal_base, field_width,
					    precision, flags);
					continue;
				case 'A':
					flags |= string_large;
					[[fallthrough]];
				case 'a':
					dest += format_float(to_number(cursor, *argv), hexadecimal_base, DigitsFormat::decimal, field_width,
					    precision, flags);
					continue;
				case 'B':
					flags |= string_large;
					[[fallthrough]];
				case 'b':
					base = binary_base;
					break;
				case 'O':
					flags |= string_large;
					[[fallthrough]];
				case 'o':
					base = octal_base;
					break;
				case 'X':
					flags |= string_large;
					[[fallthrough]];
				case 'x':
					base = hexadecimal_base;
					break;
				case 'd':
				case 'i':
					flags |= string_sign;
					break;
				case 'u':
					break;
				case 'E':
					flags |= string_large;
					[[fallthrough]];
				case 'e':
					dest += format_float(to_number(cursor, *argv), decimal_base, DigitsFormat::scientific, field_width,
					    precision, flags | string_sign);
					continue;
				case 'F':
					flags |= string_large;
					[[fallthrough]];
				case 'f':
					dest += format_float(to_number(cursor, *argv), decimal_base, DigitsFormat::decimal, field_width,
					    precision, flags | string_sign);
					continue;
				case 'G':
					flags |= string_large;
					[[fallthrough]];
				case 'g':
					dest += format_float(to_number(cursor, *argv), decimal_base, DigitsFormat::shortest, field_width,
					    precision, flags | string_sign);
					continue;
				default:
					dest += *cptr;
					continue;
				}

				if (flags & string_sign) {
					dest += format_integer(to_signed_integer(cursor, *argv), base, field_width, precision, flags);
				}
				else {
					dest += format_integer(to_unsigned_integer(cursor, *argv), base, field_width, precision, flags);
				}
			}
		}
		else {
			dest += *cptr;
		}
	}
}

}

StringClass& StringClass::instance(AbstractSyntaxTree& ast) {
	return ast.global_data().builtin<StringClass>(Class::Metatype::string);
}

String::String(AbstractSyntaxTree& ast) :
    Object(StringClass::instance(ast)) {}

String::String(AbstractSyntaxTree& ast, const char* value) :
    Object(StringClass::instance(ast)),
    str(value) {}

String::String(AbstractSyntaxTree& ast, std::string value) :
    Object(StringClass::instance(ast)),
    str(std::move(value)) {}

String::String(AbstractSyntaxTree& ast, std::string_view value) :
    Object(StringClass::instance(ast)),
    str(value) {}

String::String(String&& other) noexcept :
    Object(other.metadata),
    str(std::move(other.str)) {}

String::String(const String& other) :
    Object(other.metadata),
    str(other.str) {}

String& String::operator=(String&& other) noexcept {
	str = std::move(other.str);
	return *this;
}

String& String::operator=(const String& other) {
	str = other.str;
	return *this;
}

StringClass::StringClass(AbstractSyntaxTree& ast) :
    Class(ast.global_data(), "string", Class::Metatype::string) {

	create_builtin_member(copy_operator, ast.create_builtin_method(*this, 2, [](Cursor& cursor) {
		const auto base = get_stack_base(cursor);

		const auto& rvalue = load_from_stack(cursor, base);
		const auto& self = load_from_stack(cursor, base - 1);

		self.data<String>().str = to_string(rvalue);

		cursor.stack().pop_back();
	}));

	create_builtin_member(regex_match_operator, ast.create_builtin_method(*this, 2, [](Cursor& cursor) {
		const auto base = get_stack_base(cursor);

		const auto& rvalue = load_from_stack(cursor, base);
		const auto& self = load_from_stack(cursor, base - 1);
		const bool result = regex_search(self.data<String>().str, to_regex(rvalue));

		cursor.stack().pop_back();
		cursor.stack().pop_back();
		cursor.stack().emplace_back(create_boolean(result));
	}));

	create_builtin_member(regex_unmatch_operator, ast.create_builtin_method(*this, 2, [](Cursor& cursor) {
		const auto base = get_stack_base(cursor);

		const auto& rvalue = load_from_stack(cursor, base);
		const auto& self = load_from_stack(cursor, base - 1);
		const bool result = !regex_search(self.data<String>().str, to_regex(rvalue));

		cursor.stack().pop_back();
		cursor.stack().pop_back();
		cursor.stack().emplace_back(create_boolean(result));
	}));

	create_builtin_member(add_operator, ast.create_builtin_method(*this, 2, [](Cursor& cursor) {
		const auto base = get_stack_base(cursor);

		const auto& rvalue = load_from_stack(cursor, base);
		const auto& self = load_from_stack(cursor, base - 1);
		WeakReference result = create_string(cursor.ast(), self.data<String>().str + to_string(rvalue));

		cursor.stack().pop_back();
		cursor.stack().pop_back();
		cursor.stack().emplace_back(std::move(result));
	}));

	create_builtin_member(mul_operator, ast.create_builtin_method(*this, 2, [](Cursor& cursor) {
		const auto base = get_stack_base(cursor);

		const auto& rvalue = load_from_stack(cursor, base);
		const auto& self = load_from_stack(cursor, base - 1);
		std::string result;

		for (std::uintmax_t i = 0; i < to_unsigned_integer(cursor, rvalue); ++i) {
			result += self.data<String>().str;
		}

		cursor.stack().pop_back();
		cursor.stack().pop_back();
		cursor.stack().emplace_back(create_string(cursor.ast(), result));
	}));

	create_builtin_member(mod_operator, ast.create_builtin_method(*this, 2, [](Cursor& cursor) {
		const auto base = get_stack_base(cursor);

		auto& values = load_from_stack(cursor, base);
		const auto& self = load_from_stack(cursor, base - 1);

		std::string result;

		if (is_instance_of(values, Class::Metatype::iterator)) {
			string_format(cursor, result, self.data<String>().str, values.data<Iterator>());
		}
		else {
			auto it = create_iterator(cursor.ast());
			iterator_yield(cursor, it.data<Iterator>(), std::move(values));
			string_format(cursor, result, self.data<String>().str, it.data<Iterator>());
		}

		cursor.stack().pop_back();
		cursor.stack().pop_back();
		cursor.stack().emplace_back(create_string(cursor.ast(), result));
	}));

	create_builtin_member(shift_left_operator, ast.create_builtin_method(*this, 2, [](Cursor& cursor) {
		const auto base = get_stack_base(cursor);

		const Reference& other = load_from_stack(cursor, base);
		const Reference& self = load_from_stack(cursor, base - 1);

		if (self.flags() & Reference::const_value) {
			cursor.stack().pop_back();
			cursor.stack().back() = create_string(cursor.ast(), self.data<String>().str + to_string(other));
		}
		else {
			self.data<String>().str.append(to_string(other));
			cursor.stack().pop_back();
		}
	}));

	create_builtin_member(eq_operator, ast.create_builtin_method(*this, 2, [](Cursor& cursor) {
		const auto base = get_stack_base(cursor);

		const Reference& rvalue = load_from_stack(cursor, base);
		const Reference& self = load_from_stack(cursor, base - 1);
		WeakReference result = create_boolean(self.data<String>().str == to_string(rvalue));

		cursor.stack().pop_back();
		cursor.stack().pop_back();
		cursor.stack().emplace_back(std::forward<Reference>(result));
	}));

	create_builtin_member(ne_operator, ast.create_builtin_method(*this, 2, [](Cursor& cursor) {
		const auto base = get_stack_base(cursor);

		const Reference& rvalue = load_from_stack(cursor, base);
		const Reference& self = load_from_stack(cursor, base - 1);
		WeakReference result = create_boolean(self.data<String>().str != to_string(rvalue));

		cursor.stack().pop_back();
		cursor.stack().pop_back();
		cursor.stack().emplace_back(std::forward<Reference>(result));
	}));

	create_builtin_member(lt_operator, ast.create_builtin_method(*this, 2, [](Cursor& cursor) {
		const auto base = get_stack_base(cursor);

		const Reference& rvalue = load_from_stack(cursor, base);
		const Reference& self = load_from_stack(cursor, base - 1);
		WeakReference result = create_boolean(self.data<String>().str < to_string(rvalue));

		cursor.stack().pop_back();
		cursor.stack().pop_back();
		cursor.stack().emplace_back(std::forward<Reference>(result));
	}));

	create_builtin_member(gt_operator, ast.create_builtin_method(*this, 2, [](Cursor& cursor) {
		const auto base = get_stack_base(cursor);

		const Reference& rvalue = load_from_stack(cursor, base);
		const Reference& self = load_from_stack(cursor, base - 1);
		WeakReference result = create_boolean(self.data<String>().str > to_string(rvalue));

		cursor.stack().pop_back();
		cursor.stack().pop_back();
		cursor.stack().emplace_back(std::forward<Reference>(result));
	}));

	create_builtin_member(le_operator, ast.create_builtin_method(*this, 2, [](Cursor& cursor) {
		const auto base = get_stack_base(cursor);

		const Reference& rvalue = load_from_stack(cursor, base);
		const Reference& self = load_from_stack(cursor, base - 1);
		WeakReference result = create_boolean(self.data<String>().str <= to_string(rvalue));

		cursor.stack().pop_back();
		cursor.stack().pop_back();
		cursor.stack().emplace_back(std::forward<Reference>(result));
	}));

	create_builtin_member(ge_operator, ast.create_builtin_method(*this, 2, [](Cursor& cursor) {
		const auto base = get_stack_base(cursor);

		const Reference& rvalue = load_from_stack(cursor, base);
		const Reference& self = load_from_stack(cursor, base - 1);
		WeakReference result = create_boolean(self.data<String>().str >= to_string(rvalue));

		cursor.stack().pop_back();
		cursor.stack().pop_back();
		cursor.stack().emplace_back(std::forward<Reference>(result));
	}));

	create_builtin_member(not_operator, ast.create_builtin_method(*this, 1, [](Cursor& cursor) {
		const Reference& self = cursor.stack().back();
		WeakReference result = create_boolean(self.data<String>().str.empty());

		cursor.stack().pop_back();
		cursor.stack().emplace_back(std::forward<Reference>(result));
	}));

	create_builtin_member(subscript_operator, ast.create_builtin_method(*this, 2, [](Cursor& cursor) {
		const auto base = get_stack_base(cursor);

		const auto& index = load_from_stack(cursor, base);
		const auto& self = load_from_stack(cursor, base - 1);
		WeakReference result = create_string(cursor.ast());

		if ((index.data().format() != Data::Format::object)
		    || (index.data<Object>().metadata.metatype() != Class::Metatype::iterator)) {
			std::string& string_ref = self.data<String>().str;
			auto offset = string_index(string_ref, to_signed_integer(cursor, index));
			result.data<String>().str = *(utf8iterator(string_ref.begin()) + offset);
		}
		else if (index.data<Iterator>().ctx.get_type() == Iterator::Context::Type::range) {

			const std::string& string_ref = self.data<String>().str;
			auto range = index.data<Iterator>().ctx.view();
			std::size_t begin_index = string_index(string_ref, to_signed_integer(cursor, range.front()));
			std::size_t end_index = string_index(string_ref, to_signed_integer(cursor, range.back()));
			index.data<Iterator>().ctx.clear();

			if (begin_index > end_index) {
				std::swap(begin_index, end_index);
			}

			const auto begin = string_ref.begin()
			                   + static_cast<int>(utf8_code_point_index_to_byte_index(string_ref, begin_index));
			auto end = string_ref.begin()
			           + static_cast<int>(utf8_code_point_index_to_byte_index(string_ref, end_index));

			end += static_cast<int>(utf8_code_point_length(static_cast<byte_t>(*end)));
			result.data<String>().str = std::string(begin, end);
		}
		else {
			std::string& string_ref = self.data<String>().str;
			while (std::optional<WeakReference>&& item = iterator_next(cursor, index.data<Iterator>())) {
				result.data<String>().str += *(
				    utf8iterator(string_ref.begin()) + string_index(string_ref, to_signed_integer(cursor, *item)));
			}
		}

		cursor.stack().pop_back();
		cursor.stack().pop_back();
		cursor.stack().emplace_back(std::forward<Reference>(result));
	}));

	create_builtin_member(subscript_move_operator, ast.create_builtin_method(*this, 3, [](Cursor& cursor) {
		const auto base = get_stack_base(cursor);

		auto& value = load_from_stack(cursor, base);
		auto& index = load_from_stack(cursor, base - 1);
		const auto& self = load_from_stack(cursor, base - 2);

		if ((index.data().format() != Data::Format::object)
		    || (index.data<Object>().metadata.metatype() != Class::Metatype::iterator)) {
			std::string& string_ref = self.data<String>().str;
			auto offset = string_index(string_ref, to_signed_integer(cursor, index));
			auto utf8_index = utf8_code_point_index_to_byte_index(string_ref, offset);
			auto utf8_length = utf8_code_point_length(static_cast<byte_t>(string_ref[utf8_index]));
			string_ref.replace(utf8_index, utf8_length, to_string(value));

			cursor.stack().pop_back();
			cursor.stack().pop_back();
			cursor.stack().pop_back();
			cursor.stack().emplace_back(std::forward<Reference>(value));
		}
		else if (index.data<Iterator>().ctx.get_type() == Iterator::Context::Type::range) {

			std::string& string_ref = self.data<String>().str;
			auto range = index.data<Iterator>().ctx.view();
			std::size_t begin_index = string_index(string_ref, to_signed_integer(cursor, range.front()));
			std::size_t end_index = string_index(string_ref, to_signed_integer(cursor, range.back()));
			index.data<Iterator>().ctx.clear();

			if (begin_index > end_index) {
				std::swap(begin_index, end_index);
			}

			const auto begin = string_next(string_ref, utf8_code_point_index_to_byte_index(string_ref, begin_index));
			auto end = string_next(string_ref, utf8_code_point_index_to_byte_index(string_ref, end_index));

			std::advance(end, utf8_code_point_length(static_cast<byte_t>(*end)));
			string_ref.replace(begin, end, to_string(value));

			cursor.stack().pop_back();
			cursor.stack().pop_back();
		}
		else {

			std::size_t offset = 0;
			std::string& string_ref = self.data<String>().str;

			for_each(cursor, value, [&cursor, &string_ref, &offset, &index](const Reference& ref) {
				if (!index.data<Iterator>().ctx.empty()) {
					offset = utf8_code_point_index_to_byte_index(string_ref,
					    string_index(string_ref, to_signed_integer(cursor, index.data<Iterator>().ctx.get())));
					const auto utf8_length = utf8_code_point_length(static_cast<byte_t>(string_ref.at(offset)));
					string_ref.replace(offset, utf8_length, to_string(ref));
					index.data<Iterator>().ctx.next(cursor);
					offset += utf8_length;
				}
				else {
					const auto length = utf8_code_point_length(static_cast<byte_t>(string_ref.at(offset)));
					string_ref.insert(offset, to_string(ref));
					offset += length;
				}
			});

			std::map<std::size_t, std::size_t> to_remove;

			while (!index.data<Iterator>().ctx.empty()) {
				offset = utf8_code_point_index_to_byte_index(string_ref,
				    string_index(string_ref, to_signed_integer(cursor, index.data<Iterator>().ctx.get())));
				const auto utf8_length = utf8_code_point_length(static_cast<byte_t>(string_ref.at(offset)));
				to_remove.insert({offset, utf8_length});
				index.data<Iterator>().ctx.next(cursor);
			}

			for (const auto& i : std::views::reverse(to_remove)) {
				string_ref.erase(i.first, i.second);
			}

			cursor.stack().pop_back();
			cursor.stack().pop_back();
		}
	}));

	create_builtin_member("insert", ast.create_builtin_method(*this, 3, [](Cursor& cursor) {
		const auto base = get_stack_base(cursor);
		const auto& value = load_from_stack(cursor, base);
		const auto& index = load_from_stack(cursor, base - 1);
		const auto& self = load_from_stack(cursor, base - 2);

		std::string& string_ref = self.data<String>().str;
		auto offset = string_index(string_ref, to_signed_integer(cursor, index));
		auto utf8_index = utf8_code_point_index_to_byte_index(string_ref, offset);
		string_ref.insert(utf8_index, to_string(value));

		cursor.stack().pop_back();
		cursor.stack().pop_back();
	}));

	create_builtin_member(in_operator, ast.create_builtin_method(*this, 1, [](Cursor& cursor) {
		cursor.stack().back() = create_iterator_over(cursor, cursor.stack().back());
	}));

	create_builtin_member(in_operator, ast.create_builtin_method(*this, 2, [](Cursor& cursor) {
		const auto base = get_stack_base(cursor);
		const Reference& value = load_from_stack(cursor, base);
		const Reference& self = load_from_stack(cursor, base - 1);
		WeakReference result = create_boolean(self.data<String>().str.find(to_string(value)) != std::string::npos);

		cursor.stack().pop_back();
		cursor.stack().pop_back();
		cursor.stack().emplace_back(std::forward<Reference>(result));
	}));

	create_builtin_member("each", ast.create_builtin_method(*this, 2, R"""(
		def (const self, const func) {
			for item in self {
				func(item)
			}
		})"""));

	create_builtin_member("isEmpty", ast.create_builtin_method(*this, 1, [](Cursor& cursor) {
		const Reference& self = cursor.stack().back();
		cursor.stack().back() = create_boolean(self.data<String>().str.empty());
	}));

	create_builtin_member("size", ast.create_builtin_method(*this, 1, [](Cursor& cursor) {
		const Reference& self = cursor.stack().back();
		cursor.stack().back() = create_unsigned_number(utf8_code_point_count(self.data<String>().str));
	}));

	create_builtin_member("clear", ast.create_builtin_method(*this, 1, [](Cursor& cursor) {
		const Reference& self = cursor.stack().back();
		if (self.flags() & Reference::const_value) [[unlikely]] {
			error("invalid modification of constant value");
		}
		self.data<String>().str.clear();
		cursor.stack().back() = create_none();
	}));

	create_builtin_member("substring", ast.create_builtin_method(*this, 2, [](Cursor& cursor) {
		const auto base = get_stack_base(cursor);
		const auto& from = load_from_stack(cursor, base);
		const auto& self = load_from_stack(cursor, base - 1);

		const std::string substring = self.data<String>().str.substr(
		    utf8_code_point_index_to_byte_index(self.data<String>().str, to_signed_integer(cursor, from)));
		cursor.stack().pop_back();
		cursor.stack().back() = create_string(cursor.ast(), substring);
	}));

	create_builtin_member("substring", ast.create_builtin_method(*this, 3, [](Cursor& cursor) {
		const auto base = get_stack_base(cursor);
		const auto& length = load_from_stack(cursor, base);
		const auto& from = load_from_stack(cursor, base - 1);
		const auto& self = load_from_stack(cursor, base - 2);

		const auto utf8_start = utf8_code_point_index_to_byte_index(self.data<String>().str,
		    to_signed_integer(cursor, from));
		const auto utf8_length = length.data().format() != Data::Format::none
		                             ? utf8_substring_byte_count(self.data<String>().str, utf8_start,
		                                   to_signed_integer(cursor, length))
		                             : std::string::npos;
		const std::string substring = self.data<String>().str.substr(utf8_start, utf8_length);
		cursor.stack().pop_back();
		cursor.stack().pop_back();
		cursor.stack().back() = create_string(cursor.ast(), substring);
	}));

	create_builtin_member("replace", ast.create_builtin_method(*this, 3, [](Cursor& cursor) {
		const auto base = get_stack_base(cursor);
		const auto& value = load_from_stack(cursor, base);
		const auto& pattern = load_from_stack(cursor, base - 1);
		const auto& self = load_from_stack(cursor, base - 2);

		const auto before = to_string(pattern);
		const auto after = to_string(value);

		if (self.flags() & Reference::const_value) {

			std::string str = self.data<String>().str;

			if (is_instance_of(pattern, Class::Metatype::regex)) {
				str = regex_replace(str, to_regex(pattern), after);
			}
			else {
				std::size_t pos = 0;
				while ((pos = str.find(before, pos)) != std::string::npos) {
					str.replace(pos, before.size(), after);
					pos += after.size();
				}
			}

			cursor.stack().pop_back();
			cursor.stack().pop_back();
			cursor.stack().back() = create_string(cursor.ast(), str);
		}
		else {

			if (is_instance_of(pattern, Class::Metatype::regex)) {
				self.data<String>().str = regex_replace(self.data<String>().str, to_regex(pattern), after);
			}
			else {
				std::size_t pos = 0;
				while ((pos = self.data<String>().str.find(before, pos)) != std::string::npos) {
					self.data<String>().str.replace(pos, before.size(), after);
					pos += after.size();
				}
			}

			cursor.stack().pop_back();
			cursor.stack().pop_back();
		}
	}));

	create_builtin_member("replace", ast.create_builtin_method(*this, 4, [](Cursor& cursor) {
		const auto base = get_stack_base(cursor);
		const auto& value = load_from_stack(cursor, base);
		const auto& length = load_from_stack(cursor, base - 1);
		const auto& from = load_from_stack(cursor, base - 2);
		const auto& self = load_from_stack(cursor, base - 3);

		if (self.flags() & Reference::const_value) {

			std::string string_copy = self.data<String>().str;
			auto offset = string_index(string_copy, to_signed_integer(cursor, from));
			auto utf8_index = utf8_code_point_index_to_byte_index(string_copy, offset);
			auto utf8_length = utf8_substring_byte_count(string_copy, offset, to_signed_integer(cursor, length));
			string_copy.replace(utf8_index, utf8_length, to_string(value));

			cursor.stack().pop_back();
			cursor.stack().pop_back();
			cursor.stack().pop_back();
			cursor.stack().back() = create_string(cursor.ast(), string_copy);
		}
		else {

			std::string& string_ref = self.data<String>().str;
			auto offset = string_index(string_ref, to_signed_integer(cursor, from));
			auto utf8_index = utf8_code_point_index_to_byte_index(string_ref, offset);
			auto utf8_length = utf8_substring_byte_count(string_ref, offset, to_signed_integer(cursor, length));
			string_ref.replace(utf8_index, utf8_length, to_string(value));

			cursor.stack().pop_back();
			cursor.stack().pop_back();
			cursor.stack().pop_back();
		}
	}));

	create_builtin_member("contains", ast.create_builtin_method(*this, 2, [](Cursor& cursor) {
		const auto base = get_stack_base(cursor);
		const auto& other = load_from_stack(cursor, base);
		const auto& self = load_from_stack(cursor, base - 1);
		bool result = false;

		if (is_instance_of(other, Class::Metatype::regex)) {
			result = regex_search(self.data<String>().str, to_regex(other));
		}
		else {
			result = self.data<String>().str.find(to_string(other)) != std::string::npos;
		}

		cursor.stack().pop_back();
		cursor.stack().pop_back();
		cursor.stack().emplace_back(create_boolean(result));
	}));

	create_builtin_member("indexOf", ast.create_builtin_method(*this, 2, [](Cursor& cursor) {
		const auto base = get_stack_base(cursor);
		const auto& other = load_from_stack(cursor, base);
		const auto& self = load_from_stack(cursor, base - 1);

		auto pos = std::string::npos;
		if (is_instance_of(other, Class::Metatype::regex)) {
			std::smatch match;
			if (regex_search(self.data<String>().str, match, to_regex(other))) {
				pos = static_cast<decltype(pos)>(match.position(0));
			}
		}
		else {
			pos = self.data<String>().str.find(to_string(other));
		}

		WeakReference result = pos != std::string::npos
		                           ? create_unsigned_number(
		                                 utf8_byte_index_to_code_point_index(self.data<String>().str, pos))
		                           : create_none();

		cursor.stack().pop_back();
		cursor.stack().pop_back();
		cursor.stack().emplace_back(std::forward<Reference>(result));
	}));

	create_builtin_member("indexOf", ast.create_builtin_method(*this, 3, [](Cursor& cursor) {
		const auto base = get_stack_base(cursor);
		const auto& from = load_from_stack(cursor, base);
		const auto& other = load_from_stack(cursor, base - 1);
		const auto& self = load_from_stack(cursor, base - 2);

		auto pos = std::string::npos;
		auto start = utf8_code_point_index_to_byte_index(self.data<String>().str,
		    static_cast<std::size_t>(to_number(cursor, from)));
		if (start != std::string::npos) {
			if (is_instance_of(other, Class::Metatype::regex)) {
				const auto expr = to_regex(other);
				auto begin = std::sregex_iterator(self.data<String>().str.begin(), self.data<String>().str.end(), expr);
				auto end = std::sregex_iterator();
				for (auto i = begin; i != end; ++i) {
					if (start <= std::size_t(0) + static_cast<decltype(pos)>(i->position())) {
						pos = static_cast<decltype(pos)>(i->position());
						break;
					}
				}
			}
			else {
				pos = self.data<String>().str.find(to_string(other), start);
			}
		}

		WeakReference result = pos != std::string::npos
		                           ? create_unsigned_number(
		                                 utf8_byte_index_to_code_point_index(self.data<String>().str, pos))
		                           : create_none();

		cursor.stack().pop_back();
		cursor.stack().pop_back();
		cursor.stack().pop_back();
		cursor.stack().emplace_back(std::forward<Reference>(result));
	}));

	create_builtin_member("lastIndexOf", ast.create_builtin_method(*this, 2, [](Cursor& cursor) {
		const auto base = get_stack_base(cursor);
		const auto& other = load_from_stack(cursor, base);
		const auto& self = load_from_stack(cursor, base - 1);

		auto pos = std::string::npos;
		if (is_instance_of(other, Class::Metatype::regex)) {
			const auto expr = to_regex(other);
			auto begin = std::sregex_iterator(self.data<String>().str.begin(), self.data<String>().str.end(), expr);
			auto end = std::sregex_iterator();
			for (auto i = begin; i != end; ++i) {
				pos = static_cast<decltype(pos)>(i->position());
			}
		}
		else {
			pos = self.data<String>().str.rfind(to_string(other));
		}

		WeakReference result = pos != std::string::npos
		                           ? create_unsigned_number(
		                                 utf8_byte_index_to_code_point_index(self.data<String>().str, pos))
		                           : create_none();

		cursor.stack().pop_back();
		cursor.stack().pop_back();
		cursor.stack().emplace_back(std::forward<Reference>(result));
	}));

	create_builtin_member("lastIndexOf", ast.create_builtin_method(*this, 3, [](Cursor& cursor) {
		const auto base = get_stack_base(cursor);
		const auto& from = load_from_stack(cursor, base);
		const auto& other = load_from_stack(cursor, base - 1);
		const auto& self = load_from_stack(cursor, base - 2);

		auto pos = std::string::npos;
		auto start = utf8_code_point_index_to_byte_index(self.data<String>().str,
		    static_cast<std::size_t>(to_number(cursor, from)));
		if (start != std::string::npos) {
			if (is_instance_of(other, Class::Metatype::regex)) {
				const auto expr = to_regex(other);
				auto begin = std::sregex_iterator(self.data<String>().str.begin(), self.data<String>().str.end(), expr);
				auto end = std::sregex_iterator();
				for (auto i = begin; i != end; ++i) {
					if (start >= std::size_t(0) + static_cast<decltype(pos)>(i->position())) {
						pos = static_cast<decltype(pos)>(i->position());
					}
				}
			}
			else {
				pos = self.data<String>().str.rfind(to_string(other), start);
			}
		}

		WeakReference result = pos != std::string::npos
		                           ? create_unsigned_number(
		                                 utf8_byte_index_to_code_point_index(self.data<String>().str, pos))
		                           : create_none();

		cursor.stack().pop_back();
		cursor.stack().pop_back();
		cursor.stack().pop_back();
		cursor.stack().emplace_back(std::forward<Reference>(result));
	}));

	create_builtin_member("startsWith", ast.create_builtin_method(*this, 2, [](Cursor& cursor) {
		const auto base = get_stack_base(cursor);
		const auto& other = load_from_stack(cursor, base);
		const auto& self = load_from_stack(cursor, base - 1);
		bool result = false;

		if (is_instance_of(other, Class::Metatype::regex)) {
			std::smatch match;
			if (regex_search(self.data<String>().str, match, to_regex(other))) {
				result = match.position(0) == 0;
			}
			else {
				result = false;
			}
		}
		else {
			result = self.data<String>().str.starts_with(to_string(other));
		}

		cursor.stack().pop_back();
		cursor.stack().pop_back();
		cursor.stack().emplace_back(create_boolean(result));
	}));

	create_builtin_member("endsWith", ast.create_builtin_method(*this, 2, [](Cursor& cursor) {
		const auto base = get_stack_base(cursor);
		const auto& other = load_from_stack(cursor, base);
		const auto& self = load_from_stack(cursor, base - 1);
		bool result = false;

		if (is_instance_of(other, Class::Metatype::regex)) {
			result = false;
			const auto expr = to_regex(other);
			auto begin = std::sregex_iterator(self.data<String>().str.begin(), self.data<String>().str.end(), expr);
			auto end = std::sregex_iterator();
			for (auto i = begin; i != end; ++i) {
				if (std::size_t(0) + static_cast<std::size_t>(i->position() + i->length())
				    == self.data<String>().str.size()) {
					result = true;
					break;
				}
			}
		}
		else {
			result = self.data<String>().str.ends_with(to_string(other));
		}

		cursor.stack().pop_back();
		cursor.stack().pop_back();
		cursor.stack().emplace_back(create_boolean(result));
	}));

	create_builtin_member("split", ast.create_builtin_method(*this, 2, [](Cursor& cursor) {
		const auto base = get_stack_base(cursor);
		const Reference& sep = load_from_stack(cursor, base);
		const Reference& self = load_from_stack(cursor, base - 1);
		WeakReference result = create_array(cursor.ast());

		const auto sep_str = to_string(sep);
		const auto self_str = self.data<String>().str;

		if (sep_str.empty()) {
			for (const_utf8iterator i = self_str.begin(); i != self_str.end(); ++i) {
				array_append(result.data<Array>(), create_string(cursor.ast(), *i));
			}
		}
		else {
			std::size_t from = 0;
			std::size_t pos = self_str.find(sep_str);
			while (pos != std::string::npos) {
				array_append(result.data<Array>(), create_string(cursor.ast(), self_str.substr(from, pos - from)));
				pos = self_str.find(sep_str, from = pos + sep_str.size());
			}
			if (!self_str.empty()) {
				array_append(result.data<Array>(), create_string(cursor.ast(), self_str.substr(from)));
			}
		}

		cursor.stack().pop_back();
		cursor.stack().pop_back();
		cursor.stack().emplace_back(std::forward<Reference>(result));
	}));
}
