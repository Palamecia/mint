/**
 * Copyright (c) 2025 Gauvain CHERY.
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
#include "mint/scheduler/scheduler.h"

#include <algorithm>
#include <iterator>

using namespace mint::internal;
using namespace mint;

GeneratorData::GeneratorData(size_t stack_size) :
	m_state(nullptr),
	m_stack_size(stack_size) {}

GeneratorData::GeneratorData(const GeneratorData &other) :
	ItemsIteratorData(other),
	m_state(nullptr),
	m_stack_size(other.m_stack_size) {}

mint::internal::IteratorData *GeneratorData::copy() {
	GeneratorData::finalize();
	return new GeneratorData(*this);
}

void GeneratorData::mark() {
	ItemsIteratorData::mark();
	for (const Reference &item : m_stored_stack) {
		item.data()->mark();
	}
}

Iterator::Context::Type GeneratorData::get_type() const {
	return Iterator::Context::GENERATOR;
}

Iterator::Context::value_type &GeneratorData::last() {
	GeneratorData::finalize();
	return ItemsIteratorData::last();
}

void GeneratorData::yield(Iterator::Context::value_type &&value) {

	ItemsIteratorData::yield(std::move(value));

	switch (m_execution_mode) {
	case SINGLE_PASS:
		break;

	case INTERRUPTIBLE:
		Cursor *cursor = Scheduler::instance()->current_process()->cursor();
		move(std::next(cursor->stack().begin(), static_cast<std::vector<WeakReference>::difference_type>(m_stack_size)),
			 cursor->stack().end(), back_inserter(m_stored_stack));
		cursor->stack().resize(m_stack_size);
		m_state = cursor->interrupt();
		break;
	}
}

void GeneratorData::next() {

	ItemsIteratorData::next();

	if (m_state) {
		Cursor *cursor = Scheduler::instance()->current_process()->cursor();
		m_stack_size = cursor->stack().size();
		move(m_stored_stack.begin(), m_stored_stack.end(), back_inserter(cursor->stack()));
		m_stored_stack.clear();
		if (cursor->is_in_builtin()) {
			Scheduler::instance()->create_generator(std::move(m_state));
		}
		else {
			cursor->restore(std::move(m_state));
		}
	}
}

void GeneratorData::finalize() {
	if (m_state) {
		m_execution_mode = SINGLE_PASS;
		Cursor *cursor = Scheduler::instance()->current_process()->cursor();
		move(m_stored_stack.begin(), m_stored_stack.end(), back_inserter(cursor->stack()));
		m_stored_stack.clear();
		Scheduler::instance()->create_generator(std::move(m_state));
	}
}
