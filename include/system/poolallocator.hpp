#ifndef MINT_POOLALLOCATOR_HPP
#define MINT_POOLALLOCATOR_HPP

#include "system/assert.h"

#include <type_traits>
#include <memory>
#include <memory>

namespace mint {

template<class Type, int min_size = 0x4, int max_size = 0x4000>
struct pool_allocator {
	using value_type = Type;
	using pointer = Type *;
	using const_pointer = const Type *;
	using reference = Type &;
	using const_reference = const Type &;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;

	template<class OtherType>
	struct rebind {
		using other = pool_allocator<OtherType>;
	};

#ifdef BUILD_TYPE_RELEASE
	static constexpr const size_t MinSize = min_size;
	static constexpr const size_t MaxSize = max_size;
	static constexpr const size_t Alignment = (std::alignment_of<value_type>::value > std::alignment_of<pointer>::value) ? std::alignment_of<value_type>::value : +std::alignment_of<pointer>::value;
	static constexpr const size_t AlignedSize = ((sizeof(value_type) - 1) / Alignment + 1) * Alignment;
#else
	static const size_t MinSize;
	static const size_t MaxSize;
	static const size_t Alignment;
	static const size_t AlignedSize;
#endif

	pool_allocator() = default;

	pool_allocator(const pool_allocator &other) :
	    m_head(nullptr),
	    m_freeList(nullptr) {
		((void)other);
	}

	pool_allocator(pool_allocator &&other) noexcept :
	    m_head(other.m_head),
	    m_freeList(other.m_freeList) {
		other.m_freeList = nullptr;
		other.m_head = nullptr;
	}

	~pool_allocator() {
		reset();
	}

	pool_allocator &operator =(const pool_allocator &other) {
		((void)other);
		return *this;
	}

	pool_allocator &operator =(pool_allocator &&other) noexcept {
		reset();
		m_head = other.m_head;
		m_freeList = other.m_freeList;
		other.m_freeList = nullptr;
		other.m_head = nullptr;
		return *this;
	}

	void swap(pool_allocator &other) {
		std::swap(m_head, other.m_head);
		std::swap(m_freeList, other.m_freeList);
	}

	bool operator ==(const pool_allocator &other) {
		return this == &other;
	}

	bool operator !=(const pool_allocator &other) {
		return this != &other;
	}

	pointer allocate() {

		value_type *item = m_head;

		if (UNLIKELY(item == nullptr)) {
			m_nextToAllocate = std::min(m_nextToAllocate * 2, MaxSize);
			const size_t bytes = Alignment + AlignedSize * m_nextToAllocate;
			add(assert_not_null<std::bad_alloc>(std::malloc(bytes)), bytes);
			item = m_head;
		}

		m_head = *reinterpret_cast<value_type **>(item);
		return item;
	}

	pointer allocate(size_type size) {
		return (size == 1) ? allocate() : /** \todo */ nullptr;
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
			add(item, size);
		}
	}

	void reset() {

		while (m_freeList) {
			value_type *item = *m_freeList;
			std::free(m_freeList);
			m_freeList = reinterpret_cast<value_type **>(item);
		}

		m_head = nullptr;
	}

protected:
	void add(void *address, const size_type size) {

		assert(size >= Alignment);

		const size_t count = (size - Alignment) / AlignedSize;
		value_type **data = reinterpret_cast<value_type **>(address);

		value_type ***x = reinterpret_cast<value_type ***>(data);
		*x = m_freeList;
		m_freeList = data;

		value_type *const head_item = reinterpret_cast<value_type *>(reinterpret_cast<uint8_t *>(address) + Alignment);
		uint8_t *const head_data = reinterpret_cast<uint8_t *>(head_item);

		for (size_t i = 0; i < count; ++i) {
			*reinterpret_cast<uint8_t **>(head_data + i * AlignedSize) = head_data + (i + 1) * AlignedSize;
		}

		*reinterpret_cast<value_type **>(head_data + (count - 1) * AlignedSize) = m_head;
		m_head = head_item;
	}

private:
	value_type *m_head = nullptr;
	value_type **m_freeList = nullptr;
	size_t m_nextToAllocate = MinSize;
};

#ifdef BUILD_TYPE_DEBUG
template<class Type, int min_size, int max_size>
const size_t pool_allocator<Type, min_size, max_size>::MinSize = min_size;
template<class Type, int min_size, int max_size>
const size_t pool_allocator<Type, min_size, max_size>::MaxSize = max_size;
template<class Type, int min_size, int max_size>
const size_t pool_allocator<Type, min_size, max_size>::Alignment = (std::alignment_of<value_type>::value > std::alignment_of<pointer>::value) ? std::alignment_of<value_type>::value : +std::alignment_of<pointer>::value;
template<class Type, int min_size, int max_size>
const size_t pool_allocator<Type, min_size, max_size>::AlignedSize = ((sizeof(value_type) - 1) / Alignment + 1) * Alignment;
#endif

}

#endif // MINT_POOLALLOCATOR_HPP
