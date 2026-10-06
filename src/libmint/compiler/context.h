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
#include "mint/compiler/symbol_scope.h"
#include "mint/compiler/type_annotation.h"
#include "mint/program/symbol.h"
#include "mint/compiler/build_context.h"
#include "mint/memory/reference.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>
#include <stack>

namespace mint {

class ClassDescription;

struct Block;

struct ClassDefinition {
	std::reference_wrapper<ClassDescription> description;
	Reference::Flags flags = Reference::default_flags;
};

struct Context {
	enum class MetaBlock : std::uint8_t {
		printer,
		generator_expression,
	};

	Context* enclosing = nullptr;
	std::stack<MetaBlock> meta_blocks;
	std::stack<ClassDefinition> classes;
	std::vector<std::unique_ptr<Block>> blocks;

	// TODO: staged informations from the control structures conditions to be transferred once the block is created. The
	// blocks should then starts once the corresponding keyword is found then those attributes could disappear.
	std::stack<SubBranch> branches;
	std::unique_ptr<std::vector<const Symbol*>> condition_scoped_symbols;
	std::unique_ptr<std::vector<const Symbol*>> range_loop_scoped_symbols;
};

struct Parameter {
	const Symbol* symbol = nullptr;
	TypeAnnotation type;
	Reference::Flags flags = Reference::default_flags;
};

struct FunctionDefinition : Context {
	FunctionDescription* description = nullptr;
	Reference::Flags flags = Reference::default_flags;
	Reference* function = nullptr;
	std::stack<Parameter> parameters;
	std::vector<Branch::BackwardNodeIndex> exit_points;
	std::unordered_map<Symbol, std::size_t> fast_symbol_indexes;
	std::size_t fast_symbol_count = 0;
	std::size_t begin_offset = invalid_offset;
	std::size_t retrieve_point_count = 0;
	std::optional<SubBranch> capture;
	TypeAnnotation return_type;
	bool capture_all: 1 = false;
	bool with_fast: 1 = true;
	bool variadic: 1 = false;
	bool generator: 1 = false;
	bool async: 1 = false;
	bool returned: 1 = false;
};

std::size_t find_fast_symbol_index(const FunctionDefinition& def, const Symbol& symbol);
std::size_t create_fast_symbol_index(FunctionDefinition& def, const Symbol& symbol);
std::size_t fast_symbol_index(FunctionDefinition& def, const Symbol& symbol);

}

#endif // LIBMINT_COMPILER_CONTEXT_H
