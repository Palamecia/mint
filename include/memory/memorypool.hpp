#ifndef MINT_MEMORYPOOL_HPP
#define MINT_MEMORYPOOL_HPP

#include "system/poolallocator.hpp"
#include "system/assert.h"

#include <memory>

namespace mint {

class MemoryPool {
public:
	MemoryPool() = default;
	MemoryPool(const MemoryPool &other) = delete;
	virtual ~MemoryPool() = default;
	MemoryPool &operator =(const MemoryPool &other) = delete;
	virtual void free(void *address) = 0;
};

template<class Type>
class SystemPool : public MemoryPool {
public:
	template<typename... Args>
	Type *alloc(Args... args) {
		return assert_not_null<std::bad_alloc>(new Type(std::forward<Args>(args)...));
	}

	void free(Type *object) {
		delete object;
	}

	void free(void *address) override {
		delete static_cast<Type *>(address);
	}
};

template<>
class SystemPool<std::nullptr_t> : public MemoryPool {
public:
	template<typename... Args>
	std::nullptr_t alloc(Args... args) {
		return nullptr;
	}

	void free(std::nullptr_t object) {
		((void)object);
	}

	void free(void *address) override {
		((void)address);
	}
};

template<class Type>
class LocalPool : public MemoryPool, private pool_allocator<Type> {
public:
	template<typename... Args>
	Type *alloc(Args&&... args) {
		return new (pool_allocator<Type>::allocate()) Type(std::forward<Args>(args)...);
	}

	void free(Type *object) {
		assert(object);
		object->Type::~Type();
		pool_allocator<Type>::deallocate(object);
	}

	void free(void *address) override {
		assert(address);
		Type *object = static_cast<Type *>(address);
		object->Type::~Type();
		pool_allocator<Type>::deallocate(object);
	}
};

}

#endif // MINT_MEMORYPOOL_HPP
