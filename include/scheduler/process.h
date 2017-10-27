#ifndef PROCESS_H
#define PROCESS_H

#include "ast/cursor.h"

class Process {
public:
	Process(Cursor *cursor);
	virtual ~Process();

	static Process *fromFile(AbstractSyntaxTree *ast, const std::string &file);
	static Process *fromBuffer(AbstractSyntaxTree *ast, const std::string &buffer);
	static Process *fromStandardInput(AbstractSyntaxTree *ast);

	void parseArgument(const std::string &arg);

	bool exec(size_t maxStep);
	bool resume();

	void setThreadId(int id);

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

#endif // PROCESS_H
