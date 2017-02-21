#ifndef OPERATOR_TOOL_H
#define OPERATOR_TOOL_H

#include "casttool.h"

class AbstractSyntaxTree;

void move_operator(AbstractSyntaxTree *ast);
void copy_operator(AbstractSyntaxTree *ast);
void call_operator(AbstractSyntaxTree *ast, int signature);
void call_member_operator(AbstractSyntaxTree *ast, int signature);
void add_operator(AbstractSyntaxTree *ast);
void sub_operator(AbstractSyntaxTree *ast);
void mul_operator(AbstractSyntaxTree *ast);
void div_operator(AbstractSyntaxTree *ast);
void pow_operator(AbstractSyntaxTree *ast);
void mod_operator(AbstractSyntaxTree *ast);
void is_operator(AbstractSyntaxTree *ast);
void eq_operator(AbstractSyntaxTree *ast);
void ne_operator(AbstractSyntaxTree *ast);
void lt_operator(AbstractSyntaxTree *ast);
void gt_operator(AbstractSyntaxTree *ast);
void le_operator(AbstractSyntaxTree *ast);
void ge_operator(AbstractSyntaxTree *ast);
void and_operator(AbstractSyntaxTree *ast);
void or_operator(AbstractSyntaxTree *ast);
void band_operator(AbstractSyntaxTree *ast);
void bor_operator(AbstractSyntaxTree *ast);
void xor_operator(AbstractSyntaxTree *ast);
void inc_operator(AbstractSyntaxTree *ast);
void dec_operator(AbstractSyntaxTree *ast);
void not_operator(AbstractSyntaxTree *ast);
void compl_operator(AbstractSyntaxTree *ast);
void pos_operator(AbstractSyntaxTree *ast);
void neg_operator(AbstractSyntaxTree *ast);
void shift_left_operator(AbstractSyntaxTree *ast);
void shift_right_operator(AbstractSyntaxTree *ast);
void inclusive_range_operator(AbstractSyntaxTree *ast);
void exclusive_range_operator(AbstractSyntaxTree *ast);
void typeof_operator(AbstractSyntaxTree *ast);
void membersof_operator(AbstractSyntaxTree *ast);
void subscript_operator(AbstractSyntaxTree *ast);

void find_defined_symbol(AbstractSyntaxTree *ast, const std::string &symbol);
void find_defined_member(AbstractSyntaxTree *ast, const std::string &symbol);
void check_defined(AbstractSyntaxTree *ast);

void in_find(AbstractSyntaxTree *ast);
void in_init(AbstractSyntaxTree *ast);
void in_next(AbstractSyntaxTree *ast);
void in_check(AbstractSyntaxTree *ast);

#endif // OPERATOR_TOOL_H
