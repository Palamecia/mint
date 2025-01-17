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

#ifndef BLOCK_H
#define BLOCK_H

#include "mint/compiler/buildtool.h"
#include "catchcontext.h"
#include "branch.h"

namespace mint {

struct Block {
	Block(BuildContext::BlockType type);

	BuildContext::BlockType type;
	Branch::ForwardNodeIndex *forward = nullptr;
	Branch::BackwardNodeIndex *backward = nullptr;
	CatchContext *catch_context = nullptr;
	CaseTable *case_table = nullptr;
	size_t retrieve_point_count = 0;
	std::vector<Symbol *> *condition_scoped_symbols = nullptr;
	std::vector<Symbol *> *range_loop_scoped_symbols = nullptr;
	std::vector<Symbol *> block_scoped_symbols;

	[[nodiscard]] bool is_breakable() const;
	[[nodiscard]] bool is_continuable() const;
};

}

#endif // BLOCK_H
