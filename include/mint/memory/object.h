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

#ifndef MINT_MEMORY_OBJECT_H
#define MINT_MEMORY_OBJECT_H

#include "mint/ast/saved_state.h"
#include "mint/config.h"
#include "mint/ast/symbol.h"
#include "mint/ast/module.h"
#include "mint/memory/data.h"
#include "mint/memory/reference.h"
#include "mint/memory/memory_pool.h"

#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mint {

class Class;
class Cursor;
class PackageData;
class SymbolTable;

class MINT_EXPORT Number : public Data {
	template<typename Type>
	friend class LocalPool;
	friend class GarbageCollector;
public:
	Number() = delete;
	explicit Number(double value);
	explicit Number(std::intmax_t value);
	explicit Number(std::uintmax_t value);

	[[nodiscard]] Format format() const override {
		return Format::number;
	}

	double value;

private:
	static LocalPool<Number> g_pool;
};

MINT_EXPORT bool is_integer(const Reference& ref);

class MINT_EXPORT Boolean : public Data {
	template<typename Type>
	friend class LocalPool;
	friend class GarbageCollector;
public:
	Boolean() = delete;
	explicit Boolean(bool value);

	[[nodiscard]] Format format() const override {
		return Format::boolean;
	}

	bool value;

private:
	static LocalPool<Boolean> g_pool;
};

class MINT_EXPORT Object : public Data {
	template<typename Type>
	friend class LocalPool;
	friend class GarbageCollector;
public:
	explicit Object(Class& type);
	Object(Object&&) = delete;
	Object(const Object&) = delete;
	~Object() override;

	Object& operator=(Object&&) = delete;
	Object& operator=(const Object&) = delete;

	[[nodiscard]] Format format() const override {
		return Format::object;
	}

	Class& metadata;
	Reference* data = nullptr;

	void construct();
	void construct(const Object& other);
	void destroy();

	void mark() override;

private:
	void construct(const Object& other, std::unordered_map<const Data*, Data*>& memory_map);

	static std::allocator<Reference> g_allocator;
	static LocalPool<Object> g_pool;
};

class MINT_EXPORT Package : public Data {
	template<typename Type>
	friend class LocalPool;
	friend class GarbageCollector;
public:
	explicit Package(PackageData& package);
	Package(const Package&) = delete;
	Package(Package&&) = delete;
	~Package() = default;

	Package& operator=(const Package&) = delete;
	Package& operator=(Package&&) = delete;

	[[nodiscard]] Format format() const override {
		return Format::package;
	}

	PackageData& data;

private:
	static LocalPool<Package> g_pool;
};

class MINT_EXPORT Function : public Data {
	template<typename Type>
	friend class LocalPool;
	friend class GarbageCollector;
public:
	class MINT_EXPORT Context {
	public:
		Context(FunctionHandle& handle) :
		    _handle(handle) {}

		Context(Context&& other) = default;
		Context(const Context& other) = default;
		virtual ~Context() = default;

		Context& operator=(Context&&) = default;
		Context& operator=(const Context&) = default;

		[[nodiscard]] const FunctionHandle& handle() const {
			return _handle;
		}

		[[nodiscard]] FunctionHandle& handle() {
			return _handle;
		}

		virtual void call(int signature, Class* metadata, Cursor& cursor) = 0;
		virtual void mark() = 0;

		[[nodiscard]] virtual std::unique_ptr<Context> clone() const = 0;

	private:
		std::reference_wrapper<FunctionHandle> _handle;
	};

	class MINT_EXPORT Stateless : public Context {
	public:
		Stateless(FunctionHandle& handle) :
		    Context(handle) {}

		void call(int signature, Class* metadata, Cursor& cursor) override;

		void mark() override {}

		[[nodiscard]] std::unique_ptr<Context> clone() const override {
			return std::make_unique<Stateless>(*this);
		}
	};

	class MINT_EXPORT Stateful : public Context {
	public:
		Stateful(FunctionHandle& handle) :
		    Context(handle) {}

		void capture(const Symbol& symbol, const Reference& reference) {
			_capture.emplace(symbol, reference);
		}

		void call(int signature, Class* metadata, Cursor& cursor) override;

		void mark() override {
			for (const auto& reference : _capture) {
				reference.second.data().mark();
			}
		}

		[[nodiscard]] std::unique_ptr<Context> clone() const override {
			return std::make_unique<Stateful>(*this);
		}

	private:
		std::unordered_map<Symbol, Reference> _capture;
	};

	class MINT_EXPORT Signature {
		std::unique_ptr<Context> _context;
	public:
		Signature(std::unique_ptr<Stateless>&& context) :
		    _context(std::move(context)) {}

		Signature(std::unique_ptr<Stateful>&& context) :
		    _context(std::move(context)) {}

		Signature(const Signature& other) :
		    _context(other._context->clone()) {}

		Signature(Signature&&) = default;
		~Signature() = default;

		Signature& operator=(const Signature& other) {
			if (this == &other) {
				return *this;
			}
			_context = other._context->clone();
			return *this;
		}

		Signature& operator=(Signature&&) = default;

		[[nodiscard]] const FunctionHandle& handle() const {
			return _context->handle();
		}

		[[nodiscard]] FunctionHandle& handle() {
			return _context->handle();
		}

		template<std::derived_from<Context> T>
		T* context() {
			return dynamic_cast<T*>(_context.get());
		}

		void call(int signature, Class* metadata, Cursor& cursor) const {
			_context->call(signature, metadata, cursor);
		}

		void mark() {
			_context->mark();
		}
	};

	class MINT_EXPORT Mapping {
	public:
		using iterator = std::map<int, Signature>::iterator;
		using const_iterator = std::map<int, Signature>::const_iterator;

		Mapping() = default;
		Mapping(int signature, Signature&& handle);
		Mapping(const std::pair<int, Signature>& mapping);
		Mapping(const std::pair<int, FunctionHandle&>& mapping);

		bool operator==(const Mapping& other) const;
		bool operator!=(const Mapping& other) const;

		std::pair<iterator, bool> emplace(int signature, Signature&& handle);
		std::pair<iterator, bool> insert(const std::pair<int, Signature>& signature);
		std::pair<iterator, bool> insert(const std::pair<int, FunctionHandle&>& signature);

		[[nodiscard]] const_iterator lower_bound(int signature) const;
		[[nodiscard]] const_iterator find(int signature) const;

		[[nodiscard]] const_iterator cbegin() const;
		[[nodiscard]] const_iterator begin() const;
		iterator begin();

		[[nodiscard]] const_iterator cend() const;
		[[nodiscard]] const_iterator end() const;
		iterator end();

		[[nodiscard]] bool empty() const;

	private:
		std::map<int, Signature> _signatures;
	};

	Function();
	Function(Mapping mapping);
	Function(int signature, Function::Signature&& handle);
	Function(const std::pair<int, Function::Signature>& mapping);
	Function(const std::pair<int, FunctionHandle&>& mapping);

	[[nodiscard]] Format format() const override {
		return Format::function;
	}

	Mapping mapping;

	void mark() override;

private:
	static LocalPool<Function> g_pool;
};

MINT_EXPORT bool is_stateful_function(const Reference& object);

class MINT_EXPORT Coroutine : public Data {
	template<typename Type>
	friend class LocalPool;
	friend class GarbageCollector;
public:
	enum class State : std::uint8_t {
		ready,
		running,
		waiting,
		completed,
		failed
	};

	Coroutine(std::unique_ptr<SavedState>&& state, std::size_t stack_size);

	[[nodiscard]] Format format() const override {
		return Format::coroutine;
	}

	[[nodiscard]] inline State state() const;
	[[nodiscard]] inline SymbolTable& symbols();

	void call(Cursor& cursor, Reference&& self);
	void await(Cursor& cursor, Reference&& self);

	std::unique_ptr<mint::SavedState> yield(Cursor& cursor);
	void resume(Cursor& cursor, std::unique_ptr<mint::SavedState>&& state);

	void resume(Cursor& cursor, Reference&& value);
	void resume(Cursor& cursor);

	void suspend(Cursor& cursor);
	void raise(Cursor& cursor);
	void exit(Cursor& cursor);

	void mark() override;

private:
	static LocalPool<Coroutine> g_pool;

	struct Context {
		std::size_t stack_size;
	};

	std::unique_ptr<SavedState> _saved_state;
	std::vector<mint::Reference> _stored_stack;
	std::size_t _stack_offset = 0;

	std::unique_ptr<SavedState> _parent_saved_state;
	std::shared_ptr<Context> _context;

	State _state = State::ready;
};

Coroutine::State Coroutine::state() const {
	return _state;
}

SymbolTable& Coroutine::symbols() {
	assert(_saved_state && _saved_state->stack_frame && _saved_state->stack_frame->symbols);
	return *_saved_state->stack_frame->symbols;
}

}

#endif // MINT_MEMORY_OBJECT_H
