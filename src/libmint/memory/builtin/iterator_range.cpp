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

#include "iterator_range.h"
#include "iterator_p.h"
#include "memory/object.h"
#include "memory/reference.h"

#include <cmath>

using namespace mint::internal;
using namespace mint;

namespace {

constexpr RangeFunctions RANGE_DATA_ASCENDING_FUNCTIONS = {
	[](double current) {
		return current + 1;
	},
	[](double begin, double end) {
		return static_cast<size_t>(end - begin);
	},
};

constexpr RangeFunctions RANGE_DATA_DESCENDING_FUNCTIONS = {
	[](double current) {
		return current - 1;
	},
	[](double begin, double end) {
		return static_cast<size_t>(begin - end);
	},
};

WeakReference creat_item(double value) {
	return {Reference::DEFAULT, GarbageCollector::instance().alloc<Number>(value)};
}

}

RangeIteratorData::RangeIteratorData(double begin, double end) :
	m_head(creat_item(begin)),
	m_tail(creat_item(end - 1)),
	m_func(begin < end ? &RANGE_DATA_ASCENDING_FUNCTIONS : &RANGE_DATA_DESCENDING_FUNCTIONS) {}

RangeIteratorData::RangeIteratorData(const RangeIteratorData &other) :
	m_head(creat_item(other.m_head.data<Number>()->value)),
	m_tail(creat_item(other.m_tail.data<Number>()->value)),
	m_func(other.m_func) {}

mint::internal::IteratorData *RangeIteratorData::copy() {
	return new RangeIteratorData(*this);
}

void RangeIteratorData::mark() {
	m_head.data()->mark();
	m_tail.data()->mark();
}

Iterator::Context::Type RangeIteratorData::get_type() const {
	return Iterator::Context::RANGE;
}

Iterator::Context::value_type &RangeIteratorData::value() {
	return m_head;
}

Iterator::Context::value_type &RangeIteratorData::last() {
	return m_tail;
}

size_t RangeIteratorData::capacity() const {
	return 2;
}

void RangeIteratorData::reserve([[maybe_unused]] size_t capacity) {
	assert(false);
}

void RangeIteratorData::yield([[maybe_unused]] Iterator::Context::value_type &&value) {
	assert(false);
}

void RangeIteratorData::next() {
	m_head = creat_item(m_func->inc(m_head.data<Number>()->value));
}

void RangeIteratorData::finalize() {}

void RangeIteratorData::clear() {
	m_head = WeakReference::share(m_tail);
}

size_t RangeIteratorData::size() const {
	return m_func->size(m_head.data<Number>()->value, m_tail.data<Number>()->value + 1);
}

bool RangeIteratorData::empty() const {
	return fabs(m_head.data<Number>()->value - (m_tail.data<Number>()->value + 1)) < 1.;
}
