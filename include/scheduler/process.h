#ifndef PROCESS_H
#define PROCESS_H

#include "ast/cursor.h"

class Process {
public:
	Process(Cursor *cursor);

	static Process *fromFile(AbstractSyntaxTree *ast, const std::string &file);
	static Process *fromBuffer(AbstractSyntaxTree *ast, const std::string &buffer);
	static Process *fromStandardInput(AbstractSyntaxTree *ast);

	void parseArgument(const std::string &arg);

	bool exec(size_t maxStep);
	bool resume();

	Cursor *cursor();

private:
	Cursor *m_cursor;
	bool m_endless;
};

#endif // PROCESS_H
