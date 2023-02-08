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

#include "mint/scheduler/scheduler.h"
#include "mint/scheduler/destructor.h"
#include "mint/scheduler/exception.h"
#include "mint/scheduler/generator.h"
#include "mint/scheduler/processor.h"
#include "mint/debug/debuginterface.h"
#include "mint/debug/debugtool.h"
#include "mint/ast/savedstate.h"
#include "mint/system/terminal.h"
#include "mint/system/assert.h"
#include "mint/system/error.h"

#include <cstring>
#include <memory>

using namespace std;
using namespace mint;

Scheduler *Scheduler::g_instance = nullptr;
static thread_local vector<Process *> g_current_process;

static bool collect_safe() {
	lock_processor();
	const bool collected = GarbageCollector::instance().collect() > 0;
	unlock_processor();
	return collected;
}

Scheduler::Scheduler(int argc, char **argv) :
	m_debug_interface(nullptr),
	m_ast(new AbstractSyntaxTree),
	m_running(false),
	m_status(EXIT_SUCCESS) {

	assert_x(g_instance == nullptr, "Scheduler", "there should be only one scheduler object");
	g_instance = this;


	if (!parse_arguments(argc, argv)) {
		::exit(EXIT_SUCCESS);
	}
}

Scheduler::~Scheduler() {

	// cleanup modules
	lock_processor();
	GarbageCollector::instance().collect();
	m_ast->cleanup_modules();
	unlock_processor();

	// leaked destructors are ignored
	g_instance = nullptr;

	// cleanup metadata
	lock_processor();
	GarbageCollector::instance().collect();
	m_ast->cleanup_metadata();
	unlock_processor();

	// destroy abstract syntax tree
	delete m_ast;
}

Scheduler *Scheduler::instance() {
	return g_instance;
}

AbstractSyntaxTree *Scheduler::ast() {
	return m_ast;
}

Process *Scheduler::current_process() {
	if (g_current_process.empty()) {
		return nullptr;
	}
	return g_current_process.back();
}

void Scheduler::set_debug_interface(DebugInterface *debugInterface) {
	m_debug_interface = debugInterface;
}

void Scheduler::push_waiting_process(Process *process) {
	m_configured_process.push(process);
}

class Future : public Process {
public:
	struct ResultHandle {
		WeakReference result;
	};

	Future(Cursor *cursor) :
		Process(cursor) {

	}

	void set_result_handle(ResultHandle *handle) {
		m_handle = handle;
	}

	void cleanup() override {
		auto &stack = cursor()->stack();
		if (m_handle && !stack.empty()) {
			m_handle->result = std::move(stack.back());
			stack.pop_back();
		}
		Process::cleanup();
	}

private:
	ResultHandle *m_handle = nullptr;
};

future<WeakReference> Scheduler::create_async(Cursor *cursor) {
	Future *process = new Future(cursor);
	m_thread_pool.start(process);
	return std::async([this](Future *process) -> WeakReference {
		Future::ResultHandle handle;
		process->set_result_handle(&handle);
		schedule(process);
		return std::move(handle.result);
	}, process);
}

Process::ThreadId Scheduler::create_thread(Cursor *cursor) {
	Process *process = new Process(cursor);
	Process::ThreadId thread_id = m_thread_pool.start(process);
	process->set_thread_handle(new thread(&Scheduler::schedule, this, process));
	return thread_id;
}

Process *Scheduler::find_thread(Process::ThreadId id) const {
	return m_thread_pool.find(id);
}

void Scheduler::create_destructor(Object *object, Reference &&member, Class *owner) noexcept {
	
	Destructor *destructor = new Destructor(object, std::move(member), owner, current_process());

	try {
		unlock_processor();
		schedule(destructor);
		lock_processor();
	}
	catch (MintException &raised) {

		unlock_processor();
		finalize_process(destructor);
		lock_processor();

		g_current_process.pop_back();
		create_exception(raised.take_exception());
	}
}

void Scheduler::create_exception(Reference &&reference) {
	
	Exception *exception = new Exception(std::forward<Reference>(reference), current_process());

	try {
		unlock_processor();
		schedule(exception);
		lock_processor();
	}
	catch (MintException &) {

		unlock_processor();
		finalize_process(exception);
		lock_processor();

		g_current_process.pop_back();
		throw;
	}
}

void Scheduler::create_generator(unique_ptr<SavedState> state) {
	
	Generator *generator = new Generator(std::move(state), current_process());

	try {
		unlock_processor();
		schedule(generator);
		lock_processor();
	}
	catch (MintException &) {

		unlock_processor();
		finalize_process(generator);
		lock_processor();

		g_current_process.pop_back();
		throw;
	}
}

bool Scheduler::is_running() const {
	return m_running;
}

void Scheduler::exit(int status) {
	m_status = status;
	m_running = false;
}

int Scheduler::run() {
	
	if (m_configured_process.empty()) {
		
		if (m_debug_interface) {
			return m_status;
		}
		
		Process *process = Process::from_standard_input(m_ast);
		m_running = true;

		if (process->resume()) {
			m_configured_process.push(process);
		}
		else {
			m_running = false;
			return m_status;
		}
	}
	
	while (!m_configured_process.empty()) {
		
		Process *main_thread = m_configured_process.front();
		m_thread_pool.attach(main_thread);
		m_configured_process.pop();
		m_running = true;
		
		if (DebugInterface *handle = m_debug_interface) {
			set_exit_callback(std::bind(&DebugInterface::exit, handle, handle->declare_thread(main_thread)));
		}
		else if (main_thread->is_endless()) {
			set_exit_callback(std::bind(&Cursor::retrieve, main_thread->cursor()));
		}
		else {
			set_exit_callback(std::bind(&Scheduler::exit, this, EXIT_FAILURE));
		}

		if (schedule(main_thread)) {
			m_running = false;
		}
	}

	finalize();
	return m_status;
}

bool Scheduler::parse_arguments(int argc, char **argv) {

	bool reading_args = false;

	for (int argn = 1; argn < argc; argn++) {
		if (reading_args) {
			m_configured_process.back()->parseArgument(argv[argn]);
		}
		else if (!strcmp(argv[argn], "--version")) {
			print_version();
			return false;
		}
		else if (!strcmp(argv[argn], "--help")) {
			print_help();
			return false;
		}
		else if (!strcmp(argv[argn], "--exec")) {
			if (++argn < argc) {
				if (Process *thread = Process::from_buffer(m_ast, argv[argn])) {
					thread->parseArgument("exec");
					m_configured_process.push(thread);
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
		else if (Process *thread = Process::from_file(m_ast, argv[argn])) {
			set_main_module_path(argv[argn]);
			thread->parseArgument(argv[argn]);
			m_configured_process.push(thread);
			reading_args = true;
		}
		else {
			error("parameter %d ('%s') is not valid", argn, argv[argn]);
			return false;
		}
	}

	return true;
}

void Scheduler::print_version() {
	Terminal::print(stdout, "mint " MINT_MACRO_TO_STR(MINT_VERSION) "\n");
}

void Scheduler::print_help() {
	Terminal::print(stdout, "Usage : mint [option] [file [args]]\n");
	Terminal::print(stdout, "Options :\n");
	Terminal::print(stdout, "  --help            : Print this help message and exit\n");
	Terminal::print(stdout, "  --version         : Print mint version and exit\n");
	Terminal::print(stdout, "  --exec 'command'  : Execute a command line\n");
}

bool Scheduler::schedule(Process *thread) {

	g_current_process.emplace_back(thread);
	thread->setup();
	
	if (DebugInterface *handle = m_debug_interface) {
		
		while (is_running() || is_destructor(thread)) {
			if (!thread->debug(handle)) {
				
				bool collect = thread->collect_on_exit();

				lock_processor();
				handle->debug(handle->declare_thread(thread));
				handle->remove_thread(thread);
				unlock_processor();
				
				finalize_process(thread);
				g_current_process.pop_back();

				if (collect) {
					collect_safe();
				}

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
			if (!thread->exec()) {
				if (!resume(thread)) {
					
					bool collect = thread->collect_on_exit();
					
					finalize_process(thread);
					g_current_process.pop_back();

					if (collect) {
						collect_safe();
					}

					return true;
				}
			}
		}
	}

	/*
	 * Exit was called by an other thread befor completion.
	 */
	
	finalize_process(thread);
	g_current_process.pop_back();

	collect_safe();

	return false;
}

bool Scheduler::resume(Process *thread) {
	
	if (is_running()) {
		return thread->resume();
	}

	return false;
}

void Scheduler::finalize_process(Process *process) {

	if (!is_destructor(process) && !is_exception(process) && !is_generator(process)) {
		m_thread_pool.stop(process);
	}

	process->cleanup();
	delete process;
}

void Scheduler::finalize() {
	
	assert(!is_running());

	do {
		m_thread_pool.stop_all();
	}
	while (collect_safe());

	lock_processor();
	m_ast->cleanup_memory();
	unlock_processor();

	do {
		m_thread_pool.stop_all();
	}
	while (collect_safe());
}
