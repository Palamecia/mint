#include "scheduler/scheduler.h"
#include "compiler/compiler.h"
#include "system/error.h"

#include <algorithm>
#include <cstring>

#define __STR__(__str) #__str
#define STR(__str) __STR__(__str)

using namespace std;

Scheduler *Scheduler::g_instance = nullptr;

Scheduler::Scheduler(int argc, char **argv) : m_readingArgs(false), m_running(false), m_status(EXIT_SUCCESS) {

	g_instance = this;

	if (!parseArguments(argc, argv)) {
		::exit(EXIT_SUCCESS);
	}
}

Scheduler::~Scheduler() {

	g_instance = nullptr;

	for_each(m_threads.begin(), m_threads.end(), [](Process *thread){ delete thread; });
	m_threads.clear();

	GlobalData::instance().symbols().clear();
	while (GarbadgeCollector::free());

	Module::clearCache();
	GarbadgeCollector::clean();
}

Scheduler *Scheduler::instance() {
	return g_instance;
}

int Scheduler::run() {

	if (m_threads.empty()) {
		Process *process = Process::fromStandardInput();
		if (process->resume()) {
			m_threads.push_back(process);
		}
	}

	m_running = true;
	size_t quantum = 64;

	while (!m_threads.empty()) {
		for (auto thread = m_threads.begin(); thread != m_threads.end(); ++thread) {

			Process *process = *thread;

			if (!process->exec(quantum)) {

				if (isOver()) {
					return m_status;
				}

				if (!process->resume()) {
					delete process;
					thread = m_threads.erase(thread);
				}
			}
		}

		GarbadgeCollector::free();
	}

	return m_status;
}

void Scheduler::exit(int status) {
	m_status = status;
	m_running = false;
}

bool Scheduler::isOver() const {
	return !m_running;
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
			if (Process *thread = Process::fromBuffer(argv[argn])) {
				thread->parseArgument("exec");
				m_threads.push_back(thread);
				return true;
			}
			error("Argument is not a valid command");
			return false;
		}
		error("Argument expected for the --exec option");
		return false;
	}
	else if (Process *thread = Process::fromFile(argv[argn])) {
		thread->parseArgument(argv[argn]);
		m_threads.push_back(thread);
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
