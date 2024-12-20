#ifndef ITERATOR_RANGE_H
#define ITERATOR_RANGE_H

#include "iterator_p.h"

namespace mint::internal {

struct RangeFunctions {
	double (*inc)(double &current);
	size_t (*size)(double begin, double end);
};

class range_iterator : public data_iterator {
public:
	range_iterator(double value, RangeFunctions *func);

	mint::Iterator::ctx_type::value_type &get() const override;
	bool compare(data_iterator *other) const override;
	data_iterator *copy() override;
	void next() override;

private:
	double m_value;
	mutable mint::StrongReference m_data;
	RangeFunctions *m_func;
};

class range_data : public data {
public:
	range_data(double begin, double end);
	range_data(const range_data &other);

	void mark() override;

	mint::Iterator::ctx_type::type getType() override;
	data *copy() const override;

	data_iterator *begin() override;
	data_iterator *end() override;

	mint::Iterator::ctx_type::value_type &next() override;
	mint::Iterator::ctx_type::value_type &back() override;

	void emplace(mint::Iterator::ctx_type::value_type &&value) override;
	void pop() override;

	void finalize() override;
	void clear() override;

	size_t size() const override;
	bool empty() const override;

private:
	double m_begin;
	double m_end;

	mint::WeakReference m_head;
	mint::WeakReference m_tail;

	RangeFunctions *m_func;
};

}

#endif // ITERATOR_RANGE_HPP
