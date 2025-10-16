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

#include "iterator_generator.h"
#include "iterator_items.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/memory/reference.h"
#include "mint/scheduler/scheduler.h"
#include "mint/system/assert.h"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <utility>
#include <vector>

using namespace mint::internal;
using namespace mint;

GeneratorData::GeneratorData(std::size_t stack_size) :
    _state(nullptr),
    _stack_size(stack_size) {}

GeneratorData::GeneratorData(const GeneratorData& other) :
    ItemsIteratorData(other),
    _state(nullptr),
    _stack_size(other._stack_size) {}

std::unique_ptr<IteratorData> GeneratorData::copy() {
	GeneratorData::finalize();
	return std::make_unique<GeneratorData>(*this);
}

void GeneratorData::mark() {
	ItemsIteratorData::mark();
	for (const Reference& item : _stored_stack) {
		item.data().mark();
	}
}

Iterator::Context::Type GeneratorData::get_type() const {
	return Iterator::Context::generator;
}

void GeneratorData::yield(Iterator::Context::value_type&& value) {

	ItemsIteratorData::yield(std::move(value));

	switch (_execution_mode) {
	case single_pass:
		break;

	case interruptible:
		Cursor& cursor = mint::Scheduler::current_process()->cursor();
		std::move(std::next(cursor.stack().begin(),
		              static_cast<std::vector<WeakReference>::difference_type>(_stack_size)),
		    cursor.stack().end(), back_inserter(_stored_stack));
		cursor.stack().resize(_stack_size);
		_state = cursor.interrupt();
		break;
	}
}

void GeneratorData::next() {

	ItemsIteratorData::next();

	if (_state) {
		Cursor& cursor = mint::Scheduler::current_process()->cursor();
		_stack_size = cursor.stack().size();
		std::ranges::move(_stored_stack, back_inserter(cursor.stack()));
		_stored_stack.clear();
		if (cursor.is_in_builtin()) {
			Scheduler::instance()->create_generator(std::move(_state));
		}
		else {
			cursor.restore(std::move(_state));
		}
	}
}

void GeneratorData::finalize() {
	if (_state) {
		_execution_mode = single_pass;
		auto* scheduler = Scheduler::instance();
		assert_x(scheduler, __func__, "execution should be done using a scheduler");
		Cursor& cursor = mint::Scheduler::current_process()->cursor();
		std::ranges::move(_stored_stack, back_inserter(cursor.stack()));
		_stored_stack.clear();
		scheduler->create_generator(std::move(_state));
	}
}
