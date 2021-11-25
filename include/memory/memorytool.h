#ifndef MINT_MEMORYTOOL_H
#define MINT_MEMORYTOOL_H

#include "memory/reference.h"
#include "memory/builtin/array.h"
#include "memory/builtin/hash.h"
#include "memory/builtin/iterator.h"
#include "system/printer.h"
#include "ast/symbol.h"

namespace mint {

class SymbolTable;
class Cursor;

MINT_EXPORT std::string type_name(const Reference &reference);
MINT_EXPORT bool is_class(const Object *data);
MINT_EXPORT bool is_object(const Object *data);

MINT_EXPORT Printer *create_printer(Cursor *cursor);
MINT_EXPORT void print(Printer *printer, Reference &reference);

MINT_EXPORT void load_extra_arguments(Cursor *cursor);
MINT_EXPORT void capture_symbol(Cursor *cursor, const Symbol &symbol);
MINT_EXPORT void capture_as_symbol(Cursor *cursor, const Symbol &symbol);
MINT_EXPORT void capture_all_symbols(Cursor *cursor);
MINT_EXPORT void init_call(Cursor *cursor);
MINT_EXPORT void init_member_call(Cursor *cursor, const Symbol &member);
MINT_EXPORT void init_operator_call(Cursor *cursor, Class::Operator op);
MINT_EXPORT void exit_call(Cursor *cursor);
MINT_EXPORT void init_parameter(Cursor *cursor, const Symbol &symbol, size_t index);
MINT_EXPORT Function::mapping_type::iterator find_function_signature(Cursor *cursor, Function::mapping_type &mapping, int signature);

MINT_EXPORT void yield(Cursor *cursor, Reference &generator);
MINT_EXPORT void load_generator_result(Cursor *cursor);

MINT_EXPORT WeakReference get_symbol_reference(SymbolTable *symbols, const Symbol &symbol);
MINT_EXPORT WeakReference get_object_member(Cursor *cursor, const Reference &reference, const Symbol &member, Class **owner = nullptr);
MINT_EXPORT WeakReference get_object_operator(Cursor *cursor, const Reference &reference, Class::Operator op, Class **owner = nullptr);
MINT_EXPORT void reduce_member(Cursor *cursor, Reference &&member);
MINT_EXPORT Class::MemberInfo *get_member_infos(Object *object, const Reference &member);
MINT_EXPORT bool is_protected_accessible(Class *owner, Class *context);

MINT_EXPORT Symbol var_symbol(Cursor *cursor);
MINT_EXPORT void create_symbol(Cursor *cursor, const Symbol &symbol, Reference::Flags flags);
MINT_EXPORT void create_function(Cursor *cursor, const Symbol &symbol, Reference::Flags flags);
MINT_EXPORT void function_overload_from_stack(Cursor *cursor);

}

#endif // MINT_MEMORYTOOL_H
