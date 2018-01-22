#include "scheduler/scheduler.h"
#include "compiler/compiler.h"
#include "system/error.h"

#include <memory>
#include <algorithm>
#include <cstring>

#define __STR__(__str) #__str
#define STR(__str) __STR__(__str)

using namespace std;
using namespace mint;

Scheduler *Scheduler::g_instance = nullptr;

Scheduler::Scheduler(int argc, char **argv) :
	m_nextThreadsId(1),
	m_readingArgs(false),
	m_quantum(0),
	m_running(false),
	m_status(EXIT_SUCCESS) {

	GarbadgeCollector::instance();

	g_instance = this;

	if (!parseArguments(argc, argv)) {
		::exit(EXIT_SUCCESS);
	}
}

Scheduler::~Scheduler() {

	g_instance = nullptr;

	for_each(m_threads.begin(), m_threads.end(), default_delete<Process>());
	m_threads.clear();
}

Scheduler *Scheduler::instance() {
	return g_instance;
}

AbstractSyntaxTree *Scheduler::ast() {
	return &m_ast;
}

Process *Scheduler::currentProcess() {

	if (m_currentProcess.empty()) {
		return nullptr;
	}

	return m_currentProcess.top();
}

size_t Scheduler::createThread(Process *thread) {

	size_t thread_id = m_nextThreadsId++;

	thread->setThreadId(thread_id);
	m_threads.push_back(thread);

	return thread_id;
}

void Scheduler::exit(int status) {
	m_status = status;
	m_running = false;
}

int Scheduler::run() {

	m_running = true;
	m_quantum = 64;

	if (m_threads.empty()) {
		Process *process = Process::fromStandardInput(ast());
		if (process->resume()) {
			createThread(process);
		}
		else {
			m_running = false;
		}
	}

	while (isRunning()) {

		auto thread = m_threads.begin();

		while (thread != m_threads.end()) {

			Process *process = *thread;

			beginThreadUpdate(process);

			if (!schedule(process)) {
				if (!resume(process)) {
					delete process;
					thread = m_threads.erase(thread);
					if (m_threads.empty()) {
						m_running = false;
					}
				}
				else {
					++thread;
				}
			}
			else {
				++thread;
			}

			endThreadUpdate();
		}

		GarbadgeCollector::instance().free();
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
		m_threads.back()->parseArgument(argv[argn]);
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
				createThread(thread);
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
		createThread(thread);
		m_readingArgs = true;
		return true;
	}

	error("parameter %d ('%s') is not valid", argn, argv[argn]);
	return false;
}

void Scheduler::printVersion() {
	printf("mint " STR(MINT_VERSION) "\n");
}

void Scheduler::printHelp() {
	printf("Usage : mint [option] [file [args]]\n");
	printf("Options :\n");
	printf("  --help            : Print this help message and exit\n");
	printf("  --version         : Print mint version and exit\n");
	printf("  --exec 'command'  : Execute a command line\n");
}

void Scheduler::beginThreadUpdate(Process *thread) {
	m_currentProcess.push(thread);
}

void Scheduler::endThreadUpdate() {
	m_currentProcess.pop();
}

bool Scheduler::schedule(Process *thread) {

	try {
		return thread->exec(m_quantum);
	}
	catch (MintSystemError) {
		return false;
	}
}

bool Scheduler::resume(Process *thread) {

	try {
		if (isRunning()) {
			return thread->resume();
		}

		return false;
	}
	catch (MintSystemError) {
		return false;
	}
}
