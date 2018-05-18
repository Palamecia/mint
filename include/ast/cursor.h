#ifndef CURSOR_H
#define CURSOR_H

#include "ast/node.h"
#include "memory/symboltable.h"
#include "memory/reference.h"
#include "system/printer.h"

#include <vector>
#include <stack>

namespace mint {

class Module;
class AbstractSyntaxTree;

class MINT_EXPORT Cursor {
public:
	~Cursor();

	class Call {
	public:
		Call(Reference *ref);
		Call(const SharedReference &ref);

		void setMember(bool member);
		bool isMember() const;

		void setMetadata(Class *metadata);
		Class *getMetadata() const;

		Reference &function();

	private:
		SharedReference m_ref;
		Class *m_metadata;
		bool m_member;
	};

	Node &next();
	void jmp(size_t pos);
	bool call(int module, size_t pos, Class *metadata = nullptr);
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

	void setRetrievePoint(size_t offset);
	void unsetRetrievePoint();
	void raise(SharedReference exception);

	void resume();
	void retrieve();
	void dump();

protected:
	Cursor(AbstractSyntaxTree *ast, Module *module);
	friend class AbstractSyntaxTree;

	struct Context {
		SymbolTable symbols;
		std::stack<Printer *> printers;
		Module *module;
		size_t iptr;

		Context(Module *module, Class *metadata = nullptr);
		~Context();
	};

	struct RetrievePoint {
		size_t stackSize;
		size_t callStackSize;
		size_t waitingCallsCount;
		size_t retrieveOffset;
	};

private:
	AbstractSyntaxTree *m_ast;

	std::vector<SharedReference> m_stack;
	std::stack<Call> m_waitingCalls;
	std::stack<Context *> m_callStack;
	Context *m_currentCtx;

	std::stack<RetrievePoint> m_retrievePoints;
};

}

#endif // CURSOR_H
