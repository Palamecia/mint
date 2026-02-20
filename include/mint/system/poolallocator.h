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

#ifndef MINT_SYSTEM_POOLALLOCATOR_HPP
#define MINT_SYSTEM_POOLALLOCATOR_HPP

#include "mint/system/assert.h"

#include <algorithm>
#include <cstdint>
#include <cstddef>
#include <cstdlib>

namespace mint {

template<class Type, std::size_t pool_min_size = 0x4, std::size_t pool_max_size = 0x4000>
struct PoolAllocator {
	using value_type = Type;
	using pointer = Type*;
	using const_pointer = const Type*;
	using reference = Type&;
	using const_reference = const Type&;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;

	template<class OtherType>
	struct Rebind {
		using other = PoolAllocator<OtherType>;
	};

	static constexpr const std::size_t min_size = pool_min_size;
	static constexpr const std::size_t max_size = pool_max_size;
	static constexpr const std::size_t alignment = (std::alignment_of_v < value_type >> std::alignment_of_v<pointer>)
	                                                   ? std::alignment_of_v<value_type>
	                                                   : +std::alignment_of_v<pointer>;
	static constexpr const std::size_t aligned_size = ((sizeof(value_type) - 1) / alignment + 1) * alignment;

	PoolAllocator() = default;

	PoolAllocator(const PoolAllocator& other) = delete;

	PoolAllocator(PoolAllocator&& other) noexcept :
	    _head(other._head),
	    _free_list(other._free_list) {
		other._free_list = nullptr;
		other._head = nullptr;
	}

	~PoolAllocator() {
		reset();
	}

	PoolAllocator& operator=(const PoolAllocator& other) = delete;

	PoolAllocator& operator=(PoolAllocator&& other) noexcept {
		reset();
		_head = other._head;
		_free_list = other._free_list;
		_next_to_allocate = other._next_to_allocate;
		other._free_list = nullptr;
		other._head = nullptr;
		return *this;
	}

	void swap(PoolAllocator& other) noexcept {
		std::swap(_head, other._head);
		std::swap(_free_list, other._free_list);
	}

	bool operator==(const PoolAllocator& other) {
		return this == &other;
	}

	bool operator!=(const PoolAllocator& other) {
		return this != &other;
	}

	pointer allocate() {

		value_type* item = _head;

		if (item == nullptr) [[unlikely]] {
			_next_to_allocate = std::min(_next_to_allocate * 2, max_size);
			const std::size_t bytes = alignment + (aligned_size * _next_to_allocate);
			add(assert_not_null<std::bad_alloc>(std::malloc(bytes)), bytes);
			item = _head;
		}

		_head = *reinterpret_cast<value_type**>(item);
		return item;
	}

	pointer allocate(size_type size) {
		if (size == 1) {
			return allocate();
		}

		value_type* item = _head;
		value_type** prev = nullptr;
		size_type available = 0;

		for (value_type* next = item; next && available < size; next = *next) {
			if (*reinterpret_cast<value_type**>(next) == next + 1) {
				++available;
			}
			else {
				item = *reinterpret_cast<value_type**>(next);
				available = 0;
				prev = &next;
			}
		}

		if (available < size) {
			const std::size_t bytes = alignment + (aligned_size * size);
			item = add_array(assert_not_null<std::bad_alloc>(std::malloc(bytes)), bytes);
		}
		else if (prev) {
			*prev = *reinterpret_cast<value_type**>(item[size - 1]);
		}
		else {
			_head = *reinterpret_cast<value_type**>(item[size - 1]);
		}

		return item;
	}

	void deallocate(pointer item) {
		*reinterpret_cast<value_type**>(item) = _head;
		_head = item;
	}

	void deallocate(pointer item, size_type size) {
		if (size == 1) {
			deallocate(item);
		}
		else {
			for (std::size_t i = 0; i < size - 1; ++i) {
				*reinterpret_cast<value_type**>(item[i]) = item[i + 1];
			}
			*reinterpret_cast<value_type**>(item[size - 1]) = _head;
			_head = item;
		}
	}

	void reset() {

		while (_free_list) {
			value_type* item = *_free_list;
			std::free(_free_list);
			_free_list = reinterpret_cast<value_type**>(item);
		}

		_head = nullptr;
	}

protected:
	void add(void* address, const size_type size) {

		assert(size >= alignment);

		const std::size_t count = (size - alignment) / aligned_size;
		auto** data = reinterpret_cast<value_type**>(address);

		auto*** x = reinterpret_cast<value_type***>(data);
		*x = _free_list;
		_free_list = data;

		auto* const head_item = reinterpret_cast<value_type*>(reinterpret_cast<std::uint8_t*>(address) + alignment);
		auto* const head_data = reinterpret_cast<std::uint8_t*>(head_item);

		for (std::size_t i = 0; i < count; ++i) {
			*reinterpret_cast<std::uint8_t**>(head_data + (i * aligned_size)) = head_data + (i + 1) * aligned_size;
		}

		*reinterpret_cast<value_type**>(head_data + ((count - 1) * aligned_size)) = _head;
		_head = head_item;
	}

	value_type* add_array(void* address, const size_type size) {

		assert(size >= alignment);

		auto** data = reinterpret_cast<value_type**>(address);

		auto*** x = reinterpret_cast<value_type***>(data);
		*x = _free_list;
		_free_list = data;

		return reinterpret_cast<value_type*>(reinterpret_cast<std::uint8_t*>(address) + alignment);
	}

private:
	value_type* _head = nullptr;
	value_type** _free_list = nullptr;
	std::size_t _next_to_allocate = min_size;
};

}

#endif // MINT_SYSTEM_POOLALLOCATOR_HPP
