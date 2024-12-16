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

#include "mint/scheduler/threadpool.h"
#include "mint/scheduler/processor.h"

using namespace mint;

ThreadPool::ThreadPool() {

}

Process *ThreadPool::find(Process::ThreadId thread) const {
	std::unique_lock<std::mutex> lock(m_mutex);
	auto it = m_handles.find(thread);
	if (it != m_handles.end()) {
		return it->second;
	}
	return nullptr;
}

Process::ThreadId ThreadPool::start(Process *thread) {

	std::unique_lock<std::mutex> lock(m_mutex);
	Process::ThreadId thread_id = m_next_thread_id++;

	set_multi_thread(true);
	thread->set_thread_id(thread_id);
	m_stack.push_back(thread);
	m_handles.emplace(thread_id, thread);

	return thread_id;
}

void ThreadPool::attach(Process *thread) {
	std::unique_lock<std::mutex> lock(m_mutex);
	m_stack.push_front(thread);
}

void ThreadPool::stop(Process *thread) {

	std::unique_lock<std::mutex> lock(m_mutex);

	m_stack.remove(thread);
	if (m_handles.erase(thread->get_thread_id())) {
		set_multi_thread(!m_handles.empty());
		if (std::thread *handle = thread->get_thread_handle()) {
			handle->detach();
			delete handle;
		}
	}
}

void ThreadPool::stop_all() {

	std::unique_lock<std::mutex> lock(m_mutex);

	while (!m_stack.empty()) {

		Process *thread = m_stack.front();
		Process::ThreadId thread_id = thread->get_thread_id();
		
		if (std::thread *handle = thread->get_thread_handle()) {
			if (handle->get_id() == std::this_thread::get_id()) {
				m_stack.pop_front();
				m_handles.erase(thread_id);
			}
			else {
				thread->set_thread_handle(nullptr);
				lock.unlock();
				handle->join();
				lock.lock();
				delete handle;
			}
		}
		else {
			lock.unlock();
			std::this_thread::yield();
			lock.lock();
		}
	}

	assert(m_handles.empty());
	set_multi_thread(false);
}

void ThreadPool::join(Process *thread) {

	std::unique_lock<std::mutex> lock(m_mutex);

	if (std::thread *handle = thread->get_thread_handle()) {
		thread->set_thread_handle(nullptr);
		lock.unlock();
		handle->join();
		lock.lock();
		delete handle;
	}
}
