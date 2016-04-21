#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "Scheduler/process.h"

#include <list>

class Scheduler {
public:
    Scheduler(int argc, char **argv);
	~Scheduler();

    int run();

protected:
	void parseArguments(int argc, char **argv);
	bool parseArgument(int argc, int &argn, char **argv);

	void printVersion();
	void printHelp();

private:
	std::list<Process *> m_threads;
};

#endif // SCHEDULER_H
