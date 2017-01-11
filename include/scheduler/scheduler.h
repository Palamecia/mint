#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "scheduler/process.h"

#include <list>

class Scheduler {
public:
	Scheduler(int argc, char **argv);
	~Scheduler();

	static Scheduler *instance();

	int run();
	void exit(int status);

	bool isOver() const;

protected:
	void parseArguments(int argc, char **argv);
	bool parseArgument(int argc, int &argn, char **argv);

	void printVersion();
	void printHelp();

private:
	static Scheduler *g_instance;

	std::list<Process *> m_threads;
	bool m_running;
	int m_status;
};

#endif // SCHEDULER_H
