#ifndef LIBMINT_MEMORY_BUILTIN_ITERATOR_ITEMS_H
#define LIBMINT_MEMORY_BUILTIN_ITERATOR_ITEMS_H

#include "iterator_p.h"
#include "mint/ast/module.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/memory/reference.h"
#include <cstddef>
#include <memory>

namespace mint::internal {

class ItemsIteratorViewData : public IteratorViewData {
	mint::WeakReference* _data;
	std::size_t _capacity;
	std::size_t _size;
	std::size_t _pos;
	std::size_t _cur = 0;
public:
	ItemsIteratorViewData(mint::WeakReference* data, std::size_t capacity, std::size_t size, std::size_t pos);
	ItemsIteratorViewData(ItemsIteratorViewData&&) = delete;
	ItemsIteratorViewData(const ItemsIteratorViewData&) = delete;
	virtual ~ItemsIteratorViewData() = default;

	ItemsIteratorViewData& operator=(ItemsIteratorViewData&&) = delete;
	ItemsIteratorViewData& operator=(const ItemsIteratorViewData&) = delete;

	[[nodiscard]] mint::Iterator::Context::reference front() override;
	[[nodiscard]] mint::Iterator::Context::reference back() override;
	[[nodiscard]] mint::Iterator::Context::reference get() override;
	[[nodiscard]] bool empty() const override;

	void prev() override;
	void next() override;
};

class ItemsIteratorData : public IteratorData {
	static std::allocator<WeakReference> g_allocator;
	mint::WeakReference* _data;
	std::size_t _capacity;
	std::size_t _size = 0;
	std::size_t _pos = 0;
public:
	ItemsIteratorData();
	ItemsIteratorData(std::size_t capacity);
	ItemsIteratorData(AbstractSyntaxTree& ast, const mint::Reference& ref);
	ItemsIteratorData(AbstractSyntaxTree& ast, mint::Reference&& ref);
	ItemsIteratorData(ItemsIteratorData&& other) noexcept;
	ItemsIteratorData(const ItemsIteratorData& other);
	~ItemsIteratorData() override;

	ItemsIteratorData& operator=(ItemsIteratorData&&) = delete;
	ItemsIteratorData& operator=(const ItemsIteratorData&) = default;

	[[nodiscard]] std::unique_ptr<IteratorViewData> view() override;
	[[nodiscard]] std::unique_ptr<IteratorData> copy() override;
	void mark() override;

	[[nodiscard]] mint::Iterator::Context::Type get_type() const override;
	[[nodiscard]] mint::Iterator::Context::value_type& get() override;
	[[nodiscard]] std::size_t size() const override;
	[[nodiscard]] bool empty() const override;

	[[nodiscard]] std::size_t capacity() const override;
	void reserve(std::size_t capacity) override;

	void yield(mint::Iterator::Context::value_type&& value) override;
	void next() override;

	void finalize() override;
	void clear() override;

private:
	void increase_size();
};

}

#endif // LIBMINT_MEMORY_BUILTIN_ITERATOR_ITEMS_H
