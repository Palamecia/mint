#include "scheduler/scheduler.h"
#include "compiler/compiler.h"
#include "system/error.h"

#include <cstring>

#define __STR__(__str) #__str
#define STR(__str) __STR__(__str)

using namespace std;

Scheduler *Scheduler::g_instance = nullptr;

Scheduler::Scheduler(int argc, char **argv) : m_running(false), m_status(EXIT_SUCCESS) {

	g_instance = this;

	parseArguments(argc, argv);
}

Scheduler::~Scheduler() {

	g_instance = nullptr;

	for (auto thread : m_threads) {
		delete thread;
	}

	GarbadgeCollector::clean();
	Module::clearCache();
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

void Scheduler::parseArguments(int argc, char **argv) {

	for (int argn = 1; argn < argc; argn++) {
		if (!parseArgument(argc, argn, argv)) {
			error("parameter %d ('%s') is not valid", argn, argv[argn]);
		}
	}
}

bool Scheduler::parseArgument(int argc, int &argn, char **argv) {

	if (!m_threads.empty()) {
		m_threads.front()->parseArgument(argv[argn]);
		return true;
	}

	if (!strcmp(argv[argn], "--version")) {
		printVersion();
		::exit(EXIT_SUCCESS);
	}
	else if (!strcmp(argv[argn], "--help")) {
		printHelp();
		::exit(EXIT_SUCCESS);
	}
	else if (Process *thread = Process::create(argv[argn])) {
		thread->parseArgument(argv[argn]);
		m_threads.push_back(thread);
		return true;
	}

	return false;
}

void Scheduler::printVersion() {
	printf("mint " STR(MINT_VERSION) "\n");
}

void Scheduler::printHelp() {

}
