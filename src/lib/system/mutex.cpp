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

#include <mint/memory/functiontool.h>
#include <mint/memory/operatortool.h>
#include <mint/memory/casttool.h>
#include <mint/scheduler/processor.h>

#include <mutex>

using namespace mint;

struct AbstractMutex {
	enum Type {
		NORMAL,
		RECURSIVE
	};

	AbstractMutex() = default;
	AbstractMutex(const AbstractMutex &) = delete;
	AbstractMutex(AbstractMutex &&) = delete;
	virtual ~AbstractMutex() = default;

	AbstractMutex &operator=(const AbstractMutex &) = delete;
	AbstractMutex &operator=(AbstractMutex &&) = delete;

	[[nodiscard]] virtual Type type() const = 0;
};

struct Mutex : public AbstractMutex {
	[[nodiscard]] Type type() const override {
		return NORMAL;
	}

	std::mutex handle;
};

struct RecursiveMutex : public AbstractMutex {
	[[nodiscard]] Type type() const override {
		return RECURSIVE;
	}

	std::recursive_mutex handle;
};

namespace symbols {

static const Symbol System("System");
static const Symbol Mutex("Mutex");
static const Symbol Type("Type");

static const Symbol Normal("Normal");
static const Symbol Recursive("Recursive");

}

MINT_FUNCTION(mint_mutex_create, 1, cursor) {

	FunctionHelper helper(cursor, 1);

	WeakReference type = std::move(helper.pop_parameter());

	switch (static_cast<AbstractMutex::Type>(static_cast<int>(to_integer(cursor, type)))) {
	case AbstractMutex::NORMAL:
		helper.return_value(create_object(new Mutex));
		break;
	case AbstractMutex::RECURSIVE:
		helper.return_value(create_object(new RecursiveMutex));
		break;
	}
}

MINT_FUNCTION(mint_mutex_delete, 1, cursor) {

	FunctionHelper helper(cursor, 1);

	WeakReference self = std::move(helper.pop_parameter());

	delete self.data<LibObject<AbstractMutex>>()->impl;
}

MINT_FUNCTION(mint_mutex_get_type, 1, cursor) {

	FunctionHelper helper(cursor, 1);

	WeakReference self = std::move(helper.pop_parameter());

	switch (self.data<LibObject<AbstractMutex>>()->impl->type()) {
	case AbstractMutex::NORMAL:
		helper.return_value(
			helper.reference(symbols::System).member(symbols::Mutex).member(symbols::Type).member(symbols::Normal));
		break;
	case AbstractMutex::RECURSIVE:
		helper.return_value(
			helper.reference(symbols::System).member(symbols::Mutex).member(symbols::Type).member(symbols::Recursive));
		break;
	}
}

MINT_FUNCTION(mint_mutex_lock, 1, cursor) {

	FunctionHelper helper(cursor, 1);

	WeakReference self = std::move(helper.pop_parameter());

	unlock_processor();

	switch (self.data<LibObject<AbstractMutex>>()->impl->type()) {
	case AbstractMutex::NORMAL:
		self.data<LibObject<Mutex>>()->impl->handle.lock();
		break;
	case AbstractMutex::RECURSIVE:
		self.data<LibObject<RecursiveMutex>>()->impl->handle.lock();
		break;
	}

	lock_processor();
}

MINT_FUNCTION(mint_mutex_unlock, 1, cursor) {

	FunctionHelper helper(cursor, 1);

	WeakReference self = std::move(helper.pop_parameter());

	switch (self.data<LibObject<AbstractMutex>>()->impl->type()) {
	case AbstractMutex::NORMAL:
		self.data<LibObject<Mutex>>()->impl->handle.unlock();
		break;
	case AbstractMutex::RECURSIVE:
		self.data<LibObject<RecursiveMutex>>()->impl->handle.unlock();
		break;
	}
}

MINT_FUNCTION(mint_mutex_try_lock, 1, cursor) {

	FunctionHelper helper(cursor, 1);

	WeakReference self = std::move(helper.pop_parameter());

	switch (self.data<LibObject<AbstractMutex>>()->impl->type()) {
	case AbstractMutex::NORMAL:
		helper.return_value(create_boolean(self.data<LibObject<Mutex>>()->impl->handle.try_lock()));
		break;
	case AbstractMutex::RECURSIVE:
		helper.return_value(create_boolean(self.data<LibObject<RecursiveMutex>>()->impl->handle.try_lock()));
		break;
	}
}
