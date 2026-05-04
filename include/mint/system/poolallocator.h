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
#include <cassert>
#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <new>

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
	    _free_list(other._free_list),
	    _blocks(other._blocks) {
		other._free_list = nullptr;
		other._head = nullptr;
		other._blocks = nullptr;
	}

	~PoolAllocator() {
		reset();
	}

	PoolAllocator& operator=(const PoolAllocator& other) = delete;

	PoolAllocator& operator=(PoolAllocator&& other) noexcept {
		reset();
		_head = other._head;
		_free_list = other._free_list;
		_blocks = other._blocks;
		_next_to_allocate = other._next_to_allocate;
		other._free_list = nullptr;
		other._head = nullptr;
		other._blocks = nullptr;
		return *this;
	}

	void swap(PoolAllocator& other) noexcept {
		std::swap(_head, other._head);
		std::swap(_free_list, other._free_list);
		std::swap(_blocks, other._blocks);
	}

	bool operator==(const PoolAllocator& other) {
		return this == &other;
	}

	bool operator!=(const PoolAllocator& other) {
		return this != &other;
	}

	pointer allocate() {

		auto* item = _head;

		if (item == nullptr) [[unlikely]] {
			_next_to_allocate = std::min(_next_to_allocate * 2, max_size);
			const std::size_t bytes = sizeof(BlockHeader) + alignment + (aligned_size * _next_to_allocate);
			add(assert_not_null<std::bad_alloc>(std::malloc(bytes)), bytes, _next_to_allocate);
			item = _head;
		}

		auto* block = get_block_header(item);
		block->allocated_count++;
		_head = *reinterpret_cast<value_type**>(item);
		return item;
	}

	void deallocate(pointer item) {
		auto* block = get_block_header(item);
		block->allocated_count--;

		*reinterpret_cast<value_type**>(item) = _head;
		_head = item;

		// Free the block if all items are deallocated
		if (block->allocated_count == 0 && block != _blocks) {
			remove_block(block);
		}
	}

	void reset() {

		while (auto* block = _blocks) {
			_blocks = block->next_block;
			std::free(block);
		}

		_head = nullptr;
		_free_list = nullptr;
	}

protected:
	struct BlockHeader {
		BlockHeader* next_block = nullptr;
		value_type** free_list_ptr = nullptr;
		std::size_t allocated_count = 0;
		std::size_t total_count = 0;
		std::uint8_t* block_start = nullptr;
	};

	BlockHeader* get_block_header(value_type* item) {
		// Traverse back through the free list to find the block header
		const auto* item_ptr = reinterpret_cast<std::uint8_t*>(item);

		// The block starts at a multiple of the aligned size from the header
		// We need to find which block contains this item
		for (BlockHeader* block = _blocks; block != nullptr; block = block->next_block) {
			const auto* block_items_start = block->block_start;
			const auto* block_items_end = block_items_start + (block->total_count * aligned_size);

			if (item_ptr >= block_items_start && item_ptr < block_items_end) {
				return block;
			}
		}

		// Should never reach here if item is valid
		assert(false);
		return nullptr;
	}

	void remove_block(BlockHeader* block) {
		// Remove block from the linked list and free its memory
		if (block == _blocks) {
			return; // Don't remove the first block
		}

		BlockHeader** prev = &_blocks;
		for (BlockHeader* current = _blocks; current != nullptr; current = current->next_block) {
			if (current == block) {
				*prev = block->next_block;

				// Remove all items from this block from the free list
				const auto* block_start = block->block_start;
				const auto* block_end = block_start + (block->total_count * aligned_size);

				// Clean up the free list
				value_type** prev_ptr = &_head;
				while (*prev_ptr) {
					const auto* item_ptr = reinterpret_cast<std::uint8_t*>(*prev_ptr);
					if (item_ptr >= block_start && item_ptr < block_end) {
						*prev_ptr = *reinterpret_cast<value_type**>(*prev_ptr);
					}
					else {
						prev_ptr = reinterpret_cast<value_type**>(*prev_ptr);
					}
				}

				std::free(block);
				return;
			}
			prev = &current->next_block;
		}
	}

	void add(void* address, const size_type size, std::size_t count) {

		assert(size >= sizeof(BlockHeader) + alignment);

		auto* header = reinterpret_cast<BlockHeader*>(address);
		auto* block_start = reinterpret_cast<std::uint8_t*>(address) + sizeof(BlockHeader) + alignment;

		header->next_block = _blocks;
		header->allocated_count = 0;
		header->total_count = count;
		header->block_start = block_start;
		_blocks = header;

		// Create linked list of free items in this block
		for (std::size_t i = 0; i < count; ++i) {
			auto* item_ptr = reinterpret_cast<std::uint8_t*>(block_start) + (i * aligned_size);
			if (i < count - 1) {
				*reinterpret_cast<std::uint8_t**>(item_ptr) = item_ptr + aligned_size;
			}
			else {
				*reinterpret_cast<value_type**>(item_ptr) = _head;
			}
		}

		_head = reinterpret_cast<value_type*>(block_start);
	}

private:
	value_type* _head = nullptr;
	value_type** _free_list = nullptr;
	BlockHeader* _blocks = nullptr;
	std::size_t _next_to_allocate = min_size;
};

}

#endif // MINT_SYSTEM_POOLALLOCATOR_HPP
