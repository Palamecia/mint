#include "iterator_range.h"

#include "system/assert.h"

#include <cmath>

using namespace _mint_iterator;
using namespace mint;

static RangeFunctions g_range_data_ascending_functions = {
	[] (double &current) { return ++current; },
	[] (double &current) { return (--current) - 1; },
	[] (double begin, double end) { return static_cast<size_t>(end - begin); }
};

static RangeFunctions g_range_data_descending_functions = {
	[] (double &current) { return --current; },
	[] (double &current) { return (++current) + 1; },
	[] (double begin, double end) { return static_cast<size_t>(begin - end); }
};

static WeakReference creat_item(double value) {
	return WeakReference(Reference::standard, Reference::alloc<Number>(value));
}

range_iterator::range_iterator(double value, RangeFunctions *func) :
	m_value(value),
	m_data(creat_item(value)),
	m_func(func) {

}

Iterator::ctx_type::value_type &range_iterator::get() {
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
	m_front(creat_item(begin)), m_back(creat_item(end - 1)),
	m_func(m_begin <= m_end ? &g_range_data_ascending_functions : &g_range_data_descending_functions) {

}

range_data::range_data(const range_data &other) :
	m_begin(other.m_begin), m_end(other.m_end),
	m_front(creat_item(other.m_begin)), m_back(creat_item(other.m_end - 1)),
	m_func(other.m_func) {

}

void range_data::mark() {
	m_front.data()->mark();
	m_back.data()->mark();
}

Iterator::ctx_type::type range_data::getType() {
	return Iterator::ctx_type::range;
}

_mint_iterator::data *range_data::copy() const {
	return new range_data(*this);
}

data_iterator *range_data::begin() {
	return new range_iterator(m_begin, m_func);
}

data_iterator *range_data::end() {
	return new range_iterator(m_end, m_func);
}

Iterator::ctx_type::value_type &range_data::next() {
	return m_front;
}

Iterator::ctx_type::value_type &range_data::back() {
	return m_back;
}

void range_data::emplace_front(Iterator::ctx_type::value_type &&value) {
	((void)value);
	assert(false);
}

void range_data::emplace_next(Iterator::ctx_type::value_type &&value) {
	((void)value);
	assert(false);
}

void range_data::pop_next() {
	m_front = creat_item(m_func->inc(m_begin));
}

void range_data::pop_back() {
	m_back = creat_item(m_func->dec(m_end));
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
