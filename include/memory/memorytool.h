#ifndef MEMORY_TOOL_H
#define MEMORY_TOOL_H

#include "memory/reference.h"
#include "memory/builtin/array.h"
#include "memory/builtin/hash.h"
#include "memory/builtin/iterator.h"
#include "system/printer.h"

class SymbolTable;
class AbstractSynatxTree;

size_t get_base(AbstractSynatxTree *ast);
std::string type_name(const Reference &ref);

Printer *to_printer(SharedReference ref);
void print(Printer *printer, SharedReference ref);

void init_call(AbstractSynatxTree *ast);
void exit_call(AbstractSynatxTree *ast);
void init_parameter(AbstractSynatxTree *ast, const std::string &symbol);
Function::mapping_type::iterator find_function_signature(AbstractSynatxTree *ast, Function::mapping_type &mapping, int signature);

void yield(AbstractSynatxTree *ast);
void load_default_result(AbstractSynatxTree *ast);

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

void iterator_init(AbstractSynatxTree *ast, size_t length);
void iterator_init(Iterator *iterator, const Reference &ref);
void iterator_insert(Iterator *iterator, const SharedReference &item);
void iterator_add(Iterator *iterator, const SharedReference &item);
bool iterator_next(Iterator *iterator, SharedReference &item);

void regex_match(AbstractSynatxTree *ast);
void regex_unmatch(AbstractSynatxTree *ast);

#endif // MEMORY_TOOL_H
