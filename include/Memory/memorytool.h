#ifndef MEMORY_TOOL_H
#define MEMORY_TOOL_H

#include "Memory/reference.h"
#include "Memory/object.h"
#include "System/printer.h"

class SymbolTable;
class AbstractSynatxTree;

size_t get_base(AbstractSynatxTree *ast);
std::string type_name(const Reference &ref);

bool is_not_zero(SharedReference ref);
Printer *toPrinter(SharedReference ref);
void print(Printer *printer, SharedReference ref);

void init_call(AbstractSynatxTree *ast);
void exit_call(AbstractSynatxTree *ast);
void init_parameter(AbstractSynatxTree *ast, const std::string &symbol);
Function::mapping_type::iterator find_function_signature(AbstractSynatxTree *ast, Function::mapping_type &mapping, int signature);

SharedReference get_symbol_reference(SymbolTable *symbols, const std::string &symbol);
SharedReference get_object_member(AbstractSynatxTree *ast, const std::string &member);
void reduce_member(AbstractSynatxTree *ast);

std::string var_symbol(AbstractSynatxTree *ast);
void create_symbol(AbstractSynatxTree *ast, const std::string &symbol, Reference::Flags flags);

void array_append(AbstractSynatxTree *ast);
void array_append(Array *array, const SharedReference &item);
SharedReference array_get_item(Array *array, long index);
size_t array_index(Array *array, long index);

void hash_insert(AbstractSynatxTree *ast);
void hash_insert(Hash *hash, const SharedReference &key, const SharedReference &value);
SharedReference hash_get_item(Hash *hash, const SharedReference &key);
SharedReference hash_get_key(const Hash::values_type::value_type &item);
SharedReference hash_get_value(const Hash::values_type::value_type &item);

#endif // MEMORY_TOOL_H
