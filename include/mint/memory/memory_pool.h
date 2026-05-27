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

#ifndef MINT_MEMORY_MEMORY_POOL_H
#define MINT_MEMORY_MEMORY_POOL_H

#include "mint/system/pool_allocator.h"
#include "mint/system/assert.h"
#include <cassert>
#include <concepts>
#include <cstddef>
#include <memory>
#include <new>

namespace mint {

class MemoryPool {
public:
	MemoryPool() = default;
	MemoryPool(MemoryPool&&) = delete;
	MemoryPool(const MemoryPool& other) = delete;
	virtual ~MemoryPool() = default;

	MemoryPool& operator=(MemoryPool&&) = delete;
	MemoryPool& operator=(const MemoryPool& other) = delete;

	virtual void free(void* address) = 0;
};

template<class Type>
class SystemPool : public MemoryPool {
public:
	template<typename... Args>
	    requires std::constructible_from<Type, Args...>
	Type* alloc(Args&&... args) {
		return assert_not_null<std::bad_alloc>(new Type(std::forward<Args>(args)...));
	}

	void free(Type* object) {
		delete object;
	}

	void free(void* address) override {
		delete static_cast<Type*>(address);
	}
};

template<>
class SystemPool<std::nullptr_t> : public MemoryPool {
public:
	template<typename... Args>
	std::nullptr_t alloc([[maybe_unused]] Args&&... args) {
		return nullptr;
	}

	void free([[maybe_unused]] std::nullptr_t object) {}

	void free([[maybe_unused]] void* address) override {}
};

template<class Type>
class LocalPool : public MemoryPool, private PoolAllocator<Type> {
public:
	template<typename... Args>
	    requires std::constructible_from<Type, Args...>
	Type* alloc(Args&&... args) {
		return std::construct_at(PoolAllocator<Type>::allocate(), std::forward<Args>(args)...);
	}

	void free(Type* object) {
		assert(object);
		std::destroy_at(object);
		PoolAllocator<Type>::deallocate(object);
	}

	void free(void* address) override {
		assert(address);
		auto* object = static_cast<Type*>(address);
		std::destroy_at(object);
		PoolAllocator<Type>::deallocate(object);
	}
};

}

#endif // MINT_MEMORY_MEMORY_POOL_H
