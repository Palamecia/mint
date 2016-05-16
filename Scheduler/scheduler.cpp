#include "Scheduler/scheduler.h"
#include "Compiler/compiler.h"

#include <cstring>

using namespace std;

Scheduler::Scheduler(int argc, char **argv) {

	parseArguments(argc, argv);
}

Scheduler::~Scheduler() {

	for (auto thread : m_threads) {
		delete thread;
	}

	AbstractSynatxTree::clearCache();
}

int Scheduler::run() {

    while (!m_threads.empty()) {
		for (auto thread = m_threads.begin(); thread != m_threads.end(); ++thread) {
			if (!(*thread)->exec(42)) {
				delete *thread;
				thread = m_threads.erase(thread);
			}
        }

		GarbadgeCollector::free();
    }

    return 0;
}

void Scheduler::parseArguments(int argc, char **argv) {

	/// \todo handle first arg

	for (int argn = 1; argn < argc; argn++) {
		if (!parseArgument(argc, argn, argv)) {
			/// \todo raise error
		}
	}

}

bool Scheduler::parseArgument(int argc, int &argn, char **argv) {

	if (!strcmp(argv[argn], "--version")) {
		if (argn == 1) {
			printVersion();
			exit(0);
		}
	}
	else if (!strcmp(argv[argn], "--help")) {
		if (argn == 1) {
			printHelp();
			exit(0);
		}
	}
	else {
		m_threads.push_back(Process::create(argv[argn]));
		return true;
	}

	return false;
}

void Scheduler::printVersion() {

}

void Scheduler::printHelp() {

}
