#ifndef ABSTRACT_SYNTAX_TREE_H
#define ABSTRACT_SYNTAX_TREE_H

#include "modul.h"
#include "Memory/symboltable.h"
#include "Memory/reference.h"
#include "System/printer.h"

#include <stack>

typedef unsigned int uint;

struct Context {
    SymbolTable symbols;
	std::stack<Printer *> printers;
	Modul *modul;
	size_t iptr;
};

class AbstractSynatxTree {
public:
	AbstractSynatxTree();
	~AbstractSynatxTree();

	Instruction &next();
	void jmp(size_t pos);
	void call(size_t modul, size_t pos);
	void exit_call();

	void openPrinter(Printer *printer);
	void closePrinter();

	std::vector<SharedReference> &stack();
	std::stack<SharedReference> &waitingCalls();
	SymbolTable &symbols();
	Printer *printer();

	Modul::Context createModul();
	void loadModul(const std::string &path);

private:
	static std::vector<Modul *> g_moduls;
	std::vector<SharedReference> m_stack;
	std::stack<SharedReference> m_waitingCalls;
	std::stack<Context *> m_callStack;
	Context *m_currentCtx;
};

#endif // ABSTRACT_SYNTAX_TREE_H
