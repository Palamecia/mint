#ifndef MINT_CURSOR_H
#define MINT_CURSOR_H

#include "ast/node.h"
#include "ast/module.h"
#include "ast/printer.h"
#include "memory/symboltable.h"
#include "memory/reference.h"
#include "system/poolallocator.hpp"
#include "system/assert.h"
#include "debug/lineinfo.h"

#include <memory>
#include <vector>
#include <stack>

namespace mint {

struct SavedState;
class AbstractSyntaxTree;

class MINT_EXPORT Cursor {
public:
	enum ExecutionMode {
		single_pass,
		interruptible,
		resumed
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
		Call(Reference &function);
		Call(Reference &&function);

		Call &operator =(Call &&other);

		Flags getFlags() const;
		void setFlags(Flags flags);

		Class *getMetadata() const;
		void setMetadata(Class *metadata);

		int extraArgumentCount() const;
		void addExtraArgument();

		Reference &function();

	private:
		StrongReference m_function;
		Class *m_metadata = nullptr;
		int m_extraArgs = 0;
		Flags m_flags = standard_call;
	};

	using waiting_call_stack_t = std::stack<Call, std::vector<Call>>;

	Cursor() = delete;
	Cursor(const Cursor &other) = delete;
	~Cursor();

	Cursor &operator =(const Cursor &other) = delete;

	AbstractSyntaxTree *ast() const;
	Cursor *parent() const;

	inline Node &next();
	void jmp(size_t pos);
	void call(Module::Handle *handle, int signature, Class *metadata = nullptr);
	void call(Module *module, size_t pos, PackageData *package, Class *metadata = nullptr);
	void exitCall();
	bool callInProgress() const;

	ExecutionMode executionMode() const;
	void setExecutionMode(ExecutionMode mode);

	bool isInBuiltin() const;
	bool isInGenerator() const;
	std::unique_ptr<SavedState> interrupt();
	void restore(std::unique_ptr<SavedState> state);
	void destroy(SavedState *state);

	void openPrinter(Printer *printer);
	void closePrinter();

	inline std::vector<WeakReference> &stack();
	inline waiting_call_stack_t &waitingCalls();
	inline SymbolTable &symbols();
	inline Reference &generator();
	Printer *printer();

	void loadModule(const std::string &module);
	bool exitModule();

	void setRetrievePoint(size_t offset);
	void unsetRetrievePoint();
	void raise(WeakReference exception);

	void resume();
	void retrieve();
	LineInfoList dump();
	size_t offset() const;

	void cleanup();

protected:
	Cursor(AbstractSyntaxTree *ast, Module *module, Cursor *parent = nullptr);
	friend class AbstractSyntaxTree;
	friend class CursorDebugger;
	friend struct SavedState;

	struct Context {
		Context(Module *module);
		~Context();

		ExecutionMode executionMode = Cursor::single_pass;
		std::vector<Printer *> printers;
		SymbolTable *symbols = nullptr;
		Reference *generator = nullptr;
		Module *module = nullptr;
		size_t iptr = 0;
	};

	struct RetrievePoint {
		size_t stackSize;
		size_t callStackSize;
		size_t waitingCallsCount;
		size_t retrieveOffset;
	};

private:
	using retrieve_point_stack_t = std::stack<RetrievePoint, std::vector<RetrievePoint>>;
	static pool_allocator<Context> g_pool;

	AbstractSyntaxTree *m_ast;
	Cursor *m_parent;
	Cursor *m_child;

	std::vector<WeakReference> *m_stack;
	waiting_call_stack_t m_waitingCalls;
	std::vector<Context *> m_callStack;
	Context *m_currentContext;

	retrieve_point_stack_t m_retrievePoints;
};

inline size_t get_stack_base(Cursor *cursor) { return cursor->stack().size() - 1; }
inline WeakReference &&move_from_stack(Cursor *cursor, size_t index) { return std::move(cursor->stack()[index]); }
inline WeakReference &load_from_stack(Cursor *cursor, size_t index) { return cursor->stack()[index]; }

Node &Cursor::next() {
	assert(m_currentContext->iptr <= m_currentContext->module->end());
	return m_currentContext->module->at(m_currentContext->iptr++);
}

std::vector<WeakReference> &Cursor::stack() { return *m_stack; }
Cursor::waiting_call_stack_t &Cursor::waitingCalls() { return m_waitingCalls; }

SymbolTable &Cursor::symbols() {
	assert(m_currentContext->symbols);
	return *m_currentContext->symbols;
}

Reference &Cursor::generator() {
	assert(m_currentContext->generator);
	return *m_currentContext->generator;
}

}

#endif // MINT_CURSOR_H
