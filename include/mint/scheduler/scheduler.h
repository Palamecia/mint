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

#ifndef MINT_SCHEDULER_H
#define MINT_SCHEDULER_H

#include "mint/ast/abstractsyntaxtree.h"
#include "mint/scheduler/threadpool.h"

#include <future>
#include <atomic>
#include <queue>

namespace mint {

class DebugInterface;
struct Object;

class MINT_EXPORT Scheduler {
public:
	Scheduler(int argc, char **argv);
	~Scheduler();

	Scheduler(const Scheduler &other) = delete;
	Scheduler &operator =(const Scheduler &other) = delete;

	static Scheduler *instance();

	AbstractSyntaxTree *ast();
	Process *current_process();

	void set_debug_interface(DebugInterface *debugInterface);
	void push_waiting_process(Process *process);

	template<class... Args>
	WeakReference invoke(Reference &function, Args... args);
	WeakReference invoke(Reference &function, std::vector<WeakReference> &parameters);

	template<class... Args>
	WeakReference invoke(Reference &object, const Symbol &method, Args... args);
	WeakReference invoke(Reference &object, const Symbol &method, std::vector<WeakReference> &parameters);

	template<class... Args>
	WeakReference invoke(Reference &object, Class::Operator op, Args... args);
	WeakReference invoke(Reference &object, Class::Operator op, std::vector<WeakReference> &parameters);

	std::future<WeakReference> create_async(Cursor *cursor);
	Process::ThreadId create_thread(Cursor *cursor);
	Process *find_thread(Process::ThreadId id) const;
	void join_thread(Process::ThreadId id);

	void create_destructor(Object *object, Reference &&member, Class *owner) ;
	void create_exception(Reference &&reference);
	void create_generator(std::unique_ptr<SavedState> state);

	void add_exit_callback(const std::function<void(int)> &callback);

	bool is_running() const;
	void exit(int status);
	int run();

protected:
	bool parse_arguments(int argc, char **argv);
	void print_version();
	void print_help();

	enum RunOption {
		no_run_option = 0x00,
		collect_at_exit = 0x01
	};
	using RunOptions = std::underlying_type_t<RunOption>;

	bool schedule(Process *thread, RunOptions options);
	bool resume(Process *thread);

	void finalize_process(Process *process);
	void finalize();

private:
	static Scheduler *g_instance;

	std::queue<Process *> m_configured_process;
	DebugInterface *m_debug_interface;

	AbstractSyntaxTree *m_ast;
	ThreadPool m_thread_pool;

	std::vector<std::function<void(int)>> m_exit_callbacks;
	std::mutex m_exit_callbacks_mutex;

	std::atomic_bool m_running;
	std::atomic_int m_status;
};

template<class... Args>
WeakReference Scheduler::invoke(Reference &function, Args... args) {
	std::vector<WeakReference> parameters;
	(parameters.emplace_back(std::forward<Args>(args)), ...);
	return invoke(function, parameters);
}

template<class... Args>
WeakReference Scheduler::invoke(Reference &object, const Symbol &method, Args... args) {
	std::vector<WeakReference> parameters;
	(parameters.emplace_back(std::forward<Args>(args)), ...);
	return invoke(object, method, parameters);
}

template<class... Args>
WeakReference Scheduler::invoke(Reference &object, Class::Operator op, Args... args) {
	std::vector<WeakReference> parameters;
	(parameters.emplace_back(std::forward<Args>(args)), ...);
	return invoke(object, op, parameters);
}

}

#endif // MINT_SCHEDULER_H
