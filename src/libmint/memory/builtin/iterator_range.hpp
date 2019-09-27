#ifndef ITERATOR_RANGE_HPP
#define ITERATOR_RANGE_HPP

#include "memory/builtin/iterator.h"
#include "memory/functiontool.h"
#include "system/assert.h"

class range_data : public mint::Iterator::ctx_type::data {
public:
	class iterator : public mint::Iterator::ctx_type::iterator::data {
	public:
		iterator(double value, bool ascending) :
			m_data(mint::create_number(value)),
			m_value(value),
			m_ascending(ascending) {

		}

		bool compare(data *other) const override {
			if (iterator *it = dynamic_cast<iterator *>(other)) {
				return static_cast<long>(m_value) != static_cast<long>(it->m_value);
			}
			return true;
		}

		mint::Iterator::ctx_type::value_type &get() override {
			return m_data;
		}

		data *next() override {
			m_data = mint::create_number(m_ascending ? ++m_value : --m_value);
			return new iterator(m_value, m_ascending);
		}

	private:
		mint::SharedReference m_data;
		double m_value;
		bool m_ascending;
	};

	range_data(double begin, double end) :
		m_begin(begin), m_end(end),
		m_front(mint::create_number(begin)), m_back(mint::create_number(end - 1)) {

	}

	mint::Iterator::ctx_type::type getType() override {
		return mint::Iterator::ctx_type::range;
	}

	mint::Iterator::ctx_type::iterator::data *begin() override {
		return new iterator(m_begin, m_begin < m_end);
	}

	mint::Iterator::ctx_type::iterator::data *end() override {
		return new iterator(m_end, m_begin < m_end);
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
		m_begin < m_end ? m_begin++ : m_begin--;
		m_front = mint::create_number(m_begin);
	}

	void pop_back() override {
		m_begin < m_end ? m_end-- : m_end++;
		m_back = mint::create_number(m_begin < m_end ? m_end - 1 : m_end + 1);
	}

	void clear() override {
		m_begin = m_end = 0;
	}

	size_t size() const override {
		return m_begin < m_end ? static_cast<size_t>(m_end - m_begin) : static_cast<size_t>(m_begin - m_end);
	}

	bool empty() const override {
		return static_cast<long>(m_begin) == static_cast<long>(m_end);
	}

private:
	double m_begin;
	double m_end;

	mint::SharedReference m_front;
	mint::SharedReference m_back;
};

#endif // ITERATOR_RANGE_HPP
