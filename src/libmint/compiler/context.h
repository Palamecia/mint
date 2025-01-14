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

#ifndef CONTEXT_H
#define CONTEXT_H

#include "mint/ast/symbolmapping.hpp"
#include "branch.h"

#include <memory>
#include <vector>
#include <stack>
#include <list>

namespace mint {

class Banch;
class ClassDescription;

struct Block;

static constexpr const size_t INVALID_OFFSET = static_cast<size_t>(-1);

struct Context {
	enum ResultTarget {
		SEND_TO_PRINTER,
		SEND_TO_GENERATOR_EXPRESSION
	};

	std::stack<ResultTarget> result_targets;
	std::stack<ClassDescription *> classes;
	std::list<Block *> blocks;
	std::unique_ptr<std::vector<Symbol *>> condition_scoped_symbols;
	std::unique_ptr<std::vector<Symbol *>> range_loop_scoped_symbols;
};

struct Parameter {
	Reference::Flags flags;
	Symbol *symbol;
};

struct Definition : public Context {
	std::vector<Branch::BackwardNodeIndex> exit_points;
	SymbolMapping<int> fast_symbol_indexes;
	size_t fast_symbol_count = 0;
	std::stack<Parameter> parameters;
	size_t begin_offset = INVALID_OFFSET;
	size_t retrieve_point_count = 0;
	Reference *function = nullptr;
	Branch *capture = nullptr;
	bool capture_all = false;
	bool with_fast = true;
	bool variadic = false;
	bool generator = false;
	bool returned = false;
};

int find_fast_symbol_index(const Definition *def, const Symbol *symbol);
int create_fast_symbol_index(Definition *def, const Symbol *symbol);
int fast_symbol_index(Definition *def, const Symbol *symbol);

}

#endif // CONTEXT_H
