#ifndef PROCESS_H
#define PROCESS_H

#include "ast/cursor.h"

namespace mint {

class MINT_EXPORT Process {
public:
	enum State {
		state_new,
		state_runnable,
		state_blocked,
		state_waiting,
		state_timed_waiting,
		state_terminetad
	};

	Process(Cursor *cursor);
	virtual ~Process();

	static Process *fromFile(const std::string &file);
	static Process *fromBuffer(const std::string &buffer);
	static Process *fromStandardInput();

	void parseArgument(const std::string &arg);

	virtual void setup();
	virtual void cleanup();

	bool exec(size_t maxStep);
	bool resume();

	void wait();
	void sleep(uint64_t msec);

	void setThreadId(int id);
	int getThreadId() const;

	Cursor *cursor();

protected:
	void setEndless(bool endless);
	void installErrorHandler();
	void dump();

private:
	State m_state;
	Cursor *m_cursor;
	bool m_endless;

	int m_threadId;
	int m_errorHandler;
};

}

#endif // PROCESS_H
