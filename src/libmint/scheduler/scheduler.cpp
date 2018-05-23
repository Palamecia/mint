#include "scheduler/scheduler.h"
#include "scheduler/destructor.h"
#include "memory/globaldata.h"
#include "system/assert.h"
#include "system/error.h"

#include <algorithm>
#include <cstring>
#include <memory>

using namespace std;
using namespace mint;

Scheduler *Scheduler::g_instance = nullptr;
thread_local stack<Process *> Scheduler::g_currentProcess;

Scheduler::Scheduler(int argc, char **argv) :
	m_nextThreadsId(1),
	m_readingArgs(false),
	m_running(false),
	m_status(EXIT_SUCCESS) {

	GarbadgeCollector::instance();

	assert_x(g_instance == nullptr, "Scheduler", "there should be only one scheduler object");
	g_instance = this;

	if (!parseArguments(argc, argv)) {
		::exit(EXIT_SUCCESS);
	}
}

Scheduler::~Scheduler() {
	g_instance = nullptr;
}

Scheduler *Scheduler::instance() {
	return g_instance;
}

AbstractSyntaxTree *Scheduler::ast() {
	return &m_ast;
}

Process *Scheduler::currentProcess() {
	if (g_currentProcess.empty()) {
		return nullptr;
	}
	return g_currentProcess.top();
}

int Scheduler::createThread(Process *process) {

	unique_lock<mutex> lock(m_mutex);
	int thread_id = m_nextThreadsId++;

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
			handler->detach();
			delete handler;
		}
	}

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

void Scheduler::createDestructor(Destructor *destructor) {
	if (Process *process = currentProcess()) {
		destructor->setThreadId(process->getThreadId());
	}
	else {
		destructor->setThreadId(-1);
	}
	schedule(destructor);
}

void Scheduler::exit(int status) {
	m_status = status;
	m_running = false;
}

int Scheduler::run() {

	m_running = true;

	if (m_configuredProcess.empty()) {
		Process *process = Process::fromStandardInput(ast());
		if (process->resume()) {
			m_configuredProcess.push_back(process);
		}
		else {
			m_running = false;
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
			if (Process *thread = Process::fromBuffer(ast(), argv[argn])) {
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
	else if (Process *thread = Process::fromFile(ast(), argv[argn])) {
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

	while (isRunning() || is_destructor(thread)) {

		if (!thread->exec(quantum)) {
			if (!resume(thread)) {
				g_currentProcess.pop();
				finishThread(thread);

				GarbadgeCollector::instance().collect();
				return true;
			}
		}
	}

	/*
	 * Exit was called by an other thread befor completion.
	 */

	g_currentProcess.pop();
	finishThread(thread);

	GarbadgeCollector::instance().collect();
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
	while (GarbadgeCollector::instance().collect() > 0);

	GlobalData::instance().symbols().clear();

	do {
		finalizeThreads();
	}
	while (GarbadgeCollector::instance().collect() > 0);
}
