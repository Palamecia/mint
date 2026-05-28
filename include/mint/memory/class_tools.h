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

#ifndef MINT_MEMORY_CLASS_TOOLS_H
#define MINT_MEMORY_CLASS_TOOLS_H

#include "mint/ast/class_register.h"
#include "mint/ast/module.h"
#include "mint/ast/symbol.h"
#include "mint/config.h"
#include "mint/memory/reference.h"
#include "mint/memory/class.h"

#include <cstdint>
#include <functional>
#include <initializer_list>
#include <optional>
#include <span>
#include <string>
#include <utility>

namespace mint {

MINT_EXPORT Class& create_enum(AbstractSyntaxTree& ast, const std::string& name,
    std::span<const std::pair<Symbol, std::optional<intmax_t>>> values);
MINT_EXPORT Class& create_enum(AbstractSyntaxTree& ast, ModuleInfo& module, const std::string& name,
    std::span<const std::pair<Symbol, std::optional<intmax_t>>> values);

MINT_EXPORT Class& create_enum(AbstractSyntaxTree& ast, const std::string& name,
    std::initializer_list<std::pair<Symbol, std::optional<intmax_t>>> values);
MINT_EXPORT Class& create_enum(AbstractSyntaxTree& ast, ModuleInfo& module, const std::string& name,
    std::initializer_list<std::pair<Symbol, std::optional<intmax_t>>> values);

MINT_EXPORT Class& create_class(AbstractSyntaxTree& ast, const std::string& name,
    std::span<const std::pair<Symbol, Reference>> members);
MINT_EXPORT Class& create_class(AbstractSyntaxTree& ast, ModuleInfo& module, const std::string& name,
    std::span<const std::pair<Symbol, Reference>> members);
MINT_EXPORT Class& create_class(AbstractSyntaxTree& ast, const std::string& name,
    std::span<const std::reference_wrapper<mint::ClassDescription>> bases,
    std::span<const std::pair<Symbol, Reference>> members);
MINT_EXPORT Class& create_class(AbstractSyntaxTree& ast, ModuleInfo& module, const std::string& name,
    std::span<const std::reference_wrapper<mint::ClassDescription>> bases,
    std::span<const std::pair<Symbol, Reference>> members);
MINT_EXPORT Class& create_class(AbstractSyntaxTree& ast, const std::string& name,
    std::span<const mint::ClassRegister::Path> bases, std::span<const std::pair<Symbol, Reference>> members);
MINT_EXPORT Class& create_class(AbstractSyntaxTree& ast, ModuleInfo& module, const std::string& name,
    std::span<const mint::ClassRegister::Path> bases, std::span<const std::pair<Symbol, Reference>> members);
MINT_EXPORT Class& create_class(AbstractSyntaxTree& ast, const std::string& name,
    std::initializer_list<std::pair<Symbol, Reference>> members);
MINT_EXPORT Class& create_class(AbstractSyntaxTree& ast, ModuleInfo& module, const std::string& name,
    std::initializer_list<std::pair<Symbol, Reference>> members);
MINT_EXPORT Class& create_class(AbstractSyntaxTree& ast, const std::string& name,
    std::initializer_list<std::reference_wrapper<mint::ClassDescription>> bases,
    std::initializer_list<std::pair<Symbol, Reference>> members);
MINT_EXPORT Class& create_class(AbstractSyntaxTree& ast, ModuleInfo& module, const std::string& name,
    std::initializer_list<std::reference_wrapper<mint::ClassDescription>> bases,
    std::initializer_list<std::pair<Symbol, Reference>> members);
MINT_EXPORT Class& create_class(AbstractSyntaxTree& ast, const std::string& name,
    std::initializer_list<mint::ClassRegister::Path> bases,
    std::initializer_list<std::pair<Symbol, Reference>> members);
MINT_EXPORT Class& create_class(AbstractSyntaxTree& ast, ModuleInfo& module, const std::string& name,
    std::initializer_list<mint::ClassRegister::Path> bases,
    std::initializer_list<std::pair<Symbol, Reference>> members);

}

#endif // MINT_MEMORY_CLASS_TOOLS_H
