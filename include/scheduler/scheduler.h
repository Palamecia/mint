#ifndef MINT_SCHEDULER_H
#define MINT_SCHEDULER_H

#include "ast/abstractsyntaxtree.h"
#include "scheduler/process.h"

#include <unordered_map>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <list>

namespace mint {

class DebugInterface;
struct Object;

class MINT_EXPORT Scheduler {
public:
	Scheduler(int argc, char **argv);
	~Scheduler();

	static Scheduler *instance();

	AbstractSyntaxTree *ast();
	Process *currentProcess();

	void setDebugInterface(DebugInterface *debugInterface);

	int createThread(Process *process);
	void finishThread(Process *process);
	void joinThread(Process *process);
	Process *findThread(int id) const;

	void createDestructor(Object *object, Reference &&member, Class *owner) noexcept;
	void createException(Reference &&reference);
	void createGenerator(std::unique_ptr<SavedState> state);

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

	std::unordered_map<int, std::thread *> m_threadHandlers;
	std::list<Process *> m_threadStack;
	std::atomic_int m_nextThreadsId;
	mutable std::mutex m_mutex;

	std::list<Process *> m_configuredProcess;
	AbstractSyntaxTree *m_ast;
	bool m_readingArgs;

	DebugInterface *m_debugInterface;

	std::atomic_bool m_running;
	std::atomic_int m_status;
};

}

#endif // MINT_SCHEDULER_H
