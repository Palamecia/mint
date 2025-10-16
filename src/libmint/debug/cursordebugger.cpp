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

#include "mint/debug/cursordebugger.h"
#include "mint/ast/module.h"
#include "mint/ast/node.h"
#include "mint/debug/debuginfo.h"
#include "mint/debug/debugtool.h"
#include "mint/ast/abstractsyntaxtree.h"
#include "mint/ast/cursor.h"
#include "mint/debug/lineinfo.h"
#include "mint/debug/threadcontext.h"
#include "mint/memory/symboltable.h"
#include "mint/scheduler/process.h"

#include <cstddef>
#include <filesystem>
#include <functional>
#include <string>

using namespace mint;

CursorDebugger::CursorDebugger(Cursor& cursor, ThreadContext context) :
    _cursor(cursor),
    _context(context) {}

const ThreadContext& CursorDebugger::get_thread_context() const {
	return _context;
}

ThreadContext& CursorDebugger::get_thread_context() {
	return _context;
}

Process::ThreadId CursorDebugger::get_thread_id() const {
	return _context.thread_id;
}

void CursorDebugger::update_cursor(Cursor& cursor) {
	if (&_cursor.get() != &cursor) {
		_cursor = std::ref(cursor);
	}
}

bool CursorDebugger::close_cursor() {
	if (Cursor* cursor = _cursor.get().parent()) {
		_cursor = std::ref(*cursor);
		return true;
	}
	return false;
}

Node::Command CursorDebugger::command() const {
	return _cursor.get()._current_context->module.get().node_at(_cursor.get()._current_context->iptr).as_command();
}

const Cursor& CursorDebugger::cursor() const {
	return _cursor;
}

Cursor& CursorDebugger::cursor() {
	return _cursor;
}

const SymbolTable* CursorDebugger::symbols(std::size_t stack_frame) const {
	if (stack_frame == 0) {
		return _cursor.get()._current_context->symbols.get();
	}
	if (stack_frame > _cursor.get()._call_stack.size()) {
		return nullptr;
	}
	return _cursor.get()._call_stack[_cursor.get()._call_stack.size() - stack_frame]->symbols.get();
}

LineInfo CursorDebugger::line_info(std::size_t stack_frame) const {
	const Cursor::Context* context = nullptr;
	const auto& ast = _cursor.get().ast();
	if (stack_frame == 0) {
		context = _cursor.get()._current_context;
	}
	else if (stack_frame > _cursor.get()._call_stack.size()) {
		context = _cursor.get()._call_stack[_cursor.get()._call_stack.size() - stack_frame];
	}
	if (context) {
		std::size_t line_number = 0;
		const auto module_id = ast.get_module_id(context->module);
		if (DebugInfo* infos = ast.find_debug_info(module_id)) {
			line_number = infos->line_number(context->iptr);
		}
		return {module_id, ast.get_module_name(context->module), line_number};
	}
	return {};
}

std::string CursorDebugger::module_name() const {
	return _cursor.get().ast().get_module_name(_cursor.get()._current_context->module);
}

Module::Id CursorDebugger::module_id() const {
	return _cursor.get().ast().get_module_id(_cursor.get()._current_context->module);
}

std::size_t CursorDebugger::line_number() const {
	if (DebugInfo* info = _cursor.get().ast().find_debug_info(module_id())) {
		return info->line_number(_cursor.get()._current_context->iptr);
	}
	return 0;
}

std::size_t CursorDebugger::call_depth() const {

	std::size_t depth = 0;

	for (const auto* cursor = &_cursor.get(); cursor; cursor = cursor->_parent) {
		depth += cursor->_call_stack.size();
		if (cursor->parent()) {
			depth += 1;
		}
	}

	return depth;
}

std::filesystem::path CursorDebugger::system_path() const {
	return to_system_path(module_name());
}

std::filesystem::path CursorDebugger::system_file_name() const {
	return system_path().filename();
}
