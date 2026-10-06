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

#ifndef LIBMINT_COMPILER_CONTEXT_H
#define LIBMINT_COMPILER_CONTEXT_H

#include "branch.h"
#include "mint/compiler/descriptions.h"
#include "mint/compiler/parser_data.h"
#include "mint/compiler/symbol_scope.h"
#include "mint/compiler/type_annotation.h"
#include "mint/memory/garbage_collector.h"
#include "mint/program/module.h"
#include "mint/program/symbol.h"
#include "mint/compiler/build_context.h"
#include "mint/memory/reference.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <variant>
#include <vector>

namespace mint {

class ClassDescription;

struct Block;
struct FunctionContext;

struct ClassContext {
	std::reference_wrapper<ClassDescription> description;
	Reference::Flags flags = Reference::default_flags;
};

struct PendingOverride {
	std::size_t index = invalid_index;
	TypeAnnotation type;
};

struct ConditionContext {
	ParserData condition_result;
	std::vector<const Symbol*> scoped_symbols;
	std::vector<PendingOverride> pending_type_overrides;
	bool is_narrowing_valid = true;
};

struct RangeLoopContext {
	std::vector<const Symbol*> scoped_symbols;
};

class SymbolEntry :
    public std::variant<PackageDefinition, ClassDefinition, FunctionDefinition, VariableDefinition, AliasDefinition> {
public:
	explicit(false) SymbolEntry(const PackageDescription& description);
	explicit(false) SymbolEntry(const ClassDescription& description);
	explicit(false) SymbolEntry(FunctionDefinition description);
	explicit(false) SymbolEntry(const VariableDescription& description);
	explicit(false) SymbolEntry(const AliasDescription& description);

	[[nodiscard]] Symbol get_name() const;
	[[nodiscard]] ParserData to_parser_data() const;
};

struct Context {
	enum class MetaBlock : std::uint8_t {
		printer,
		generator_expression,
	};

	std::vector<MetaBlock> meta_blocks;
	std::vector<ClassContext> classes;
	std::vector<std::unique_ptr<Block>> blocks;
	std::vector<SymbolEntry> local_symbols;
	std::vector<const TypeAnnotation*> active_type_overrides;

	// TODO: staged informations from the control structures conditions to be transferred once the block is created. The
	// blocks should then starts once the corresponding keyword is found then those attributes could disappear.
	std::unique_ptr<ConditionContext> current_condition;
	std::unique_ptr<RangeLoopContext> current_range_loop;
	std::vector<SubBranch> branches;
};

struct FunctionParameter {
	const Symbol* symbol = nullptr;
	TypeAnnotation type;
	Reference::Flags flags = Reference::default_flags;
};

struct FunctionSignature {
	int parameter_count = 0;
	std::reference_wrapper<FunctionHandle> handle;
};

struct FunctionContext : Context {
	FunctionContext* enclosing = nullptr;
	FunctionDescription* description = nullptr;
	std::vector<FunctionSignature> signatures;
	std::vector<FunctionParameter> parameters;
	std::vector<std::unique_ptr<VariableDescription>> capture_parameters;
	std::vector<Branch::BackwardNodeIndex> exit_points;
	std::size_t begin_offset = invalid_offset;
	std::size_t retrieve_point_count = 0;
	std::optional<SubBranch> capture;
	bool capture_all: 1 = false;
	bool with_fast: 1 = true;
	bool variadic: 1 = false;
	bool generator: 1 = false;
	bool async: 1 = false;
	bool terminated: 1 = false;
};

VariableDefinition create_variable_definition(FunctionContext& def, const VariableDescription& description);

std::size_t find_fast_symbol_index(const FunctionContext& def, const Symbol& symbol);
std::size_t create_fast_symbol_index(FunctionContext& def, SymbolDefinition definition);
std::size_t create_fast_package_index(FunctionContext& def, PackageDefinition definition);
std::size_t create_fast_class_index(FunctionContext& def, ClassDefinition definition);
std::size_t create_fast_function_index(FunctionContext& def, const FunctionDefinition& definition);
std::size_t create_fast_variable_index(FunctionContext& def, VariableDefinition definition);
std::size_t fast_symbol_index(FunctionContext& def, const VariableDescription& description);

}

#endif // LIBMINT_COMPILER_CONTEXT_H
