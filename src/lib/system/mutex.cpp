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
static const mint::Symbol type("Type");

static const mint::Symbol normal("Normal");
static const mint::Symbol recursive("Recursive");

}

namespace {

struct AbstractMutex {
	enum Type : std::uint8_t {
		normal,
		recursive
	};

	AbstractMutex() = default;
	AbstractMutex(const AbstractMutex&) = delete;
	AbstractMutex(AbstractMutex&&) = delete;
	virtual ~AbstractMutex() = default;

	AbstractMutex& operator=(const AbstractMutex&) = delete;
	AbstractMutex& operator=(AbstractMutex&&) = delete;

	[[nodiscard]] virtual Type type() const = 0;
};

struct Mutex : public AbstractMutex {
	[[nodiscard]] Type type() const override {
		return normal;
	}

	std::mutex handle;
};

struct RecursiveMutex : public AbstractMutex {
	[[nodiscard]] Type type() const override {
		return recursive;
	}

	std::recursive_mutex handle;
};

AbstractMutex::Type to_abstract_mutex_type(mint::Cursor& cursor, const mint::Reference& value) {
	return static_cast<AbstractMutex::Type>(
	    mint::to_integer<std::underlying_type_t<AbstractMutex::Type>>(cursor, value));
}

mint::Reference mint_mutex_create(mint::Cursor& cursor, const mint::Reference& type) {
	switch (to_abstract_mutex_type(cursor, type)) {
	case AbstractMutex::normal:
		return mint::create_c_object(cursor.ast(), new Mutex);
	case AbstractMutex::recursive:
		return mint::create_c_object(cursor.ast(), new RecursiveMutex);
	}
	return {};
}

mint::Reference mint_mutex_delete(mint::Cursor& /*cursor*/, const mint::Reference& self) {
	delete self.data<mint::LibObject<AbstractMutex>>().ptr;
	return {};
}

mint::Reference mint_mutex_get_type(mint::FunctionHelper& helper, const mint::Reference& self) {
	switch (self.data<mint::LibObject<AbstractMutex>>().ptr->type()) {
	case AbstractMutex::normal:
		return helper.reference(symbols::system)
		    .member(symbols::mutex)
		    .member(symbols::type)
		    .member(symbols::normal)
		    .share();
	case AbstractMutex::recursive:
		return helper.reference(symbols::system)
		    .member(symbols::mutex)
		    .member(symbols::type)
		    .member(symbols::recursive)
		    .share();
	}
	return {};
}

mint::Reference mint_mutex_lock(mint::Cursor& /*cursor*/, const mint::Reference& self) {

	mint::unlock_processor();

	switch (self.data<mint::LibObject<AbstractMutex>>().ptr->type()) {
	case AbstractMutex::normal:
		self.data<mint::LibObject<Mutex>>().ptr->handle.lock();
		break;
	case AbstractMutex::recursive:
		self.data<mint::LibObject<RecursiveMutex>>().ptr->handle.lock();
		break;
	}

	mint::lock_processor();
	return {};
}

mint::Reference mint_mutex_unlock(mint::Cursor& /*cursor*/, const mint::Reference& self) {
	switch (self.data<mint::LibObject<AbstractMutex>>().ptr->type()) {
	case AbstractMutex::normal:
		self.data<mint::LibObject<Mutex>>().ptr->handle.unlock();
		break;
	case AbstractMutex::recursive:
		self.data<mint::LibObject<RecursiveMutex>>().ptr->handle.unlock();
		break;
	}
	return {};
}

mint::Reference mint_mutex_try_lock(mint::Cursor& /*cursor*/, const mint::Reference& self) {
	switch (self.data<mint::LibObject<AbstractMutex>>().ptr->type()) {
	case AbstractMutex::normal:
		return mint::create_boolean(self.data<mint::LibObject<Mutex>>().ptr->handle.try_lock());
	case AbstractMutex::recursive:
		return mint::create_boolean(self.data<mint::LibObject<RecursiveMutex>>().ptr->handle.try_lock());
	}
	return {};
}

}

MINT_EXPORT_FUNCTION(mint_mutex_create, 1)
MINT_EXPORT_FUNCTION(mint_mutex_delete, 1)
MINT_EXPORT_FUNCTION(mint_mutex_get_type, 1)
MINT_EXPORT_FUNCTION(mint_mutex_lock, 1)
MINT_EXPORT_FUNCTION(mint_mutex_unlock, 1)
MINT_EXPORT_FUNCTION(mint_mutex_try_lock, 1)
