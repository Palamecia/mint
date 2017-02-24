#ifndef PROCESS_H
#define PROCESS_H

#include "ast/abstractsyntaxtree.h"

class Process {
public:
	Process();

	static Process *create(const std::string &file);
	static Process *fromStandardInput();


	void parseArgument(const std::string &arg);

	bool exec(size_t maxStep);
	bool resume();

private:
	AbstractSyntaxTree m_ast;
	bool m_endless;
};

#endif // PROCESS_H
