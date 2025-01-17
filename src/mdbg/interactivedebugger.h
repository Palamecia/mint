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

class InteractiveDebugger : public DebuggerBackend {
public:
	InteractiveDebugger() = default;
	InteractiveDebugger(const InteractiveDebugger &) = delete;
	InteractiveDebugger(InteractiveDebugger &&) = delete;
	~InteractiveDebugger();

	InteractiveDebugger &operator=(const InteractiveDebugger &) = delete;
	InteractiveDebugger &operator=(InteractiveDebugger &&) = delete;

	bool setup(Debugger *debugger, mint::Scheduler *scheduler) override;
	bool handle_events(Debugger *debugger, mint::CursorDebugger *cursor) override;
	bool check(Debugger *debugger, mint::CursorDebugger *cursor) override;
	void cleanup(Debugger *debugger, mint::Scheduler *scheduler) override;

	void on_thread_started(Debugger *debugger, mint::CursorDebugger *cursor) override;
	void on_thread_exited(Debugger *debugger, mint::CursorDebugger *cursor) override;

	void on_breakpoint_created(Debugger *debugger, const mint::Breakpoint &breakpoint) override;
	void on_breakpoint_deleted(Debugger *debugger, const mint::Breakpoint &breakpoint) override;

	void on_module_loaded(Debugger *debugger, mint::CursorDebugger *cursor, mint::Module *module) override;

	bool on_breakpoint(Debugger *debugger, mint::CursorDebugger *cursor,
					   const std::unordered_set<mint::Breakpoint::Id> &breakpoints) override;
	bool on_exception(Debugger *debugger, mint::CursorDebugger *cursor) override;
	bool on_pause(Debugger *debugger, mint::CursorDebugger *cursor) override;
	bool on_step(Debugger *debugger, mint::CursorDebugger *cursor) override;

	void on_exit(Debugger *debugger, int code) override;
	void on_error(Debugger *debugger) override;

protected:
	bool on_continue(Debugger *debugger, mint::CursorDebugger *cursor, std::istringstream &stream);
	bool on_next(Debugger *debugger, mint::CursorDebugger *cursor, std::istringstream &stream);
	bool on_enter(Debugger *debugger, mint::CursorDebugger *cursor, std::istringstream &stream);
	bool on_return(Debugger *debugger, mint::CursorDebugger *cursor, std::istringstream &stream);
	bool on_thread(Debugger *debugger, mint::CursorDebugger *cursor, std::istringstream &stream);
	bool on_backtrace(Debugger *debugger, mint::CursorDebugger *cursor, std::istringstream &stream);
	bool on_breakpoint(Debugger *debugger, mint::CursorDebugger *cursor, std::istringstream &stream);
	bool on_print(Debugger *debugger, mint::CursorDebugger *cursor, std::istringstream &stream);
	bool on_list(Debugger *debugger, mint::CursorDebugger *cursor, std::istringstream &stream);
	bool on_show(Debugger *debugger, mint::CursorDebugger *cursor, std::istringstream &stream);
	bool on_eval(Debugger *debugger, mint::CursorDebugger *cursor, std::istringstream &stream);
	bool on_quit(Debugger *debugger, mint::CursorDebugger *cursor, std::istringstream &stream);

	void print_commands();

private:
	struct Command {
		std::vector<std::string> names;
		std::string desc;
		bool (InteractiveDebugger::*func)(Debugger *, mint::CursorDebugger *, std::istringstream &);
	};

	static std::vector<Command> g_commands;

	bool call_command(const std::string &command, Debugger *debugger, mint::CursorDebugger *cursor,
					  std::istringstream &stream);

	mint::Terminal m_terminal;
};

#endif // MDBG_INTERACTIVEDEBUGGER_H
