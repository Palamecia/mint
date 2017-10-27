#ifndef CURSOR_H
#define CURSOR_H

#include "ast/node.h"
#include "memory/symboltable.h"
#include "memory/reference.h"
#include "system/printer.h"

#include <vector>
#include <stack>

class Module;
class AbstractSyntaxTree;

class Cursor {
public:
	~Cursor();

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

	Node &next();
	void jmp(size_t pos);
	bool call(int module, size_t pos);
	void exitCall();
	bool callInProgress() const;

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

	void retrive();
	void dump();

protected:
	Cursor(AbstractSyntaxTree *ast, Module *module);
	friend class AbstractSyntaxTree;

	struct Context {
		SymbolTable symbols;
		std::stack<Printer *> printers;
		Module *module;
		size_t iptr;
	};

	struct RetivePoint {
		size_t stackSize;
		size_t callStackSize;
		size_t waitingCallsCount;
		size_t retriveOffset;
	};

private:
	AbstractSyntaxTree *m_ast;

	std::vector<SharedReference> m_stack;
	std::stack<Call> m_waitingCalls;
	std::stack<Context *> m_callStack;
	Context *m_currentCtx;

	std::stack<RetivePoint> m_retrivePoints;
};

#endif // CURSOR_H
