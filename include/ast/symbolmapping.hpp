#ifndef MINT_SYMBOLMAPPING_HPP
#define MINT_SYMBOLMAPPING_HPP

#include "ast/symbol.h"
#include "system/poolallocator.hpp"
#include "system/assert.h"

#include <algorithm>
#include <stdexcept>
#include <utility>
#include <limits>
#include <tuple>

#ifdef OS_WINDOWS
#include <intrin.h>
#ifdef _WIN64
#pragma intrinsic(_BitScanForward64)
#else
#pragma intrinsic(_BitScanForward)
#endif
#else
#include <endian.h>
#endif

namespace mint {

template<class Type>
inline Type unaligned_load(const void *ptr) noexcept {
	Type buffer;
	memcpy(&buffer, ptr, sizeof (Type));
	return buffer;
}
struct fast_forward_tag {};

template<class ValueType, bool optimized>
class abstract_node {};

template <class ValueType, bool optimized>
struct abstract_node_allocator {};

template <class ValueType>
class abstract_node<ValueType, true> {
public:
	using value_type = ValueType;
	using mapped_type = typename ValueType::second_type;

	template <typename... Args>
	explicit abstract_node(abstract_node_allocator<ValueType, true> &allocator, Args&&... args) :
		m_data(std::forward<Args>(args)...) {
		((void)allocator);
	}

	abstract_node(abstract_node_allocator<ValueType, true> &allocator, abstract_node<ValueType, true> &&other) :
		m_data(std::move(other.m_data)) {
		((void)allocator);
	}

	void destroy(abstract_node_allocator<ValueType, true> &allocator) {
		((void)allocator);
	}

	void destroyDoNotDeallocate() {

	}

	const value_type *operator ->() const {
		return &m_data;
	}

	value_type *operator ->() {
		return &m_data;
	}

	const value_type &operator *() const {
		return m_data;
	}

	value_type &operator *() {
		return m_data;
	}

	const typename value_type::first_type &getFirst() const {
		return m_data.first;
	}

	typename value_type::first_type &getFirst() {
		return m_data.first;
	}

	const mapped_type &getSecond() const {
		return m_data.second;
	}

	mapped_type &getSecond() {
		return m_data.second;
	}

	void swap(abstract_node<ValueType, true> &other) {
		m_data.swap(other.m_data);
	}

private:
	value_type m_data;
};

template <class ValueType>
class abstract_node<ValueType, false> {
public:
	using value_type = ValueType;
	using mapped_type = typename ValueType::second_type;

	template <typename... Args>
	explicit abstract_node(abstract_node_allocator<ValueType, false> &allocator, Args&&... args) :
		m_data(allocator.allocate()) {
		new (static_cast<void *>(m_data)) value_type(std::forward<Args>(args)...);
	}

	abstract_node(abstract_node_allocator<ValueType, false> &allocator, abstract_node<ValueType, false> &&other) :
		m_data(std::move(other.m_data)) {
		((void)allocator);
	}

	void destroy(abstract_node_allocator<ValueType, false> &allocator) {
		m_data->~value_type();
		allocator.deallocate(m_data);
	}

	void destroyDoNotDeallocate() {
		m_data->~value_type();
	}

	value_type const* operator ->() const {
		return m_data;
	}

	value_type* operator ->() {
		return m_data;
	}

	const value_type& operator *() const {
		return *m_data;
	}

	value_type& operator *() {
		return *m_data;
	}

	const typename value_type::first_type &getFirst() const {
		return m_data->first;
	}

	typename value_type::first_type &getFirst() {
		return m_data->first;
	}

	const mapped_type &getSecond() const {
		return m_data->second;
	}

	mapped_type &getSecond() {
		return m_data->second;
	}

	void swap(abstract_node<ValueType, false> &other) {
		std::swap(m_data, other.m_data);
	}

private:
	value_type *m_data;
};

template <class ValueType>
struct abstract_node_allocator<ValueType, true> {
	void add_or_free(void *address, size_t size) {
		((void)size);
		std::free(address);
	}
};

template <class ValueType>
struct abstract_node_allocator<ValueType, false> : public pool_allocator<ValueType> {
	void add_or_free(void *address, const size_t size) {
		if (size < pool_allocator<ValueType>::Alignment + pool_allocator<ValueType>::AlignedSize) {
			std::free(address);
		}
		else {
			pool_allocator<ValueType>::add(address, size);
		}
	}
};

template<class NodeType>
class abstract_node_iterator {
	template<class Type> friend class SymbolMapping;
public:
	using difference_type = std::ptrdiff_t;
	using value_type = typename NodeType::value_type;
	using reference = typename std::conditional<std::is_const<NodeType>::value, value_type const &, value_type &>::type;
	using pointer = typename std::conditional<std::is_const<NodeType>::value, value_type const *, value_type *>::type;
	using iterator_category = std::forward_iterator_tag;

	abstract_node_iterator() = default;

	template <class OtherNodeType, typename = typename std::enable_if<std::is_const<NodeType>::value && !std::is_const<OtherNodeType>::value>::type>
	abstract_node_iterator(const abstract_node_iterator<OtherNodeType> &other) :
		m_node(other.m_node),
		m_info(other.m_info) {

	}

	abstract_node_iterator(NodeType *node, const uint8_t *info) :
		m_node(node),
		m_info(info) {

	}

	abstract_node_iterator(NodeType *node, const uint8_t *info, fast_forward_tag tag) :
		m_node(node),
		m_info(info) {
		fastForward();
		((void)tag);
	}

	template <class OtherNodeType, typename = typename std::enable_if<std::is_const<NodeType>::value && !std::is_const<OtherNodeType>::value>::type>
	abstract_node_iterator& operator =(const abstract_node_iterator<OtherNodeType> &other) {
		m_node = other.m_node;
		m_info = other.m_info;
		return *this;
	}

	abstract_node_iterator& operator ++() {
		m_info++;
		m_node++;
		fastForward();
		return *this;
	}

	abstract_node_iterator operator ++(int) {
		abstract_node_iterator tmp = *this;
		++(*this);
		return tmp;
	}

	reference operator *() const {
		return **m_node;
	}

	pointer operator ->() const {
		return &**m_node;
	}

	template <class OtherNodeType>
	bool operator ==(const abstract_node_iterator<OtherNodeType> &other) const {
		return m_node == other.m_node;
	}

	template <class OtherNodeType>
	bool operator !=(const abstract_node_iterator<OtherNodeType> &other) const {
		return m_node != other.m_node;
	}

private:
	void fastForward() {

		size_t n = 0;

		while ((n = unaligned_load<size_t>(m_info)) == 0u) {
			m_info += sizeof(size_t);
			m_node += sizeof(size_t);
		}

		const int inc = countZeroes(n) / 8;
		m_info += inc;
		m_node += inc;
	}

	static inline int countZeroes(size_t mask) noexcept {
#ifdef OS_WINDOWS
		unsigned long index;
#ifdef _WIN64
		return _BitScanForward64(&index, mask) ? static_cast<int>(index) : 64;
#else
		return _BitScanForward(&index, mask) ? static_cast<int>(index) : 32;
#endif
#else
#ifdef __x86_64__
#if LITTLE_ENDIAN
		return ((mask) ? __builtin_ctzll(mask) : 64);
#else
		return ((mask) ? __builtin_clzll(mask) : 64);
#endif
#else
#if LITTLE_ENDIAN
		return ((mask) ? __builtin_ctzl(mask) : 32);
#else
		return ((mask) ? __builtin_clzl(mask) : 32);
#endif
#endif
#endif
	}

	NodeType *m_node = nullptr;
	const uint8_t *m_info = nullptr;
};

template <typename Type>
class SymbolMapping {
public:
	using info_t = uint32_t;
	static constexpr size_t InitialMaxElements = sizeof(uint64_t);
	static constexpr uint32_t InitialInfoSize = 5;
	static constexpr uint8_t InitialInfoOffset = 1U << InitialInfoSize;
	static constexpr size_t InfoMask = InitialInfoOffset - 1U;
	static constexpr uint8_t InitialInfoHashShift = 0;
	static constexpr const size_t OptimizationMaxSize = sizeof(size_t) * 8;
	static constexpr const bool Optimizable = (sizeof(std::pair<Symbol, Type>) <= OptimizationMaxSize
											   && std::is_nothrow_move_constructible<std::pair<Symbol, Type>>::value
											   && std::is_nothrow_move_assignable<std::pair<Symbol, Type>>::value);

	using node_type = abstract_node<std::pair<Symbol, Type>, Optimizable>;
	using node_allocator = abstract_node_allocator<std::pair<Symbol, Type>, Optimizable>;

	using iterator = abstract_node_iterator<node_type>;
	using const_iterator = abstract_node_iterator<const node_type>;

	using key_type = Symbol;
	using mapped_type = typename node_type::mapped_type;
	using value_type = typename node_type::value_type;
	using size_type = size_t;
	using difference_type = typename iterator::difference_type;

	using reference = value_type &;
	using const_reference = const value_type &;
	using pointer = value_type *;
	using const_pointer = const value_type *;

	SymbolMapping() {

	}

	template <typename IteratorType>
	SymbolMapping(IteratorType first, IteratorType last) {
		insert(first, last);
	}

	SymbolMapping(std::initializer_list<value_type> init) {
		insert(init.begin(), init.end());
	}

	SymbolMapping(const SymbolMapping &other) : m_pool(other.m_pool) {

		if (!other.empty()) {

			const size_t numElementsWithBuffer = calcNumElementsWithBuffer(other.m_mask + 1);
			const size_t numBytesTotal = calcNumBytesTotal(numElementsWithBuffer);

			m_hashMultiplier = other.m_hashMultiplier;
			m_nodes = static_cast<node_type *>(assert_not_null<std::bad_alloc>(std::malloc(numBytesTotal)));
			m_info = reinterpret_cast<uint8_t*>(m_nodes + numElementsWithBuffer);
			m_size = other.m_size;
			m_mask = other.m_mask;
			m_capacity = other.m_capacity;
			m_infoOffset = other.m_infoOffset;
			m_infoHashShift = other.m_infoHashShift;
			cloneData(other);
		}
	}

	SymbolMapping(SymbolMapping &&other) : m_pool(std::move(other.m_pool)) {

		if (other.m_mask) {
			m_hashMultiplier = std::move(other.m_hashMultiplier);
			m_nodes = std::move(other.mKeyVals);
			m_info = std::move(other.mInfo);
			m_size = std::move(other.mNumElements);
			m_mask = std::move(other.m_mask);
			m_capacity = std::move(other.m_capacity);
			m_infoOffset = std::move(other.m_infoOffset);
			m_infoHashShift = std::move(other.m_infoHashShift);
			other.init();
		}
	}

	~SymbolMapping() {
		destroy();
	}

	SymbolMapping &operator =(const SymbolMapping &other) {

		if (this != &other) {
			if (other.empty()) {
				if (m_mask != 0) {
					destroy();
					init();
					m_pool = other.m_pool;
				}
			}
			else {

				destroyNodes();

				if (m_mask != other.m_mask) {

					if (m_mask != 0) {
						std::free(m_nodes);
					}

					const size_t numElementsWithBuffer = calcNumElementsWithBuffer(other.m_mask + 1);
					const size_t numBytesTotal = calcNumBytesTotal(numElementsWithBuffer);
					m_nodes = static_cast<node_type *>(assert_not_null<std::bad_alloc>(std::malloc(numBytesTotal)));
					m_info = reinterpret_cast<uint8_t*>(m_nodes + numElementsWithBuffer);
				}

				m_pool = other.m_pool;
				m_hashMultiplier = other.m_hashMultiplier;
				m_size = other.m_size;
				m_mask = other.m_mask;
				m_capacity = other.m_capacity;
				m_infoOffset = other.m_infoOffset;
				m_infoHashShift = other.m_infoHashShift;
				cloneData(other);
			}
		}

		return *this;
	}

	SymbolMapping &operator =(SymbolMapping &&other) {

		if (this != &other) {
			if (other.m_mask) {
				destroy();
				m_hashMultiplier = std::move(other.m_hashMultiplier);
				m_nodes = std::move(other.mKeyVals);
				m_info = std::move(other.mInfo);
				m_size = std::move(other.mNumElements);
				m_mask = std::move(other.m_mask);
				m_capacity = std::move(other.m_capacity);
				m_infoOffset = std::move(other.m_infoOffset);
				m_infoHashShift = std::move(other.m_infoHashShift);
				m_pool = std::move(other.m_pool);

				other.init();
			}
			else {
				clear();
			}
		}

		return *this;
	}

	bool operator ==(const SymbolMapping &other) const {

		if (other.size() != size()) {
			return false;
		}

		for (auto const &other_entry : other) {
			if (!has(other_entry)) {
				return false;
			}
		}

		return true;
	}

	bool operator !=(const SymbolMapping &other) const {
		return !operator ==(other);
	}

	mapped_type &operator [](const key_type &key) {

		auto idxAndState = insertSymbolPrepareEmptySpot(key);
		switch (idxAndState.second) {
		case InsertionState::symbol_found:
			break;

		case InsertionState::new_node:
			new (static_cast<void *>(&m_nodes[idxAndState.first])) node_type(m_pool, std::piecewise_construct, std::forward_as_tuple(key), std::forward_as_tuple());
			break;

		case InsertionState::overwrite_node:
			m_nodes[idxAndState.first] = node_type(m_pool, std::piecewise_construct, std::forward_as_tuple(key), std::forward_as_tuple());
			break;

		case InsertionState::overflow_error:
			throw std::overflow_error("SymbolMapping overflow");
		}

		return m_nodes[idxAndState.first].getSecond();
	}

	mapped_type &operator [](key_type &&key) {

		auto idxAndState = insertSymbolPrepareEmptySpot(key);
		switch (idxAndState.second) {
		case InsertionState::symbol_found:
			break;

		case InsertionState::new_node:
			new (static_cast<void*>(&m_nodes[idxAndState.first])) node_type(m_pool, std::piecewise_construct, std::forward_as_tuple(std::move(key)), std::forward_as_tuple());
			break;

		case InsertionState::overwrite_node:
			m_nodes[idxAndState.first] = node_type(m_pool, std::piecewise_construct, std::forward_as_tuple(std::move(key)), std::forward_as_tuple());
			break;

		case InsertionState::overflow_error:
			throw std::overflow_error("SymbolMapping overflow");
		}

		return m_nodes[idxAndState.first].getSecond();
	}

	void swap(SymbolMapping &other) {
		std::swap(*this, other);
	}

	template <typename IteratorType>
	void insert(IteratorType first, IteratorType last) {
		while (first != last) {
			insert(value_type(*first++));
		}
	}

	void insert(std::initializer_list<value_type> ilist) {
		for (value_type &&vt : ilist) {
			insert(std::move(vt));
		}
	}

	template <typename... Args>
	std::pair<iterator, bool> emplace(Args&&... args) {

		node_type node(m_pool, std::forward<Args>(args)...);
		auto idxAndState = insertSymbolPrepareEmptySpot(node.getFirst());

		switch (idxAndState.second) {
		case InsertionState::symbol_found:
			node.destroy(m_pool);
			break;

		case InsertionState::new_node:
			new (static_cast<void*>(&m_nodes[idxAndState.first])) node_type(m_pool, std::move(node));
			break;

		case InsertionState::overwrite_node:
			m_nodes[idxAndState.first] = std::move(node);
			break;

		case InsertionState::overflow_error:
			node.destroy(m_pool);
			throw std::overflow_error("SymbolMapping overflow");
			break;
		}

		return std::make_pair(iterator(m_nodes + idxAndState.first, m_info + idxAndState.first), InsertionState::symbol_found != idxAndState.second);
	}

	std::pair<iterator, bool> insert(const value_type& keyval) {
		return emplace(keyval);
	}

	std::pair<iterator, bool> insert(value_type &&keyval) {
		return emplace(std::move(keyval));
	}

	size_t count(const key_type &key) const {

		auto kv = m_nodes + findIndex(key);

		if (kv != reinterpret_cast<node_type *>(m_info)) {
			return 1;
		}

		return 0;
	}

	mapped_type &at(const key_type &key) {

		auto kv = m_nodes + findIndex(key);

		if (kv == reinterpret_cast<node_type *>(m_info)) {
			throw std::out_of_range("Symbol not found");
		}

		return kv->getSecond();
	}

	const mapped_type &at(const key_type &key) const {

		auto kv = m_nodes + findIndex(key);

		if (kv == reinterpret_cast<node_type *>(m_info)) {
			throw std::out_of_range("Symbol not found");
		}

		return kv->getSecond();
	}

	const_iterator find(const key_type& key) const {
		const size_t idx = findIndex(key);
		return const_iterator{m_nodes + idx, m_info + idx};
	}

	iterator find(const key_type& key) {
		const size_t index = findIndex(key);
		return iterator(m_nodes + index, m_info + index);
	}

	iterator begin() {
		return m_size != 0 ? iterator(m_nodes, m_info, fast_forward_tag()) : end();
	}

	const_iterator begin() const {
		return cbegin();
	}

	const_iterator cbegin() const {
		return m_size != 0 ? const_iterator(m_nodes, m_info, fast_forward_tag()) : cend();
	}

	iterator end() {
		return iterator(reinterpret_cast<node_type *>(m_info), nullptr);
	}

	const_iterator end() const {
		return cend();
	}

	const_iterator cend() const {
		return const_iterator(reinterpret_cast<node_type *>(m_info), nullptr);
	}

	iterator erase(const_iterator pos) {
		return erase(iterator(const_cast<node_type *>(pos.mKeyVals), const_cast<uint8_t *>(pos.mInfo)));
	}

	iterator erase(iterator pos) {

		const size_t idx = static_cast<size_t>(pos.m_node - m_nodes);
		shiftDown(idx);
		--m_size;

		if (*pos.m_info) {
			return pos;
		}

		return ++pos;
	}

	size_t erase(const key_type &key) {

		size_t index = 0;
		info_t info = 0;
		symbolToIndex(key, &index, &info);

		do {
			if (info == m_info[index] && (key == m_nodes[index].getFirst())) {
				shiftDown(index);
				--m_size;
				return 1;
			}
			next(&info, &index);
		}
		while (info <= m_info[index]);

		return 0;
	}

	void clear() {

		if (empty()) {
			return;
		}

		destroyNodes();

		const size_t numElementsWithBuffer = calcNumElementsWithBuffer(m_mask + 1);
		const uint8_t z = 0;
		std::fill(m_info, m_info + calcNumBytesInfo(numElementsWithBuffer), z);
		m_info[numElementsWithBuffer] = 1;

		m_infoOffset = InitialInfoOffset;
		m_infoHashShift = InitialInfoHashShift;
	}

	void rehash(size_t c) {
		reserve(c, true);
	}

	void reserve(size_t c) {
		reserve(c, false);
	}

	void compact() {

		size_t new_size = InitialMaxElements;

		while (calcMaxNumElementsAllowed(new_size) < m_size && new_size != 0) {
			new_size *= 2;
		}

		if (UNLIKELY(new_size == 0)) {
			throw std::overflow_error("SymbolMapping overflow");
		}

		if (new_size < m_mask + 1) {
			rehashPowerOfTwo(new_size, true);
		}
	}

	size_type size() const {
		return m_size;
	}

	size_type max_size() const {
		return static_cast<size_type>(-1);
	}

	bool empty() const {
		return m_size == 0;
	}

	float max_load_factor() const {
		return 0.8f;
	}

	float load_factor() const {
		return static_cast<float>(size()) / static_cast<float>(m_mask + 1);
	}

private:
	size_t calcMaxNumElementsAllowed(size_t max_elements) const {

		if (LIKELY(max_elements <= std::numeric_limits<size_t>::max() / 100)) {
			return max_elements * 80 / 100;
		}

		return (max_elements / 100) * 80;
	}

	size_t calcNumBytesInfo(size_t element_count) const {
		return element_count + sizeof(uint64_t);
	}

	size_t calcNumElementsWithBuffer(size_t numElements) const {
		size_t maxNumElementsAllowed = calcMaxNumElementsAllowed(numElements);
		return numElements + std::min(maxNumElementsAllowed, static_cast<size_t>(0xFF));
	}

	// calculation only allowed for 2^n values
	size_t calcNumBytesTotal(size_t numElements) const {
#if !defined (__x86_64__) && !defined (_WIN64)
		// make sure we're doing 64bit operations, so we are at least safe against 32bit overflows.
		auto const ne = static_cast<uint64_t>(numElements);
		auto const s = static_cast<uint64_t>(sizeof(node_type));
		auto const infos = static_cast<uint64_t>(calcNumBytesInfo(numElements));

		auto const total64 = ne * s + infos;
		auto const total = static_cast<size_t>(total64);

		if (UNLIKELY(static_cast<uint64_t>(total) != total64)) {
			throw std::overflow_error("SymbolMapping overflow");
		}

		return total;
#else
		return numElements * sizeof(node_type) + calcNumBytesInfo(numElements);
#endif
	}

	bool has(const value_type &element) const {
		auto it = find(element.first);
		return it != end() && it->second == element.second;
	}

	void reserve(size_t count, bool force_rehash) {

		const size_t minElementsAllowed = std::max(count, m_size);
		size_t new_size = InitialMaxElements;

		while (calcMaxNumElementsAllowed(new_size) < minElementsAllowed && new_size != 0) {
			new_size *= 2;
		}

		if (UNLIKELY(new_size == 0)) {
			throw std::overflow_error("SymbolMapping overflow");
		}

		if (force_rehash || new_size > m_mask + 1) {
			rehashPowerOfTwo(new_size, false);
		}
	}

	// reserves space for at least the specified number of elements.
	// only works if numBuckets if power of two
	// True on success, false otherwise
	void rehashPowerOfTwo(size_t buckets_count, bool force_free) {

		node_type *const old_nodes = m_nodes;
		const uint8_t *const old_info = m_info;
		const size_t oldMaxElementsWithBuffer = calcNumElementsWithBuffer(m_mask + 1);

		// resize operation: move stuff
		initData(buckets_count);

		if (oldMaxElementsWithBuffer > 1) {
			for (size_t i = 0; i < oldMaxElementsWithBuffer; ++i) {
				if (old_info[i] != 0) {
					// might throw an exception, which is really bad since we are in the middle of
					// moving stuff.
					insert_move(std::move(old_nodes[i]));
					// destroy the node but DON'T destroy the data.
					old_nodes[i].~node_type();
				}
			}

			// this check is not necessary as it's guarded by the previous if, but it helps
			// silence g++'s overeager "attempt to free a non-heap object 'map'
			// [-Werror=free-nonheap-object]" warning.
			if (old_nodes != reinterpret_cast<node_type *>(&m_mask)) {
				// don't destroy old data: put it into the pool instead
				if (force_free) {
					std::free(old_nodes);
				}
				else {
					m_pool.add_or_free(old_nodes, calcNumBytesTotal(oldMaxElementsWithBuffer));
				}
			}
		}
	}

	void initData(size_t max_elements) {

		m_size = 0;
		m_mask = max_elements - 1;
		m_capacity = calcMaxNumElementsAllowed(max_elements);

		auto const numElementsWithBuffer = calcNumElementsWithBuffer(max_elements);

		// calloc also zeroes everything
		auto const numBytesTotal = calcNumBytesTotal(numElementsWithBuffer);
		m_nodes = reinterpret_cast<node_type *>(assert_not_null<std::bad_alloc>(std::calloc(1, numBytesTotal)));
		m_info = reinterpret_cast<uint8_t*>(m_nodes + numElementsWithBuffer);

		// set sentinel
		m_info[numElementsWithBuffer] = 1;

		m_infoOffset = InitialInfoOffset;
		m_infoHashShift = InitialInfoHashShift;
	}

	enum class InsertionState { overflow_error, symbol_found, new_node, overwrite_node };

	// Finds key, and if not already present prepares a spot where to pot the key & value.
	// This potentially shifts nodes out of the way, updates mInfo and number of inserted
	// elements, so the only operation left to do is create/assign a new node at that spot.
	std::pair<size_t, InsertionState> insertSymbolPrepareEmptySpot(const Symbol &symbol) {
		for (int i = 0; i < 256; ++i) {

			size_t index = 0;
			info_t info = 0;

			symbolToIndex(symbol, &index, &info);
			nextWhileLess(&info, &index);

			// while we potentially have a match
			while (info == m_info[index]) {
				if (symbol == m_nodes[index].getFirst()) {
					// key already exists, do NOT insert.
					// see http://en.cppreference.com/w/cpp/container/unordered_map/insert
					return std::make_pair(index, InsertionState::symbol_found);
				}
				next(&info, &index);
			}

			// unlikely that this evaluates to true
			if (UNLIKELY(m_size >= m_capacity)) {
				if (!increase_size()) {
					return std::make_pair(size_t(0), InsertionState::overflow_error);
				}
				continue;
			}

			// Symbol not found, so we are now exactly where we want to insert it.
			const size_t insertion_index = index;
			const info_t insertion_info = info;

			if (UNLIKELY(insertion_info + m_infoOffset > 0xFF)) {
				m_capacity = 0;
			}

			// find an empty spot
			while (0 != m_info[index]) {
				next(&info, &index);
			}

			if (index != insertion_index) {
				shiftUp(index, insertion_index);
			}

			// put at empty spot
			m_info[insertion_index] = static_cast<uint8_t>(insertion_info);
			++m_size;
			return std::make_pair(insertion_index, index == insertion_index ? InsertionState::new_node : InsertionState::overwrite_node);
		}

		// enough attempts failed, so finally give up.
		return std::make_pair(size_t(0), InsertionState::overflow_error);
	}

	bool try_increase_info() {

		if (m_infoOffset <= 2) {
			// need to be > 2 so that shift works (otherwise undefined behavior!)
			return false;
		}
		// we got space left, try to make info smaller
		m_infoOffset = static_cast<uint8_t>(m_infoOffset >> 1U);

		// remove one bit of the hash, leaving more space for the distance info.
		// This is extremely fast because we can operate on 8 bytes at once.
		++m_infoHashShift;
		auto const numElementsWithBuffer = calcNumElementsWithBuffer(m_mask + 1);

		for (size_t i = 0; i < numElementsWithBuffer; i += 8) {
			auto val = unaligned_load<uint64_t>(m_info + i);
			val = (val >> 1U) & UINT64_C(0x7f7f7f7f7f7f7f7f);
			std::memcpy(m_info + i, &val, sizeof(val));
		}
		// update sentinel, which might have been cleared out!
		m_info[numElementsWithBuffer] = 1;

		m_capacity = calcMaxNumElementsAllowed(m_mask + 1);
		return true;
	}

	// True if resize was possible, false otherwise
	bool increase_size() {
		// nothing allocated yet? just allocate InitialNumElements
		if (0 == m_mask) {
			initData(InitialMaxElements);
			return true;
		}

		auto const maxNumElementsAllowed = calcMaxNumElementsAllowed(m_mask + 1);
		if (m_size < maxNumElementsAllowed && try_increase_info()) {
			return true;
		}

		if (m_size * 2 < calcMaxNumElementsAllowed(m_mask + 1)) {
			// we have to resize, even though there would still be plenty of space left!
			// Try to rehash instead. Delete freed memory so we don't steadyily increase mem in case
			// we have to rehash a few times
			nextHashMultiplier();
			rehashPowerOfTwo(m_mask + 1, true);
		}
		else {
			// we've reached the capacity of the map, so the hash seems to work nice. Keep using it.
			rehashPowerOfTwo((m_mask + 1) * 2, false);
		}

		return true;
	}

	void nextHashMultiplier() {
		// adding an *even* number, so that the multiplier will always stay odd. This is necessary
		// so that the hash stays a mixing function (and thus doesn't have any information loss).
		m_hashMultiplier += UINT64_C(0xc4ceb9fe1a85ec54);
	}

	void init() {
		m_nodes = reinterpret_cast<node_type *>(&m_mask);
		m_info = reinterpret_cast<uint8_t *>(&m_mask);
		m_size = 0;
		m_mask = 0;
		m_capacity = 0;
		m_infoOffset = InitialInfoOffset;
		m_infoHashShift = InitialInfoHashShift;
	}

	void destroy() {

		if (m_mask == 0) {
			// don't deallocate!
			return;
		}

		destroyNodesDoNotDeallocate();

		// This protection against not deleting mMask shouldn't be needed as it's sufficiently
		// protected with the 0==mMask check, but I have this anyways because g++ 7 otherwise
		// reports a compile error: attempt to free a non-heap object 'fm'
		// [-Werror=free-nonheap-object]
		if (m_nodes != reinterpret_cast<node_type *>(&m_mask)) {
			std::free(m_nodes);
		}
	}

	// highly performance relevant code.
	// Lower bits are used for indexing into the array (2^n size)
	// The upper 1-5 bits need to be a reasonable good hash, to save comparisons.
	void symbolToIndex(const Symbol &symbol, size_t *index, info_t *info) const {
		// In addition to whatever hash is used, add another mul & shift so we get better hashing.
		// This serves as a bad hash prevention, if the given data is
		// badly mixed.
		uint64_t hash = static_cast<uint64_t>(symbol.hash());
		hash *= m_hashMultiplier;
		hash ^= hash >> 33U;

		// the lower InitialInfoNumBits are reserved for info.
		*info = m_infoOffset + static_cast<info_t>((hash & InfoMask) >> m_infoHashShift);
		*index = (static_cast<size_t>(hash) >> InitialInfoSize) & m_mask;
	}

	// forwards the index by one, wrapping around at the end
	void next(info_t *info, size_t *index) const {
		*index = *index + 1;
		*info += m_infoOffset;
	}

	void nextWhileLess(info_t* info, size_t* idx) const {
		// unrolling this by hand did not bring any speedups.
		while (*info < m_info[*idx]) {
			next(info, idx);
		}
	}

	// Shift everything up by one element. Tries to move stuff around.
	void shiftUp(size_t startIdx, const size_t insertion_idx) {
		auto idx = startIdx;
		new (static_cast<void*>(m_nodes + idx)) node_type(std::move(m_nodes[idx - 1]));
		while (--idx != insertion_idx) {
			m_nodes[idx] = std::move(m_nodes[idx - 1]);
		}

		idx = startIdx;
		while (idx != insertion_idx) {
			m_info[idx] = static_cast<uint8_t>(m_info[idx - 1] + m_infoOffset);
			if (UNLIKELY(m_info[idx] + m_infoOffset > 0xFF)) {
				m_capacity = 0;
			}
			--idx;
		}
	}

	void shiftDown(size_t index) {
		// until we find one that is either empty or has zero offset.
		// TODO(martinus) we don't need to move everything, just the last one for the same
		// bucket.
		m_nodes[index].destroy(m_pool);

		// until we find one that is either empty or has zero offset.
		while (m_info[index + 1] >= 2 * m_infoOffset) {
			m_info[index] = static_cast<uint8_t>(m_info[index + 1] - m_infoOffset);
			m_nodes[index] = std::move(m_nodes[index + 1]);
			++index;
		}

		m_info[index] = 0;
		// don't destroy, we've moved it
		// m_nodes[idx].destroy(m_pool);
		m_nodes[index].~node_type();
	}

	// copy of find(), except that it returns iterator instead of const_iterator.
	size_t findIndex(const Symbol &symbol) const {

		size_t index = 0;
		info_t info = 0;
		symbolToIndex(symbol, &index, &info);

		do {
			// unrolling this twice gives a bit of a speedup. More unrolling did not help.
			if (info == m_info[index] && LIKELY(symbol == m_nodes[index].getFirst())) {
				return index;
			}
			next(&info, &index);
			if (info == m_info[index] && LIKELY(symbol == m_nodes[index].getFirst())) {
				return index;
			}
			next(&info, &index);
		}
		while (info <= m_info[index]);

		// nothing found!
		return m_mask == 0 ? 0 : static_cast<size_t>(std::distance(m_nodes, reinterpret_cast<node_type *>(m_info)));
	}

	void cloneData(const SymbolMapping &other) {

		const auto numElementsWithBuffer = calcNumElementsWithBuffer(m_mask + 1);
		std::copy(other.m_info, other.m_info + calcNumBytesInfo(numElementsWithBuffer), m_info);

		for (size_t i = 0; i < numElementsWithBuffer; ++i) {
			if (m_info[i]) {
				new (static_cast<void *>(m_nodes + i)) node_type(m_pool, *other.m_nodes[i]);
			}
		}
	}

	// inserts a keyval that is guaranteed to be new, e.g. when the hashmap is resized.
	// @return True on success, false if something went wrong
	void insert_move(node_type &&keyval) {
		// we don't retry, fail if overflowing
		// don't need to check max num elements
		if (m_capacity == 0 && !try_increase_info()) {
			throw std::overflow_error("SymbolMapping overflow");
		}

		size_t idx = 0;
		info_t info = 0;
		symbolToIndex(keyval.getFirst(), &idx, &info);

		// skip forward. Use <= because we are certain that the element is not there.
		while (info <= m_info[idx]) {
			idx = idx + 1;
			info += m_infoOffset;
		}

		// Symbol not found, so we are now exactly where we want to insert it.
		auto const insertion_idx = idx;
		auto const insertion_info = static_cast<uint8_t>(info);

		if (UNLIKELY(insertion_info + m_infoOffset > 0xFF)) {
			m_capacity = 0;
		}

		// find an empty spot
		while (0 != m_info[idx]) {
			next(&info, &idx);
		}

		auto &l = m_nodes[insertion_idx];

		if (idx == insertion_idx) {
			new (static_cast<void *>(&l)) node_type(std::move(keyval));
		}
		else {
			shiftUp(idx, insertion_idx);
			l = std::move(keyval);
		}

		// put at empty spot
		m_info[insertion_idx] = insertion_info;

		++m_size;
	}

	void destroyNodes() {

		m_size = 0;

		// clear also resets mInfo to 0, that's sometimes not necessary.
		const size_t numElementsWithBuffer = calcNumElementsWithBuffer(m_mask + 1);

		for (size_t index = 0; index < numElementsWithBuffer; ++index) {
			if (m_info[index] != 0) {
				node_type &node = m_nodes[index];
				node.destroy(m_pool);
				node.~node_type();
			}
		}
	}

	void destroyNodesDoNotDeallocate() {

		m_size = 0;

		// clear also resets mInfo to 0, that's sometimes not necessary.
		const size_t numElementsWithBuffer = calcNumElementsWithBuffer(m_mask + 1);

		for (size_t index = 0; index < numElementsWithBuffer; ++index) {
			if (m_info[index] != 0) {
				node_type &node = m_nodes[index];
				node.destroyDoNotDeallocate();
				node.~node_type();
			}
		}
	}

	// members are sorted so no padding occurs
	node_allocator m_pool;
	uint64_t m_hashMultiplier = UINT64_C(0xc4ceb9fe1a85ec53);
	node_type *m_nodes = reinterpret_cast<node_type *>(&m_mask);
	uint8_t *m_info = reinterpret_cast<uint8_t *>(&m_mask);
	size_t m_size = 0;
	size_t m_mask = 0;
	size_t m_capacity = 0;
	info_t m_infoOffset = InitialInfoOffset;
	info_t m_infoHashShift = InitialInfoHashShift;
};

}

#endif // MINT_SYMBOLMAPPING_HPP
