#include "scheduler/scheduler.h"
#include "scheduler/destructor.h"
#include "scheduler/exception.h"
#include "scheduler/processor.h"
#include "debug/debuginterface.h"
#include "debug/debugtool.h"
#include "memory/globaldata.h"
#include "system/assert.h"
#include "system/error.h"

#include <algorithm>
#include <cstring>
#include <memory>

using namespace std;
using namespace mint;

Scheduler *Scheduler::g_instance = nullptr;
static thread_local stack<Process *> g_currentProcess;

Scheduler::Scheduler(int argc, char **argv) :
	m_nextThreadsId(1),
	m_readingArgs(false),
	m_debugInterface(nullptr),
	m_running(false),
	m_status(EXIT_SUCCESS) {

	assert_x(g_instance == nullptr, "Scheduler", "there should be only one scheduler object");
	g_instance = this;

	if (!parseArguments(argc, argv)) {
		::exit(EXIT_SUCCESS);
	}
}

Scheduler::~Scheduler() {
	g_instance = nullptr;
	Process::cleanupMetadata();
}

Scheduler *Scheduler::instance() {
	return g_instance;
}

Process *Scheduler::currentProcess() {
	if (g_currentProcess.empty()) {
		return nullptr;
	}
	return g_currentProcess.top();
}

void Scheduler::setDebugInterface(DebugInterface *interface) {
	m_debugInterface = interface;
}

int Scheduler::createThread(Process *process) {

	unique_lock<mutex> lock(m_mutex);
	int thread_id = m_nextThreadsId++;

	set_multi_thread(true);
	process->setThreadId(thread_id);
	m_threadStack.push_back(process);
	m_threadHandlers.emplace(thread_id, new thread(&Scheduler::schedule, this, process));

	return thread_id;
}

void Scheduler::finishThread(Process *process) {

	if (!is_destructor(process)) {

		unique_lock<mutex> lock(m_mutex);
		int thread_id = process->getThreadId();
		m_threadStack.remove(process);

		auto i = m_threadHandlers.find(thread_id);
		if (i != m_threadHandlers.end()) {
			thread *handler = i->second;
			m_threadHandlers.erase(i);
			set_multi_thread(!m_threadHandlers.empty());
			handler->detach();
			delete handler;
		}
	}

	process->cleanup();
	delete process;
}

Process *Scheduler::findThread(int id) const {

	unique_lock<mutex> lock(m_mutex);

	for (Process *thread : m_threadStack) {
		if (thread->getThreadId() == id) {
			return thread;
		}
	}

	return nullptr;
}

void Scheduler::createDestructor(Object *object, Reference &&member, Class *owner) {

	Destructor *destructor = new Destructor(object, move(member), owner, currentProcess());

	unlock_processor();
	schedule(destructor);
	lock_processor();
}

void Scheduler::createException(Reference &&reference) {

	Exception *exception = nullptr;

	if (Process *process = currentProcess()) {
		exception = new Exception(move(reference), process);
	}

	assert(exception);
	unlock_processor();
	schedule(exception);
	lock_processor();
}

void Scheduler::exit(int status) {
	m_status = status;
	m_running = false;
}

int Scheduler::run() {

	set_exit_callback(bind(&Scheduler::exit, this, EXIT_FAILURE));

	m_running = true;

	if (m_configuredProcess.empty()) {
		if (m_debugInterface) {
			m_running = false;
		}
		else {

			Process *process = Process::fromStandardInput();

			if (process->resume()) {
				m_configuredProcess.push_back(process);
			}
			else {
				m_running = false;
			}
		}
	}

	if (isRunning()) {

		while (m_configuredProcess.size() > 1) {
			createThread(m_configuredProcess.back());
			m_configuredProcess.pop_back();
		}

		Process *main_thread = m_configuredProcess.front();
		m_threadStack.push_front(main_thread);

		if (schedule(main_thread)) {
			m_running = false;
		}

		finalize();
	}

	return m_status;
}

bool Scheduler::isRunning() const {
	return m_running;
}

bool Scheduler::parseArguments(int argc, char **argv) {

	for (int argn = 1; argn < argc; argn++) {
		if (!parseArgument(argc, argn, argv)) {
			return false;
		}
	}

	return true;
}

bool Scheduler::parseArgument(int argc, int &argn, char **argv) {

	if (m_readingArgs) {
		m_configuredProcess.back()->parseArgument(argv[argn]);
		return true;
	}

	if (!strcmp(argv[argn], "--version")) {
		printVersion();
		return false;
	}
	else if (!strcmp(argv[argn], "--help")) {
		printHelp();
		return false;
	}
	else if (!strcmp(argv[argn], "--exec")) {
		if (++argn < argc) {
			if (Process *thread = Process::fromBuffer(argv[argn])) {
				thread->parseArgument("exec");
				m_configuredProcess.push_back(thread);
				return true;
			}
			error("Argument is not a valid command");
			return false;
		}
		error("Argument expected for the --exec option");
		return false;
	}
	else if (Process *thread = Process::fromFile(argv[argn])) {
		set_main_module_path(argv[argn]);
		thread->parseArgument(argv[argn]);
		m_configuredProcess.push_back(thread);
		m_readingArgs = true;
		return true;
	}

	error("parameter %d ('%s') is not valid", argn, argv[argn]);
	return false;
}

void Scheduler::printVersion() {
	printf("mint " MINT_MACRO_TO_STR(MINT_VERSION) "\n");
}

void Scheduler::printHelp() {
	printf("Usage : mint [option] [file [args]]\n");
	printf("Options :\n");
	printf("  --help            : Print this help message and exit\n");
	printf("  --version         : Print mint version and exit\n");
	printf("  --exec 'command'  : Execute a command line\n");
}

void Scheduler::finalizeThreads() {

	unique_lock<mutex> lock(m_mutex);

	while (!m_threadStack.empty()) {

		Process *process = m_threadStack.front();
		int thread_id = process->getThreadId();

		auto i = m_threadHandlers.find(thread_id);
		if (i != m_threadHandlers.end()) {
			thread *handler = i->second;
			if (handler->get_id() != this_thread::get_id()) {
				lock.unlock();
				handler->join();
				lock.lock();
			}
			else {
				m_threadStack.pop_front();
				m_threadHandlers.erase(i);
				handler->detach();
			}
		}
	}

	assert(m_threadHandlers.empty());
}

bool Scheduler::schedule(Process *thread) {

	g_currentProcess.push(thread);
	thread->setup();

	if (DebugInterface *interface = m_debugInterface) {

		if (!g_currentProcess.top()->cursor()->parent()) {
			set_exit_callback(bind(&DebugInterface::exit, interface, thread->cursor()));
			interface->declareThread(thread->getThreadId());
		}

		while (isRunning() || is_destructor(thread)) {
			if (!thread->debug(interface)) {

				bool collect = !is_destructor(thread);
				int thread_id = thread->getThreadId();

				interface->debug(thread->cursor());
				finishThread(thread);
				g_currentProcess.pop();

				if (g_currentProcess.empty()) {
					interface->removeThread(thread_id);
				}

				if (collect) {
					lock_processor();
					GarbageCollector::instance().collect();
					unlock_processor();
				}

				return true;
			}
		}

		interface->debug(thread->cursor());
	}
	else {
		while (isRunning() || is_destructor(thread)) {
			if (!thread->exec()) {
				if (!resume(thread)) {

					bool collect = !is_destructor(thread);

					finishThread(thread);
					g_currentProcess.pop();

					if (collect) {
						lock_processor();
						GarbageCollector::instance().collect();
						unlock_processor();
					}

					return true;
				}
			}
		}
	}

	/*
	 * Exit was called by an other thread befor completion.
	 */

	finishThread(thread);
	g_currentProcess.pop();

	lock_processor();
	GarbageCollector::instance().collect();
	unlock_processor();

	return false;
}

bool Scheduler::resume(Process *thread) {

	if (isRunning()) {
		return thread->resume();
	}

	return false;
}

void Scheduler::finalize() {

	assert(!isRunning());

	do {
		finalizeThreads();
	}
	while (GarbageCollector::instance().collect() > 0);

	Process::cleanupMemory();

	do {
		finalizeThreads();
	}
	while (GarbageCollector::instance().collect() > 0);
}
