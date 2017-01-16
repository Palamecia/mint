#ifndef ABSTRACT_SYNTAX_TREE_H
#define ABSTRACT_SYNTAX_TREE_H

#include "module.h"
#include "memory/symboltable.h"
#include "memory/reference.h"
#include "system/printer.h"

#include <functional>
#include <stack>

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

	Reference &function();
	bool isMember() const;

private:
	SharedReference m_ref;
	bool m_member;
};

class AbstractSynatxTree {
public:
	AbstractSynatxTree(size_t rootModuleId = Module::main().moduleId);
	~AbstractSynatxTree();

	typedef size_t CallHandler;
	typedef std::function<void(AbstractSynatxTree *)> Builtin;

	Instruction &next();
	void jmp(size_t pos);
	bool call(int module, size_t pos);
	void exitCall();

	void openPrinter(Printer *printer);
	void closePrinter();

	std::vector<SharedReference> &stack();
	std::stack<Call> &waitingCalls();
	SymbolTable &symbols();
	Printer *printer();

	void loadModule(const std::string &module);
	bool exitModule();

	void setRetrivePoint(size_t offset);
	void unsetRetivePoint();
	void raise(SharedReference exception);

	CallHandler getCallHandler() const;
	bool callInProgress(CallHandler handler) const;

	static std::pair<int, int> createBuiltinMethode(int type, Builtin methode);

protected:
	void dumpCallStack();

private:
	static std::map<int, std::map<int, Builtin>> g_builtinMembers;

	std::vector<SharedReference> m_stack;
	std::stack<Call> m_waitingCalls;
	std::stack<Context *> m_callStack;
	Context *m_currentCtx;

	std::stack<RetiveContext> m_retrivePoints;
	int m_callbackId;
};

#endif // ABSTRACT_SYNTAX_TREE_H
