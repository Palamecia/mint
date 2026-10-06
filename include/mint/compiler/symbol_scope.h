/**
 * Copyright (c) 2026 Gauvain CHÉRY.
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

#ifndef MINT_COMPILER_SYMBOL_SCOPE_H
#define MINT_COMPILER_SYMBOL_SCOPE_H

#include "mint/compiler/type_annotation.h"
#include "mint/program/symbol.h"
#include "mint/config.h"

#include <functional>
#include <variant>
#include <vector>

namespace mint {

class ClassDescription;
class FunctionDescription;
class PackageDescription;
class Program;
class SymbolScope;
struct VariableDescription;

using PackageDefinition = std::reference_wrapper<const PackageDescription>;
using ClassDefinition = std::reference_wrapper<const ClassDescription>;
using FunctionDefinition = std::vector<std::reference_wrapper<const FunctionDescription>>;
using VariableDefinition = std::reference_wrapper<const VariableDescription>;

class SymbolDefinition :
    public std::variant<std::monostate, PackageDefinition, ClassDefinition, FunctionDefinition, VariableDefinition> {
public:
	SymbolDefinition() = default;
	explicit(false) SymbolDefinition(const PackageDescription& description);
	explicit(false) SymbolDefinition(const ClassDescription& description);
	explicit(false) SymbolDefinition(FunctionDefinition description);
	explicit(false) SymbolDefinition(const VariableDescription& description);

	[[nodiscard]] Symbol get_name() const;
	[[nodiscard]] TypeAnnotation get_type() const;

	[[nodiscard]] bool is_valid() const {
		return !std::holds_alternative<std::monostate>(*this);
	}
};

class MINT_EXPORT SymbolScope {
	SymbolScope* _owner = nullptr;
public:
	explicit SymbolScope(SymbolScope* owner = nullptr);
	virtual ~SymbolScope() = default;

	[[nodiscard]] const SymbolScope& get_root_scope() const;
	[[nodiscard]] SymbolScope& get_root_scope();

	[[nodiscard]] const SymbolScope* get_owner_scope() const;
	[[nodiscard]] SymbolScope* get_owner_scope();

	[[nodiscard]] virtual bool is_root_scope() const = 0;

	[[nodiscard]] SymbolDefinition locate(const SymbolPath& symbols) const;
	[[nodiscard]] virtual SymbolDefinition locate(const Symbol& symbol) const = 0;

	virtual void cleanup_memory() = 0;
	virtual void cleanup_metadata() = 0;
};

}

#endif // MINT_COMPILER_SYMBOL_SCOPE_H
