#ifndef ITERATOR_ITEMS_H
#define ITERATOR_ITEMS_H

#include "iterator_p.h"

namespace _mint_iterator {

struct item {
	item(mint::Iterator::ctx_type::value_type &&value) :
		value(std::move(value)) {}

	mint::WeakReference value;
	item *prev = nullptr;
	item *next = nullptr;
};

class items_iterator : public data_iterator {
public:
	items_iterator(item *impl);

	mint::Iterator::ctx_type::value_type &get() override;
	bool compare(data_iterator *other) const override;
	data_iterator *copy() override;
	void next() override;

private:
	item *m_impl;
};

class items_data : public data {
public:
	items_data() = default;
	items_data(mint::Reference &ref);
	items_data(const items_data &other);
	~items_data() override;

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
	static mint::LocalPool<item> g_pool;
	item *m_head = nullptr;
	item *m_tail = nullptr;
};

}

#endif // ITERATOR_ITEMS_H