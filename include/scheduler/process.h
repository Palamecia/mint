#ifndef PROCESS_H
#define PROCESS_H

#include "ast/abstractsyntaxtree.h"

class Process {
public:
	Process();

	static Process *create(const std::string &file);
	static Process *readInput(Process *process = nullptr);

	void parseArgument(const std::string &arg);

	bool exec(size_t nbStep);
	bool isOver();

private:
	AbstractSynatxTree m_ast;
	bool m_endless;
};

#endif // PROCESS_H
