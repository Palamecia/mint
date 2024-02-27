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

#include "mint/scheduler/process.h"
#include "mint/scheduler/processor.h"
#include "mint/scheduler/exception.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/memory/functiontool.h"
#include "mint/compiler/compiler.h"
#include "mint/debug/debuginterface.h"
#include "mint/debug/debugtool.h"
#include "mint/system/mintsystemerror.hpp"
#include "mint/system/filestream.h"
#include "mint/system/bufferstream.h"
#include "mint/system/terminal.h"
#include "mint/system/pipe.h"
#include "mint/system/error.h"
#include "mint/ast/abstractsyntaxtree.h"
#include "mint/scheduler/inputstream.h"
#include "mint/scheduler/output.h"

#include "bracematcher.h"
#include "completer.h"
#include "highlighter.h"

#include <sstream>
#include <thread>

using namespace std;
using namespace mint;

Process::Process(Cursor *cursor) :
	m_cursor(cursor),
	m_endless(false),
	m_thread_id(0),
	m_error_handler(0) {

}

Process::~Process() {
	lock_processor();
	delete m_cursor;
	unlock_processor();
}

Process *Process::from_main_file(AbstractSyntaxTree *ast, const string &file) {

	try {

		const string module_file_path = is_module_file(file) ? file : FileSystem::instance().get_script_path(file);

		Compiler compiler;
		FileStream stream(module_file_path);

		if (stream.is_valid()) {

			Module::Info info = ast->create_main_module(Module::ready);
			if (compiler.build(&stream, info)) {
				set_main_module_path(module_file_path);
				return new Process(ast->create_cursor(info.id));
			}
		}
	}
	catch (const MintSystemError &) {
		return nullptr;
	}

	return nullptr;
}

Process *Process::from_file(AbstractSyntaxTree *ast, const string &file) {

	try {

		Compiler compiler;
		FileStream stream(file);

		if (stream.is_valid()) {

			Module::Info info = ast->create_module_from_file_path(file, Module::ready);
			if (compiler.build(&stream, info)) {
				return new Process(ast->create_cursor(info.id));
			}
		}
	}
	catch (const MintSystemError &) {
		return nullptr;
	}

	return nullptr;
}

Process *Process::from_buffer(AbstractSyntaxTree *ast, const string &buffer) {

	try {
		Compiler compiler;
		BufferStream stream(buffer);

		if (stream.is_valid()) {
			
			Module::Info info = ast->create_module(Module::ready);
			if (compiler.build(&stream, info)) {
				return new Process(ast->create_cursor(info.id));
			}
		}
	}
	catch (const MintSystemError &) {
		return nullptr;
	}

	return nullptr;
}

Process *Process::from_standard_input(AbstractSyntaxTree *ast) {

	if (InputStream::instance().is_valid()) {

		Module::Info info = ast->create_main_module(Module::ready);
		Process *process = new Process(ast->create_cursor(info.id));
		process->cursor()->open_printer(&Output::instance());
		process->set_endless(true);

		InputStream::instance().set_higlighter([](const string_view &input, string_view::size_type offset) -> string {
			string output;
			Highlighter highlighter(output, offset);
			stringstream stream(input.data());
			if (highlighter.parse(stream)) {
				return output;
			}
			return input.data();
		});

		InputStream::instance().set_completion_generator([cursor = process->cursor()](const string_view &input, string_view::size_type offset, vector<completion_t> &completions) -> bool {
			if (offset == 0) {
				return false;
			}
			for (auto i = offset; i != 0 && input[i - 1] != '\n'; --i) {
				if (input[i - 1] != ' ') {
					Completer completer(completions, offset, cursor);
					stringstream stream(input.data());
					completer.parse(stream);
					return true;
				}
			}
			return false;
		});

		InputStream::instance().set_brace_matcher([](const string_view &input, string_view::size_type offset) -> pair<string_view::size_type, bool> {
			pair<string_view::size_type, bool> match;
			BraceMatcher matcher(match, offset);
			stringstream stream(input.data());
			matcher.parse(stream);
			return match;
		});

		return process;
	}

	return nullptr;
}

void Process::parse_argument(const string &arg) {

	auto args = m_cursor->symbols().find("va_args");
	if (args == m_cursor->symbols().end()) {

		Iterator *va_args = GarbageCollector::instance().alloc<Iterator>();
		va_args->construct();
		args = m_cursor->symbols().emplace("va_args", WeakReference(Reference::standard, va_args)).first;
	}

	iterator_insert(args->second.data<Iterator>(), create_string(arg));
}

void Process::setup() {
	if (!m_cursor->parent()) {
		m_error_handler = add_error_callback(bind(&Process::dump, this));
	}
}

void Process::cleanup() {
	if (m_error_handler) {
		remove_error_callback(m_error_handler);
	}
	lock_processor();
	m_cursor->cleanup();
	unlock_processor();
}

bool Process::exec() {
	try {
		return run_steps(m_cursor);
	}
	catch (MintException &raised) {
		if (m_cursor == raised.cursor()) {
			m_cursor->raise(raised.take_exception());
			unlock_processor();
			return true;
		}
		else {
			throw;
		}
	}
	catch (const MintSystemError &) {
		unlock_processor();
		return false;
	}
}

bool Process::debug(DebugInterface *debugInterface) {
	try {
		return debug_steps(debugInterface->declare_thread(this), debugInterface);
	}
	catch (MintException &raised) {
		if (m_cursor == raised.cursor()) {
			m_cursor->raise(raised.take_exception());
			unlock_processor();
			return true;
		}
		else {
			throw;
		}
	}
	catch (const MintSystemError &) {
		unlock_processor();
		return false;
	}
}

bool Process::resume() {

	while (m_endless) {
		try {
			Compiler compiler;
			compiler.set_printing(true);
			m_cursor->resume();
			InputStream::instance().next();
			return compiler.build(&InputStream::instance(), m_cursor->ast()->main());
		}
		catch (const MintSystemError &) {
			continue;
		}
	}

	return false;
}

Process::ThreadId Process::get_thread_id() const {
	return m_thread_id;
}

void Process::set_thread_id(ThreadId id) {
	m_thread_id = id;
}

thread *Process::get_thread_handle() const {
	return m_thread_handle;
}

void Process::set_thread_handle(thread *handle) {
	m_thread_handle = handle;
}

bool Process::is_endless() const {
	return m_endless;
}

Cursor *Process::cursor() {
	return m_cursor;
}

void Process::set_endless(bool endless) {
	m_endless = endless;
}

void Process::dump() {
	mint::printf(stderr, "Traceback thread %d : \n", m_thread_id);
	for (const LineInfo &call : m_cursor->dump()) {
		string call_str = call.to_string();
		string line_str = get_module_line(call.module_name(), call.line_number());
		mint::printf(stderr, "  %s\n", call_str.c_str());
		mint::printf(stderr, "  %s\n", line_str.c_str());
	}
}
