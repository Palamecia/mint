#ifndef MEMORY_TOOL_H
#define MEMORY_TOOL_H

#include "memory/reference.h"
#include "memory/builtin/array.h"
#include "memory/builtin/hash.h"
#include "memory/builtin/iterator.h"
#include "system/printer.h"

class SymbolTable;
class Cursor;

size_t get_base(Cursor *cursor);
std::string type_name(const Reference &ref);

Printer *to_printer(SharedReference ref);
void print(Printer *printer, SharedReference ref);

void capture_symbol(Cursor *cursor, const char *symbol);
void capture_all_symbols(Cursor *cursor);
void init_call(Cursor *cursor);
void exit_call(Cursor *cursor);
void init_parameter(Cursor *cursor, const std::string &symbol);
Function::mapping_type::iterator find_function_signature(Cursor *cursor, Function::mapping_type &mapping, int signature);

void yield(Cursor *cursor);
void load_default_result(Cursor *cursor);

SharedReference get_symbol_reference(SymbolTable *symbols, const std::string &symbol);
SharedReference get_object_member(Cursor *cursor, const std::string &member);
void reduce_member(Cursor *cursor);

std::string var_symbol(Cursor *cursor);
void create_symbol(Cursor *cursor, const std::string &symbol, Reference::Flags flags);

void array_append(Cursor *cursor);
void array_append(Array *array, const SharedReference &item);
SharedReference array_get_item(Array *array, long index);
size_t array_index(Array *array, long index);

void hash_insert(Cursor *cursor);
void hash_insert(Hash *hash, const Hash::key_type &key, const SharedReference &value);
SharedReference hash_get_item(Hash *hash, const Hash::key_type &key);
Hash::key_type hash_get_key(const Hash::values_type::value_type &item);
SharedReference hash_get_value(const Hash::values_type::value_type &item);

void iterator_init(Cursor *cursor, size_t length);
void iterator_init(Iterator *iterator, const Reference &ref);
void iterator_insert(Iterator *iterator, const SharedReference &item);
void iterator_add(Iterator *iterator, const SharedReference &item);
bool iterator_next(Iterator *iterator, SharedReference &item);

void regex_match(Cursor *cursor);
void regex_unmatch(Cursor *cursor);

#endif // MEMORY_TOOL_H
