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

#ifndef MDBG_INTERACTIVEDEBUGGER_H
#define MDBG_INTERACTIVEDEBUGGER_H

#include "debuggerbackend.h"

#include <mint/system/terminal.h>
#include <functional>

class InteractiveDebugger : public DebuggerBackend {
public:
	InteractiveDebugger();
	~InteractiveDebugger();

	bool setup(mint::DebugInterface *debugger, mint::Scheduler *scheduler) override;
	bool handle_events(mint::DebugInterface *debugger, mint::CursorDebugger *cursor) override;
	bool check(mint::DebugInterface *debugger, mint::CursorDebugger *cursor) override;
	void cleanup(mint::DebugInterface *debugger, mint::Scheduler *scheduler) override;

	void on_thread_started(mint::DebugInterface *debugger, mint::CursorDebugger *cursor) override;
	void on_thread_exited(mint::DebugInterface *debugger, mint::CursorDebugger *cursor) override;

	bool on_breakpoint(mint::DebugInterface *debugger, mint::CursorDebugger *cursor, const std::unordered_set<mint::Breakpoint::Id> &breakpoints) override;
	bool on_exception(mint::DebugInterface *debugger, mint::CursorDebugger *cursor) override;
	bool on_step(mint::DebugInterface *debugger, mint::CursorDebugger *cursor) override;

protected:
	void print_commands();
	bool run_command(const std::string &command, mint::DebugInterface *debugger, mint::CursorDebugger *cursor, std::istringstream &stream);

private:
	struct Command {
		std::vector<std::string> names;
		std::string desc;
		std::function<bool(mint::DebugInterface *, mint::CursorDebugger *, std::istringstream &)> func;
	};

	std::vector<Command> m_commands;
	mint::Terminal m_terminal;
};

#endif // MDBG_INTERACTIVEDEBUGGER_H
