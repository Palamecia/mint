/**
 * Copyright (c) 2026 Gauvain CHERY.
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

#ifndef MINT_AST_CURSOR_H
#define MINT_AST_CURSOR_H

#include "mint/ast/module.h"
#include "mint/ast/node.h"
#include "mint/ast/printer.h"
#include "mint/config.h"
#include "mint/debug/lineinfo.h"
#include "mint/memory/garbagecollector.h"
#include "mint/memory/reference.h"
#include "mint/memory/symboltable.h"
#include "mint/system/poolallocator.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <stack>

namespace mint {

struct SavedState;
class AbstractSyntaxTree;

class MINT_EXPORT Cursor {
	friend class CursorDebugger;
	friend struct SavedState;
public:
	class MINT_EXPORT Call {
	public:
		using Flags = std::uint8_t;
		static constexpr Flags standard_call = 0x00;
		static constexpr Flags member_call = 0x01;
		static constexpr Flags operator_call = 0x02;

		Call(const Reference& function, Class* metadata = nullptr);
		Call(const Reference& function, Class& metadata);
		Call(Reference&& function, Class* metadata = nullptr);
		Call(Reference&& function, Class& metadata);

		[[nodiscard]] Flags get_flags() const;
		void set_flags(Flags flags);

		[[nodiscard]] Class* get_metadata() const;
		void set_metadata(Class* metadata);
		void set_metadata(Class& metadata);

		[[nodiscard]] int extra_argument_count() const;
		void add_extra_argument(std::size_t count);

		Reference& function();

	private:
		WeakReference _function;
		Class* _metadata = nullptr;
		int _extra_args = 0;
		Flags _flags = standard_call;
	};

	class MINT_EXPORT WaitingCallStack : public MemoryRoot {
		std::vector<Call> _calls;
	public:
		WaitingCallStack();
		WaitingCallStack(const WaitingCallStack&) = delete;
		WaitingCallStack(WaitingCallStack&&) = delete;
		~WaitingCallStack();

		WaitingCallStack& operator=(const WaitingCallStack&) = delete;
		WaitingCallStack& operator=(WaitingCallStack&&) = delete;

		[[nodiscard]] bool empty() const noexcept {
			return _calls.empty();
		}

		[[nodiscard]] std::size_t size() const noexcept {
			return _calls.size();
		}

		[[nodiscard]] Call& top() noexcept {
			return _calls.back();
		}

		[[nodiscard]] const Call& top() const noexcept {
			return _calls.back();
		}

		void push(const Call& call) {
			_calls.push_back(call);
		}

		void push(Call&& call) {
			_calls.push_back(std::move(call));
		}

		template<class... Args>
		auto emplace(Args&&... args) {
			return _calls.emplace_back(std::forward<Args>(args)...);
		}

		void pop() {
			_calls.pop_back();
		}

		void swap(WaitingCallStack& other) noexcept {
			std::swap(_calls, other._calls);
		}

		void mark() override;
	};

	Cursor(AbstractSyntaxTree& ast, Module& module, Cursor* parent = nullptr);
	Cursor(AbstractSyntaxTree& ast, Cursor* parent = nullptr);
	Cursor(Cursor&& other) = delete;
	Cursor(const Cursor& other) = delete;
	~Cursor();

	Cursor& operator=(Cursor&& other) = delete;
	Cursor& operator=(const Cursor& other) = delete;

	std::unique_ptr<Cursor> make_thread();
	[[nodiscard]] bool is_thread() const;

	[[nodiscard]] inline const AbstractSyntaxTree& ast() const;
	[[nodiscard]] inline AbstractSyntaxTree& ast();
	[[nodiscard]] inline Cursor* parent() const;

	inline const Node& next();
	void jmp(std::size_t pos);

	[[nodiscard]] bool call_in_progress() const;
	void call_generator_expression(std::size_t offset);
	void call_async_generator_expression(std::size_t offset);
	void call(const Module::Handle& handle, int signature, Class* metadata = nullptr);
	void call(const Module& module, std::size_t pos, PackageData& package, Class* metadata = nullptr);
	void exit_call();

	[[nodiscard]] bool is_in_builtin() const;
	[[nodiscard]] bool is_in_generator() const;
	[[nodiscard]] bool is_in_coroutine() const;
	std::unique_ptr<SavedState> suspend(std::unique_ptr<SavedState> state, std::size_t stack_offset = 0);
	std::unique_ptr<SavedState> interrupt(std::size_t stack_offset = 0);
	void restore(std::unique_ptr<SavedState> state, std::size_t stack_offset = 0);
	void destroy(SavedState* state);

	void open_printer(std::unique_ptr<Printer>&& printer);
	void close_printer();
	Printer* printer();

	[[nodiscard]] inline std::vector<WeakReference>& stack();
	[[nodiscard]] inline WaitingCallStack& waiting_calls();
	[[nodiscard]] inline const SymbolTable& symbols() const;
	[[nodiscard]] inline SymbolTable& symbols();
	[[nodiscard]] inline Reference& generator();
	[[nodiscard]] inline Reference& coroutine();

	bool load_module(const std::string& module);
	bool exit_module();

	void set_retrieve_point(std::size_t offset);
	void unset_retrieve_point();
	void raise(WeakReference&& exception);

	void resume();
	void retrieve();
	[[nodiscard]] LineInfoList dump() const;
	[[nodiscard]] std::size_t offset() const;

	void cleanup();

protected:
	struct StackFrame {
		explicit StackFrame(const Module& module);

		std::size_t iptr = 0;
		const Module& module;

		WaitingCallStack waiting_calls;
		std::shared_ptr<SymbolTable> symbols;
		std::unique_ptr<WeakReference> generator;
		std::unique_ptr<WeakReference> coroutine;
		std::vector<std::unique_ptr<Printer>> printers;
	};

	struct RetrievePoint {
		std::size_t stack_size;
		std::size_t call_stack_size;
		std::size_t retrieve_offset;
		StackFrame* current_stack_frame;
	};

private:
	using retrieve_point_stack_t = std::stack<RetrievePoint, std::vector<RetrievePoint>>;
	static PoolAllocator<StackFrame> g_pool;

	std::vector<WeakReference>* _stack;
	StackFrame* _current_stack_frame;

	std::reference_wrapper<AbstractSyntaxTree> _ast;
	Cursor* _parent;
	Cursor* _child;

	std::vector<StackFrame*> _call_stack;
	retrieve_point_stack_t _retrieve_points;
};

inline std::size_t get_stack_base(Cursor& cursor) {
	return cursor.stack().size() - 1;
}

inline WeakReference&& move_from_stack(Cursor& cursor, std::size_t index) {
	return std::move(cursor.stack()[index]);
}

inline WeakReference& load_from_stack(Cursor& cursor, std::size_t index) {
	return cursor.stack()[index];
}

const AbstractSyntaxTree& Cursor::ast() const {
	return _ast;
}

AbstractSyntaxTree& Cursor::ast() {
	return _ast;
}

Cursor* Cursor::parent() const {
	return _parent;
}

const Node& Cursor::next() {
	assert(_current_stack_frame->iptr <= _current_stack_frame->module.end());
	return _current_stack_frame->module.node_at(_current_stack_frame->iptr++);
}

std::vector<WeakReference>& Cursor::stack() {
	return *_stack;
}

Cursor::WaitingCallStack& Cursor::waiting_calls() {
	return _current_stack_frame->waiting_calls;
}

const SymbolTable& Cursor::symbols() const {
	assert(_current_stack_frame->symbols);
	return *_current_stack_frame->symbols;
}

SymbolTable& Cursor::symbols() {
	assert(_current_stack_frame->symbols);
	return *_current_stack_frame->symbols;
}

Reference& Cursor::generator() {
	assert(_current_stack_frame->generator);
	return *_current_stack_frame->generator;
}

Reference& Cursor::coroutine() {
	assert(_current_stack_frame->coroutine);
	return *_current_stack_frame->coroutine;
}

}

#endif // MINT_AST_CURSOR_H
