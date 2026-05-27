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

#ifndef LIBMINT_MEMORY_BUILTIN_ITERATOR_ITEMS_H
#define LIBMINT_MEMORY_BUILTIN_ITERATOR_ITEMS_H

#include "iterator_p.h"
#include "mint/ast/cursor.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/memory/reference.h"
#include <cstddef>
#include <memory>

namespace mint::internal {

class ItemsIteratorViewData : public IteratorViewData {
	mint::Reference* _data;
	std::size_t _capacity;
	std::size_t _size;
	std::size_t _pos;
	std::size_t _cur = 0;
public:
	ItemsIteratorViewData(mint::Reference* data, std::size_t capacity, std::size_t size, std::size_t pos);
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
	static std::allocator<Reference> g_allocator;
	mint::Reference* _data;
	std::size_t _capacity;
	std::size_t _size = 0;
	std::size_t _pos = 0;
public:
	ItemsIteratorData();
	ItemsIteratorData(std::size_t capacity);
	ItemsIteratorData(Cursor& cursor, const mint::Reference& ref);
	ItemsIteratorData(Cursor& cursor, mint::Reference&& ref);
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

	void yield(mint::Cursor& cursor, mint::Iterator::Context::value_type&& value,
	    Iterator::ResumeKind resume_kind) override;
	void next(mint::Cursor& cursor) override;

	void finalize(mint::Cursor& cursor) override;
	void clear() override;

private:
	void increase_size();
};

}

#endif // LIBMINT_MEMORY_BUILTIN_ITERATOR_ITEMS_H
