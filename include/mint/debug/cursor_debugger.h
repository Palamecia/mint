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

#ifndef MINT_DEBUG_CURSOR_DEBUGGER_H
#define MINT_DEBUG_CURSOR_DEBUGGER_H

#include "mint/ast/module.h"
#include "mint/ast/node.h"
#include "mint/config.h"
#include "mint/debug/line_info.h"
#include "mint/debug/thread_context.h"
#include "mint/memory/symbol_table.h"
#include "mint/scheduler/process.h"

#include <cstddef>
#include <filesystem>
#include <functional>
#include <string>

namespace mint {

class Cursor;
struct ThreadContext;

class MINT_EXPORT CursorDebugger {
public:
	CursorDebugger(Cursor& cursor, ThreadContext context);

	[[nodiscard]] const ThreadContext& get_thread_context() const;
	ThreadContext& get_thread_context();
	[[nodiscard]] Process::ThreadId get_thread_id() const;

	void update_cursor(Cursor& cursor);
	bool close_cursor();

	[[nodiscard]] Node::Command command() const;
	[[nodiscard]] const Cursor& cursor() const;
	[[nodiscard]] Cursor& cursor();

	[[nodiscard]] const SymbolTable* symbols(std::size_t stack_frame_index = 0) const;
	[[nodiscard]] LineInfo line_info(std::size_t stack_frame_index = 0) const;

	[[nodiscard]] std::string module_name() const;
	[[nodiscard]] Module::Id module_id() const;
	[[nodiscard]] std::size_t line_number() const;
	[[nodiscard]] std::size_t call_depth() const;

	[[nodiscard]] std::filesystem::path system_path() const;
	[[nodiscard]] std::filesystem::path system_file_name() const;

private:
	std::reference_wrapper<Cursor> _cursor;
	ThreadContext _context;
};

}

#endif // MINT_DEBUG_CURSOR_DEBUGGER_H
