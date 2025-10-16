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

#include "mint/scheduler/scheduler.h"
#include "mint/ast/cursor.h"
#include "mint/ast/symbol.h"
#include "mint/memory/class.h"
#include "mint/memory/functiontool.h"
#include "mint/memory/garbagecollector.h"
#include "mint/memory/memorytool.h"
#include "mint/memory/object.h"
#include "mint/memory/reference.h"
#include "mint/scheduler/destructor.h"
#include "mint/scheduler/exception.h"
#include "mint/scheduler/generator.h"
#include "mint/scheduler/process.h"
#include "mint/scheduler/processor.h"
#include "mint/memory/operatortool.h"
#include "mint/debug/debuginterface.h"
#include "mint/debug/debugtool.h"
#include "mint/ast/savedstate.h"
#include "mint/system/error.h"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <future>
#include <iterator>
#include <memory>
#include <mutex>
#include <print>
#include <string>
#include <thread>
#include <utility>
#include <vector>

using namespace mint;

namespace {

thread_local struct {
	Scheduler* scheduler = nullptr;
	std::vector<std::reference_wrapper<Process>> process;
} g_current;

bool collect_safe() {
	auto _ = ProcessorLocker();
	return GarbageCollector::instance().collect() > 0;
}

}

SchedulerContextSwitcher::SchedulerContextSwitcher(Scheduler* scheduler) :
    _previous(g_current.scheduler) {
	g_current.scheduler = scheduler;
}

SchedulerContextSwitcher::~SchedulerContextSwitcher() {
	g_current.scheduler = _previous;
}

Scheduler* SchedulerContextSwitcher::current() {
	return g_current.scheduler;
}

TestProcess::TestProcess(Scheduler& scheduler, std::unique_ptr<Cursor>&& cursor) :
    Process(std::move(cursor)),
    _scheduler(scheduler),
    _context(&scheduler) {}

TestProcess::~TestProcess() {
	_scheduler.get().disable_testing(*this);
}

Scheduler::Scheduler(const std::vector<std::string>& args) {
	if (!parse_arguments(args)) {
		::exit(EXIT_SUCCESS);
	}
}

Scheduler::~Scheduler() {

	auto& garbage_collector = GarbageCollector::instance();

	{
		// collect global data using this scheduler
		const SchedulerContextSwitcher _(this);

		// cleanup memory
		lock_processor();
		_ast.cleanup_memory();
		unlock_processor();

		// cleanup threads
		finalize();

		// cleanup modules
		lock_processor();
		garbage_collector.collect();
		_ast.cleanup_modules();
		unlock_processor();

		// leaked destructors are ignored
	}

	// cleanup metadata
	lock_processor();
	garbage_collector.collect();
	_ast.cleanup_metadata();
	unlock_processor();
}

Scheduler* Scheduler::instance() {
	return SchedulerContextSwitcher::current();
}

AbstractSyntaxTree& Scheduler::ast() {
	return _ast;
}

Process* Scheduler::current_process() {
	if (g_current.process.empty()) [[unlikely]] {
		return nullptr;
	}
	return &g_current.process.back().get();
}

void Scheduler::set_debug_interface(DebugInterface* debug_interface) {
	_debug_interface = debug_interface;
}

void Scheduler::push_waiting_process(std::unique_ptr<Process>&& process) {
	_configured_process.emplace(std::move(process));
}

WeakReference Scheduler::invoke(const Reference& function, std::vector<WeakReference>& parameters) {

	if (g_current.process.empty()) {
		return {};
	}

	Cursor& cursor = g_current.process.back().get().cursor();
	auto process = Process(cursor.make_thread());

	try {

		Cursor& callback_cursor = process.cursor();

		init_call(callback_cursor, function);
		std::ranges::move(parameters, std::back_inserter(callback_cursor.stack()));
		call_operator(callback_cursor, static_cast<int>(parameters.size()));

		unlock_processor();
		schedule(process);
		lock_processor();
	}
	catch (MintException& raised) {
		finalize_process(process);
		lock_processor();
		create_exception(raised.take_exception());
	}

	WeakReference result = std::move(cursor.stack().back());
	cursor.stack().pop_back();
	return result;
}

WeakReference Scheduler::invoke(Class& type, std::vector<WeakReference>& parameters) {

	if (g_current.process.empty()) {
		return {};
	}

	auto object = create_object(type);

	if (type.find_operator(Class::new_operator)) {

		Cursor& cursor = g_current.process.back().get().cursor();
		auto process = Process(cursor.make_thread());

		try {

			Cursor& callback_cursor = process.cursor();

			callback_cursor.stack().emplace_back(object);
			init_operator_call(callback_cursor, Class::new_operator);
			std::ranges::move(parameters, std::back_inserter(callback_cursor.stack()));
			call_member_operator(callback_cursor, static_cast<int>(parameters.size()));

			unlock_processor();
			schedule(process);
			lock_processor();
		}
		catch (MintException& raised) {
			finalize_process(process);
			lock_processor();
			create_exception(raised.take_exception());
		}

		WeakReference result = std::move(cursor.stack().back());
		cursor.stack().pop_back();
		return result;
	}

	return object;
}

WeakReference Scheduler::invoke(const Reference& object, const Symbol& method, std::vector<WeakReference>& parameters) {

	if (g_current.process.empty()) {
		return {};
	}

	Cursor& cursor = g_current.process.back().get().cursor();
	auto process = Process(cursor.make_thread());

	try {

		Cursor& callback_cursor = process.cursor();

		callback_cursor.stack().emplace_back(object);
		init_member_call(callback_cursor, method);
		std::ranges::move(parameters, std::back_inserter(callback_cursor.stack()));
		call_member_operator(callback_cursor, static_cast<int>(parameters.size()));

		unlock_processor();
		schedule(process);
		lock_processor();
	}
	catch (MintException& raised) {
		finalize_process(process);
		lock_processor();
		create_exception(raised.take_exception());
	}

	WeakReference result = std::move(cursor.stack().back());
	cursor.stack().pop_back();
	return result;
}

WeakReference Scheduler::invoke(const Reference& object, Class::Operator op, std::vector<WeakReference>& parameters) {

	if (g_current.process.empty()) {
		return {};
	}

	Cursor& cursor = g_current.process.back().get().cursor();
	auto process = Process(cursor.make_thread());

	try {

		Cursor& callback_cursor = process.cursor();

		callback_cursor.stack().emplace_back(object);
		init_operator_call(callback_cursor, op);
		std::ranges::move(parameters, std::back_inserter(callback_cursor.stack()));
		call_member_operator(callback_cursor, static_cast<int>(parameters.size()));

		unlock_processor();
		schedule(process);
		lock_processor();
	}
	catch (MintException& raised) {
		finalize_process(process);
		lock_processor();
		create_exception(raised.take_exception());
	}

	WeakReference result = std::move(cursor.stack().back());
	cursor.stack().pop_back();
	return result;
}

class Future : public Process {
public:
	struct ResultHandle {
		WeakReference result;
	};

	explicit Future(std::unique_ptr<Cursor>&& cursor) :
	    Process(std::move(cursor)) {}

	void set_result_handle(ResultHandle* handle) {
		_handle = handle;
	}

	void cleanup() override {
		auto& stack = cursor().stack();
		if (_handle && !stack.empty()) {
			_handle->result = std::move(stack.back());
			stack.pop_back();
		}
		Process::cleanup();
	}

private:
	ResultHandle* _handle = nullptr;
};

std::future<WeakReference> Scheduler::create_async(std::unique_ptr<Cursor>&& cursor) {
	auto future = std::make_unique<Future>(std::move(cursor));
	_thread_pool.start(*future);
	return std::async(
	    [this](std::unique_ptr<Future>&& future) -> WeakReference {
		    auto process = std::move(future);
		    Future::ResultHandle handle;
		    process->set_result_handle(&handle);
		    schedule(*process);
		    return std::move(handle.result);
	    },
	    std::move(future));
}

Process::ThreadId Scheduler::create_thread(std::unique_ptr<Cursor>&& cursor) {
	auto process = std::make_unique<Process>(std::move(cursor));
	const auto thread_id = _thread_pool.start(*process);
	process->set_thread_handle(std::make_unique<std::thread>(
	    [this](std::unique_ptr<Process>&& thread) {
		    auto process = std::move(thread);
		    schedule(*process);
	    },
	    std::move(process)));
	return thread_id;
}

Process* Scheduler::find_thread(Process::ThreadId id) const {
	return _thread_pool.find(id);
}

void Scheduler::join_thread(Process::ThreadId id) {
	if (Process* thread = _thread_pool.find(id)) {
		_thread_pool.join(*thread);
	}
}

void Scheduler::create_destructor(Object* object, const Reference& member, Class& owner) {

	auto destructor = Destructor(object, member, owner, current_process());

	try {
		unlock_processor();
		schedule(destructor);
		lock_processor();
	}
	catch (MintException& raised) {
		finalize_process(destructor);
		lock_processor();
		create_exception(raised.take_exception());
	}
}

void Scheduler::create_exception(Reference&& reference) {

	auto* thread = current_process();
	if (!thread) {
		error("cannot create exception without parent thread");
	}

	auto exception = Exception(std::move(reference), *thread);

	try {
		unlock_processor();
		schedule(exception);
		lock_processor();
	}
	catch (MintException&) {
		finalize_process(exception);
		lock_processor();
		throw;
	}
}

void Scheduler::create_generator(std::unique_ptr<SavedState>&& state) {

	auto* thread = current_process();
	if (!thread) {
		error("cannot create generator without parent thread");
	}

	auto generator = Generator(std::move(state), *thread);

	try {
		unlock_processor();
		schedule(generator);
		lock_processor();
	}
	catch (MintException&) {
		finalize_process(generator);
		lock_processor();
		throw;
	}
}

void Scheduler::add_exit_callback(const std::function<void(int)>& callback) {
	const std::unique_lock _(_exit_callbacks_mutex);
	_exit_callbacks.push_back(callback);
}

bool Scheduler::is_running() const {
	return _running;
}

void Scheduler::exit(int status) {
	_status = status;
	const std::unique_lock _(_exit_callbacks_mutex);
	for (const auto& callback : _exit_callbacks) {
		callback(status);
	}
	_running = false;
}

int Scheduler::run() {

	if (_configured_process.empty()) {

		if (_debug_interface) {
			return _status;
		}

		if (auto process = Process::from_standard_input(*this)) {
			_configured_process.emplace(std::move(process));
		}
		else {
			return _status;
		}
	}

	while (!_configured_process.empty()) {

		auto main_thread = std::move(_configured_process.front());
		_configured_process.pop();
		_running = true;

		if (DebugInterface* handle = _debug_interface) {
			set_exit_callback([handle, &cursor = handle->declare_thread(*main_thread)] {
				handle->exit(cursor);
			});
		}
		else if (main_thread->is_endless()) {
			set_exit_callback([&cursor = main_thread->cursor()] {
				cursor.retrieve();
			});
			if (!main_thread->resume()) {
				_running = false;
			}
		}
		else {
			set_exit_callback([this] {
				exit(EXIT_FAILURE);
			});
		}

		if (schedule(*main_thread)) {
			_running = false;
		}
	}

	finalize();
	return _status;
}

std::unique_ptr<TestProcess> Scheduler::enable_testing() {

	if (_running) {
		return {};
	}

	if (!g_current.process.empty()) {
		return {};
	}

	auto thread = std::make_unique<TestProcess>(*this, std::make_unique<Cursor>(_ast));
	g_current.process.emplace_back(*thread);
	thread->setup();
	lock_processor();
	_running = true;
	return thread;
}

bool Scheduler::disable_testing(TestProcess& process) {

	if (!_running) {
		return false;
	}

	if (g_current.process.empty()) {
		return false;
	}

	if (&process != &g_current.process.back().get()) {
		return false;
	}

	unlock_processor();
	finalize_process(process);

	if (g_current.process.empty()) {
		_running = false;
		finalize();
	}

	return true;
}

bool Scheduler::parse_arguments(const std::vector<std::string>& args) {

	bool reading_args = false;

	for (auto it = args.begin(); it != args.end(); ++it) {
		if (reading_args) {
			_configured_process.back()->parse_argument(*it);
		}
		else if (*it == "--version") {
			print_version();
			return false;
		}
		else if (*it == "--help") {
			print_help();
			return false;
		}
		else if (*it == "--exec") {
			if (++it != args.end()) {
				if (auto thread = Process::from_buffer(*this, *it)) {
					thread->parse_argument("exec");
					_configured_process.emplace(std::move(thread));
				}
				else {
					error("Argument is not a valid command");
					return false;
				}
			}
			else {
				error("Argument expected for the --exec option");
				return false;
			}
		}
		else if (auto thread = Process::from_main_file(*this, *it)) {
			thread->parse_argument(*it);
			_configured_process.emplace(std::move(thread));
			reading_args = true;
		}
		else {
			print_help();
			error("parameter '{}' is not valid", *it);
			return false;
		}
	}

	return true;
}

void Scheduler::print_version() {
	std::println(stdout, "mint " MINT_MACRO_TO_STR(MINT_VERSION));
}

void Scheduler::print_help() {
	std::println(stdout, "Usage: mint [option] [file [args]]");
	std::println(stdout, "Options:");
	std::println(stdout, "  --help            : Print this help message and exit");
	std::println(stdout, "  --version         : Print mint version and exit");
	std::println(stdout, "  --exec 'command'  : Execute a command line");
}

bool Scheduler::schedule(Process& thread) {

	const SchedulerContextSwitcher _(this);

	initialize_process(thread);

	if (DebugInterface* handle = _debug_interface) {

		while (is_running() || is_destructor(thread)) {
			if (!thread.debug(*handle)) {

				lock_processor();
				handle->debug(handle->declare_thread(thread));
				handle->remove_thread(thread);
				unlock_processor();

				finalize_process(thread);
				return true;
			}
		}

		lock_processor();
		handle->debug(handle->declare_thread(thread));
		handle->remove_thread(thread);
		unlock_processor();
	}
	else {
		while (is_running() || is_destructor(thread)) {
			if (!thread.exec()) {
				if (!resume(thread)) {
					finalize_process(thread);
					return true;
				}
			}
		}
	}

	/*
	 * Exit was called by an other thread befor completion.
	 */

	finalize_process(thread);
	return false;
}

bool Scheduler::resume(Process& thread) const {
	if (is_running()) {
		return thread.resume();
	}
	return false;
}

void Scheduler::initialize_process(Process& process) {
	g_current.process.emplace_back(process);
	process.setup();
}

void Scheduler::finalize_process(Process& process) {

	assert(&process == &g_current.process.back().get());

	if (process.is_thread()) {
		_thread_pool.stop(process);
	}

	process.cleanup();
	g_current.process.pop_back();
}

void Scheduler::finalize() {

	assert(!is_running());

	do {
		_thread_pool.stop_all();
	}
	while (collect_safe());
}
