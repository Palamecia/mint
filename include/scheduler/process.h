#ifndef PROCESS_H
#define PROCESS_H

#include "ast/cursor.h"

namespace mint {

class DebugInterface;

class MINT_EXPORT Process {
public:
	Process(Cursor *cursor);
	virtual ~Process();

	static Process *fromFile(const std::string &file);
	static Process *fromBuffer(const std::string &buffer);
	static Process *fromStandardInput();
	static void cleanupAll();

	void parseArgument(const std::string &arg);

	virtual void setup();
	virtual void cleanup();

	bool exec();
	bool debug(DebugInterface *interface);
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
	Cursor *m_cursor;
	bool m_endless;

	int m_threadId;
	int m_errorHandler;
};

}

#endif // PROCESS_H
