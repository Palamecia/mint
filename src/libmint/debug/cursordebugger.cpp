/**
 * Copyright (c) 2025 Gauvain CHERY.
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
#include "mint/debug/debugtool.h"
#include "mint/system/filesystem.h"
#include "mint/ast/abstractsyntaxtree.h"
#include "mint/ast/cursor.h"
#include "threadcontext.hpp"

#include <filesystem>

using namespace mint;

CursorDebugger::CursorDebugger(Cursor *cursor, ThreadContext *context) :
	m_cursor(cursor),
	m_context(context) {}

const ThreadContext *CursorDebugger::get_thread_context() const {
	return m_context;
}

ThreadContext *CursorDebugger::get_thread_context() {
	return m_context;
}

Process::ThreadId CursorDebugger::get_thread_id() const {
	return m_context->thread_id;
}

void CursorDebugger::update_cursor(Cursor *cursor) {
	if (m_cursor != cursor) {
		m_cursor = cursor;
	}
}

bool CursorDebugger::close_cursor() {
	if (Cursor *cursor = m_cursor->parent()) {
		m_cursor = cursor;
		return true;
	}
	return false;
}

Node::Command CursorDebugger::command() const {
	return m_cursor->m_current_context->module->at(m_cursor->m_current_context->iptr).command;
}

Cursor *CursorDebugger::cursor() const {
	return m_cursor;
}

const SymbolTable *CursorDebugger::symbols(size_t stack_frame) const {
	if (stack_frame == 0) {
		return m_cursor->m_current_context->symbols;
	}
	if (stack_frame > m_cursor->m_call_stack.size()) {
		return nullptr;
	}
	return m_cursor->m_call_stack[m_cursor->m_call_stack.size() - stack_frame]->symbols;
}

LineInfo CursorDebugger::line_info(size_t stack_frame) const {
	const Cursor::Context *context = nullptr;
	AbstractSyntaxTree *ast = m_cursor->ast();
	if (stack_frame == 0) {
		context = m_cursor->m_current_context;
	}
	else if (stack_frame > m_cursor->m_call_stack.size()) {
		context = m_cursor->m_call_stack[m_cursor->m_call_stack.size() - stack_frame];
	}
	if (context) {
		size_t line_number = 0;
		Module::Id module_id = ast->get_module_id(context->module);
		if (DebugInfo *infos = m_cursor->ast()->get_debug_info(module_id)) {
			line_number = infos->line_number(context->iptr);
		}
		return {module_id, ast->get_module_name(context->module), line_number};
	}
	return {};
}

std::string CursorDebugger::module_name() const {
	return m_cursor->ast()->get_module_name(m_cursor->m_current_context->module);
}

Module::Id CursorDebugger::module_id() const {
	return m_cursor->ast()->get_module_id(m_cursor->m_current_context->module);
}

size_t CursorDebugger::line_number() const {
	if (DebugInfo *info = m_cursor->ast()->get_debug_info(module_id())) {
		return info->line_number(m_cursor->m_current_context->iptr);
	}
	return 0;
}

size_t CursorDebugger::call_depth() const {

	size_t depth = 0;

	for (Cursor *cursor = m_cursor; cursor; cursor = cursor->m_parent) {

		depth += cursor->m_call_stack.size();

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
