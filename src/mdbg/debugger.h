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

#ifndef MDBG_DEBUGGER_H
#define MDBG_DEBUGGER_H

#include <mint/debug/debuginterface.h>
#include <mint/scheduler/scheduler.h>

class DebuggerBackend;

class Debugger : public mint::DebugInterface {
public:
	Debugger(int argc, char **argv);
	~Debugger();

	void add_pending_breakpoint(const std::string &file_path, size_t line_number);
	void pause_on_next_step();

	int run();

protected:
	bool parse_arguments(int argc, char **argv, std::vector<char *> &args);

	void print_version();
	void print_help();

	bool handle_events(mint::CursorDebugger *cursor) override;
	bool check(mint::CursorDebugger *cursor) override;

	void on_thread_started(mint::CursorDebugger *cursor) override;
	void on_thread_exited(mint::CursorDebugger *cursor) override;

	void on_breakpoint_created(const mint::Breakpoint &breakpoint) override;
	void on_breakpoint_deleted(const mint::Breakpoint &breakpoint) override;

	bool on_breakpoint(mint::CursorDebugger *cursor, const std::unordered_set<mint::Breakpoint::Id> &breakpoints) override;
	bool on_exception(mint::CursorDebugger *cursor) override;
	bool on_step(mint::CursorDebugger *cursor) override;

private:
	std::vector<std::pair<std::string, size_t>> m_pending_breakpoints;
	bool m_pause_on_next_step = false;

	std::unique_ptr<DebuggerBackend> m_backend;
	std::unique_ptr<mint::Scheduler> m_scheduler;
};

#endif // MDBG_DEBUGGER_H
