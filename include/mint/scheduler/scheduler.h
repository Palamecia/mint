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

#ifndef MINT_SCHEDULER_SCHEDULER_H
#define MINT_SCHEDULER_SCHEDULER_H

#include "mint/ast/abstract_syntax_tree.h"
#include "mint/ast/saved_state.h"
#include "mint/ast/symbol.h"
#include "mint/config.h"
#include "mint/memory/class.h"
#include "mint/memory/reference.h"
#include "mint/scheduler/thread_pool.h"
#include "mint/scheduler/process.h"

#include <cstdlib>
#include <functional>
#include <future>
#include <atomic>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

namespace mint {

class DebugInterface;
class Object;
class Scheduler;

class MINT_EXPORT SchedulerContextSwitcher {
	Scheduler* _previous = nullptr;
public:
	SchedulerContextSwitcher(const SchedulerContextSwitcher&) = default;
	SchedulerContextSwitcher(SchedulerContextSwitcher&&) = delete;
	SchedulerContextSwitcher(Scheduler* scheduler);
	~SchedulerContextSwitcher();

	static Scheduler* current();

	SchedulerContextSwitcher& operator=(const SchedulerContextSwitcher&) = default;
	SchedulerContextSwitcher& operator=(SchedulerContextSwitcher&&) = delete;
};

class MINT_EXPORT TestProcess : public Process {
	std::reference_wrapper<Scheduler> _scheduler;
	SchedulerContextSwitcher _context;
public:
	TestProcess(Scheduler& scheduler, std::unique_ptr<Cursor>&& cursor);
	TestProcess(const TestProcess&) = delete;
	TestProcess(TestProcess&&) = delete;
	~TestProcess();

	TestProcess& operator=(const TestProcess&) = delete;
	TestProcess& operator=(TestProcess&&) = delete;
};

class MINT_EXPORT Scheduler {
public:
	Scheduler(const std::vector<std::string>& args);
	Scheduler(Scheduler&&) = delete;
	Scheduler(const Scheduler&) = delete;
	~Scheduler();

	Scheduler& operator=(Scheduler&&) = delete;
	Scheduler& operator=(const Scheduler&) = delete;

	static Scheduler* instance();

	AbstractSyntaxTree& ast();
	static Process* current_process();

	void set_debug_interface(DebugInterface* debug_interface);
	void push_waiting_process(std::unique_ptr<Process>&& process);

	template<class... Args>
	Reference invoke(const Reference& function, Args... args);
	Reference invoke(const Reference& function, std::vector<Reference>& parameters);

	template<class... Args>
	Reference invoke(Class& type, Args... args);
	Reference invoke(Class& type, std::vector<Reference>& parameters);

	template<class... Args>
	Reference invoke(const Reference& object, const Symbol& method, Args... args);
	Reference invoke(const Reference& object, const Symbol& method, std::vector<Reference>& parameters);

	template<class... Args>
	Reference invoke(const Reference& object, Class::Operator op, Args... args);
	Reference invoke(const Reference& object, Class::Operator op, std::vector<Reference>& parameters);

	template<class... Args>
	Reference invoke(const Reference& object, const Symbol& method, const Class::MemberInfo& info, Args... args);
	Reference invoke(const Reference& object, const Symbol& method, const Class::MemberInfo& info,
	    std::vector<Reference>& parameters);

	std::future<Reference> create_async_thread(std::unique_ptr<Cursor>&& cursor);
	Process::ThreadId create_thread(std::unique_ptr<Cursor>&& cursor);
	Process* find_thread(Process::ThreadId id) const;
	void join_thread(Process::ThreadId id);

	void create_destructor(Object* object, const Reference& member, Class& owner);
	void create_generator(std::unique_ptr<SavedState>&& state);

	void add_exit_callback(const std::function<void(int)>& callback);

	bool is_running() const;
	void exit(int status);
	int run();

	std::unique_ptr<TestProcess> enable_testing();
	bool disable_testing(TestProcess& process);

protected:
	bool parse_arguments(const std::vector<std::string>& args);
	static void print_version();
	static void print_help();

	bool schedule(Process& thread);
	bool resume(Process& thread) const;

	static void initialize_process(Process& process);
	void finalize_process(Process& process);
	void abort_process(Process& process);
	void finalize();

private:
	std::queue<std::unique_ptr<Process>> _configured_process;
	DebugInterface* _debug_interface = nullptr;

	AbstractSyntaxTree _ast;
	ThreadPool _thread_pool;

	std::vector<std::function<void(int)>> _exit_callbacks;
	std::mutex _exit_callbacks_mutex;

	std::atomic_bool _running {false};
	std::atomic_int _status = EXIT_SUCCESS;
};

template<class... Args>
Reference Scheduler::invoke(const Reference& function, Args... args) {
	std::vector<Reference> parameters;
	parameters.reserve(sizeof...(args));
	(parameters.emplace_back(std::forward<Args>(args)), ...);
	return invoke(function, parameters);
}

template<class... Args>
Reference Scheduler::invoke(Class& type, Args... args) {
	std::vector<Reference> parameters;
	parameters.reserve(sizeof...(args));
	(parameters.emplace_back(std::forward<Args>(args)), ...);
	return invoke(type, parameters);
}

template<class... Args>
Reference Scheduler::invoke(const Reference& object, const Symbol& method, Args... args) {
	std::vector<Reference> parameters;
	parameters.reserve(sizeof...(args));
	(parameters.emplace_back(std::forward<Args>(args)), ...);
	return invoke(object, method, parameters);
}

template<class... Args>
Reference Scheduler::invoke(const Reference& object, Class::Operator op, Args... args) {
	std::vector<Reference> parameters;
	parameters.reserve(sizeof...(args));
	(parameters.emplace_back(std::forward<Args>(args)), ...);
	return invoke(object, op, parameters);
}

template<class... Args>
Reference Scheduler::invoke(const Reference& object, const Symbol& method, const Class::MemberInfo& info, Args... args) {
	std::vector<Reference> parameters;
	parameters.reserve(sizeof...(args));
	(parameters.emplace_back(std::forward<Args>(args)), ...);
	return invoke(object, method, info, parameters);
}

}

#endif // MINT_SCHEDULER_SCHEDULER_H
