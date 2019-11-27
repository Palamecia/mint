#ifndef DEBUG_INTERFACE_H
#define DEBUG_INTERFACE_H

#include "debug/lineinfo.h"

#include <atomic>
#include <mutex>
#include <map>
#include <set>

namespace mint {

class Cursor;
class CursorDebugger;

class MINT_EXPORT DebugInterface {
public:
	DebugInterface();
	virtual ~DebugInterface();

	void declareThread(int id);
	void removeThread(int id);

	bool debug(Cursor *cursor);
	void exit(Cursor *cursor);

	void doRun();
	void doPause();
	void doNext();
	void doEnter();
	void doReturn();

protected:
	virtual bool check(CursorDebugger *cursor) = 0;

	void createBreackpoint(const LineInfo &info);
	void removeBreackpoint(const LineInfo &info);
	LineInfoList listBreakpoints() const;

private:
	enum State {
		debugger_run,
		debugger_pause,
		debugger_next,
		debugger_enter,
		debugger_return
	};

	struct ThreadContext {
		size_t lineNumber;
		size_t callDepth;
		State state;
	};

	ThreadContext *getThreadContext() const;

	std::recursive_mutex m_mutex;
	std::atomic<bool> m_running;

	std::map<int, ThreadContext *> m_threads;
	std::map<std::string, std::set<size_t>> m_breackpoints;
};

}

#endif // DEBUG_INTERFACE_H
