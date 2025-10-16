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

#include "mint/scheduler/threadpool.h"
#include "mint/scheduler/process.h"
#include "mint/scheduler/processor.h"
#include "mint/system/assert.h"
#include <algorithm>
#include <cassert>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>

using namespace mint;

Process* ThreadPool::find(Process::ThreadId thread) const {
	const std::unique_lock _(_mutex);
	if (auto it = _handles.find(thread); it != _handles.end()) {
		return &it->second.get();
	}
	return nullptr;
}

Process::ThreadId ThreadPool::start(Process& thread) {

	const std::unique_lock _(_mutex);
	const auto thread_id = _next_thread_id++;

	set_multi_thread(true);
	thread.set_thread_id(thread_id);
	_stack.emplace_back(thread);
	_handles.emplace(thread_id, thread);

	return thread_id;
}

void ThreadPool::stop(Process& thread) {

	const std::unique_lock _(_mutex);

	assert(thread.is_thread());

	auto it = std::ranges::find(_stack, &thread, [](Process& thread) {
		return &thread;
	});
	assert_x(it != _stack.end(), __func__, "cannot stop unstarted thread");
	_stack.erase(it);
	if (_handles.erase(thread.get_thread_id())) {
		set_multi_thread(!_handles.empty());
		if (auto handle = thread.release_thread_handle()) {
			thread.set_thread_id(0);
			handle->detach();
		}
	}
}

void ThreadPool::stop_all() {

	std::unique_lock lock(_mutex);

	while (!_stack.empty()) {

		Process& thread = _stack.front();
		const auto thread_id = thread.get_thread_id();

		if (const auto* handle = thread.get_thread_handle()) {
			if (handle->get_id() == std::this_thread::get_id()) {
				_stack.pop_front();
				_handles.erase(thread_id);
			}
			else {
				auto handle = thread.release_thread_handle();
				lock.unlock();
				handle->join();
				lock.lock();
			}
		}
		else {
			lock.unlock();
			std::this_thread::yield();
			lock.lock();
		}
	}

	assert(_handles.empty());
	set_multi_thread(false);
}

void ThreadPool::join(Process& thread) {

	std::unique_lock lock(_mutex);

	auto it = std::ranges::find(_stack, &thread, [](Process& thread) {
		return &thread;
	});
	assert_x(it != _stack.end(), __func__, "cannot stop unstarted thread");
	_stack.erase(it);
	if (_handles.erase(thread.get_thread_id())) {
		set_multi_thread(!_handles.empty());
		if (auto handle = thread.release_thread_handle()) {
			thread.set_thread_id(0);
			lock.unlock();
			handle->join();
			lock.lock();
		}
	}
}
