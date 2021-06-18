#ifndef ITERATOR_RANGE_HPP
#define ITERATOR_RANGE_HPP

#include "memory/builtin/iterator.h"
#include "memory/functiontool.h"
#include "system/assert.h"

#include <math.h>

struct RangeFunctions {
	double (*inc)(double &current);
	double (*dec)(double &current);
	size_t (*size)(double begin, double end);
};

static double range_data_ascending_inc(double &current) {
	return ++current;
}

static double range_data_ascending_dec(double &current) {
	return (--current) - 1;
}

static size_t range_data_ascending_size(double begin, double end) {
	return static_cast<size_t>(end - begin);
}

static RangeFunctions g_range_data_ascending_functions = {
	range_data_ascending_inc,
	range_data_ascending_dec,
	range_data_ascending_size
};

static double range_data_descending_inc(double &current) {
	return --current;
}

static double range_data_descending_dec(double &current) {
	return (++current) + 1;
}

static size_t range_data_descending_size(double begin, double end) {
	return static_cast<size_t>(begin - end);
}

static RangeFunctions g_range_data_descending_functions = {
	range_data_descending_inc,
	range_data_descending_dec,
	range_data_descending_size
};

class range_data : public mint::Iterator::ctx_type::data {
public:
	class iterator : public mint::Iterator::ctx_type::iterator::data {
	public:
		iterator(double value, RangeFunctions *func) :
			m_value(value),
			m_data(mint::create_number(value)),
			m_func(func) {

		}

		bool compare(data *other) const override {
			if (iterator *it = dynamic_cast<iterator *>(other)) {
				return fabs(m_value - it->m_value) < 1.;
			}
			return true;
		}

		mint::Iterator::ctx_type::value_type &get() override {
			return m_data;
		}

		data *next() override {
			m_data = mint::create_number(m_func->inc(m_value));
			return new iterator(m_value, m_func);
		}

	private:
		double m_value;
		mint::SharedReference m_data;
		RangeFunctions *m_func;
	};

	range_data(double begin, double end) :
		m_begin(begin), m_end(end),
		m_front(mint::create_number(begin)), m_back(mint::create_number(end - 1)),
		m_func(m_begin < m_end ? &g_range_data_ascending_functions : &g_range_data_descending_functions) {

	}

	void mark() override {
		m_front->data()->mark();
		m_back->data()->mark();
	}

	mint::Iterator::ctx_type::type getType() override {
		return mint::Iterator::ctx_type::range;
	}

	mint::Iterator::ctx_type::iterator::data *begin() override {
		return new iterator(m_begin, m_func);
	}

	mint::Iterator::ctx_type::iterator::data *end() override {
		return new iterator(m_end, m_func);
	}

	mint::Iterator::ctx_type::value_type &front() override {
		return m_front;
	}

	mint::Iterator::ctx_type::value_type &back() override {
		return m_back;
	}

	void emplace_front(mint::Iterator::ctx_type::value_type &value) override {
		((void)value);
		assert(false);
	}

	void emplace_back(mint::Iterator::ctx_type::value_type &value) override {
		((void)value);
		assert(false);
	}

	void pop_front() override {
		m_front = mint::create_number(m_func->inc(m_begin));
	}

	void pop_back() override {
		m_back = mint::create_number(m_func->dec(m_end));
	}

	void clear() override {
		m_begin = m_end = 0;
	}

	size_t size() const override {
		return m_func->size(m_begin, m_end);
	}

	bool empty() const override {
		return fabs(m_begin - m_end) < 1.;
	}

private:
	double m_begin;
	double m_end;

	mint::SharedReference m_front;
	mint::SharedReference m_back;

	RangeFunctions *m_func;
};

#endif // ITERATOR_RANGE_HPP
