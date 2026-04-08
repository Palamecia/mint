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

#include "iterator_asyncgenerator.h"
#include "iterator_items.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/memory/object.h"
#include "mint/scheduler/scheduler.h"
#include "mint/system/assert.h"

#include <cstddef>
#include <memory>
#include <ranges>
#include <utility>

using namespace mint::internal;
using namespace mint;

AsyncGeneratorData::AsyncGeneratorData(Coroutine& coroutine, std::size_t stack_size) :
    _coroutine(coroutine),
    _stack_size(stack_size) {}

AsyncGeneratorData::AsyncGeneratorData(const AsyncGeneratorData& other) :
    ItemsIteratorData(other),
    _coroutine(other._coroutine),
    _stack_size(other._stack_size) {}

std::unique_ptr<IteratorData> AsyncGeneratorData::copy() {
	AsyncGeneratorData::finalize(Scheduler::current_process()->cursor());
	return std::make_unique<AsyncGeneratorData>(*this);
}

void AsyncGeneratorData::mark() {
	ItemsIteratorData::mark();
	_coroutine.get().mark();
	for (auto& ref : _stored_stack) {
		ref.data().mark();
	}
}

Iterator::Context::Type AsyncGeneratorData::get_type() const {
	return Iterator::Context::Type::generator;
}

void AsyncGeneratorData::yield(Cursor& cursor, Iterator::Context::value_type&& value, Iterator::ResumeKind resume_kind) {

	ItemsIteratorData::yield(cursor, std::move(value), resume_kind);

	switch (resume_kind) {
	case Iterator::ResumeKind::next:
		_stored_stack.append_range(std::views::drop(cursor.stack(), static_cast<std::ptrdiff_t>(_stack_size)));
		cursor.stack().resize(_stack_size);
		_state = _coroutine.get().yield(cursor);
		break;
	case Iterator::ResumeKind::close:
	case Iterator::ResumeKind::raise:
		break;
	}
}

void AsyncGeneratorData::next(Cursor& cursor) {

	ItemsIteratorData::next(cursor);

	if (_state) {
		_coroutine.get().resume(cursor, std::move(_state));
		_stack_size = cursor.stack().size();
		cursor.stack().append_range(std::move(_stored_stack));
		_stored_stack.clear();
	}
}

void AsyncGeneratorData::finalize(Cursor& /*cursor*/) {
	assert_x(!_state, __func__, "cannot finalize an async generator that is currently suspended");
}
