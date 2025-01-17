#ifndef ITERATOR_ITEMS_H
#define ITERATOR_ITEMS_H

#include "iterator_p.h"
#include <cstddef>

namespace mint::internal {

class ItemsIteratorData : public IteratorData {
public:
	ItemsIteratorData();
	ItemsIteratorData(size_t capacity);
	ItemsIteratorData(mint::Reference &ref);
	ItemsIteratorData(mint::Reference &&ref);
	ItemsIteratorData(ItemsIteratorData &&other) noexcept;
	ItemsIteratorData(const ItemsIteratorData &other);
	~ItemsIteratorData() override;

	ItemsIteratorData &operator=(ItemsIteratorData &&) = delete;
	ItemsIteratorData &operator=(const ItemsIteratorData &) = default;

	[[nodiscard]] IteratorData *copy() override;
	void mark() override;

	[[nodiscard]] mint::Iterator::Context::Type get_type() const override;
	[[nodiscard]] mint::Iterator::Context::value_type &value() override;
	[[nodiscard]] mint::Iterator::Context::value_type &last() override;
	[[nodiscard]] size_t size() const override;
	[[nodiscard]] bool empty() const override;

	[[nodiscard]] size_t capacity() const override;
	void reserve(size_t capacity) override;

	void yield(mint::Iterator::Context::value_type &&value) override;
	void next() override;

	void finalize() override;
	void clear() override;

private:
	void increase_size();

	mint::WeakReference *m_data;
	size_t m_capacity;
	size_t m_size = 0;
	size_t m_pos = 0;
};

}

#endif // ITERATOR_ITEMS_H
