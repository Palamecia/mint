#ifndef ITERATOR_ITEMS_HPP
#define ITERATOR_ITEMS_HPP

#include "memory/builtin/iterator.h"

class items_data : public mint::Iterator::ctx_type::data {
public:
	class iterator : public mint::Iterator::ctx_type::iterator::data {
	public:
		iterator(const std::deque<mint::SharedReference>::iterator &impl) : m_impl(impl) {

		}

		bool compare(data *other) const override {
			if (iterator *it = dynamic_cast<iterator *>(other)) {
				return m_impl != it->m_impl;
			}
			return true;
		}

		mint::Iterator::ctx_type::value_type &get() override {
			return *m_impl;
		}

		data *next() override {
			return new iterator(++m_impl);
		}

	private:
		std::deque<mint::SharedReference>::iterator m_impl;
	};

	mint::Iterator::ctx_type::type getType() override {
		return mint::Iterator::ctx_type::items;
	}

	mint::Iterator::ctx_type::iterator::data *begin() override {
		return new iterator(m_items.begin());
	}

	mint::Iterator::ctx_type::iterator::data *end() override {
		return new iterator(m_items.end());
	}

	mint::Iterator::ctx_type::value_type &front() override {
		return m_items.front();
	}

	mint::Iterator::ctx_type::value_type &back() override {
		return m_items.back();
	}

	void emplace_front(mint::Iterator::ctx_type::value_type &value) override {
		m_items.emplace_front(std::move(value));
	}

	void emplace_back(mint::Iterator::ctx_type::value_type &value) override {
		m_items.emplace_back(std::move(value));
	}

	void pop_front() override {
		m_items.pop_front();
	}

	void pop_back() override {
		m_items.pop_back();
	}

	void clear() override {
		m_items.clear();
	}

	size_t size() const override {
		return m_items.size();
	}

	bool empty() const override {
		return m_items.empty();
	}

private:
	std::deque<mint::SharedReference> m_items;
};

#endif // ITERATOR_ITEMS_HPP
