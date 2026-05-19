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

#include "mint/ast/module.h"
#include "mint/ast/node.h"
#include "mint/memory/builtin/hash.h"
#include "mint/memory/functiontool.h"
#include "mint/memory/builtin/string.h"
#include "mint/debug/debugtool.h"
#include "mint/ast/abstractsyntaxtree.h"
#include "mint/ast/asttools.h"
#include "mint/ast/cursor.h"
#include "mint/memory/reference.h"
#include <cstddef>
#include <sstream>
#include <utility>

namespace {

mint::Reference mint_assembly_from_function(mint::Cursor& cursor, const mint::Reference& object) {

	mint::Reference result = mint::create_hash(cursor.ast());

	for (auto& signature : object.data<mint::Function>().mapping) {

		mint::Module::Handle& handle = signature.second.handle();
		auto dump_cursor = mint::Cursor(cursor.ast(), handle.module);
		dump_cursor.jmp(handle.offset - 1);

		const auto end_offset = static_cast<std::size_t>(dump_cursor.next().as_parameter());
		auto stream = std::stringstream();

		for (std::size_t offset = dump_cursor.offset(); offset < end_offset; offset = dump_cursor.offset()) {
			mint::dump_command(dump_cursor, stream);
		}

		hash_insert(result.data<mint::Hash>(), mint::create_signed_number(signature.first),
		    mint::create_string(cursor.ast(), std::move(stream).str()));
	}

	return result;
}

mint::Reference mint_assembly_from_module(mint::Cursor& cursor, const mint::Reference& object) {

	auto dump_cursor = load_module(object.data<mint::String>().str, cursor.ast());
	auto stream = std::stringstream();

	while (mint::dump_command(*dump_cursor, stream) != mint::Node::Command::exit_module) {
		;
	}

	return mint::create_string(cursor.ast(), std::move(stream).str());
}

}

MINT_EXPORT_FUNCTION(mint_assembly_from_function, 1)
MINT_EXPORT_FUNCTION(mint_assembly_from_module, 1)
