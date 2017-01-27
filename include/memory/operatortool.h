#ifndef OPERATOR_TOOL_H
#define OPERATOR_TOOL_H

#include "casttool.h"

class AbstractSynatxTree;

void move_operator(AbstractSynatxTree *ast);
void copy_operator(AbstractSynatxTree *ast);
void call_operator(AbstractSynatxTree *ast, int signature);
void call_member_operator(AbstractSynatxTree *ast, int signature);
void add_operator(AbstractSynatxTree *ast);
void sub_operator(AbstractSynatxTree *ast);
void mul_operator(AbstractSynatxTree *ast);
void div_operator(AbstractSynatxTree *ast);
void pow_operator(AbstractSynatxTree *ast);
void mod_operator(AbstractSynatxTree *ast);
void is_operator(AbstractSynatxTree *ast);
void eq_operator(AbstractSynatxTree *ast);
void ne_operator(AbstractSynatxTree *ast);
void lt_operator(AbstractSynatxTree *ast);
void gt_operator(AbstractSynatxTree *ast);
void le_operator(AbstractSynatxTree *ast);
void ge_operator(AbstractSynatxTree *ast);
void and_operator(AbstractSynatxTree *ast);
void or_operator(AbstractSynatxTree *ast);
void band_operator(AbstractSynatxTree *ast);
void bor_operator(AbstractSynatxTree *ast);
void xor_operator(AbstractSynatxTree *ast);
void inc_operator(AbstractSynatxTree *ast);
void dec_operator(AbstractSynatxTree *ast);
void not_operator(AbstractSynatxTree *ast);
void compl_operator(AbstractSynatxTree *ast);
void pos_operator(AbstractSynatxTree *ast);
void neg_operator(AbstractSynatxTree *ast);
void shift_left_operator(AbstractSynatxTree *ast);
void shift_right_operator(AbstractSynatxTree *ast);
void inclusive_range_operator(AbstractSynatxTree *ast);
void exclusive_range_operator(AbstractSynatxTree *ast);
void typeof_operator(AbstractSynatxTree *ast);
void membersof_operator(AbstractSynatxTree *ast);
void subscript_operator(AbstractSynatxTree *ast);

void find_defined_symbol(AbstractSynatxTree *ast, const std::string &symbol);
void find_defined_member(AbstractSynatxTree *ast, const std::string &symbol);
void check_defined(AbstractSynatxTree *ast);

void in_find(AbstractSynatxTree *ast);
void in_init(AbstractSynatxTree *ast);
void in_next(AbstractSynatxTree *ast);
void in_check(AbstractSynatxTree *ast);

#endif // OPERATOR_TOOL_H
