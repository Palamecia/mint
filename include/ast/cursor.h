#ifndef CURSOR_H
#define CURSOR_H

#include "ast/node.h"
#include "memory/symboltable.h"
#include "memory/reference.h"
#include "system/printer.h"
#include "debug/lineinfo.h"

#include <memory>
#include <vector>
#include <stack>

namespace mint {

class Module;
struct SavedState;

class MINT_EXPORT Cursor {
public:
	enum ExecutionMode {
		single_pass,
		interruptible
	};

	class MINT_EXPORT Call {
	public:
		enum Flag {
			standard_call = 0x00,
			member_call = 0x01,
			operator_call = 0x02
		};
		using Flags = int;

		Call(Call &&other);
		Call(SharedReference &&function);

		Call &operator =(Call &&other);

		Flags getFlags() const;
		void setFlags(Flags flags);

		Class *getMetadata() const;
		void setMetadata(Class *metadata);

		int extraArgumentCount() const;
		void addExtraArgument();

		SharedReference &function();

	private:
		SharedReference m_function;
		Class *m_metadata;
		int m_extraArgs;
		Flags m_flags;
	};

	using waiting_call_stack_t = std::stack<Call, std::vector<Call>>;

	Cursor() = delete;
	Cursor(const Cursor &other) = delete;
	~Cursor();

	Cursor &operator =(const Cursor &other) = delete;

	Cursor *parent() const;

	Node &next();
	void jmp(size_t pos);
	bool call(int module, size_t pos, PackageData *package, Class *metadata = nullptr);
	bool call(Module *module, size_t pos, PackageData *package, Class *metadata = nullptr);
	void exitCall();
	bool callInProgress() const;

	ExecutionMode executionMode() const;
	void setExecutionMode(ExecutionMode mode);

	std::unique_ptr<SavedState> interrupt();
	void restore(std::unique_ptr<SavedState> state);

	void openPrinter(Printer *printer);
	void closePrinter();

	std::vector<SharedReference> &stack();
	waiting_call_stack_t &waitingCalls();
	SymbolTable &symbols();
	Printer *printer();

	void loadModule(const std::string &module);
	bool exitModule();

	void setRetrievePoint(size_t offset);
	void unsetRetrievePoint();
	void raise(SharedReference exception);

	void resume();
	void retrieve();
	LineInfoList dump();

protected:
	Cursor(Module *module, Cursor *parent = nullptr);
	friend class AbstractSyntaxTree;
	friend class CursorDebugger;
	friend struct SavedState;

	struct Context {
		Context(Module *module, Class *metadata = nullptr);
		~Context();

		ExecutionMode executionMode;
		SymbolTable symbols;
		std::stack<Printer *> printers;
		Module *module;
		size_t iptr;
	};

	struct RetrievePoint {
		size_t stackSize;
		size_t callStackSize;
		size_t waitingCallsCount;
		size_t retrieveOffset;
	};

private:
	using retrieve_point_stack_t = std::stack<RetrievePoint, std::vector<RetrievePoint>>;

	Cursor *m_parent;
	Cursor *m_child;

	std::vector<SharedReference> m_stack;
	waiting_call_stack_t m_waitingCalls;
	std::vector<Context *> m_callStack;
	Context *m_currentContext;

	retrieve_point_stack_t m_retrievePoints;
};

#define move_from_stack(cursor, index) std::move(cursor->stack()[index])
#define load_from_stack(cursor, index) cursor->stack()[index]

}

#endif // CURSOR_H
