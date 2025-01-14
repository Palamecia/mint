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

#ifndef MDBG_DEBUGGERBACKEND_H
#define MDBG_DEBUGGERBACKEND_H

#include <mint/debug/debuginterface.h>
#include <mint/debug/cursordebugger.h>
#include <mint/scheduler/scheduler.h>

class Debugger;

class DebuggerBackend {
public:
	DebuggerBackend();
	virtual ~DebuggerBackend();

	virtual bool setup(Debugger *debugger, mint::Scheduler *scheduler) = 0;
	virtual bool handle_events(Debugger *debugger, mint::CursorDebugger *cursor) = 0;
	virtual bool check(Debugger *debugger, mint::CursorDebugger *cursor) = 0;
	virtual void cleanup(Debugger *debugger, mint::Scheduler *scheduler) = 0;

	virtual void on_thread_started(Debugger *debugger, mint::CursorDebugger *cursor) = 0;
	virtual void on_thread_exited(Debugger *debugger, mint::CursorDebugger *cursor) = 0;

	virtual void on_breakpoint_created(Debugger *debugger, const mint::Breakpoint &breakpoint) = 0;
	virtual void on_breakpoint_deleted(Debugger *debugger, const mint::Breakpoint &breakpoint) = 0;

	virtual void on_module_loaded(Debugger *debugger, mint::CursorDebugger *cursor, mint::Module *module) = 0;

	virtual bool on_breakpoint(Debugger *debugger, mint::CursorDebugger *cursor,
							   const std::unordered_set<mint::Breakpoint::Id> &breakpoints) = 0;
	virtual bool on_exception(Debugger *debugger, mint::CursorDebugger *cursor) = 0;
	virtual bool on_pause(Debugger *debugger, mint::CursorDebugger *cursor) = 0;
	virtual bool on_step(Debugger *debugger, mint::CursorDebugger *cursor) = 0;

	virtual void on_exit(Debugger *debugger, int code) = 0;
	virtual void on_error(Debugger *debugger) = 0;
};

#endif // MDBG_DEBUGGERBACKEND_H
