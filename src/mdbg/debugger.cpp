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

#include "debugger.h"
#include "interactive_debugger.h"
#include "dap_debugger.h"
#include "dap_stream.h"
#include "mint/config.h"
#include "mint/debug/cursor_debugger.h"
#include "mint/debug/debug_info.h"
#include "mint/debug/debug_interface.h"
#include "mint/scheduler/scheduler.h"
#include "mint/system/error.h"

#include "mint/ast/module.h"
#include "mint/debug/debug_tools.h"
#include "mint/system/stdio.h"

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

Debugger::Debugger(const std::vector<std::string>& args) {

	std::vector<std::string> process_args;

	if (parse_arguments(args, process_args)) {
		_scheduler = std::make_unique<mint::Scheduler>(process_args);
		_scheduler->set_debug_interface(this);
	}
}

Debugger::~Debugger() {}

void Debugger::add_pending_breakpoint_from_file(const std::string& file_path, std::size_t line_number) {
	_pending_breakpoints.push_back({
	    .type = PendingBreakpoint::From::file_path,
	    .module = file_path,
	    .line_number = line_number,
	});
}

void Debugger::add_pending_breakpoint_from_module(const std::string& module, std::size_t line_number) {
	_pending_breakpoints.push_back({
	    .type = PendingBreakpoint::From::module_path,
	    .module = module,
	    .line_number = line_number,
	});
}

void Debugger::pause_on_next_step() {
	_pause_on_next_step = true;
}

int Debugger::run() {

	if (!_scheduler) {
		return EXIT_FAILURE;
	}

	if (!_backend->setup(*this, *_scheduler)) {
		return EXIT_FAILURE;
	}

	_scheduler->add_exit_callback([&](int code) {
		_backend->on_exit(*this, code);
	});

	const int code = _scheduler->run();
	_backend->cleanup(*this, *_scheduler);
	return code;
}

bool Debugger::parse_arguments(const std::vector<std::string>& args, std::vector<std::string>& process_args) {

	bool configuring = true;

	for (auto it = args.begin(); it != args.end(); ++it) {
		if (configuring) {
			if (*it == "-b" || *it == "--breakpoint") {
				if (++it != args.end()) {
					const std::string module = *it;
					if (++it != args.end()) {
						const auto line_number = static_cast<std::size_t>(std::stol(*it));
						add_pending_breakpoint_from_module(module, line_number);
						continue;
					}
				}
				return false;
			}
			if (*it == "--wait") {
				_pause_on_next_step = true;
				continue;
			}
			if (*it == "--stdio") {
				_backend = std::make_unique<DapDebugger>(std::make_unique<DapStreamReader>(),
				    std::make_unique<DapStreamWriter>());
				continue;
			}
			if (*it == "--version") {
				print_version();
				return false;
			}
			if (*it == "--help") {
				print_help();
				return false;
			}
			if (*it == "--") {
				configuring = false;
				continue;
			}
		}

		process_args.push_back(*it);
	}

	if (!_backend) {
		_backend = std::make_unique<InteractiveDebugger>();
	}

	return true;
}

void Debugger::print_version() {
	mint::print(stdout, "mdbg " MINT_MACRO_TO_STR(MINT_VERSION) "\n");
}

void Debugger::print_help() {
	mint::print(stdout, "Usage : mdbg [option] [file [args]] [-- args]\n");
	mint::print(stdout, "Options :\n");
	mint::print(stdout, "  --help            : Print this help message and exit\n");
	mint::print(stdout, "  --version         : Print mint version and exit\n");
	mint::print(stdout, "  -b, --breakpoint 'module' 'line'\n");
	mint::print(stdout, "                    : Creates a new breakpoint for the given module at the given line\n");
	mint::print(stdout, "  --wait            : Wait before the first instruction\n");
	mint::print(stdout, "  --stdio           : Starts the debug using the Debug Adapter Protocol over stdio\n");
}

bool Debugger::process_events(mint::CursorDebugger& cursor) {

	mint::AbstractSyntaxTree& ast = cursor.cursor().ast();

	while (const auto* module = ast.find_module(_module_count)) {
		_backend->on_module_loaded(*this, cursor, *module);
		++_module_count;
	}

	for (auto it = _pending_breakpoints.begin(); it != _pending_breakpoints.end();) {
		const PendingBreakpoint& breakpoint = *it;
		const std::string module_name = breakpoint.type == PendingBreakpoint::From::file_path
		                                    ? mint::to_module_path(breakpoint.module)
		                                    : breakpoint.module;
		const auto& module = ast.module_info(module_name);
		if (module.state != mint::Module::State::not_compiled) {
			create_breakpoint(
			    {module.id, module_name, module.debug_info.to_executable_line_number(breakpoint.line_number)});
			it = _pending_breakpoints.erase(it);
		}
		else {
			++it;
		}
	}

	if (_pause_on_next_step) {
		_pause_on_next_step = false;
		do_pause(cursor);
		if (!_backend->on_pause(*this, cursor)) {
			return false;
		}
	}

	return _backend->process_events(*this, cursor);
}

bool Debugger::check(mint::CursorDebugger& cursor) {
	return _backend->check(*this, cursor);
}

void Debugger::on_thread_started(mint::CursorDebugger& cursor) {
	_backend->on_thread_started(*this, cursor);
}

void Debugger::on_thread_exited(mint::CursorDebugger& cursor) {
	_backend->on_thread_exited(*this, cursor);
}

void Debugger::on_breakpoint_created(const mint::Breakpoint& breakpoint) {
	_backend->on_breakpoint_created(*this, breakpoint);
}

void Debugger::on_breakpoint_deleted(const mint::Breakpoint& breakpoint) {
	_backend->on_breakpoint_deleted(*this, breakpoint);
}

bool Debugger::on_breakpoint(mint::CursorDebugger& cursor, const std::unordered_set<mint::Breakpoint::Id>& breakpoints) {
	return _backend->on_breakpoint(*this, cursor, breakpoints);
}

bool Debugger::on_exception(mint::CursorDebugger& cursor) {
	return _backend->on_exception(*this, cursor);
}

bool Debugger::on_error(mint::CursorDebugger& /*cursor*/) {
	_backend->on_error(*this);
	return true;
}

bool Debugger::on_step(mint::CursorDebugger& cursor) {
	return _backend->on_step(*this, cursor);
}
