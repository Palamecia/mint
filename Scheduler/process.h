#ifndef PROCESS_H
#define PROCESS_H

#include "AbstractSyntaxTree/abstractsyntaxtree.h"

class Process {
public:
    Process();

	static Process *create(const std::string &file);
	static Process *readInput(Process *process = nullptr);

	bool exec(uint nbStep);
	bool isOver();

private:
    AbstractSynatxTree m_ast;
	bool m_endless;
};

#endif // PROCESS_H
