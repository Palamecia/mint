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

#ifndef LIBMINT_COMPILER_CONTEXT_H
#define LIBMINT_COMPILER_CONTEXT_H

#include "branch.h"
#include "mint/ast/symbol.h"
#include "mint/compiler/buildtool.h"
#include "mint/memory/reference.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>
#include <stack>
#include <list>

namespace mint {

class Branch;
class ClassDescription;

struct Block;

struct Context {
	enum class MetaBlock : std::uint8_t {
		printer,
		generator_expression
	};

	std::stack<MetaBlock> meta_blocks;
	std::stack<std::unique_ptr<ClassDescription>> classes;
	std::stack<SubBranch> branches;
	std::list<std::unique_ptr<Block>> blocks;
	std::unique_ptr<std::vector<const Symbol*>> condition_scoped_symbols;
	std::unique_ptr<std::vector<const Symbol*>> range_loop_scoped_symbols;
};

struct Parameter {
	Reference::Flags flags;
	const Symbol* symbol;
};

struct Definition : public Context {
	std::vector<Branch::BackwardNodeIndex> exit_points;
	std::unordered_map<Symbol, std::size_t> fast_symbol_indexes;
	std::size_t fast_symbol_count = 0;
	std::stack<Parameter> parameters;
	std::size_t begin_offset = invalid_offset;
	std::size_t retrieve_point_count = 0;
	Reference* function = nullptr;
	std::unique_ptr<Branch> capture;
	bool capture_all: 1 = false;
	bool with_fast: 1 = true;
	bool variadic: 1 = false;
	bool generator: 1 = false;
	bool async: 1 = false;
	bool returned: 1 = false;
};

std::size_t find_fast_symbol_index(const Definition& def, const Symbol& symbol);
std::size_t create_fast_symbol_index(Definition& def, const Symbol& symbol);
std::size_t fast_symbol_index(Definition& def, const Symbol& symbol);

}

#endif // LIBMINT_COMPILER_CONTEXT_H
