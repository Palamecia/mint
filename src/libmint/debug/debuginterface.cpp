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

#include "mint/debug/debuginterface.h"
#include "mint/debug/cursordebugger.h"
#include "threadcontext.hpp"

#include <algorithm>
#include <iterator>

using namespace mint;

bool DebugInterface::debug(CursorDebugger *cursor) {

	if (m_running) {

		std::unique_lock<std::recursive_mutex> lock(m_runtime_mutex);

		ThreadContext *context = cursor->get_thread_context();
		if (context == nullptr) {
			return false;
		}

		if (!handle_events(cursor)) {
			m_running = false;
			return false;
		}

		const size_t line_number = cursor->line_number();
		const size_t call_depth = cursor->call_depth();

		if (context->line_number != line_number || context->call_depth != call_depth) {

			std::unique_lock<std::mutex> config_lock(m_config_mutex);

			auto module = m_breakpoints.position.find(cursor->module_id());
			if (module != m_breakpoints.position.end()) {
				auto line = module->second.find(line_number);
				if (line != module->second.end()) {
					const auto &breakpoints = line->second;
					config_lock.unlock();
					if (on_breakpoint(cursor, breakpoints)) {
						context->state = ThreadContext::DEBUGGER_PAUSE;
					}
					else {
						return false;
					}
				}
			}
		}

		switch (context->state) {
		case ThreadContext::DEBUGGER_RUN:
		case ThreadContext::DEBUGGER_PAUSE:
			if (context->line_number != line_number || context->call_depth != call_depth) {
				context->line_number = line_number;
				context->call_depth = call_depth;
			}
			break;

		case ThreadContext::DEBUGGER_NEXT:
			if (context->line_number != line_number && context->call_depth >= call_depth) {
				context->line_number = line_number;
				context->call_depth = call_depth;
				if (on_step(cursor)) {
					context->state = ThreadContext::DEBUGGER_PAUSE;
				}
				else {
					return false;
				}
			}
			break;

		case ThreadContext::DEBUGGER_ENTER:
			if (context->line_number != line_number || context->call_depth < call_depth) {
				context->line_number = line_number;
				context->call_depth = call_depth;
				if (on_step(cursor)) {
					context->state = ThreadContext::DEBUGGER_PAUSE;
				}
				else {
					return false;
				}
			}
			break;

		case ThreadContext::DEBUGGER_RETURN:
			if (context->line_number != line_number && context->call_depth > call_depth) {
				context->line_number = line_number;
				context->call_depth = call_depth;
				if (on_step(cursor)) {
					context->state = ThreadContext::DEBUGGER_PAUSE;
				}
				else {
					return false;
				}
			}
			break;
		}

		while (context->state == ThreadContext::DEBUGGER_PAUSE) {
			if (!check(cursor)) {
				m_running = false;
				return false;
			}
		}

		return true;
	}

	std::unique_lock<std::recursive_mutex> lock(m_runtime_mutex);

	if (m_exiting == cursor) {

		ThreadContext *context = cursor->get_thread_context();
		if (context == nullptr) {
			return false;
		}

		if (on_exception(cursor)) {
			context->state = ThreadContext::DEBUGGER_PAUSE;
		}
		else {
			return false;
		}

		while (context->state == ThreadContext::DEBUGGER_PAUSE) {
			if (!check(cursor)) {
				return false;
			}
		}
	}

	return false;
}

void DebugInterface::exit(CursorDebugger *cursor) {
	m_exiting = cursor;
	m_running = false;
}

void DebugInterface::do_run(CursorDebugger *cursor) {
	if (ThreadContext *context = cursor->get_thread_context()) {
		context->state = ThreadContext::DEBUGGER_RUN;
	}
}

void DebugInterface::do_pause(CursorDebugger *cursor) {
	if (ThreadContext *context = cursor->get_thread_context()) {
		context->state = ThreadContext::DEBUGGER_PAUSE;
	}
}

void DebugInterface::do_next(CursorDebugger *cursor) {
	if (ThreadContext *context = cursor->get_thread_context()) {
		context->state = ThreadContext::DEBUGGER_NEXT;
	}
}

void DebugInterface::do_enter(CursorDebugger *cursor) {
	if (ThreadContext *context = cursor->get_thread_context()) {
		context->state = ThreadContext::DEBUGGER_ENTER;
	}
}

void DebugInterface::do_return(CursorDebugger *cursor) {
	if (ThreadContext *context = cursor->get_thread_context()) {
		context->state = ThreadContext::DEBUGGER_RETURN;
	}
}

ThreadList DebugInterface::get_threads() const {

	std::unique_lock<std::mutex> lock(m_config_mutex);

	ThreadList threads;
	transform(m_threads.begin(), m_threads.end(), back_inserter(threads), [](const auto &thread) {
		return thread.second;
	});
	return threads;
}

CursorDebugger *DebugInterface::get_thread(Process::ThreadId id) const {

	std::unique_lock<std::mutex> lock(m_config_mutex);

	auto it = m_threads.find(id);
	if (it != m_threads.end()) {
		return it->second;
	}
	return nullptr;
}

CursorDebugger *DebugInterface::declare_thread(const Process *thread) {

	std::unique_lock<std::mutex> lock(m_config_mutex);

	auto it = m_threads.find(thread->get_thread_id());
	if (it != m_threads.end()) {
		it->second->update_cursor(thread->cursor());
		return it->second;
	}

	auto *context = new ThreadContext;
	auto *cursor = new CursorDebugger(thread->cursor(), context);

	context->state = ThreadContext::DEBUGGER_RUN;
	context->line_number = 0;
	context->call_depth = 0;

	context->thread_id = thread->get_thread_id();

	m_threads.emplace(context->thread_id, cursor);
	on_thread_started(cursor);
	return cursor;
}

void DebugInterface::remove_thread(const Process *thread) {

	std::unique_lock<std::mutex> lock(m_config_mutex);

	auto it = m_threads.find(thread->get_thread_id());
	if (it != m_threads.end()) {
		assert(it->second->cursor() == thread->cursor());
		if (!it->second->close_cursor()) {
			on_thread_exited(it->second);
			delete it->second->get_thread_context();
			delete it->second;
			m_threads.erase(it);
		}
	}
}

BreakpointList DebugInterface::get_breakpoints() const {

	std::unique_lock<std::mutex> lock(m_config_mutex);

	BreakpointList breakpoints;
	breakpoints.reserve(m_breakpoints.list.size());
	std::transform(m_breakpoints.list.begin(), m_breakpoints.list.end(), std::back_inserter(breakpoints),
				   [](auto breakpoint) {
					   return std::move(breakpoint.second);
				   });
	return breakpoints;
}

Breakpoint DebugInterface::get_breakpoint(Breakpoint::Id id) const {

	std::unique_lock<std::mutex> lock(m_config_mutex);

	auto i = m_breakpoints.list.find(id);
	if (i != m_breakpoints.list.end()) {
		return i->second;
	}
	return {};
}

Breakpoint::Id DebugInterface::create_breakpoint(const LineInfo &info) {

	std::unique_lock<std::mutex> lock(m_config_mutex);

	assert(info.module_id() != Module::INVALID_ID);

	Breakpoint::Id id = next_breakpoint_id();
	m_breakpoints.position[info.module_id()][info.line_number()].emplace(id);
	const Breakpoint &breakpoint = m_breakpoints.list.emplace(id, Breakpoint {id, info}).first->second;
	on_breakpoint_created(breakpoint);
	return id;
}

void DebugInterface::remove_breakpoint(const LineInfo &info) {

	std::unique_lock<std::mutex> lock(m_config_mutex);

	auto i = m_breakpoints.position.find(info.module_id());
	if (i != m_breakpoints.position.end()) {
		auto j = i->second.find(info.line_number());
		if (j != i->second.end()) {
			for (Breakpoint::Id id : j->second) {
				auto k = m_breakpoints.list.find(id);
				on_breakpoint_deleted(k->second);
				m_breakpoints.list.erase(k);
			}
			i->second.erase(j);
			if (i->second.empty()) {
				m_breakpoints.position.erase(i);
			}
		}
	}
}

void DebugInterface::remove_breakpoint(Breakpoint::Id id) {

	std::unique_lock<std::mutex> lock(m_config_mutex);

	auto i = m_breakpoints.list.find(id);
	if (i != m_breakpoints.list.end()) {
		auto j = m_breakpoints.position.find(i->second.info.module_id());
		if (j != m_breakpoints.position.end()) {
			auto k = j->second.find(id);
			if (k != j->second.end()) {
				on_breakpoint_deleted(i->second);
				k->second.erase(id);
				if (k->second.empty()) {
					j->second.erase(k);
				}
			}
			if (j->second.empty()) {
				m_breakpoints.position.erase(j);
			}
		}
		m_breakpoints.list.erase(i);
	}
}

void DebugInterface::clear_breakpoints() {
	std::unique_lock<std::mutex> lock(m_config_mutex);
	for (auto &[id, breakpoint] : m_breakpoints.list) {
		on_breakpoint_deleted(breakpoint);
	}
	m_breakpoints.position.clear();
	m_breakpoints.list.clear();
}

Breakpoint::Id DebugInterface::next_breakpoint_id() const {
	Breakpoint::Id id = 0;
	while (m_breakpoints.list.find(id) != m_breakpoints.list.end()) {
		++id;
	}
	return id;
}
