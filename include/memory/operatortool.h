#ifndef OPERATOR_TOOL_H
#define OPERATOR_TOOL_H

#include "casttool.h"

class Cursor;

void move_operator(Cursor *cursor);
void copy_operator(Cursor *cursor);
void call_operator(Cursor *cursor, int signature);
void call_member_operator(Cursor *cursor, int signature);
void add_operator(Cursor *cursor);
void sub_operator(Cursor *cursor);
void mul_operator(Cursor *cursor);
void div_operator(Cursor *cursor);
void pow_operator(Cursor *cursor);
void mod_operator(Cursor *cursor);
void is_operator(Cursor *cursor);
void eq_operator(Cursor *cursor);
void ne_operator(Cursor *cursor);
void lt_operator(Cursor *cursor);
void gt_operator(Cursor *cursor);
void le_operator(Cursor *cursor);
void ge_operator(Cursor *cursor);
void and_operator(Cursor *cursor);
void or_operator(Cursor *cursor);
void band_operator(Cursor *cursor);
void bor_operator(Cursor *cursor);
void xor_operator(Cursor *cursor);
void inc_operator(Cursor *cursor);
void dec_operator(Cursor *cursor);
void not_operator(Cursor *cursor);
void compl_operator(Cursor *cursor);
void pos_operator(Cursor *cursor);
void neg_operator(Cursor *cursor);
void shift_left_operator(Cursor *cursor);
void shift_right_operator(Cursor *cursor);
void inclusive_range_operator(Cursor *cursor);
void exclusive_range_operator(Cursor *cursor);
void typeof_operator(Cursor *cursor);
void membersof_operator(Cursor *cursor);
void subscript_operator(Cursor *cursor);

void find_defined_symbol(Cursor *cursor, const std::string &symbol);
void find_defined_member(Cursor *cursor, const std::string &symbol);
void check_defined(Cursor *cursor);

void in_find(Cursor *cursor);
void in_init(Cursor *cursor);
void in_next(Cursor *cursor);
void in_check(Cursor *cursor);

#endif // OPERATOR_TOOL_H
