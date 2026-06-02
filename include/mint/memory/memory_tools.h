/**
 * Copyright (c) 2026 Gauvain CHERY.
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

#ifndef MINT_MEMORY_MEMORY_TOOLS_H
#define MINT_MEMORY_MEMORY_TOOLS_H

#include "mint/ast/class_register.h"
#include "mint/config.h"
#include "mint/memory/data.h"
#include "mint/memory/object.h"
#include "mint/memory/reference.h"
#include "mint/memory/class.h"
#include "mint/ast/printer.h"
#include "mint/ast/symbol.h"
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <utility>

namespace mint {

class Cursor;
class GlobalData;
class SymbolTable;

MINT_EXPORT std::string type_name(const Reference& reference);
MINT_EXPORT inline bool is_instance_of(const Reference& reference, Data::Format format);
MINT_EXPORT inline bool is_instance_of(const Reference& reference, Class::Metatype metatype);
MINT_EXPORT bool is_instance_of(const Reference& reference, const std::string& type_name);
MINT_EXPORT inline bool is_class(const Reference& reference);
MINT_EXPORT inline bool is_class(const Object& object);
MINT_EXPORT inline bool is_object(const Object& object);

MINT_EXPORT std::unique_ptr<Printer> create_printer(Cursor& cursor);

MINT_EXPORT void load_extra_arguments(Cursor& cursor);
MINT_EXPORT void capture_symbol(Cursor& cursor, const Symbol& symbol);
MINT_EXPORT void capture_as_symbol(Cursor& cursor, const Symbol& symbol);
MINT_EXPORT void capture_all_symbols(Cursor& cursor);
MINT_EXPORT void init_call(Cursor& cursor);
MINT_EXPORT void init_call(Cursor& cursor, const Reference& function);
MINT_EXPORT void init_member_call(Cursor& cursor, const Symbol& member);
MINT_EXPORT void init_member_call(Cursor& cursor, const Symbol& member, const Class::MemberInfo& info);
MINT_EXPORT void init_operator_call(Cursor& cursor, Class::Operator op);
MINT_EXPORT void exit_call(Cursor& cursor);
MINT_EXPORT void init_exception(Cursor& cursor, const Symbol& symbol);
MINT_EXPORT void reset_exception(Cursor& cursor, const Symbol& symbol);
MINT_EXPORT void reset_exception(Cursor& cursor);
MINT_EXPORT void init_parameter(Cursor& cursor, const Symbol& symbol, Reference::Flags flags, std::size_t index);
MINT_EXPORT Function::Mapping::const_iterator find_function_signature(Cursor& cursor, Function::Mapping& mapping,
    int signature);
MINT_EXPORT bool has_signature(Function::Mapping& mapping, int signature);
MINT_EXPORT bool has_signature(const Reference& reference, int signature);

MINT_EXPORT Reference get_symbol(Cursor& cursor, const Symbol& symbol);
MINT_EXPORT Reference get_symbol(SymbolTable& symbols, const Symbol& symbol);
MINT_EXPORT std::tuple<Reference, Class*> get_member(Cursor& cursor, const Reference& reference, const Symbol& member);
MINT_EXPORT std::tuple<Reference, Class*> get_operator(Cursor& cursor, const Reference& reference, Class::Operator op);
MINT_EXPORT void reduce_member(Cursor& cursor, Reference&& member);
MINT_EXPORT std::optional<std::pair<Symbol, std::reference_wrapper<const Class::MemberInfo>>> find_member(Object& object,
    const Reference& member);
MINT_EXPORT const Class::MemberInfo* find_member_info(Object& object, const Reference& member);
MINT_EXPORT std::optional<Symbol> find_member_symbol(Object& object, const Class::MemberInfo& member);
MINT_EXPORT bool is_protected_accessible(const Class& owner, const Class* context);
MINT_EXPORT bool is_protected_accessible(const Cursor& cursor, const Class& owner);
MINT_EXPORT bool is_private_accessible(const Cursor& cursor, const Class& owner);
MINT_EXPORT bool is_package_accessible(const Cursor& cursor, const Class& owner);

MINT_EXPORT Symbol var_symbol(Cursor& cursor);
MINT_EXPORT void declare_class(Cursor& cursor, ClassDescription& desc, Reference::Flags flags);
MINT_EXPORT void declare_symbol(Cursor& cursor, const Symbol& symbol, Reference::Flags flags);
MINT_EXPORT void declare_symbol(Cursor& cursor, const Symbol& symbol, std::size_t index, Reference::Flags flags);
MINT_EXPORT void declare_function(Cursor& cursor, const Symbol& symbol, Reference::Flags flags);
MINT_EXPORT void function_overload_from_stack(Cursor& cursor);

bool is_instance_of(const Reference& reference, Data::Format format) {
	return reference.data().format() == format;
}

bool is_instance_of(const Reference& reference, Class::Metatype metatype) {
	return reference.data().format() == Data::Format::object
	       && reference.data<Object>().metadata.metatype() == metatype;
}

bool is_class(const Reference& reference) {
	return reference.data().format() == Data::Format::object && is_class(reference.data<Object>());
}

bool is_class(const Object& object) {
	return object.data == nullptr;
}

bool is_object(const Object& object) {
	return object.data != nullptr;
}

}

#endif // MINT_MEMORY_MEMORY_TOOLS_H
