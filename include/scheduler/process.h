#ifndef MINT_PROCESS_H
#define MINT_PROCESS_H

#include "ast/cursor.h"

namespace mint {

class DebugInterface;

class MINT_EXPORT Process {
public:
	Process(Cursor *cursor);
	virtual ~Process();

	static Process *fromFile(AbstractSyntaxTree *ast, const std::string &file);
	static Process *fromBuffer(AbstractSyntaxTree *ast, const std::string &buffer);
	static Process *fromStandardInput(AbstractSyntaxTree *ast);

	void parseArgument(const std::string &arg);

	virtual void setup();
	virtual void cleanup();
	virtual bool collectOnExit() const;

	bool exec();
	bool debug(DebugInterface *debugInterface);
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

#endif // MINT_PROCESS_H
