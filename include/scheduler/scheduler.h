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
	Process *currentProcess();

	size_t createThread(Process *thread);
	void exit(int status);
	int run();

	bool isRunning() const;

protected:
	bool parseArguments(int argc, char **argv);
	bool parseArgument(int argc, int &argn, char **argv);

	void printVersion();
	void printHelp();

	void beginThreadUpdate(Process *thread);
	void endThreadUpdate();
	bool schedule(Process *thread);
	bool resume(Process *thread);

private:
	static Scheduler *g_instance;

	AbstractSyntaxTree m_ast;

	size_t m_nextThreadsId;
	std::list<Process *> m_threads;
	std::stack<Process *> m_currentProcess;

	bool m_readingArgs;
	size_t m_quantum;
	bool m_running;
	int m_status;
};

#endif // SCHEDULER_H
