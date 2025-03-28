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

#include "mint/memory/functiontool.h"
#include "mint/memory/casttool.h"
#include "mint/system/utf8.h"

using namespace mint;

MINT_FUNCTION(mint_utf8_byte_count, 1, cursor) {
	FunctionHelper helper(cursor, 1);
	const Reference &self = helper.pop_parameter();
	helper.return_value(create_number(to_string(self).size()));
}

MINT_FUNCTION(mint_string_compare_case_insensitive, 2, cursor) {
	FunctionHelper helper(cursor, 2);
	const Reference &other = helper.pop_parameter();
	const Reference &self = helper.pop_parameter();
	helper.return_value(create_number(utf8_compare_case_insensitive(to_string(self), to_string(other))));
}

MINT_FUNCTION(mint_string_is_alnum, 1, cursor) {
	FunctionHelper helper(cursor, 1);
	const Reference &self = helper.pop_parameter();
	helper.return_value(create_boolean(utf8_is_alnum(to_string(self))));
}

MINT_FUNCTION(mint_string_is_alpha, 1, cursor) {
	FunctionHelper helper(cursor, 1);
	const Reference &self = helper.pop_parameter();
	helper.return_value(create_boolean(utf8_is_alpha(to_string(self))));
}

MINT_FUNCTION(mint_string_is_digit, 1, cursor) {
	FunctionHelper helper(cursor, 1);
	const Reference &self = helper.pop_parameter();
	helper.return_value(create_boolean(utf8_is_digit(to_string(self))));
}

MINT_FUNCTION(mint_string_is_blank, 1, cursor) {
	FunctionHelper helper(cursor, 1);
	const Reference &self = helper.pop_parameter();
	helper.return_value(create_boolean(utf8_is_blank(to_string(self))));
}

MINT_FUNCTION(mint_string_is_space, 1, cursor) {
	FunctionHelper helper(cursor, 1);
	const Reference &self = helper.pop_parameter();
	helper.return_value(create_boolean(utf8_is_space(to_string(self))));
}

MINT_FUNCTION(mint_string_is_cntrl, 1, cursor) {
	FunctionHelper helper(cursor, 1);
	const Reference &self = helper.pop_parameter();
	helper.return_value(create_boolean(utf8_is_cntrl(to_string(self))));
}

MINT_FUNCTION(mint_string_is_graph, 1, cursor) {
	FunctionHelper helper(cursor, 1);
	const Reference &self = helper.pop_parameter();
	helper.return_value(create_boolean(utf8_is_graph(to_string(self))));
}

MINT_FUNCTION(mint_string_is_print, 1, cursor) {
	FunctionHelper helper(cursor, 1);
	const Reference &self = helper.pop_parameter();
	helper.return_value(create_boolean(utf8_is_print(to_string(self))));
}

MINT_FUNCTION(mint_string_is_punct, 1, cursor) {
	FunctionHelper helper(cursor, 1);
	const Reference &self = helper.pop_parameter();
	helper.return_value(create_boolean(utf8_is_punct(to_string(self))));
}

MINT_FUNCTION(mint_string_is_lower, 1, cursor) {
	FunctionHelper helper(cursor, 1);
	const Reference &self = helper.pop_parameter();
	helper.return_value(create_boolean(utf8_is_lower(to_string(self))));
}

MINT_FUNCTION(mint_string_is_upper, 1, cursor) {
	FunctionHelper helper(cursor, 1);
	const Reference &self = helper.pop_parameter();
	helper.return_value(create_boolean(utf8_is_upper(to_string(self))));
}

MINT_FUNCTION(mint_string_to_lower, 1, cursor) {
	FunctionHelper helper(cursor, 1);
	const Reference &self = helper.pop_parameter();
	helper.return_value(create_string(utf8_to_lower(to_string(self))));
}

MINT_FUNCTION(mint_string_to_upper, 1, cursor) {
	FunctionHelper helper(cursor, 1);
	const Reference &self = helper.pop_parameter();
	helper.return_value(create_string(utf8_to_upper(to_string(self))));
}
