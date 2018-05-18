#ifndef MEMORY_TOOL_H
#define MEMORY_TOOL_H

#include "memory/reference.h"
#include "memory/builtin/array.h"
#include "memory/builtin/hash.h"
#include "memory/builtin/iterator.h"
#include "system/printer.h"

namespace mint {

class SymbolTable;
class Cursor;

MINT_EXPORT size_t get_stack_base(Cursor *cursor);
MINT_EXPORT std::string type_name(const Reference &ref);
MINT_EXPORT bool is_class(Object *data);
MINT_EXPORT bool is_object(Object *data);

MINT_EXPORT Printer *create_printer(Cursor *cursor);
MINT_EXPORT void print(Printer *printer, SharedReference ref);

MINT_EXPORT void capture_symbol(Cursor *cursor, const char *symbol);
MINT_EXPORT void capture_all_symbols(Cursor *cursor);
MINT_EXPORT void init_call(Cursor *cursor);
MINT_EXPORT void init_member_call(Cursor *cursor, const std::string &member);
MINT_EXPORT void exit_call(Cursor *cursor);
MINT_EXPORT void init_parameter(Cursor *cursor, const std::string &symbol);
MINT_EXPORT Function::mapping_type::iterator find_function_signature(Cursor *cursor, Function::mapping_type &mapping, int signature);

MINT_EXPORT void yield(Cursor *cursor);
MINT_EXPORT void load_default_result(Cursor *cursor);

MINT_EXPORT SharedReference get_symbol_reference(SymbolTable *symbols, const std::string &symbol);
MINT_EXPORT SharedReference get_object_member(Cursor *cursor, const std::string &member, Class::MemberInfo *infos = nullptr);
MINT_EXPORT void reduce_member(Cursor *cursor, SharedReference member);

MINT_EXPORT std::string var_symbol(Cursor *cursor);
MINT_EXPORT void create_symbol(Cursor *cursor, const std::string &symbol, Reference::Flags flags);

MINT_EXPORT void array_append_from_stack(Cursor *cursor);
MINT_EXPORT void array_append(Array *array, const SharedReference &item);
MINT_EXPORT SharedReference array_get_item(const Array *array, long index);
MINT_EXPORT size_t array_index(const Array *array, long index);

MINT_EXPORT void hash_insert_from_stack(Cursor *cursor);
MINT_EXPORT void hash_insert(Hash *hash, const Hash::key_type &key, const SharedReference &value);
MINT_EXPORT SharedReference hash_get_item(Hash *hash, const Hash::key_type &key);
MINT_EXPORT Hash::key_type hash_get_key(const Hash::values_type::value_type &item);
MINT_EXPORT SharedReference hash_get_value(const Hash::values_type::value_type &item);

MINT_EXPORT void iterator_init(Cursor *cursor, size_t length);
MINT_EXPORT void iterator_init(Iterator *iterator, const Reference &ref);
MINT_EXPORT void iterator_insert(Iterator *iterator, const SharedReference &item);
MINT_EXPORT void iterator_add(Iterator *iterator, const SharedReference &item);
MINT_EXPORT SharedReference iterator_next(Iterator *iterator);

}

#endif // MEMORY_TOOL_H
