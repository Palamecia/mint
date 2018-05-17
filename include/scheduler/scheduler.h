#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "scheduler/process.h"
#include "ast/abstractsyntaxtree.h"

#include <functional>
#include <thread>
#include <atomic>
#include <list>
#include <map>

namespace mint {

class MINT_EXPORT Scheduler {
public:
	static constexpr const size_t quantum = 64 * 1024;

	Scheduler(int argc, char **argv);
	~Scheduler();

	static Scheduler *instance();

	AbstractSyntaxTree *ast();
	Process *currentProcess();

	int createThread(Process *process);
	void finishThread(Process *process);
	Process *findThread(int id) const;

	void exit(int status);
	int run();

	bool isRunning() const;

protected:
	bool parseArguments(int argc, char **argv);
	bool parseArgument(int argc, int &argn, char **argv);

	void printVersion();
	void printHelp();

	void finalizeThreads();

	bool schedule(Process *thread);
	bool resume(Process *thread);
	void finalize();

private:
	static Scheduler *g_instance;
	AbstractSyntaxTree m_ast;

	static thread_local Process *g_currentProcess;
	std::map<int, std::thread *> m_threadHandlers;
	std::list<Process *> m_threadStack;
	std::atomic_int m_nextThreadsId;
	mutable std::mutex m_mutex;

	std::list<Process *> m_configuredProcess;
	bool m_readingArgs;

	std::atomic_bool m_running;
	std::atomic_int m_status;
};

}

#endif // SCHEDULER_H
