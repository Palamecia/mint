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

#ifndef MINT_OPERATORTOOL_H
#define MINT_OPERATORTOOL_H

#include "mint/memory/class.h"
#include "mint/ast/cursor.h"

namespace mint {

class Cursor;

MINT_EXPORT bool call_overload(Cursor *cursor, Class::Operator operator_overload, int signature);
MINT_EXPORT bool call_overload(Cursor *cursor, const Symbol &operator_overload, int signature);

MINT_EXPORT void move_operator(Cursor *cursor);
MINT_EXPORT void copy_operator(Cursor *cursor);
MINT_EXPORT void call_operator(Cursor *cursor, int signature);
MINT_EXPORT void call_member_operator(Cursor *cursor, int signature);
MINT_EXPORT void add_operator(Cursor *cursor);
MINT_EXPORT void sub_operator(Cursor *cursor);
MINT_EXPORT void mul_operator(Cursor *cursor);
MINT_EXPORT void div_operator(Cursor *cursor);
MINT_EXPORT void pow_operator(Cursor *cursor);
MINT_EXPORT void mod_operator(Cursor *cursor);
MINT_EXPORT void is_operator(Cursor *cursor);
MINT_EXPORT void eq_operator(Cursor *cursor);
MINT_EXPORT void ne_operator(Cursor *cursor);
MINT_EXPORT void lt_operator(Cursor *cursor);
MINT_EXPORT void gt_operator(Cursor *cursor);
MINT_EXPORT void le_operator(Cursor *cursor);
MINT_EXPORT void ge_operator(Cursor *cursor);
MINT_EXPORT void and_pre_check(Cursor *cursor, size_t pos);
MINT_EXPORT void and_operator(Cursor *cursor);
MINT_EXPORT void or_pre_check(Cursor *cursor, size_t pos);
MINT_EXPORT void or_operator(Cursor *cursor);
MINT_EXPORT void band_operator(Cursor *cursor);
MINT_EXPORT void bor_operator(Cursor *cursor);
MINT_EXPORT void xor_operator(Cursor *cursor);
MINT_EXPORT void inc_operator(Cursor *cursor);
MINT_EXPORT void dec_operator(Cursor *cursor);
MINT_EXPORT void not_operator(Cursor *cursor);
MINT_EXPORT void compl_operator(Cursor *cursor);
MINT_EXPORT void pos_operator(Cursor *cursor);
MINT_EXPORT void neg_operator(Cursor *cursor);
MINT_EXPORT void shift_left_operator(Cursor *cursor);
MINT_EXPORT void shift_right_operator(Cursor *cursor);
MINT_EXPORT void inclusive_range_operator(Cursor *cursor);
MINT_EXPORT void exclusive_range_operator(Cursor *cursor);
MINT_EXPORT void typeof_operator(Cursor *cursor);
MINT_EXPORT void membersof_operator(Cursor *cursor);
MINT_EXPORT void subscript_operator(Cursor *cursor);
MINT_EXPORT void subscript_move_operator(Cursor *cursor);

MINT_EXPORT void regex_match(Cursor *cursor);
MINT_EXPORT void regex_unmatch(Cursor *cursor);

MINT_EXPORT void find_defined_symbol(Cursor *cursor, const Symbol &symbol);
MINT_EXPORT void find_defined_member(Cursor *cursor, const Symbol &symbol);
MINT_EXPORT void check_defined(Cursor *cursor);

MINT_EXPORT void find_operator(Cursor *cursor);
MINT_EXPORT void find_init(Cursor *cursor);
MINT_EXPORT void find_next(Cursor *cursor);
MINT_EXPORT void find_check(Cursor *cursor, size_t pos);
MINT_EXPORT void in_operator(Cursor *cursor);
MINT_EXPORT void range_init(Cursor *cursor);
MINT_EXPORT void range_next(Cursor *cursor);
MINT_EXPORT void range_check(Cursor *cursor, size_t pos);
MINT_EXPORT void range_iterator_check(Cursor *cursor, size_t pos);

}

#endif // MINT_OPERATORTOOL_H
