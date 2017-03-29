#ifndef PROCESS_H
#define PROCESS_H

#include "ast/abstractsyntaxtree.h"

class Process {
public:
	Process(size_t moduleId);

	static Process *fromFile(const std::string &file);
	static Process *fromBuffer(const std::string &buffer);
	static Process *fromStandardInput();

	void parseArgument(const std::string &arg);

	bool exec(size_t maxStep);
	bool resume();

private:
	AbstractSyntaxTree m_ast;
	bool m_endless;
};

#endif // PROCESS_H
