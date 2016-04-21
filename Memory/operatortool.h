#ifndef OPERATOR_TOOL_H
#define OPERATOR_TOOL_H

#include "casttool.h"

class AbstractSynatxTree;

void move_operator(AbstractSynatxTree *ast);
void copy_operator(AbstractSynatxTree *ast);
void call_operator(AbstractSynatxTree *ast);
void add_operator(AbstractSynatxTree *ast);
void sub_operator(AbstractSynatxTree *ast);
void mul_operator(AbstractSynatxTree *ast);
void div_operator(AbstractSynatxTree *ast);
void mod_operator(AbstractSynatxTree *ast);
void eq_operator(AbstractSynatxTree *ast);

#endif // OPERATOR_TOOL_H
