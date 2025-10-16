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

#include "mint/memory/functiontool.h"
#include "mint/memory/casttool.h"
#include "mint/memory/reference.h"
#include "mint/system/utf8.h"
#include <compare>

namespace {

mint::WeakReference mint_utf8_byte_count(mint::Cursor& /*cursor*/, const mint::Reference& self) {
	return mint::create_unsigned_number(to_string(self).size());
}

mint::WeakReference mint_string_compare_case_insensitive(mint::Cursor& /*cursor*/, const mint::Reference& self,
    const mint::Reference& other) {
	const auto ordering = mint::utf8_compare_case_insensitive(to_string(self), to_string(other));
	if (ordering == std::strong_ordering::less) {
		return mint::create_number(-1);
	}
	if (ordering == std::strong_ordering::greater) {
		return mint::create_number(+1);
	}
	if (ordering == std::strong_ordering::equal || ordering == std::strong_ordering::equivalent) {
		return mint::create_number(0);
	}
	return {};
}

mint::WeakReference mint_string_is_alnum(mint::Cursor& /*cursor*/, const mint::Reference& self) {
	return mint::create_boolean(mint::utf8_is_alnum(to_string(self)));
}

mint::WeakReference mint_string_is_alpha(mint::Cursor& /*cursor*/, const mint::Reference& self) {
	return mint::create_boolean(mint::utf8_is_alpha(to_string(self)));
}

mint::WeakReference mint_string_is_digit(mint::Cursor& /*cursor*/, const mint::Reference& self) {
	return mint::create_boolean(mint::utf8_is_digit(to_string(self)));
}

mint::WeakReference mint_string_is_blank(mint::Cursor& /*cursor*/, const mint::Reference& self) {
	return mint::create_boolean(mint::utf8_is_blank(to_string(self)));
}

mint::WeakReference mint_string_is_space(mint::Cursor& /*cursor*/, const mint::Reference& self) {
	return mint::create_boolean(mint::utf8_is_space(to_string(self)));
}

mint::WeakReference mint_string_is_cntrl(mint::Cursor& /*cursor*/, const mint::Reference& self) {
	return mint::create_boolean(mint::utf8_is_cntrl(to_string(self)));
}

mint::WeakReference mint_string_is_graph(mint::Cursor& /*cursor*/, const mint::Reference& self) {
	return mint::create_boolean(mint::utf8_is_graph(to_string(self)));
}

mint::WeakReference mint_string_is_print(mint::Cursor& /*cursor*/, const mint::Reference& self) {
	return mint::create_boolean(mint::utf8_is_print(to_string(self)));
}

mint::WeakReference mint_string_is_punct(mint::Cursor& /*cursor*/, const mint::Reference& self) {
	return mint::create_boolean(mint::utf8_is_punct(to_string(self)));
}

mint::WeakReference mint_string_is_lower(mint::Cursor& /*cursor*/, const mint::Reference& self) {
	return mint::create_boolean(mint::utf8_is_lower(to_string(self)));
}

mint::WeakReference mint_string_is_upper(mint::Cursor& /*cursor*/, const mint::Reference& self) {
	return mint::create_boolean(mint::utf8_is_upper(to_string(self)));
}

mint::WeakReference mint_string_to_lower(mint::Cursor& cursor, const mint::Reference& self) {
	return mint::create_string(cursor.ast(), mint::utf8_to_lower(to_string(self)));
}

mint::WeakReference mint_string_to_upper(mint::Cursor& cursor, const mint::Reference& self) {
	return mint::create_string(cursor.ast(), mint::utf8_to_upper(to_string(self)));
}

}

MINT_EXPORT_FUNCTION(mint_utf8_byte_count, 1);
MINT_EXPORT_FUNCTION(mint_string_compare_case_insensitive, 2);
MINT_EXPORT_FUNCTION(mint_string_is_alnum, 1);
MINT_EXPORT_FUNCTION(mint_string_is_alpha, 1);
MINT_EXPORT_FUNCTION(mint_string_is_digit, 1);
MINT_EXPORT_FUNCTION(mint_string_is_blank, 1);
MINT_EXPORT_FUNCTION(mint_string_is_space, 1);
MINT_EXPORT_FUNCTION(mint_string_is_cntrl, 1);
MINT_EXPORT_FUNCTION(mint_string_is_graph, 1);
MINT_EXPORT_FUNCTION(mint_string_is_print, 1);
MINT_EXPORT_FUNCTION(mint_string_is_punct, 1);
MINT_EXPORT_FUNCTION(mint_string_is_lower, 1);
MINT_EXPORT_FUNCTION(mint_string_is_upper, 1);
MINT_EXPORT_FUNCTION(mint_string_to_lower, 1);
MINT_EXPORT_FUNCTION(mint_string_to_upper, 1);
