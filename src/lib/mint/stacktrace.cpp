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

#include "mint/ast/cursor.h"
#include "mint/ast/module.h"
#include "mint/debug/debug_tools.h"
#include "mint/debug/line_info.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/memory/builtin/libobject.h"
#include "mint/memory/cast_tools.h"
#include "mint/memory/data.h"
#include "mint/memory/function_tools.h"
#include "mint/memory/memory_tools.h"
#include "mint/memory/reference.h"
#include "mint/scheduler/process.h"
#include "mint/scheduler/scheduler.h"
#include <span>
#include <utility>

namespace {

mint::Reference mint_stack_frame_from_id(mint::Cursor& cursor, const mint::Reference& module_id,
    const mint::Reference& module_name, const mint::Reference& line_number) {
	return mint::create_c_object(cursor.ast(),
	    new mint::LineInfo(mint::to_integer<mint::Module::Id>(cursor, module_id), mint::to_string(module_name),
	        mint::to_integer<std::size_t>(cursor, line_number)));
}

mint::Reference mint_stack_frame_from_name(mint::Cursor& cursor, const mint::Reference& module_name,
    const mint::Reference& line_number) {
	return mint::create_c_object(cursor.ast(), new mint::LineInfo(cursor.ast(), mint::to_string(module_name),
	                                               mint::to_integer<std::size_t>(cursor, line_number)));
}

mint::Reference mint_stack_frame_delete(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr) {
	delete d_ptr.data<mint::LibObject<mint::LineInfo>>().ptr;
	return {};
}

mint::Reference mint_stack_frame_get_module_id(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr) {
	return mint::create_unsigned_number(d_ptr.data<mint::LibObject<mint::LineInfo>>().ptr->module_id());
}

mint::Reference mint_stack_frame_get_module_name(mint::Cursor& cursor, const mint::Reference& d_ptr) {
	return mint::create_string(cursor.ast(), d_ptr.data<mint::LibObject<mint::LineInfo>>().ptr->module_name());
}

mint::Reference mint_stack_frame_line_number(mint::Cursor& cursor, const mint::Reference& d_ptr) {
	return mint::create_unsigned_number(d_ptr.data<mint::LibObject<mint::LineInfo>>().ptr->line_number());
}

mint::Reference mint_stack_frame_source_line(mint::Cursor& cursor, const mint::Reference& d_ptr) {
	return mint::create_string(cursor.ast(),
	    mint::get_module_line(d_ptr.data<mint::LibObject<mint::LineInfo>>().ptr->module_name(),
	        d_ptr.data<mint::LibObject<mint::LineInfo>>().ptr->line_number()));
}

mint::Reference mint_stack_frame_to_string(mint::Cursor& cursor, const mint::Reference& d_ptr) {
	return mint::create_string(cursor.ast(), d_ptr.data<mint::LibObject<mint::LineInfo>>().ptr->to_string(cursor.ast()));
}

mint::Reference mint_stack_frame_get_system_path(mint::Cursor& cursor, const mint::Reference& d_ptr) {
	return mint::create_string(cursor.ast(),
	    d_ptr.data<mint::LibObject<mint::LineInfo>>().ptr->system_path().generic_string());
}

mint::Reference mint_stack_frame_get_system_file_name(mint::Cursor& cursor, const mint::Reference& d_ptr) {
	return mint::create_string(cursor.ast(),
	    d_ptr.data<mint::LibObject<mint::LineInfo>>().ptr->system_file_name().generic_string());
}

}

MINT_RAW_FUNCTION(mint_lang_stacktrace, 1, cursor) {

	const auto& thread_id = cursor.stack().back();
	auto result = mint::create_iterator(cursor.ast());

	cursor.exit_call();
	cursor.exit_call();

	if (mint::is_instance_of(thread_id, mint::Data::Format::none)) {

		auto* root_cursor = &cursor;

		while (auto* parent = root_cursor->parent()) {
			root_cursor = parent;
		}

		for (const auto& frame : root_cursor->dump()) {
			mint::iterator_yield(cursor, result.data<mint::Iterator>(),
			    mint::create_iterator_from(cursor, mint::create_unsigned_number(frame.module_id()),
			        mint::create_string(cursor.ast(), frame.module_name()),
			        mint::create_unsigned_number(frame.line_number())));
		}
	}
	else if (const auto* scheduler = mint::Scheduler::instance()) {
		if (const auto* thread = scheduler->find_thread(mint::to_integer<mint::Process::ThreadId>(cursor, thread_id))) {
			for (const auto& frame : thread->cursor().dump()) {
				mint::iterator_yield(cursor, result.data<mint::Iterator>(),
				    mint::create_iterator_from(cursor, mint::create_unsigned_number(frame.module_id()),
				        mint::create_string(cursor.ast(), frame.module_name()),
				        mint::create_unsigned_number(frame.line_number())));
			}
		}
	}

	cursor.stack().back() = std::move(result);
}

MINT_EXPORT_FUNCTION(mint_stack_frame_from_id, 3)
MINT_EXPORT_FUNCTION(mint_stack_frame_from_name, 2)
MINT_EXPORT_FUNCTION(mint_stack_frame_delete, 1)
MINT_EXPORT_FUNCTION(mint_stack_frame_get_module_id, 1)
MINT_EXPORT_FUNCTION(mint_stack_frame_get_module_name, 1)
MINT_EXPORT_FUNCTION(mint_stack_frame_line_number, 1)
MINT_EXPORT_FUNCTION(mint_stack_frame_source_line, 1)
MINT_EXPORT_FUNCTION(mint_stack_frame_to_string, 1)
MINT_EXPORT_FUNCTION(mint_stack_frame_get_system_path, 1)
MINT_EXPORT_FUNCTION(mint_stack_frame_get_system_file_name, 1)
