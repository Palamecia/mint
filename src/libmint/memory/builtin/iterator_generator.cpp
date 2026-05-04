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

#include <cassert>
#include <cstddef>
#include <memory>
#include <ranges>
#include <utility>
#include <vector>

using namespace mint::internal;
using namespace mint;

GeneratorData::GeneratorData(std::size_t stack_size) :
    _stack_size(stack_size) {}

GeneratorData::GeneratorData(const GeneratorData& other) :
    ItemsIteratorData(other),
    _stack_size(other._stack_size) {}

std::unique_ptr<IteratorData> GeneratorData::copy() {
	GeneratorData::finalize(Scheduler::current_process()->cursor());
	return std::make_unique<GeneratorData>(*this);
}

void GeneratorData::mark() {
	ItemsIteratorData::mark();
	if (_state) {
		_state->stack_frame->mark();
	}
	for (const Reference& item : _stored_stack) {
		item.data().mark();
	}
}

Iterator::Context::Type GeneratorData::get_type() const {
	return Iterator::Context::Type::generator;
}

void GeneratorData::yield(Cursor& cursor, Iterator::Context::value_type&& value, Iterator::ResumeKind resume_kind) {

	assert(!_state);

	ItemsIteratorData::yield(cursor, std::move(value), resume_kind);

	switch (_execution_mode) {
	case ExecutionMode::single_pass:
		break;

	case ExecutionMode::interruptible:
		switch (resume_kind) {
		case Iterator::ResumeKind::next:
			_stored_stack.append_range(std::views::drop(cursor.stack(), static_cast<std::ptrdiff_t>(_stack_size)));
			cursor.stack().resize(_stack_size);
			_state = cursor.interrupt();
			break;
		case Iterator::ResumeKind::close:
		case Iterator::ResumeKind::raise:
			break;
		}
		break;
	}
}

void GeneratorData::next(Cursor& cursor) {

	ItemsIteratorData::next(cursor);

	if (_state) {
		_stack_size = cursor.stack().size();
		cursor.stack().append_range(std::move(_stored_stack));
		_stored_stack.clear();
		if (cursor.is_in_builtin()) {
			Scheduler::instance()->create_generator(std::move(_state));
		}
		else {
			cursor.restore(std::move(_state));
		}
	}
}

void GeneratorData::finalize(Cursor& cursor) {
	if (_state) {
		_execution_mode = ExecutionMode::single_pass;
		auto* scheduler = Scheduler::instance();
		assert_x(scheduler, __func__, "execution should be done using a scheduler");
		cursor.stack().append_range(std::move(_stored_stack));
		_stored_stack.clear();
		scheduler->create_generator(std::move(_state));
	}
}
