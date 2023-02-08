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

int Debugger::run() {

	if (!m_scheduler) {
		return EXIT_FAILURE;
	}

	while (!m_configured_breakpoints.empty()) {
		auto [module, line] = m_configured_breakpoints.front();
		create_breakpoint(LineInfo(m_scheduler->ast(), module, line));
		m_configured_breakpoints.pop();
	}

	if (!m_backend->setup(this, m_scheduler.get())) {
		return EXIT_FAILURE;
	}

	int code = m_scheduler->run();
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
					string module = argv[argn];
					if (++argn < argc) {
						size_t line = static_cast<size_t>(atol(argv[argn]));
						m_configured_breakpoints.push({module, line});
						continue;
					}
				}
				return false;
			}
			if (!strcmp(argv[argn], "--stdio")) {
				m_backend.reset(new DapDebugger(new DapStreamReader, new DapStreamWriter));
				continue;
			}
			if (!strcmp(argv[argn], "--version")) {
				printVersion();
				return false;
			}
			if (!strcmp(argv[argn], "--help")) {
				printHelp();
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

void Debugger::printVersion() {
	Terminal::print(stdout, "mdbg " MINT_MACRO_TO_STR(MINT_VERSION) "\n");
}

void Debugger::printHelp() {
	Terminal::print(stdout, "Usage : mdbg [option] [file [args]] [-- args]\n");
	Terminal::print(stdout, "Options :\n");
	Terminal::print(stdout, "  --help            : Print this help message and exit\n");
	Terminal::print(stdout, "  --version         : Print mint version and exit\n");
	Terminal::print(stdout, "  -b, --breakpoint 'module' 'line'\n");
	Terminal::print(stdout, "                    : Creates a new breakpoint for the given module at the given line\n");
	Terminal::print(stdout, "  --stdio           : Starts the debug using the Debug Adapter Protocol over stdio\n");
}

bool Debugger::handle_events(CursorDebugger *cursor) {
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

bool Debugger::on_breakpoint(CursorDebugger *cursor, const unordered_set<Breakpoint::Id> &breakpoints) {
	return m_backend->on_breakpoint(this, cursor, breakpoints);
}

bool Debugger::on_exception(CursorDebugger *cursor) {
	return m_backend->on_exception(this, cursor);
}

bool Debugger::on_step(CursorDebugger *cursor) {
	return m_backend->on_step(this, cursor);
}
