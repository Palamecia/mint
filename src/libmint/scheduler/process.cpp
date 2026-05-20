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

#include "mint/scheduler/process.h"
#include "mint/ast/abstractsyntaxtree.h"
#include "mint/ast/cursor.h"
#include "mint/ast/module.h"
#include "mint/compiler/compiler.h"
#include "mint/debug/debuginterface.h"
#include "mint/debug/debugtool.h"
#include "mint/debug/lineinfo.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/memory/functiontool.h"
#include "mint/memory/reference.h"
#include "mint/scheduler/exception.h"
#include "mint/scheduler/inputstream.h"
#include "mint/scheduler/output.h"
#include "mint/scheduler/processor.h"
#include "mint/scheduler/scheduler.h"
#include "mint/system/bufferstream.h"
#include "mint/system/error.h"
#include "mint/system/filestream.h"
#include "mint/system/filesystem.h"
#include "mint/system/mintruntimeerror.h"
#include "mint/system/terminal.h"

#include "bracematcher.h"
#include "completer.h"
#include "highlighter.h"

#include <cstdio>
#include <filesystem>
#include <memory>
#include <optional>
#include <print>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

using namespace mint;

Process::Process(std::unique_ptr<Cursor>&& cursor) :
    _cursor(std::move(cursor)) {}

std::unique_ptr<Process> Process::from_main_file(Scheduler& scheduler, const std::filesystem::path& file) {

	try {

		const std::filesystem::path module_file_path = is_module_file(file)
		                                                   ? file
		                                                   : FileSystem::instance().get_script_path(file);

		auto& ast = scheduler.ast();
		auto compiler = Compiler(ast);
		auto stream = FileStream(module_file_path);

		if (stream.is_valid()) {
			if (const auto info = ast.create_main_module(Module::State::ready); compiler.build(stream, info)) {
				FileSystem::instance().set_main_module_path(module_file_path);
				return std::make_unique<Process>(std::make_unique<Cursor>(ast, *info.bytecode));
			}
		}
	}
	catch (const MintRuntimeError&) {
		return nullptr;
	}

	return nullptr;
}

std::unique_ptr<Process> Process::from_file(Scheduler& scheduler, const std::filesystem::path& file) {

	try {

		auto& ast = scheduler.ast();
		auto compiler = Compiler(ast);
		auto stream = FileStream(file);

		if (stream.is_valid()) {
			if (const auto info = ast.create_module_from_file_path(file, Module::State::ready);
			    compiler.build(stream, info)) {
				return std::make_unique<Process>(std::make_unique<Cursor>(ast, *info.bytecode));
			}
		}
	}
	catch (const MintRuntimeError&) {
		return nullptr;
	}

	return nullptr;
}

std::unique_ptr<Process> Process::from_buffer(Scheduler& scheduler, const std::string& buffer) {

	try {
		auto& ast = scheduler.ast();
		auto compiler = Compiler(ast);
		auto stream = BufferStream(buffer);

		if (stream.is_valid()) {
			if (const auto info = ast.create_module(Module::State::ready); compiler.build(stream, info)) {
				return std::make_unique<Process>(std::make_unique<Cursor>(ast, *info.bytecode));
			}
		}
	}
	catch (const MintRuntimeError&) {
		return nullptr;
	}

	return nullptr;
}

std::unique_ptr<Process> Process::from_standard_input(Scheduler& scheduler) {

	if (InputStream::instance().is_valid()) {

		AbstractSyntaxTree& ast = scheduler.ast();
		const auto info = ast.create_main_module(Module::State::ready);
		auto process = std::make_unique<Process>(std::make_unique<Cursor>(ast, *info.bytecode));
		process->cursor().open_printer(std::make_unique<Output>(ast));
		process->set_endless(true);

		InputStream::instance().set_highlighter([&ast](std::string_view input, std::string_view::size_type offset) {
			auto highlighter = Highlighter(ast, offset);
			auto stream = std::stringstream(std::string {input});
			if (highlighter.parse(stream)) {
				return highlighter.output();
			}
			return std::string {input};
		});

		InputStream::instance().set_completion_generator(
		    [&cursor = process->cursor()](std::string_view input,
		        std::string_view::size_type offset) -> std::optional<std::vector<Completion>> {
			    if (offset == 0) {
				    return std::nullopt;
			    }
			    for (auto i = offset; i != 0 && input[i - 1] != '\n'; --i) {
				    if (input[i - 1] != ' ') {
					    auto completer = Completer(offset, cursor);
					    auto stream = std::stringstream(std::string {input});
					    completer.parse(stream);
					    return completer.completions();
				    }
			    }
			    return std::nullopt;
		    });

		InputStream::instance().set_brace_matcher(
		    [](std::string_view input,
		        std::string_view::size_type offset) -> std::pair<std::string_view::size_type, bool> {
			    auto matcher = BraceMatcher(offset);
			    auto stream = std::stringstream(std::string {input});
			    matcher.parse(stream);
			    return matcher.match();
		    });

		return process;
	}

	return nullptr;
}

void Process::parse_argument(const std::string& arg) {
	auto args = _cursor->symbols().find("va_args");
	if (args == _cursor->symbols().end()) {
		auto va_args = make_reference<Iterator>(Reference::default_flags, _cursor->ast());
		va_args.data<Iterator>().construct();
		args = _cursor->symbols().emplace("va_args", std::move(va_args)).first;
	}
	iterator_yield(*_cursor, args->second.data<Iterator>(), create_string(_cursor->ast(), arg));
}

void Process::setup() {
	if (!_cursor->parent()) {
		_error_handler = add_error_callback([this](const std::string&) {
			dump();
		});
	}
}

void Process::cleanup() {
	if (_error_handler) {
		remove_error_callback(_error_handler);
	}

	auto _ = ProcessorLocker();
	_cursor->cleanup();
}

ProcessStatus Process::exec() {

	auto _ = ProcessorLocker();

	try {
		return run_steps(*_cursor);
	}
	catch (MintException& raised) {
		if (_cursor.get() == &raised.cursor()) {
			_cursor->raise(raised.take_exception());
			return ProcessStatus::paused;
		}
		throw;
	}
	catch (const MintRuntimeError&) {
		return ProcessStatus::failed;
	}
}

bool Process::resume() {

	while (_endless) {
		try {
			auto compiler = Compiler(_cursor->ast());
			compiler.set_printing(true);
			_cursor->resume();
			InputStream::instance().next();
			return compiler.build(InputStream::instance(), _cursor->ast().main());
		}
		catch (const MintRuntimeError&) {
			continue;
		}
	}

	return false;
}

Process::ThreadId Process::get_thread_id() const {
	return _thread_id;
}

void Process::set_thread_id(ThreadId id) {
	_thread_id = id;
}

std::thread* Process::get_thread_handle() const {
	return _thread_handle.get();
}

std::unique_ptr<std::thread> Process::release_thread_handle() {
	return std::move(_thread_handle);
}

void Process::set_thread_handle(std::unique_ptr<std::thread>&& handle) {
	_thread_handle = std::move(handle);
}

bool Process::is_endless() const {
	return _endless;
}

bool Process::is_thread() const {
	return _thread_id != 0 && _cursor->is_thread();
}

Cursor& Process::cursor() const {
	return *_cursor;
}

void Process::set_endless(bool endless) {
	_endless = endless;
}

void Process::dump() {
	std::println(stderr, "Traceback thread {}:", _thread_id);
	for (const LineInfo& call : _cursor->dump()) {
		std::println(stderr, "  {}", call.to_string());
		std::println(stderr, "  {}", get_module_line(call.module_name(), call.line_number()));
	}
}
