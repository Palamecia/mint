/**
 * Copyright (c) 2024 Gauvain CHERY.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef MINT_MEMORYTOOL_H
#define MINT_MEMORYTOOL_H

#include "mint/memory/reference.h"
#include "mint/memory/class.h"
#include "mint/ast/printer.h"
#include "mint/ast/symbol.h"

namespace mint {

class SymbolTable;
class Cursor;

MINT_EXPORT std::string type_name(const Reference &reference);
MINT_EXPORT inline bool is_instance_of(const Reference &reference, Data::Format format);
MINT_EXPORT inline bool is_instance_of(const Reference &reference, Class::Metatype metatype);
MINT_EXPORT bool is_instance_of(const Reference &reference, const std::string &type_name);
MINT_EXPORT inline bool is_class(const Reference &reference);
MINT_EXPORT inline bool is_class(const Object *data);
MINT_EXPORT inline bool is_object(const Object *data);

MINT_EXPORT Printer *create_printer(Cursor *cursor);
MINT_EXPORT void print(Printer *printer, Reference &reference);

MINT_EXPORT void load_extra_arguments(Cursor *cursor);
MINT_EXPORT void capture_symbol(Cursor *cursor, const Symbol &symbol);
MINT_EXPORT void capture_as_symbol(Cursor *cursor, const Symbol &symbol);
MINT_EXPORT void capture_all_symbols(Cursor *cursor);
MINT_EXPORT void init_call(Cursor *cursor);
MINT_EXPORT void init_call(Cursor *cursor, Reference &function);
MINT_EXPORT void init_member_call(Cursor *cursor, const Symbol &member);
MINT_EXPORT void init_operator_call(Cursor *cursor, Class::Operator op);
MINT_EXPORT void exit_call(Cursor *cursor);
MINT_EXPORT void init_exception(Cursor *cursor, const Symbol &symbol);
MINT_EXPORT void reset_exception(Cursor *cursor, const Symbol &symbol);
MINT_EXPORT void init_parameter(Cursor *cursor, const Symbol &symbol, Reference::Flags flags, size_t index);
MINT_EXPORT Function::mapping_type::iterator find_function_signature(Cursor *cursor, Function::mapping_type &mapping, int signature);
MINT_EXPORT bool has_signature(Function::mapping_type &mapping, int signature);
MINT_EXPORT bool has_signature(Reference &reference, int signature);

MINT_EXPORT void yield(Cursor *cursor, Reference &generator);

MINT_EXPORT WeakReference get_symbol_reference(SymbolTable *symbols, const Symbol &symbol);
MINT_EXPORT WeakReference get_object_member(Cursor *cursor, const Reference &reference, const Symbol &member, Class **owner = nullptr);
MINT_EXPORT WeakReference get_object_operator(Cursor *cursor, const Reference &reference, Class::Operator op, Class **owner = nullptr);
MINT_EXPORT void reduce_member(Cursor *cursor, Reference &&member);
MINT_EXPORT Class::MemberInfo *get_member_infos(Object *object, const Reference &member);
MINT_EXPORT bool is_protected_accessible(Class *owner, Class *context);
MINT_EXPORT bool is_protected_accessible(Cursor *cursor, Class *owner);
MINT_EXPORT bool is_private_accessible(Cursor *cursor, Class *owner);
MINT_EXPORT bool is_package_accessible(Cursor *cursor, Class *owner);

MINT_EXPORT Symbol var_symbol(Cursor *cursor);
MINT_EXPORT void create_symbol(Cursor *cursor, const Symbol &symbol, Reference::Flags flags);
MINT_EXPORT void create_symbol(Cursor *cursor, const Symbol &symbol, size_t index, Reference::Flags flags);
MINT_EXPORT void create_function(Cursor *cursor, const Symbol &symbol, Reference::Flags flags);
MINT_EXPORT void function_overload_from_stack(Cursor *cursor);

bool is_instance_of(const Reference &reference, Data::Format format) {
	return reference.data()->format == format;
}

bool is_instance_of(const Reference &reference, Class::Metatype metatype) {
	return reference.data()->format == Data::fmt_object && reference.data<Object>()->metadata->metatype() == metatype;
}

bool is_class(const Reference &reference) {
	return reference.data()->format == Data::fmt_object && is_class(reference.data<Object>());
}

bool is_class(const Object *data) {
	return data->data == nullptr;
}

bool is_object(const Object *data) {
	return data->data != nullptr;
}

}

#endif // MINT_MEMORYTOOL_H
