/**
 * Copyright (c) 2024 Gauvain CHERY.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef MINT_CURSOR_H
#define MINT_CURSOR_H

#include "mint/ast/node.h"
#include "mint/ast/module.h"
#include "mint/ast/printer.h"
#include "mint/memory/symboltable.h"
#include "mint/memory/reference.h"
#include "mint/system/poolallocator.hpp"
#include "mint/debug/lineinfo.h"

#include <memory>
#include <vector>
#include <stack>

namespace mint {

struct SavedState;
class AbstractSyntaxTree;

class MINT_EXPORT Cursor {
	friend class AbstractSyntaxTree;
	friend class CursorDebugger;
	friend struct SavedState;
public:
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

		Flags get_flags() const;
		void set_flags(Flags flags);

		Class *get_metadata() const;
		void set_metadata(Class *metadata);

		int extra_argument_count() const;
		void add_extra_argument(size_t count);

		Reference &function();

	private:
		StrongReference m_function;
		Class *m_metadata = nullptr;
		int m_extra_args = 0;
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
	void exit_call();
	bool call_in_progress() const;

	bool is_in_builtin() const;
	bool is_in_generator() const;
	std::unique_ptr<SavedState> interrupt();
	void restore(std::unique_ptr<SavedState> state);
	void destroy(SavedState *state);

	void begin_generator_expression();
	void end_generator_expression();
	void yield_expression(const Reference &ref);

	void open_printer(Printer *printer);
	void close_printer();

	inline std::vector<WeakReference> &stack();
	inline waiting_call_stack_t &waiting_calls();
	inline SymbolTable &symbols();
	inline Reference &generator();
	Printer *printer();

	bool load_module(const std::string &module);
	bool exit_module();

	void set_retrieve_point(size_t offset);
	void unset_retrieve_point();
	void raise(WeakReference exception);

	void resume();
	void retrieve();
	LineInfoList dump();
	size_t offset() const;

	void cleanup();

protected:
	Cursor(AbstractSyntaxTree *ast, Module *module, Cursor *parent = nullptr);

	struct Context {
		Context(Module *module);
		~Context();

		std::vector<StrongReference> gerenator_expression;
		std::vector<Printer *> printers;
		SymbolTable *symbols = nullptr;
		Reference *generator = nullptr;
		Module *module = nullptr;
		size_t iptr = 0;
	};

	struct RetrievePoint {
		size_t stack_size;
		size_t call_stack_size;
		size_t waiting_calls_count;
		size_t retrieve_offset;
	};

private:
	using retrieve_point_stack_t = std::stack<RetrievePoint, std::vector<RetrievePoint>>;
	static pool_allocator<Context> g_pool;

	AbstractSyntaxTree *m_ast;
	Cursor *m_parent;
	Cursor *m_child;

	std::vector<WeakReference> *m_stack;
	waiting_call_stack_t m_waiting_calls;
	std::vector<Context *> m_call_stack;
	Context *m_current_context;

	retrieve_point_stack_t m_retrieve_points;
};

inline size_t get_stack_base(Cursor *cursor) { return cursor->stack().size() - 1; }
inline WeakReference &&move_from_stack(Cursor *cursor, size_t index) { return std::move(cursor->stack()[index]); }
inline WeakReference &load_from_stack(Cursor *cursor, size_t index) { return cursor->stack()[index]; }

Node &Cursor::next() {
	assert(m_current_context->iptr <= m_current_context->module->end());
	return m_current_context->module->at(m_current_context->iptr++);
}

std::vector<WeakReference> &Cursor::stack() { return *m_stack; }
Cursor::waiting_call_stack_t &Cursor::waiting_calls() { return m_waiting_calls; }

SymbolTable &Cursor::symbols() {
	assert(m_current_context->symbols);
	return *m_current_context->symbols;
}

Reference &Cursor::generator() {
	assert(m_current_context->generator);
	return *m_current_context->generator;
}

}

#endif // MINT_CURSOR_H
