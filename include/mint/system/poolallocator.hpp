/**
 * Copyright (c) 2025 Gauvain CHERY.
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

#ifndef MINT_POOLALLOCATOR_HPP
#define MINT_POOLALLOCATOR_HPP

#include "mint/system/assert.h"

#include <algorithm>
#include <cstdint>
#include <cstddef>
#include <cstdlib>

namespace mint {

template<class Type, size_t pool_min_size = 0x4, size_t pool_max_size = 0x4000>
struct PoolAllocator {
	using value_type = Type;
	using pointer = Type *;
	using const_pointer = const Type *;
	using reference = Type &;
	using const_reference = const Type &;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;

	template<class OtherType>
	struct Rebind {
		using other = PoolAllocator<OtherType>;
	};

	static constexpr const size_t MIN_SIZE = pool_min_size;
	static constexpr const size_t MAX_SIZE = pool_max_size;
	static constexpr const size_t ALIGNMENT = (std::alignment_of_v<value_type>> std::alignment_of_v<pointer>)
												  ? std::alignment_of_v<value_type>
												  : +std::alignment_of_v<pointer>;
	static constexpr const size_t ALIGNED_SIZE = ((sizeof(value_type) - 1) / ALIGNMENT + 1) * ALIGNMENT;

	PoolAllocator() = default;

	PoolAllocator(const PoolAllocator &other) = delete;

	PoolAllocator(PoolAllocator &&other) noexcept :
		m_head(other.m_head),
		m_free_list(other.m_free_list) {
		other.m_free_list = nullptr;
		other.m_head = nullptr;
	}

	~PoolAllocator() {
		reset();
	}

	PoolAllocator &operator=(const PoolAllocator &other) = delete;

	PoolAllocator &operator=(PoolAllocator &&other) noexcept {
		reset();
		m_head = other.m_head;
		m_free_list = other.m_free_list;
		m_next_to_allocate = other.m_next_to_allocate;
		other.m_free_list = nullptr;
		other.m_head = nullptr;
		return *this;
	}

	void swap(PoolAllocator &other) noexcept {
		std::swap(m_head, other.m_head);
		std::swap(m_free_list, other.m_free_list);
	}

	bool operator==(const PoolAllocator &other) {
		return this == &other;
	}

	bool operator!=(const PoolAllocator &other) {
		return this != &other;
	}

	pointer allocate() {

		value_type *item = m_head;

		if (UNLIKELY(item == nullptr)) {
			m_next_to_allocate = std::min(m_next_to_allocate * 2, MAX_SIZE);
			const size_t bytes = ALIGNMENT + (ALIGNED_SIZE * m_next_to_allocate);
			add(assert_not_null<std::bad_alloc>(std::malloc(bytes)), bytes);
			item = m_head;
		}

		m_head = *reinterpret_cast<value_type **>(item);
		return item;
	}

	pointer allocate(size_type size) {
		if (size == 1) {
			return allocate();
		}

		value_type *item = m_head;
		value_type **prev = nullptr;
		size_type available = 0;

		for (value_type *next = item; next && available < size; next = *next) {
			if (*reinterpret_cast<value_type **>(next) == next + 1) {
				++available;
			}
			else {
				item = *reinterpret_cast<value_type **>(next);
				available = 0;
				prev = &next;
			}
		}

		if (available < size) {
			const size_t bytes = ALIGNMENT + (ALIGNED_SIZE * size);
			item = add_array(assert_not_null<std::bad_alloc>(std::malloc(bytes)), bytes);
		}
		else if (prev) {
			*prev = *reinterpret_cast<value_type **>(item[size - 1]);
		}
		else {
			m_head = *reinterpret_cast<value_type **>(item[size - 1]);
		}

		return item;
	}

	void deallocate(pointer item) {
		*reinterpret_cast<value_type **>(item) = m_head;
		m_head = item;
	}

	void deallocate(pointer item, size_type size) {
		if (size == 1) {
			deallocate(item);
		}
		else {
			for (size_t i = 0; i < size - 1; ++i) {
				*reinterpret_cast<value_type **>(item[i]) = item[i + 1];
			}
			*reinterpret_cast<value_type **>(item[size - 1]) = m_head;
			m_head = item;
		}
	}

	void reset() {

		while (m_free_list) {
			value_type *item = *m_free_list;
			std::free(m_free_list);
			m_free_list = reinterpret_cast<value_type **>(item);
		}

		m_head = nullptr;
	}

protected:
	void add(void *address, const size_type size) {

		assert(size >= ALIGNMENT);

		const size_t count = (size - ALIGNMENT) / ALIGNED_SIZE;
		auto **data = reinterpret_cast<value_type **>(address);

		auto ***x = reinterpret_cast<value_type ***>(data);
		*x = m_free_list;
		m_free_list = data;

		auto *const head_item = reinterpret_cast<value_type *>(reinterpret_cast<uint8_t *>(address) + ALIGNMENT);
		auto *const head_data = reinterpret_cast<uint8_t *>(head_item);

		for (size_t i = 0; i < count; ++i) {
			*reinterpret_cast<uint8_t **>(head_data + (i * ALIGNED_SIZE)) = head_data + (i + 1) * ALIGNED_SIZE;
		}

		*reinterpret_cast<value_type **>(head_data + ((count - 1) * ALIGNED_SIZE)) = m_head;
		m_head = head_item;
	}

	value_type *add_array(void *address, const size_type size) {

		assert(size >= ALIGNMENT);

		auto **data = reinterpret_cast<value_type **>(address);

		auto ***x = reinterpret_cast<value_type ***>(data);
		*x = m_free_list;
		m_free_list = data;

		return reinterpret_cast<value_type *>(reinterpret_cast<uint8_t *>(address) + ALIGNMENT);
	}

private:
	value_type *m_head = nullptr;
	value_type **m_free_list = nullptr;
	size_t m_next_to_allocate = MIN_SIZE;
};

}

#endif // MINT_POOLALLOCATOR_HPP
