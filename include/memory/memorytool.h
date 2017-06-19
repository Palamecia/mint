#ifndef MEMORY_TOOL_H
#define MEMORY_TOOL_H

#include "memory/reference.h"
#include "memory/builtin/array.h"
#include "memory/builtin/hash.h"
#include "memory/builtin/iterator.h"
#include "system/printer.h"

class SymbolTable;
class AbstractSyntaxTree;

size_t get_base(AbstractSyntaxTree *ast);
std::string type_name(const Reference &ref);

Printer *to_printer(SharedReference ref);
void print(Printer *printer, SharedReference ref);

void capture_symbol(AbstractSyntaxTree *ast, const char *symbol);
void capture_all_symbols(AbstractSyntaxTree *ast);
void init_call(AbstractSyntaxTree *ast);
void exit_call(AbstractSyntaxTree *ast);
void init_parameter(AbstractSyntaxTree *ast, const std::string &symbol);
Function::mapping_type::iterator find_function_signature(AbstractSyntaxTree *ast, Function::mapping_type &mapping, int signature);

void yield(AbstractSyntaxTree *ast);
void load_default_result(AbstractSyntaxTree *ast);

SharedReference get_symbol_reference(SymbolTable *symbols, const std::string &symbol);
SharedReference get_object_member(AbstractSyntaxTree *ast, const std::string &member);
void reduce_member(AbstractSyntaxTree *ast);

std::string var_symbol(AbstractSyntaxTree *ast);
void create_symbol(AbstractSyntaxTree *ast, const std::string &symbol, Reference::Flags flags);

void array_append(AbstractSyntaxTree *ast);
void array_append(Array *array, const SharedReference &item);
SharedReference array_get_item(Array *array, long index);
size_t array_index(Array *array, long index);

void hash_insert(AbstractSyntaxTree *ast);
void hash_insert(Hash *hash, const Hash::key_type &key, const SharedReference &value);
SharedReference hash_get_item(Hash *hash, const Hash::key_type &key);
Hash::key_type hash_get_key(const Hash::values_type::value_type &item);
SharedReference hash_get_value(const Hash::values_type::value_type &item);

void iterator_init(AbstractSyntaxTree *ast, size_t length);
void iterator_init(Iterator *iterator, const Reference &ref);
void iterator_insert(Iterator *iterator, const SharedReference &item);
void iterator_add(Iterator *iterator, const SharedReference &item);
bool iterator_next(Iterator *iterator, SharedReference &item);

void regex_match(AbstractSyntaxTree *ast);
void regex_unmatch(AbstractSyntaxTree *ast);

#endif // MEMORY_TOOL_H
