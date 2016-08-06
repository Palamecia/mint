#ifndef ABSTRACT_SYNTAX_TREE_H
#define ABSTRACT_SYNTAX_TREE_H

#include "module.h"
#include "Memory/symboltable.h"
#include "Memory/reference.h"
#include "System/printer.h"

#include <functional>
#include <stack>

typedef unsigned int uint;

struct Context {
    SymbolTable symbols;
	std::stack<Printer *> printers;
	Module *module;
	size_t iptr;
};

struct RetiveContext {
	size_t stackSize;
	size_t callStackSize;
	size_t waitingCallsCount;
	size_t retriveOffset;
};

class Call {
public:
	Call(Reference *ref);
	Call(const SharedReference &ref);

	void setMember(bool member);

	Reference &get();
	bool isMember() const;

private:
	SharedReference m_ref;
	bool m_isMember;
};

class AbstractSynatxTree {
public:
	AbstractSynatxTree();
	~AbstractSynatxTree();

	typedef std::function<void(AbstractSynatxTree *)> Builtin;

	Instruction &next();
	void jmp(size_t pos);
	void call(int module, size_t pos);
	void exit_call();

	void openPrinter(Printer *printer);
	void closePrinter();

	std::vector<SharedReference> &stack();
	std::stack<Call> &waitingCalls();
	SymbolTable &symbols();
	Printer *printer();

	Module::Context createModule();
	void loadModule(const std::string &module);
	bool exitModule();

	void setRetrivePoint(size_t offset);
	void unsetRetivePoint();
	void raise(SharedReference exception);

	static std::pair<int, int> createBuiltinMethode(int type, Builtin methode);

private:
	static std::map<int, std::map<int, Builtin>> g_builtinMembers;

	std::vector<SharedReference> m_stack;
	std::stack<Call> m_waitingCalls;
	std::stack<Context *> m_callStack;
	Context *m_currentCtx;

	std::stack<RetiveContext> m_retrivePoints;
};

#endif // ABSTRACT_SYNTAX_TREE_H
