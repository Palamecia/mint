#ifndef MEMORY_TOOL_H
#define MEMORY_TOOL_H

#include "Memory/reference.h"
#include "Memory/object.h"
#include "System/printer.h"

class SymbolTable;
class AbstractSynatxTree;

bool is_not_zero(const Reference &ref);
Printer *toPrinter(const Reference &ref);
void print(Printer *printer, const Reference &ref);

void init_call(AbstractSynatxTree *ast);
void init_parameter(AbstractSynatxTree *ast, const std::string &symbol);

SharedReference get_symbol_reference(SymbolTable *symbols, const std::string &symbol);
SharedReference get_object_member(AbstractSynatxTree *ast, const std::string &member);
void reduce_member(AbstractSynatxTree *ast);

std::string var_symbol(AbstractSynatxTree *ast);
void create_symbol(AbstractSynatxTree *ast, const std::string &symbol, Reference::Flags flags);
void create_global_symbol(AbstractSynatxTree *ast, const std::string &symbol, Reference::Flags flags);

void array_insert(AbstractSynatxTree *ast);
void hash_insert(AbstractSynatxTree *ast);

#endif // MEMORY_TOOL_H
