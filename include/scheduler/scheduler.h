#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "scheduler/process.h"
#include "ast/abstractsyntaxtree.h"

#include <list>

class Scheduler {
public:
	Scheduler(int argc, char **argv);
	~Scheduler();

	static Scheduler *instance();

	AbstractSyntaxTree *ast();

	size_t createThread(Process *thread);
	int run();
	void exit(int status);

	bool isOver() const;

protected:
	bool parseArguments(int argc, char **argv);
	bool parseArgument(int argc, int &argn, char **argv);

	void printVersion();
	void printHelp();

private:
	static Scheduler *g_instance;

	AbstractSyntaxTree m_ast;

	std::list<Process *> m_threads;
	bool m_readingArgs;
	bool m_running;
	int m_status;
};

#endif // SCHEDULER_H
