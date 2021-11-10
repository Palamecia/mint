#ifndef ITERATOR_RANGE_H
#define ITERATOR_RANGE_H

#include "iterator_p.h"

namespace _mint_iterator {

struct RangeFunctions {
	double (*inc)(double &current);
	double (*dec)(double &current);
	size_t (*size)(double begin, double end);
};

class range_iterator : public data_iterator {
public:
	range_iterator(double value, RangeFunctions *func);

	mint::Iterator::ctx_type::value_type &get() override;
	bool compare(data_iterator *other) const override;
	data_iterator *copy() override;
	void next() override;

private:
	double m_value;
	mint::StrongReference m_data;
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

	mint::Iterator::ctx_type::value_type &front() override;
	mint::Iterator::ctx_type::value_type &back() override;

	void emplace_front(mint::Iterator::ctx_type::value_type &&value) override;
	void emplace_back(mint::Iterator::ctx_type::value_type &&value) override;

	void pop_front() override;
	void pop_back() override;

	void finalize() override;
	void clear() override;

	size_t size() const override;
	bool empty() const override;

private:
	double m_begin;
	double m_end;

	mint::WeakReference m_front;
	mint::WeakReference m_back;

	RangeFunctions *m_func;
};

}

#endif // ITERATOR_RANGE_HPP
