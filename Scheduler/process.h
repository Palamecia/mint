#ifndef PROCESS_H
#define PROCESS_H

#include "AbstractSyntaxTree/abstractsyntaxtree.h"

class Process {
public:
    Process();

	static Process *create(const std::string &file);

	bool exec(uint nbStep);

private:
    AbstractSynatxTree m_ast;
};

#endif // PROCESS_H
