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

#include <cmath>

using namespace mint::internal;
using namespace mint;

static RangeFunctions g_range_data_ascending_functions = {
	[] (double &current) { return ++current; },
	[] (double begin, double end) { return static_cast<size_t>(end - begin); }
};

static RangeFunctions g_range_data_descending_functions = {
	[] (double &current) { return --current; },
	[] (double begin, double end) { return static_cast<size_t>(begin - end); }
};

static WeakReference creat_item(double value) {
	return WeakReference(Reference::DEFAULT, GarbageCollector::instance().alloc<Number>(value));
}

range_iterator::range_iterator(double value, RangeFunctions *func) :
	m_value(value),
	m_data(creat_item(value)),
	m_func(func) {

}

Iterator::ctx_type::value_type &range_iterator::get() const {
	return m_data;
}

bool range_iterator::compare(data_iterator *other) const {
	return fabs(m_value - static_cast<range_iterator *>(other)->m_value) >= 1.;
}

data_iterator *range_iterator::copy() {
	return new range_iterator(m_value, m_func);
}

void range_iterator::next() {
	m_data = creat_item(m_func->inc(m_value));
}

range_data::range_data(double begin, double end) :
	m_begin(begin), m_end(end),
	m_head(creat_item(begin)), m_tail(creat_item(end - 1)),
	m_func(m_begin <= m_end ? &g_range_data_ascending_functions : &g_range_data_descending_functions) {

}

range_data::range_data(const range_data &other) :
	m_begin(other.m_begin), m_end(other.m_end),
	m_head(creat_item(other.m_begin)), m_tail(creat_item(other.m_end - 1)),
	m_func(other.m_func) {

}

void range_data::mark() {
	m_head.data()->mark();
	m_tail.data()->mark();
}

Iterator::ctx_type::type range_data::getType() {
	return Iterator::ctx_type::RANGE;
}

mint::internal::data *range_data::copy() const {
	return new range_data(*this);
}

data_iterator *range_data::begin() {
	return new range_iterator(m_begin, m_func);
}

data_iterator *range_data::end() {
	return new range_iterator(m_end, m_func);
}

Iterator::ctx_type::value_type &range_data::next() {
	return m_head;
}

Iterator::ctx_type::value_type &range_data::back() {
	return m_tail;
}

void range_data::emplace(Iterator::ctx_type::value_type &&value) {
	((void)value);
	assert(false);
}

void range_data::pop() {
	m_head = creat_item(m_func->inc(m_begin));
}

void range_data::finalize() {

}

void range_data::clear() {
	m_begin = m_end = 0;
}

size_t range_data::size() const {
	return m_func->size(m_begin, m_end);
}

bool range_data::empty() const {
	return fabs(m_begin - m_end) < 1.;
}
