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

#include <mint/memory/functiontool.h>
#include <mint/memory/builtin/string.h>
#include <mint/debug/debugtool.h>
#include <mint/ast/abstractsyntaxtree.h>
#include <mint/ast/asttools.h>
#include <mint/ast/cursor.h>
#include <sstream>

using namespace mint;

MINT_FUNCTION(mint_assembly_from_function, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &object = helper.pop_parameter();
	WeakReference result = create_hash();

	for (const auto &signature : object.data<Function>()->mapping) {

		const Module::Handle *handle = signature.second.handle;
		Cursor *dump_cursor = cursor->ast()->create_cursor(handle->module);
		dump_cursor->jmp(handle->offset - 1);

		size_t end_offset = static_cast<size_t>(dump_cursor->next().parameter);
		std::stringstream stream;

		for (size_t offset = dump_cursor->offset(); offset < end_offset; offset = dump_cursor->offset()) {
			dump_command(offset, dump_cursor->next().command, dump_cursor, stream);
		}

		hash_insert(result.data<Hash>(), create_number(signature.first), create_string(stream.str()));
	}
	
	helper.return_value(std::move(result));
}

MINT_FUNCTION(mint_assembly_from_module, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &object = helper.pop_parameter();

	Cursor *dump_cursor = load_module(object.data<String>()->str, cursor->ast());
	bool has_next = true;
	std::stringstream stream;

	while (has_next) {
		const size_t offset = dump_cursor->offset();
		switch (Node::Command command = dump_cursor->next().command) {
		case Node::exit_module:
			dump_command(offset, command, dump_cursor, stream);
			has_next = false;
			break;
		default:
			dump_command(offset, command, dump_cursor, stream);
		}
	}
	
	helper.return_value(create_string(stream.str()));
}
