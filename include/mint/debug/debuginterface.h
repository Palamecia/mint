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

#ifndef MINT_DEBUGINTERFACE_H
#define MINT_DEBUGINTERFACE_H

#include "mint/scheduler/process.h"
#include "mint/debug/lineinfo.h"

#include <unordered_map>
#include <unordered_set>
#include <atomic>
#include <mutex>

namespace mint {

class Cursor;
class Process;
class CursorDebugger;

struct MINT_EXPORT Breakpoint {

	using Id = size_t;

	static constexpr const Id INVALID_ID = static_cast<size_t>(-1);

	Id id = INVALID_ID;
	LineInfo info;
};

using ThreadList = std::vector<CursorDebugger *>;
using BreakpointList = std::vector<Breakpoint>;

class MINT_EXPORT DebugInterface {
public:
	DebugInterface();
	virtual ~DebugInterface();

	bool debug(CursorDebugger *cursor);
	void exit(CursorDebugger *cursor);

	void do_run(CursorDebugger *cursor);
	void do_pause(CursorDebugger *cursor);
	void do_next(CursorDebugger *cursor);
	void do_enter(CursorDebugger *cursor);
	void do_return(CursorDebugger *cursor);

	ThreadList get_threads() const;
	CursorDebugger *get_thread(Process::ThreadId id) const;
	CursorDebugger *declare_thread(const Process *thread);
	void remove_thread(const Process *thread);

	BreakpointList get_breakpoints() const;
	Breakpoint get_breakpoint(Breakpoint::Id id) const;
	Breakpoint::Id create_breakpoint(const LineInfo &info);
	void remove_breakpoint(const LineInfo &info);
	void remove_breakpoint(Breakpoint::Id id);
	void clear_breakpoints();

protected:
	virtual bool handle_events(CursorDebugger *cursor) = 0;
	virtual bool check(CursorDebugger *cursor) = 0;

	virtual void on_thread_started(CursorDebugger *cursor) = 0;
	virtual void on_thread_exited(CursorDebugger *cursor) = 0;

	virtual void on_breakpoint_created(const Breakpoint &breakpoint) = 0;
	virtual void on_breakpoint_deleted(const Breakpoint &breakpoint) = 0;

	virtual bool on_breakpoint(CursorDebugger *cursor, const std::unordered_set<Breakpoint::Id> &breakpoints) = 0;
	virtual bool on_exception(CursorDebugger *cursor) = 0;
	virtual bool on_step(CursorDebugger *cursor) = 0;

private:
	Breakpoint::Id next_breakpoint_id() const;

	std::recursive_mutex m_runtime_mutex;
	std::atomic_bool m_running = { true };
	CursorDebugger *m_exiting = nullptr;

	mutable std::mutex m_config_mutex;
	std::unordered_map<Process::ThreadId, CursorDebugger *> m_threads;
	struct {
		std::unordered_map<Breakpoint::Id, Breakpoint> list;
		std::unordered_map<Module::Id, std::unordered_map<size_t, std::unordered_set<Breakpoint::Id>>> position;
	} m_breakpoints;
};

}

#endif // MINT_DEBUGINTERFACE_H
