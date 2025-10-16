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

#include "iterator_range.h"
#include "iterator_p.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/memory/object.h"
#include "mint/memory/reference.h"

#include <cassert>
#include <cmath>
#include <cstddef>
#include <memory>
#include <utility>

using namespace mint::internal;
using namespace mint;

namespace {

constexpr RangeFunctions range_data_ascending_functions = {
    .inc =
        [](double current) {
	        return current + 1;
        },
    .dec =
        [](double current) {
	        return current - 1;
        },
    .size =
        [](double begin, double end) {
	        return static_cast<std::size_t>(end - begin);
        },
};

constexpr RangeFunctions range_data_descending_functions = {
    .inc =
        [](double current) {
	        return current - 1;
        },
    .dec =
        [](double current) {
	        return current + 1;
        },
    .size =
        [](double begin, double end) {
	        return static_cast<std::size_t>(begin - end);
        },
};

WeakReference creat_item(double value) {
	return make_weak_reference<Number>(Reference::default_flags, value);
}

}

RangeIteratorViewData::RangeIteratorViewData(const RangeFunctions& func, mint::WeakReference& head,
    mint::WeakReference& tail) :
    _func(func),
    _head(head),
    _tail(tail),
    _cur(head) {}

mint::Iterator::Context::reference RangeIteratorViewData::front() {
	return _head;
}

mint::Iterator::Context::reference RangeIteratorViewData::back() {
	return _tail;
}

mint::Iterator::Context::reference RangeIteratorViewData::get() {
	return _cur;
}

bool RangeIteratorViewData::empty() const {
	return fabs(_cur.data<Number>().value - (_tail.data<Number>().value + 1)) < 1.;
}

void RangeIteratorViewData::prev() {
	_cur = creat_item(_func.get().dec(_head.data<Number>().value));
}

void RangeIteratorViewData::next() {
	_cur = creat_item(_func.get().inc(_head.data<Number>().value));
}

RangeIteratorData::RangeIteratorData(double begin, double end) :
    _func(begin < end ? range_data_ascending_functions : range_data_descending_functions),
    _head(creat_item(begin)),
    _tail(creat_item(end - 1)) {}

RangeIteratorData::RangeIteratorData(const RangeIteratorData& other) :
    _func(other._func),
    _head(creat_item(other._head.data<Number>().value)),
    _tail(creat_item(other._tail.data<Number>().value)) {}

std::unique_ptr<IteratorViewData> RangeIteratorData::view() {
	return std::make_unique<RangeIteratorViewData>(_func.get(), _head, _tail);
}

std::unique_ptr<IteratorData> RangeIteratorData::copy() {
	return std::make_unique<RangeIteratorData>(*this);
}

void RangeIteratorData::mark() {
	_head.data().mark();
	_tail.data().mark();
}

Iterator::Context::Type RangeIteratorData::get_type() const {
	return Iterator::Context::range;
}

Iterator::Context::value_type& RangeIteratorData::get() {
	return _head;
}

std::size_t RangeIteratorData::capacity() const {
	return 2;
}

void RangeIteratorData::reserve(std::size_t /*capacity*/) {
	assert(false);
}

void RangeIteratorData::yield(Iterator::Context::value_type&& value) {
	const auto consumer = WeakReference(std::move(value));
	assert(false);
}

void RangeIteratorData::next() {
	_head = creat_item(_func.get().inc(_head.data<Number>().value));
}

void RangeIteratorData::finalize() {}

void RangeIteratorData::clear() {
	_head = _tail;
}

std::size_t RangeIteratorData::size() const {
	return _func.get().size(_head.data<Number>().value, _tail.data<Number>().value + 1);
}

bool RangeIteratorData::empty() const {
	return fabs(_head.data<Number>().value - (_tail.data<Number>().value + 1)) < 1.;
}
