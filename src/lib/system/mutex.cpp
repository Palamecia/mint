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

#include "mint/ast/symbol.h"
#include "mint/memory/builtin/libobject.h"
#include "mint/memory/function_tools.h"
#include "mint/memory/operator_tools.h"
#include "mint/memory/cast_tools.h"
#include "mint/memory/reference.h"
#include "mint/scheduler/processor.h"

#include <cstdint>
#include <mutex>
#include <type_traits>

namespace symbols {

static const mint::Symbol system("System");
static const mint::Symbol mutex("Mutex");
static const mint::Symbol kind("Kind");

static const mint::Symbol normal("Normal");
static const mint::Symbol recursive("Recursive");

}

namespace {

class AbstractMutex {
public:
	enum class Kind : std::uint8_t {
		normal,
		recursive
	};

	AbstractMutex() = default;
	AbstractMutex(const AbstractMutex&) = delete;
	AbstractMutex(AbstractMutex&&) = delete;
	virtual ~AbstractMutex() = default;

	AbstractMutex& operator=(const AbstractMutex&) = delete;
	AbstractMutex& operator=(AbstractMutex&&) = delete;

	[[nodiscard]] virtual Kind kind() const = 0;

	virtual void lock() = 0;
	virtual bool try_lock() noexcept = 0;
	virtual void unlock() noexcept = 0;
};

class Mutex : public AbstractMutex {
	std::mutex _handle;
public:
	[[nodiscard]] Kind kind() const override {
		return Kind::normal;
	}

	void lock() override {
		_handle.lock();
	}

	bool try_lock() noexcept override {
		return _handle.try_lock();
	}

	void unlock() noexcept override {
		_handle.unlock();
	}
};

class RecursiveMutex : public AbstractMutex {
	std::recursive_mutex _handle;
public:
	[[nodiscard]] Kind kind() const override {
		return Kind::recursive;
	}

	void lock() override {
		_handle.lock();
	}

	bool try_lock() noexcept override {
		return _handle.try_lock();
	}

	void unlock() noexcept override {
		_handle.unlock();
	}
};

AbstractMutex::Kind to_abstract_mutex_kind(mint::Cursor& cursor, const mint::Reference& value) {
	return static_cast<AbstractMutex::Kind>(
	    mint::to_integer<std::underlying_type_t<AbstractMutex::Kind>>(cursor, value));
}

mint::Reference mint_mutex_create(mint::Cursor& cursor, const mint::Reference& kind) {
	switch (to_abstract_mutex_kind(cursor, kind)) {
	case AbstractMutex::Kind::normal:
		return mint::create_c_object<AbstractMutex>(cursor.ast(), new Mutex);
	case AbstractMutex::Kind::recursive:
		return mint::create_c_object<AbstractMutex>(cursor.ast(), new RecursiveMutex);
	}
	return {};
}

mint::Reference mint_mutex_delete(mint::Cursor& /*cursor*/, const mint::Reference& self) {
	delete self.data<mint::LibObject<AbstractMutex>>().ptr;
	return {};
}

mint::Reference mint_mutex_get_kind(mint::FunctionHelper& helper, const mint::Reference& self) {
	switch (self.data<mint::LibObject<AbstractMutex>>().ptr->kind()) {
	case AbstractMutex::Kind::normal:
		return helper.reference(symbols::system)
		    .member(symbols::mutex)
		    .member(symbols::kind)
		    .member(symbols::normal)
		    .share();
	case AbstractMutex::Kind::recursive:
		return helper.reference(symbols::system)
		    .member(symbols::mutex)
		    .member(symbols::kind)
		    .member(symbols::recursive)
		    .share();
	}
	return {};
}

mint::Reference mint_mutex_lock(mint::Cursor& /*cursor*/, const mint::Reference& self) {
	{
		const auto _ = mint::ProcessorUnlocker();
		self.data<mint::LibObject<AbstractMutex>>().ptr->lock();
	}
	return {};
}

mint::Reference mint_mutex_unlock(mint::Cursor& /*cursor*/, const mint::Reference& self) {
	self.data<mint::LibObject<AbstractMutex>>().ptr->unlock();
	return {};
}

mint::Reference mint_mutex_try_lock(mint::Cursor& /*cursor*/, const mint::Reference& self) {
	return mint::create_boolean(self.data<mint::LibObject<AbstractMutex>>().ptr->try_lock());
}

}

MINT_EXPORT_FUNCTION(mint_mutex_create, 1)
MINT_EXPORT_FUNCTION(mint_mutex_delete, 1)
MINT_EXPORT_FUNCTION(mint_mutex_get_kind, 1)
MINT_EXPORT_FUNCTION(mint_mutex_lock, 1)
MINT_EXPORT_FUNCTION(mint_mutex_unlock, 1)
MINT_EXPORT_FUNCTION(mint_mutex_try_lock, 1)
