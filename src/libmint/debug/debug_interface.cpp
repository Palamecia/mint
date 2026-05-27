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

#include "mint/debug/debug_interface.h"
#include "mint/debug/cursor_debugger.h"
#include "mint/debug/line_info.h"
#include "mint/debug/thread_context.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <memory>
#include <mutex>
#include <ranges>
#include <utility>

using namespace mint;

DebugThreadLocker::DebugThreadLocker(DebugInterface& handle, Process& process) :
    _handle(handle),
    _cursor(handle.declare_thread(process)) {}

DebugThreadLocker::~DebugThreadLocker() {
	_handle.get().remove_thread(_cursor);
}

bool DebugInterface::debug(CursorDebugger& cursor) {

	if (_running) {

		const std::unique_lock _(_runtime_mutex);

		if (!process_events(cursor)) {
			_running = false;
			return false;
		}

		const std::size_t line_number = cursor.line_number();
		const std::size_t call_depth = cursor.call_depth();

		auto& context = cursor.get_thread_context();
		if (context.line_number != line_number || context.call_depth != call_depth) {

			std::unique_lock<std::mutex> config_lock(_config_mutex);

			auto module = _breakpoints.position.find(cursor.module_id());
			if (module != _breakpoints.position.end()) {
				auto line = module->second.find(line_number);
				if (line != module->second.end()) {
					const auto& breakpoints = line->second;
					config_lock.unlock();
					if (on_breakpoint(cursor, breakpoints)) {
						context.state = ThreadContext::State::debugger_pause;
					}
					else {
						return false;
					}
				}
			}
		}

		switch (context.state) {
		case ThreadContext::State::debugger_run:
		case ThreadContext::State::debugger_pause:
			if (context.line_number != line_number || context.call_depth != call_depth) {
				context.line_number = line_number;
				context.call_depth = call_depth;
			}
			break;

		case ThreadContext::State::debugger_next:
			if (context.line_number != line_number && context.call_depth >= call_depth) {
				context.line_number = line_number;
				context.call_depth = call_depth;
				if (on_step(cursor)) {
					context.state = ThreadContext::State::debugger_pause;
				}
				else {
					return false;
				}
			}
			break;

		case ThreadContext::State::debugger_enter:
			if (context.line_number != line_number || context.call_depth < call_depth) {
				context.line_number = line_number;
				context.call_depth = call_depth;
				if (on_step(cursor)) {
					context.state = ThreadContext::State::debugger_pause;
				}
				else {
					return false;
				}
			}
			break;

		case ThreadContext::State::debugger_return:
			if (context.line_number != line_number && context.call_depth > call_depth) {
				context.line_number = line_number;
				context.call_depth = call_depth;
				if (on_step(cursor)) {
					context.state = ThreadContext::State::debugger_pause;
				}
				else {
					return false;
				}
			}
			break;
		}

		while (context.state == ThreadContext::State::debugger_pause) {
			if (!check(cursor)) {
				_running = false;
				return false;
			}
		}

		return true;
	}

	const std::unique_lock _(_runtime_mutex);

	if (_exiting == &cursor) {

		ThreadContext& context = cursor.get_thread_context();

		if (on_exception(cursor)) {
			context.state = ThreadContext::State::debugger_pause;
		}
		else {
			return false;
		}

		while (context.state == ThreadContext::State::debugger_pause) {
			if (!check(cursor)) {
				return false;
			}
		}
	}

	return false;
}

void DebugInterface::exit(CursorDebugger& cursor) {
	_exiting = &cursor;
	_running = false;
}

void DebugInterface::do_run(CursorDebugger& cursor) {
	cursor.get_thread_context().state = ThreadContext::State::debugger_run;
}

void DebugInterface::do_pause(CursorDebugger& cursor) {
	cursor.get_thread_context().state = ThreadContext::State::debugger_pause;
}

void DebugInterface::do_next(CursorDebugger& cursor) {
	cursor.get_thread_context().state = ThreadContext::State::debugger_next;
}

void DebugInterface::do_enter(CursorDebugger& cursor) {
	cursor.get_thread_context().state = ThreadContext::State::debugger_enter;
}

void DebugInterface::do_return(CursorDebugger& cursor) {
	cursor.get_thread_context().state = ThreadContext::State::debugger_return;
}

ThreadList DebugInterface::get_threads() const {

	const std::unique_lock _(_config_mutex);

	ThreadList threads;
	std::ranges::transform(_threads, std::back_inserter(threads), [](auto& thread) {
		return std::ref(*thread.second);
	});
	return threads;
}

CursorDebugger* DebugInterface::find_thread(Process::ThreadId id) const {

	const std::unique_lock _(_config_mutex);

	auto it = _threads.find(id);
	if (it != _threads.end()) {
		return it->second.get();
	}
	return nullptr;
}

CursorDebugger& DebugInterface::declare_thread(const Process& thread) {

	const std::unique_lock _(_config_mutex);

	if (auto it = _threads.find(thread.get_thread_id()); it != _threads.end()) {
		it->second->update_cursor(thread.cursor());
		return *it->second;
	}

	const auto context = ThreadContext {
	    .state = ThreadContext::State::debugger_run,
	    .line_number = 0,
	    .call_depth = 0,
	    .thread_id = thread.get_thread_id(),
	};

	auto cursor = std::make_unique<CursorDebugger>(thread.cursor(), context);
	auto it = _threads.emplace(context.thread_id, std::move(cursor)).first;
	on_thread_started(*it->second);
	return *it->second;
}

void DebugInterface::remove_thread(const CursorDebugger& cursor) {

	const std::unique_lock _(_config_mutex);

	if (auto it = _threads.find(cursor.get_thread_id()); it != _threads.end()) {
		assert(&it->second->cursor() == &cursor.cursor());
		if (!it->second->close_cursor()) {
			on_thread_exited(*it->second);
			_threads.erase(it);
		}
	}
}

BreakpointList DebugInterface::get_breakpoints() const {

	const std::unique_lock _(_config_mutex);

	return {std::from_range, std::views::values(_breakpoints.list)};
}

Breakpoint DebugInterface::get_breakpoint(Breakpoint::Id id) const {

	const std::unique_lock _(_config_mutex);

	auto i = _breakpoints.list.find(id);
	if (i != _breakpoints.list.end()) {
		return i->second;
	}
	return {};
}

Breakpoint::Id DebugInterface::create_breakpoint(const LineInfo& info) {

	const std::unique_lock _(_config_mutex);

	assert(info.module_id() != Module::invalid_id);

	const auto id = next_breakpoint_id();
	_breakpoints.position[info.module_id()][info.line_number()].emplace(id);
	const Breakpoint& breakpoint = _breakpoints.list.emplace(id, Breakpoint {id, info}).first->second;
	on_breakpoint_created(breakpoint);
	return id;
}

void DebugInterface::remove_breakpoint(const LineInfo& info) {

	const std::unique_lock _(_config_mutex);

	auto i = _breakpoints.position.find(info.module_id());
	if (i != _breakpoints.position.end()) {
		auto j = i->second.find(info.line_number());
		if (j != i->second.end()) {
			for (const auto id : j->second) {
				auto k = _breakpoints.list.find(id);
				on_breakpoint_deleted(k->second);
				_breakpoints.list.erase(k);
			}
			i->second.erase(j);
			if (i->second.empty()) {
				_breakpoints.position.erase(i);
			}
		}
	}
}

void DebugInterface::remove_breakpoint(Breakpoint::Id id) {

	const std::unique_lock _(_config_mutex);

	auto i = _breakpoints.list.find(id);
	if (i != _breakpoints.list.end()) {
		auto j = _breakpoints.position.find(i->second.info.module_id());
		if (j != _breakpoints.position.end()) {
			auto k = j->second.find(id);
			if (k != j->second.end()) {
				on_breakpoint_deleted(i->second);
				k->second.erase(id);
				if (k->second.empty()) {
					j->second.erase(k);
				}
			}
			if (j->second.empty()) {
				_breakpoints.position.erase(j);
			}
		}
		_breakpoints.list.erase(i);
	}
}

void DebugInterface::clear_breakpoints() {
	const std::unique_lock _(_config_mutex);
	for (auto& [id, breakpoint] : _breakpoints.list) {
		on_breakpoint_deleted(breakpoint);
	}
	_breakpoints.position.clear();
	_breakpoints.list.clear();
}

Breakpoint::Id DebugInterface::next_breakpoint_id() const {
	Breakpoint::Id id = 0;
	while (_breakpoints.list.contains(id)) {
		++id;
	}
	return id;
}
