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

#ifndef MINT_THREADPOOL_H
#define MINT_THREADPOOL_H

#include "mint/scheduler/process.h"

#include <unordered_map>
#include <mutex>
#include <list>

namespace mint {

class MINT_EXPORT ThreadPool {
public:
	ThreadPool() = default;

	Process *find(Process::ThreadId thread) const;
	Process::ThreadId start(Process *thread);
	void attach(Process *thread);
	void stop(Process *thread);
	void stop_all();

	void join(Process *thread);

private:
	std::unordered_map<Process::ThreadId, Process *> m_handles;
	Process::ThreadId m_next_thread_id = 1;
	std::list<Process *> m_stack;
	mutable std::mutex m_mutex;
};

}

#endif // MINT_THREADPOOL_H
