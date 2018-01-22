#ifndef OPERATOR_TOOL_H
#define OPERATOR_TOOL_H

#include "casttool.h"

namespace mint {

class Cursor;

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
MINT_EXPORT void and_operator(Cursor *cursor);
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

MINT_EXPORT void find_defined_symbol(Cursor *cursor, const std::string &symbol);
MINT_EXPORT void find_defined_member(Cursor *cursor, const std::string &symbol);
MINT_EXPORT void check_defined(Cursor *cursor);

MINT_EXPORT void in_find(Cursor *cursor);
MINT_EXPORT void in_init(Cursor *cursor);
MINT_EXPORT void in_next(Cursor *cursor);
MINT_EXPORT void in_check(Cursor *cursor);

}

#endif // OPERATOR_TOOL_H
