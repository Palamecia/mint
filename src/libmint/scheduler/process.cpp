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
#include "mint/ast/abstract_syntax_tree.h"
#include "mint/ast/cursor.h"
#include "mint/ast/exception.h"
#include "mint/ast/module.h"
#include "mint/compiler/compiler.h"
#include "mint/debug/debug_interface.h"
#include "mint/debug/debug_tools.h"
#include "mint/debug/line_info.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/memory/cast_tools.h"
#include "mint/memory/function_tools.h"
#include "mint/memory/reference.h"
#include "mint/scheduler/input_stream.h"
#include "mint/scheduler/output.h"
#include "mint/scheduler/processor.h"
#include "mint/scheduler/scheduler.h"
#include "mint/system/buffer_stream.h"
#include "mint/system/error.h"
#include "mint/system/file_stream.h"
#include "mint/system/filesystem.h"
#include "mint/system/mint_runtime_error.h"
#include "mint/system/terminal.h"

#include "brace_matcher.h"
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

namespace {

class ReplProcess : public Process {
public:
	ReplProcess(Scheduler& scheduler, std::unique_ptr<Cursor>&& cursor) :
	    Process(scheduler, std::move(cursor)) {}

	void setup() override {
		Process::setup();
		read_next();
	}

	bool resume() override {
		cursor().retrieve();
		return read_next();
	}

private:
	bool read_next() {
		for (;;) {
			try {
				auto compiler = Compiler(cursor().ast());
				compiler.set_printing(true);
				cursor().resume();
				InputStream::instance().next();
				return compiler.build(InputStream::instance(), cursor().ast().main());
			}
			catch (const MintRuntimeError& error) {
				print_error(error.what());
				continue;
			}
		}
		return false;
	}
};

}

Process::Process(Scheduler& scheduler, std::unique_ptr<Cursor>&& cursor) :
    _scheduler(scheduler),
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
			if (auto& module = ast.create_main_module(Module::State::ready); compiler.build(stream, module)) {
				FileSystem::instance().set_main_module_path(module_file_path);
				return std::make_unique<Process>(scheduler, std::make_unique<Cursor>(ast, module.bytecode));
			}
		}
	}
	catch (const MintRuntimeError& error) {
		print_error(error.what());
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
			if (auto& module = ast.create_module_from_file_path(file, Module::State::ready);
			    compiler.build(stream, module)) {
				return std::make_unique<Process>(scheduler, std::make_unique<Cursor>(ast, module.bytecode));
			}
		}
	}
	catch (const MintRuntimeError& error) {
		print_error(error.what());
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
			if (auto& module = ast.create_module(Module::State::ready); compiler.build(stream, module)) {
				return std::make_unique<Process>(scheduler, std::make_unique<Cursor>(ast, module.bytecode));
			}
		}
	}
	catch (const MintRuntimeError& error) {
		print_error(error.what());
		return nullptr;
	}

	return nullptr;
}

std::unique_ptr<Process> Process::from_standard_input(Scheduler& scheduler) {

	if (InputStream::instance().is_valid()) {

		AbstractSyntaxTree& ast = scheduler.ast();
		auto& module = ast.create_main_module(Module::State::ready);
		auto process = std::make_unique<ReplProcess>(scheduler, std::make_unique<Cursor>(ast, module.bytecode));
		process->cursor().open_printer(std::make_unique<Output>(ast));

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
			on_error();
		});
	}
}

bool Process::resume() {
	return false;
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
			try {
				_cursor->raise(raised.take_exception());
				return ProcessStatus::paused;
			}
			catch (const MintRuntimeError& error) {
				print_error(error.what());
				return ProcessStatus::failed;
			}
		}
		throw;
	}
	catch (const MintRuntimeError& error) {
		print_error(error.what());
		return ProcessStatus::failed;
	}
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

bool Process::is_thread() const {
	return _thread_id != 0 && _cursor->is_thread();
}

Cursor& Process::cursor() const {
	return *_cursor;
}

Scheduler& Process::scheduler() const {
	return _scheduler;
}

namespace {

std::string format_exception(const Reference& exception) {
	try {
		return to_string(exception);
	}
	catch (...) {
		return "<exception formatting failed>";
	}
}

void dump_cause(const Cursor::Exception& cause, const AbstractSyntaxTree& ast) {
	if (cause.cause) {
		dump_cause(*cause.cause, ast);
	}
	std::println(stderr, "Caused by:");
	for (const LineInfo& call : cause.stacktrace) {
		std::println(stderr, "  {}", call.to_string(ast));
		std::println(stderr, "    {}", get_module_line(call.module_name(), call.line_number()));
	}
	std::println("{}", format_exception(cause.object));
	std::println();
}

}

void Process::on_error() {
	if (const auto* exception = _cursor->get_exception()) {
		if (exception->cause) {
			dump_cause(*exception->cause, _cursor->ast());
		}
		std::println(stderr, "Stacktrace thread {}:", _thread_id);
		for (const LineInfo& call : exception->stacktrace) {
			std::println(stderr, "  {}", call.to_string(_cursor->ast()));
			std::println(stderr, "    {}", get_module_line(call.module_name(), call.line_number()));
		}
	}
	else {
		std::println(stderr, "Stacktrace thread {}:", _thread_id);
		for (const LineInfo& call : _cursor->dump()) {
			std::println(stderr, "  {}", call.to_string(_cursor->ast()));
			std::println(stderr, "    {}", get_module_line(call.module_name(), call.line_number()));
		}
	}
}
