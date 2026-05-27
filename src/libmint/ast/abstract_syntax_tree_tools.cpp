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

#include "mint/ast/abstract_syntax_tree_tools.h"
#include "mint/ast/abstract_syntax_tree.h"
#include "mint/ast/module.h"
#include "mint/system/error.h"
#include <memory>
#include <string>

using namespace mint;

void mint::load_module(Cursor& cursor, const std::string& module) {
	if (!cursor.load_module(module)) [[unlikely]] {
		error("module '{}' not found", module);
	}
}

std::unique_ptr<Cursor> mint::load_module(const std::string& module, AbstractSyntaxTree& ast) {
	const auto infos = ast.load_module(module);
	if (infos.id == Module::invalid_id) [[unlikely]] {
		error("module '{}' not found", module);
	}
	return std::make_unique<Cursor>(ast, *infos.bytecode);
}
