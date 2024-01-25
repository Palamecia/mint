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

#include "debugger.h"
#include "interactivedebugger.h"
#include "dapdebugger.h"
#include "dapstream.h"
#include "mint/system/error.h"

#include <mint/debug/debugtool.h>
#include <mint/system/terminal.h>

using namespace std;
using namespace mint;

Debugger::Debugger(int argc, char **argv) {

	vector<char *> args;

	if (parse_arguments(argc, argv, args)) {
		m_scheduler.reset(new Scheduler(static_cast<int>(args.size()), args.data()));
		m_scheduler->set_debug_interface(this);
	}
}

Debugger::~Debugger() {

}

void Debugger::add_pending_breakpoint_from_file(const string &file_path, size_t line_number) {
	m_pending_breakpoints.push_back({pending_breakpoint_t::from_file_path, file_path, line_number});
}

void Debugger::add_pending_breakpoint_from_module(const string &module, size_t line_number) {
	m_pending_breakpoints.push_back({pending_breakpoint_t::from_module_path, module, line_number});
}

void Debugger::pause_on_next_step() {
	m_pause_on_next_step = true;
}

int Debugger::run() {

	if (!m_scheduler) {
		return EXIT_FAILURE;
	}

	set_exit_callback([&] {
		m_backend->on_error(this);
	});

	if (!m_backend->setup(this, m_scheduler.get())) {
		return EXIT_FAILURE;
	}

	int code = m_scheduler->run();
	m_backend->on_exit(this, code);
	m_backend->cleanup(this, m_scheduler.get());
	return code;
}

bool Debugger::parse_arguments(int argc, char **argv, vector<char *> &args) {

	bool configuring = true;
	args.push_back(argv[0]);

	for (int argn = 1; argn < argc; ++argn) {
		if (configuring) {
			if (!strcmp(argv[argn], "-b") || !strcmp(argv[argn], "--breakpoint")) {
				if (++argn < argc) {
					const string module = argv[argn];
					if (++argn < argc) {
						const size_t line_number = static_cast<size_t>(atol(argv[argn]));
						add_pending_breakpoint_from_module(module, line_number);
						continue;
					}
				}
				return false;
			}
			if (!strcmp(argv[argn], "--wait")) {
				m_pause_on_next_step = true;
				continue;
			}
			if (!strcmp(argv[argn], "--stdio")) {
				m_backend.reset(new DapDebugger(new DapStreamReader, new DapStreamWriter));
				continue;
			}
			if (!strcmp(argv[argn], "--version")) {
				print_version();
				return false;
			}
			if (!strcmp(argv[argn], "--help")) {
				print_help();
				return false;
			}
			if (!strcmp(argv[argn], "--")) {
				configuring = false;
				continue;
			}
		}

		args.push_back(argv[argn]);
	}

	if (!m_backend) {
		m_backend.reset(new InteractiveDebugger);
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

bool Debugger::handle_events(CursorDebugger *cursor) {

	for (auto it = m_pending_breakpoints.begin(); it != m_pending_breakpoints.end();) {
		pending_breakpoint_t &breakpoint = *it;
		const string module = breakpoint.type == pending_breakpoint_t::from_file_path
								  ? to_module_path(breakpoint.module)
								  : breakpoint.module;
		Module::Info info = Scheduler::instance()->ast()->module_info(module);
		if (DebugInfo *debug_info = info.debug_info; debug_info && info.state != Module::not_compiled) {
			create_breakpoint({info.id, module, debug_info->to_executable_line_number(breakpoint.line_number)});
			it = m_pending_breakpoints.erase(it);
		}
		else {
			++it;
		}
	}

	if (m_pause_on_next_step) {
		m_pause_on_next_step = false;
		do_pause(cursor);
		if (!m_backend->on_pause(this, cursor)) {
			return false;
		}
	}

	return m_backend->handle_events(this, cursor);
}

bool Debugger::check(CursorDebugger *cursor) {
	return m_backend->check(this, cursor);
}

void Debugger::on_thread_started(CursorDebugger *cursor) {
	m_backend->on_thread_started(this, cursor);
}

void Debugger::on_thread_exited(CursorDebugger *cursor) {
	m_backend->on_thread_exited(this, cursor);
}

void Debugger::on_breakpoint_created(const Breakpoint &breakpoint) {
	m_backend->on_breakpoint_created(this, breakpoint);
}

void Debugger::on_breakpoint_deleted(const Breakpoint &breakpoint) {
	m_backend->on_breakpoint_deleted(this, breakpoint);
}

bool Debugger::on_breakpoint(CursorDebugger *cursor, const unordered_set<Breakpoint::Id> &breakpoints) {
	return m_backend->on_breakpoint(this, cursor, breakpoints);
}

bool Debugger::on_exception(CursorDebugger *cursor) {
	return m_backend->on_exception(this, cursor);
}

bool Debugger::on_step(CursorDebugger *cursor) {
	return m_backend->on_step(this, cursor);
}
