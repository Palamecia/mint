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

#ifndef MINT_SYMBOLMAPPING_HPP
#define MINT_SYMBOLMAPPING_HPP

#include "mint/ast/symbol.h"
#include "mint/system/assert.h"

#include <algorithm>
#include <stdexcept>
#include <cstdlib>
#include <cstdint>
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
	memcpy(&buffer, ptr, sizeof(Type));
	return buffer;
}

struct FastForwardTag {};

template<class NodeType>
class AbstractNodeIterator {
	template<class Type>
	friend class SymbolMapping;
public:
	using value_type = NodeType;
	using difference_type = std::ptrdiff_t;
	using reference =  std::conditional_t<std::is_const_v<NodeType>, value_type const &, value_type &>;
	using pointer =  std::conditional_t<std::is_const_v<NodeType>, value_type const *, value_type *>;
	using iterator_category = std::forward_iterator_tag;

	AbstractNodeIterator() = default;

	template<class OtherNodeType,
			 typename = std::enable_if_t<std::is_const_v<NodeType> && !std::is_const_v<OtherNodeType>>>
	explicit AbstractNodeIterator(const AbstractNodeIterator<OtherNodeType> &other) :
		m_node(other.m_node),
		m_info(other.m_info) {}

	AbstractNodeIterator(NodeType *node, const uint8_t *info) :
		m_node(node),
		m_info(info) {}

	AbstractNodeIterator(NodeType *node, const uint8_t *info, [[maybe_unused]] FastForwardTag tag) :
		m_node(node),
		m_info(info) {
		fast_forward();
	}

	template<class OtherNodeType,
			 typename = std::enable_if_t<std::is_const_v<NodeType> && !std::is_const_v<OtherNodeType>>>
	AbstractNodeIterator &operator=(const AbstractNodeIterator<OtherNodeType> &other) {
		m_node = other.m_node;
		m_info = other.m_info;
		return *this;
	}

	AbstractNodeIterator &operator++() {
		m_info++;
		m_node++;
		fast_forward();
		return *this;
	}

	AbstractNodeIterator operator++(int) {
		AbstractNodeIterator tmp = *this;
		++(*this);
		return tmp;
	}

	reference operator*() const {
		return *m_node;
	}

	pointer operator->() const {
		return m_node;
	}

	template<class OtherNodeType>
	bool operator==(const AbstractNodeIterator<OtherNodeType> &other) const {
		return m_node == other.m_node;
	}

	template<class OtherNodeType>
	bool operator!=(const AbstractNodeIterator<OtherNodeType> &other) const {
		return m_node != other.m_node;
	}

private:
	void fast_forward() {

		size_t n = 0;

		while ((n = unaligned_load<size_t>(m_info)) == 0u) {
			m_info += sizeof(size_t);
			m_node += sizeof(size_t);
		}

		const int inc = count_zeroes(n) / 8;
		m_info += inc;
		m_node += inc;
	}

	static int count_zeroes(size_t mask) noexcept {
#ifdef OS_WINDOWS
		unsigned long index = 0;
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

template<typename Type>
class SymbolMapping {
public:
	using info_t = uint32_t;
	static constexpr size_t INITIAL_MAX_ELEMENTS = sizeof(uint64_t);
	static constexpr uint32_t INITIAL_INFO_SIZE = 5;
	static constexpr uint8_t INITIAL_INFO_OFFSET = 1U << INITIAL_INFO_SIZE;
	static constexpr size_t INFO_MASK = INITIAL_INFO_OFFSET - 1U;
	static constexpr uint8_t INITIAL_INFO_HASH_SHIFT = 0;

	using size_type = std::size_t;
	using node_type = std::pair<Symbol, Type>;

	using iterator = AbstractNodeIterator<node_type>;
	using const_iterator = AbstractNodeIterator<const node_type>;

	using key_type = Symbol;
	using mapped_type = Type;
	using value_type = node_type;
	using reference = value_type &;
	using const_reference = const value_type &;
	using pointer = value_type *;
	using const_pointer = const value_type *;

	SymbolMapping() = default;

	template<typename IteratorType>
	SymbolMapping(IteratorType first, IteratorType last) {
		insert(first, last);
	}

	SymbolMapping(std::initializer_list<value_type> init) {
		insert(init.begin(), init.end());
	}

	SymbolMapping(const SymbolMapping &other) {

		if (!other.empty()) {

			const size_t num_elements_with_buffer = calc_num_elements_with_buffer(other.m_mask + 1);
			const size_t num_bytes_total = calc_num_bytes_total(num_elements_with_buffer);

			m_hash_multiplier = other.m_hash_multiplier;
			m_nodes = static_cast<node_type *>(assert_not_null<std::bad_alloc>(std::malloc(num_bytes_total)));
			m_info = reinterpret_cast<uint8_t *>(m_nodes + num_elements_with_buffer);
			m_size = other.m_size;
			m_mask = other.m_mask;
			m_capacity = other.m_capacity;
			m_info_offset = other.m_info_offset;
			m_info_hash_shift = other.m_info_hash_shift;
			clone_data(other);
		}
	}

	SymbolMapping(SymbolMapping &&other) noexcept {

		if (other.m_mask) {
			m_hash_multiplier = other.m_hash_multiplier;
			m_nodes = std::move(other.m_nodes);
			m_info = other.m_info;
			m_size = other.m_size;
			m_mask = other.m_mask;
			m_capacity = other.m_capacity;
			m_info_offset = other.m_info_offset;
			m_info_hash_shift = other.m_info_hash_shift;
			other.init();
		}
	}

	~SymbolMapping() {
		destroy();
	}

	SymbolMapping &operator=(const SymbolMapping &other) {

		if (this != &other) {
			if (other.empty()) {
				if (m_mask != 0) {
					destroy();
					init();
				}
			}
			else {

				destroy_nodes();

				if (m_mask != other.m_mask) {

					if (m_mask != 0) {
						std::free(m_nodes);
					}

					const size_t num_elements_with_buffer = calc_num_elements_with_buffer(other.m_mask + 1);
					const size_t num_bytes_total = calc_num_bytes_total(num_elements_with_buffer);
					m_nodes = static_cast<node_type *>(assert_not_null<std::bad_alloc>(std::malloc(num_bytes_total)));
					m_info = reinterpret_cast<uint8_t *>(m_nodes + num_elements_with_buffer);
				}

				m_hash_multiplier = other.m_hash_multiplier;
				m_size = other.m_size;
				m_mask = other.m_mask;
				m_capacity = other.m_capacity;
				m_info_offset = other.m_info_offset;
				m_info_hash_shift = other.m_info_hash_shift;
				clone_data(other);
			}
		}

		return *this;
	}

	SymbolMapping &operator=(SymbolMapping &&other) noexcept {

		if (this != &other) {
			if (other.m_mask) {
				destroy();
				m_hash_multiplier = other.m_hash_multiplier;
				m_nodes = std::move(other.m_nodes);
				m_info = other.m_info;
				m_size = other.m_size;
				m_mask = other.m_mask;
				m_capacity = other.m_capacity;
				m_info_offset = other.m_info_offset;
				m_info_hash_shift = other.m_info_hash_shift;
				other.init();
			}
			else {
				clear();
			}
		}

		return *this;
	}

	bool operator==(const SymbolMapping &other) const {

		if (other.size() != size()) {
			return false;
		}

		return std::all_of(other.begin(), other.end(), [this](auto const &other_entry) {
			return has(other_entry);
		});
	}

	bool operator!=(const SymbolMapping &other) const {
		return !operator==(other);
	}

	mapped_type &operator[](const key_type &key) {

		auto [index, state] = insert_symbol_prepare_empty_spot(key);

		switch (state) {
		case InsertionState::SYMBOL_FOUND:
			break;

		case InsertionState::NEW_NODE:
			new (static_cast<void *>(&m_nodes[index]))
				node_type(std::piecewise_construct, std::forward_as_tuple(key), std::forward_as_tuple());
			break;

		case InsertionState::OVERWRITE_NODE:
			m_nodes[index] = node_type(std::piecewise_construct, std::forward_as_tuple(key), std::forward_as_tuple());
			break;

		case InsertionState::OVERFLOW_ERROR:
			throw std::overflow_error("SymbolMapping overflow");
		}

		return m_nodes[index].second;
	}

	mapped_type &operator[](key_type &&key) {

		auto [index, state] = insert_symbol_prepare_empty_spot(key);

		switch (state) {
		case InsertionState::SYMBOL_FOUND:
			break;

		case InsertionState::NEW_NODE:
			new (static_cast<void *>(&m_nodes[index]))
				node_type(std::piecewise_construct, std::forward_as_tuple(std::move(key)), std::forward_as_tuple());
			break;

		case InsertionState::OVERWRITE_NODE:
			m_nodes[index] = node_type(std::piecewise_construct, std::forward_as_tuple(std::move(key)),
									   std::forward_as_tuple());
			break;

		case InsertionState::OVERFLOW_ERROR:
			throw std::overflow_error("SymbolMapping overflow");
		}

		return m_nodes[index].second;
	}

	void swap(SymbolMapping &other) noexcept {
		std::swap(*this, other);
	}

	template<typename IteratorType>
	void insert(IteratorType first, IteratorType last) {
		while (first != last) {
			emplace(first->first, first->second);
			++first;
		}
	}

	void insert(std::initializer_list<value_type> ilist) {
		for (value_type &&vt : ilist) {
			emplace(std::move(vt));
		}
	}

	template<typename... Args>
	std::pair<iterator, bool> emplace(const Symbol &symbol, Args &&...args) {

		auto [index, state] = insert_symbol_prepare_empty_spot(symbol);

		switch (state) {
		case InsertionState::SYMBOL_FOUND:
			break;

		case InsertionState::NEW_NODE:
			new (static_cast<void *>(&m_nodes[index]))
				node_type(std::forward<const Symbol>(symbol), std::forward<Args>(args)...);
			break;

		case InsertionState::OVERWRITE_NODE:
			m_nodes[index] = node_type(std::forward<const Symbol>(symbol), std::forward<Args>(args)...);
			break;

		case InsertionState::OVERFLOW_ERROR:
			throw std::overflow_error("SymbolMapping overflow");
			break;
		}

		return std::make_pair(iterator(m_nodes + index, m_info + index), InsertionState::SYMBOL_FOUND != state);
	}

	template<typename... Args>
	std::pair<iterator, bool> emplace(Symbol &&symbol, Args &&...args) {

		auto [index, state] = insert_symbol_prepare_empty_spot(symbol);

		switch (state) {
		case InsertionState::SYMBOL_FOUND:
			break;

		case InsertionState::NEW_NODE:
			new (static_cast<void *>(&m_nodes[index])) node_type(std::move(symbol), std::forward<Args>(args)...);
			break;

		case InsertionState::OVERWRITE_NODE:
			m_nodes[index] = node_type(std::move(symbol), std::forward<Args>(args)...);
			break;

		case InsertionState::OVERFLOW_ERROR:
			throw std::overflow_error("SymbolMapping overflow");
			break;
		}

		return std::make_pair(iterator(m_nodes + index, m_info + index), InsertionState::SYMBOL_FOUND != state);
	}

	std::pair<iterator, bool> emplace(value_type &&node) {

		auto [index, state] = insert_symbol_prepare_empty_spot(node.first);

		switch (state) {
		case InsertionState::SYMBOL_FOUND:
			break;

		case InsertionState::NEW_NODE:
			new (static_cast<void *>(&m_nodes[index])) node_type(std::move(node));
			break;

		case InsertionState::OVERWRITE_NODE:
			m_nodes[index] = node_type(std::move(node));
			break;

		case InsertionState::OVERFLOW_ERROR:
			throw std::overflow_error("SymbolMapping overflow");
			break;
		}

		return std::make_pair(iterator(m_nodes + index, m_info + index), InsertionState::symbol_found != state);
	}

	std::pair<iterator, bool> insert(const value_type &keyval) {
		return emplace(keyval);
	}

	std::pair<iterator, bool> insert(value_type &&keyval) {
		return emplace(std::move(keyval));
	}

	[[nodiscard]] size_t count(const key_type &key) const {

		const auto kv = m_nodes + find_index(key);

		if (kv != reinterpret_cast<node_type *>(m_info)) {
			return 1;
		}

		return 0;
	}

	mapped_type &at(const key_type &key) {

		auto kv = m_nodes + find_index(key);

		if (kv == reinterpret_cast<node_type *>(m_info)) {
			throw std::out_of_range("Symbol not found");
		}

		return kv->second;
	}

	const mapped_type &at(const key_type &key) const {

		const auto kv = m_nodes + find_index(key);

		if (kv == reinterpret_cast<node_type *>(m_info)) {
			throw std::out_of_range("Symbol not found");
		}

		return kv->second;
	}

	bool contains(const key_type &key) const {
		return m_nodes + find_index(key) != reinterpret_cast<node_type *>(m_info);
	}

	const_iterator find(const key_type &key) const {
		const size_t index = find_index(key);
		return const_iterator {m_nodes + index, m_info + index};
	}

	iterator find(const key_type &key) {
		const size_t index = find_index(key);
		return iterator(m_nodes + index, m_info + index);
	}

	iterator begin() {
		return m_size != 0 ? iterator(m_nodes, m_info, FastForwardTag()) : end();
	}

	const_iterator begin() const {
		return cbegin();
	}

	const_iterator cbegin() const {
		return m_size != 0 ? const_iterator(m_nodes, m_info, FastForwardTag()) : cend();
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
		shift_down(idx);
		--m_size;

		if (*pos.m_info) {
			return pos;
		}

		return ++pos;
	}

	size_t erase(const key_type &key) {

		size_t index = 0;
		info_t info = 0;
		symbol_to_index(key, &index, &info);

		do {
			if (info == m_info[index] && (key == m_nodes[index].first)) {
				shift_down(index);
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

		destroy_nodes();

		const size_t num_elements_with_buffer = calc_num_elements_with_buffer(m_mask + 1);
		std::fill(m_info, m_info + calc_num_bytes_info(num_elements_with_buffer), 0u);
		m_info[num_elements_with_buffer] = 1;

		m_info_offset = INITIAL_INFO_OFFSET;
		m_info_hash_shift = INITIAL_INFO_HASH_SHIFT;
	}

	void rehash(size_t c) {
		reserve(c, true);
	}

	void reserve(size_t c) {
		reserve(c, false);
	}

	void compact() {

		size_t new_size = INITIAL_MAX_ELEMENTS;

		while (calc_max_num_elements_allowed(new_size) < m_size && new_size != 0) {
			new_size *= 2;
		}

		if (UNLIKELY(new_size == 0)) {
			throw std::overflow_error("SymbolMapping overflow");
		}

		if (new_size < m_mask + 1) {
			rehash_power_of_two(new_size, true);
		}
	}

	[[nodiscard]] size_type size() const {
		return m_size;
	}

	[[nodiscard]] size_type max_size() const {
		return static_cast<size_type>(-1);
	}

	[[nodiscard]] bool empty() const {
		return m_size == 0;
	}

	[[nodiscard]] float max_load_factor() const {
		return 0.8f;
	}

	[[nodiscard]] float load_factor() const {
		return static_cast<float>(size()) / static_cast<float>(m_mask + 1);
	}

private:
	[[nodiscard]] size_t calc_max_num_elements_allowed(size_t max_elements) const {

		if (LIKELY(max_elements <= std::numeric_limits<size_t>::max() / 100)) {
			return max_elements * 80 / 100;
		}

		return (max_elements / 100) * 80;
	}

	[[nodiscard]] size_t calc_num_bytes_info(size_t element_count) const {
		return element_count + sizeof(uint64_t);
	}

	[[nodiscard]] size_t calc_num_elements_with_buffer(size_t num_elements) const {
		size_t max_num_elements_allowed = calc_max_num_elements_allowed(num_elements);
		return num_elements + std::min(max_num_elements_allowed, static_cast<size_t>(0xFF));
	}

	// calculation only allowed for 2^n values
	[[nodiscard]] size_t calc_num_bytes_total(size_t num_elements) const {
#if !defined(__x86_64__) && !defined(_WIN64)
		// make sure we're doing 64bit operations, so we are at least safe against 32bit overflows.
		auto const ne = static_cast<uint64_t>(num_elements);
		auto const s = static_cast<uint64_t>(sizeof(node_type));
		auto const infos = static_cast<uint64_t>(calcNumBytesInfo(num_elements));

		auto const total64 = ne * s + infos;
		auto const total = static_cast<size_t>(total64);

		if (UNLIKELY(static_cast<uint64_t>(total) != total64)) {
			throw std::overflow_error("SymbolMapping overflow");
		}

		return total;
#else
		return (num_elements * sizeof(node_type)) + calc_num_bytes_info(num_elements);
#endif
	}

	bool has(const value_type &element) const {
		auto it = find(element.first);
		return it != end() && it->second == element.second;
	}

	void reserve(size_t count, bool force_rehash) {

		const size_t min_elements_allowed = std::max(count, m_size);
		size_t new_size = INITIAL_MAX_ELEMENTS;

		while (calc_max_num_elements_allowed(new_size) < min_elements_allowed && new_size != 0) {
			new_size *= 2;
		}

		if (UNLIKELY(new_size == 0)) {
			throw std::overflow_error("SymbolMapping overflow");
		}

		if (force_rehash || new_size > m_mask + 1) {
			rehash_power_of_two(new_size, false);
		}
	}

	// reserves space for at least the specified number of elements.
	// only works if numBuckets if power of two
	// True on success, false otherwise
	void rehash_power_of_two(size_t buckets_count) {

		node_type *const old_nodes = m_nodes;
		const uint8_t *const old_info = m_info;
		const size_t old_max_elements_with_buffer = calc_num_elements_with_buffer(m_mask + 1);

		// resize operation: move stuff
		init_data(buckets_count);

		if (old_max_elements_with_buffer > 1) {
			for (size_t i = 0; i < old_max_elements_with_buffer; ++i) {
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
				std::free(old_nodes);
			}
		}
	}

	void init_data(size_t max_elements) {

		m_size = 0;
		m_mask = max_elements - 1;
		m_capacity = calc_max_num_elements_allowed(max_elements);

		auto const num_elements_with_buffer = calc_num_elements_with_buffer(max_elements);

		// calloc also zeroes everything
		auto const num_bytes_total = calc_num_bytes_total(num_elements_with_buffer);
		m_nodes = reinterpret_cast<node_type *>(assert_not_null<std::bad_alloc>(std::calloc(1, num_bytes_total)));
		m_info = reinterpret_cast<uint8_t *>(m_nodes + num_elements_with_buffer);

		// set sentinel
		m_info[num_elements_with_buffer] = 1;

		m_info_offset = INITIAL_INFO_OFFSET;
		m_info_hash_shift = INITIAL_INFO_HASH_SHIFT;
	}

	enum class InsertionState : std::uint8_t {
		OVERFLOW_ERROR,
		SYMBOL_FOUND,
		NEW_NODE,
		OVERWRITE_NODE
	};

	// Finds key, and if not already present prepares a spot where to pot the key & value.
	// This potentially shifts nodes out of the way, updates mInfo and number of inserted
	// elements, so the only operation left to do is create/assign a new node at that spot.
	std::tuple<size_t, InsertionState> insert_symbol_prepare_empty_spot(const Symbol &symbol) {
		for (int i = 0; i < 0x100; ++i) {

			size_t index = 0;
			info_t info = 0;

			symbol_to_index(symbol, &index, &info);
			next_while_less(&info, &index);

			// while we potentially have a match
			while (info == m_info[index]) {
				if (symbol == m_nodes[index].first) {
					// key already exists, do NOT insert.
					// see http://en.cppreference.com/w/cpp/container/unordered_map/insert
					return std::make_tuple(index, InsertionState::SYMBOL_FOUND);
				}
				next(&info, &index);
			}

			// unlikely that this evaluates to true
			if (UNLIKELY(m_size >= m_capacity)) {
				if (!increase_size()) {
					return std::make_tuple(size_t(0), InsertionState::OVERFLOW_ERROR);
				}
				continue;
			}

			// Symbol not found, so we are now exactly where we want to insert it.
			const size_t insertion_index = index;
			const info_t insertion_info = info;

			if (UNLIKELY(insertion_info + m_info_offset > 0xFF)) {
				m_capacity = 0;
			}

			// find an empty spot
			while (0 != m_info[index]) {
				next(&info, &index);
			}

			if (index != insertion_index) {
				shift_up(index, insertion_index);
			}

			// put at empty spot
			m_info[insertion_index] = static_cast<uint8_t>(insertion_info);
			++m_size;
			return std::make_tuple(insertion_index, index == insertion_index ? InsertionState::NEW_NODE
																			 : InsertionState::OVERWRITE_NODE);
		}

		// enough attempts failed, so finally give up.
		return std::make_tuple(size_t(0), InsertionState::OVERFLOW_ERROR);
	}

	bool try_increase_info() {

		if (m_info_offset <= 2) {
			// need to be > 2 so that shift works (otherwise undefined behavior!)
			return false;
		}
		// we got space left, try to make info smaller
		m_info_offset = static_cast<uint8_t>(m_info_offset >> 1U);

		// remove one bit of the hash, leaving more space for the distance info.
		// This is extremely fast because we can operate on 8 bytes at once.
		++m_info_hash_shift;
		auto const num_elements_with_buffer = calc_num_elements_with_buffer(m_mask + 1);

		for (size_t i = 0; i < num_elements_with_buffer; i += 8) {
			auto val = unaligned_load<uint64_t>(m_info + i);
			val = (val >> 1U) & UINT64_C(0x7f7f7f7f7f7f7f7f);
			std::memcpy(m_info + i, &val, sizeof(val));
		}
		// update sentinel, which might have been cleared out!
		m_info[num_elements_with_buffer] = 1;

		m_capacity = calc_max_num_elements_allowed(m_mask + 1);
		return true;
	}

	// True if resize was possible, false otherwise
	bool increase_size() {
		// nothing allocated yet? just allocate InitialNumElements
		if (m_mask == 0) {
			init_data(INITIAL_MAX_ELEMENTS);
			return true;
		}

		auto const max_num_elements_allowed = calc_max_num_elements_allowed(m_mask + 1);
		if (m_size < max_num_elements_allowed && try_increase_info()) {
			return true;
		}

		if (m_size * 2 < calc_max_num_elements_allowed(m_mask + 1)) {
			// we have to resize, even though there would still be plenty of space left!
			// Try to rehash instead. Delete freed memory so we don't steadily increase mem in case
			// we have to rehash a few times
			next_hash_multiplier();
			rehash_power_of_two(m_mask + 1);
		}
		else {
			// we've reached the capacity of the map, so the hash seems to work nice. Keep using it.
			rehash_power_of_two((m_mask + 1) * 2);
		}

		return true;
	}

	void next_hash_multiplier() {
		// adding an *even* number, so that the multiplier will always stay odd. This is necessary
		// so that the hash stays a mixing function (and thus doesn't have any information loss).
		m_hash_multiplier += UINT64_C(0xc4ceb9fe1a85ec54);
	}

	void init() {
		m_nodes = reinterpret_cast<node_type *>(&m_mask);
		m_info = reinterpret_cast<uint8_t *>(&m_mask);
		m_size = 0;
		m_mask = 0;
		m_capacity = 0;
		m_info_offset = INITIAL_INFO_OFFSET;
		m_info_hash_shift = INITIAL_INFO_HASH_SHIFT;
	}

	void destroy() {

		if (m_mask == 0) {
			// don't deallocate!
			return;
		}

		destroy_nodes();

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
	void symbol_to_index(const Symbol &symbol, size_t *index, info_t *info) const {
		// In addition to whatever hash is used, add another mul & shift so we get better hashing.
		// This serves as a bad hash prevention, if the given data is
		// badly mixed.
		uint64_t hash = static_cast<uint64_t>(symbol.hash());
		hash *= m_hash_multiplier;
		hash ^= hash >> 33U;

		// the lower InitialInfoNumBits are reserved for info.
		*info = m_info_offset + static_cast<info_t>((hash & INFO_MASK) >> m_info_hash_shift);
		*index = (static_cast<size_t>(hash) >> INITIAL_INFO_SIZE) & m_mask;
	}

	// forwards the index by one, wrapping around at the end
	void next(info_t *info, size_t *index) const {
		*index = *index + 1;
		*info += m_info_offset;
	}

	void next_while_less(info_t *info, size_t *idx) const {
		// unrolling this by hand did not bring any speedups.
		while (*info < m_info[*idx]) {
			next(info, idx);
		}
	}

	// Shift everything up by one element. Tries to move stuff around.
	void shift_up(size_t start_idx, const size_t insertion_idx) {
		auto idx = start_idx;
		new (static_cast<void *>(m_nodes + idx)) node_type(std::move(m_nodes[idx - 1]));
		while (--idx != insertion_idx) {
			m_nodes[idx] = std::move(m_nodes[idx - 1]);
		}

		idx = start_idx;
		while (idx != insertion_idx) {
			m_info[idx] = static_cast<uint8_t>(m_info[idx - 1] + m_info_offset);
			if (UNLIKELY(m_info[idx] + m_info_offset > 0xFF)) {
				m_capacity = 0;
			}
			--idx;
		}
	}

	void shift_down(size_t index) {

		// until we find one that is either empty or has zero offset.
		while (m_info[index + 1] >= 2 * m_info_offset) {
			m_info[index] = static_cast<uint8_t>(m_info[index + 1] - m_info_offset);
			m_nodes[index] = std::move(m_nodes[index + 1]);
			++index;
		}

		m_info[index] = 0;
		m_nodes[index].~node_type();
	}

	// copy of find(), except that it returns iterator instead of const_iterator.
	size_t find_index(const Symbol &symbol) const {

		size_t index = 0;
		info_t info = 0;
		symbol_to_index(symbol, &index, &info);

		do {
			// unrolling this twice gives a bit of a speedup. More unrolling did not help.
			if (info == m_info[index] && LIKELY(symbol == m_nodes[index].first)) {
				return index;
			}
			next(&info, &index);
			if (info == m_info[index] && LIKELY(symbol == m_nodes[index].first)) {
				return index;
			}
			next(&info, &index);
		}
		while (info <= m_info[index]);

		// nothing found!
		return m_mask == 0 ? 0 : static_cast<size_t>(std::distance(m_nodes, reinterpret_cast<node_type *>(m_info)));
	}

	void clone_data(const SymbolMapping &other) {

		const auto num_elements_with_buffer = calc_num_elements_with_buffer(m_mask + 1);
		std::copy(other.m_info, other.m_info + calc_num_bytes_info(num_elements_with_buffer), m_info);

		for (size_t i = 0; i < num_elements_with_buffer; ++i) {
			if (m_info[i]) {
				new (static_cast<void *>(m_nodes + i)) node_type(other.m_nodes[i]);
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
		symbol_to_index(keyval.first, &idx, &info);

		// skip forward. Use <= because we are certain that the element is not there.
		while (info <= m_info[idx]) {
			idx = idx + 1;
			info += m_info_offset;
		}

		// Symbol not found, so we are now exactly where we want to insert it.
		auto const insertion_idx = idx;
		auto const insertion_info = static_cast<uint8_t>(info);

		if (UNLIKELY(insertion_info + m_info_offset > 0xFF)) {
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
			shift_up(idx, insertion_idx);
			l = std::move(keyval);
		}

		// put at empty spot
		m_info[insertion_idx] = insertion_info;

		++m_size;
	}

	void destroy_nodes() {

		m_size = 0;

		// clear also resets m_info to 0, that's sometimes not necessary.
		const size_t num_elements_with_buffer = calc_num_elements_with_buffer(m_mask + 1);

		for (size_t index = 0; index < num_elements_with_buffer; ++index) {
			if (m_info[index] != 0) {
				m_nodes[index].~node_type();
			}
		}
	}

	// members are sorted so no padding occurs
	uint64_t m_hash_multiplier = UINT64_C(0xc4ceb9fe1a85ec53);
	node_type *m_nodes = reinterpret_cast<node_type *>(&m_mask);
	uint8_t *m_info = reinterpret_cast<uint8_t *>(&m_mask);
	size_t m_size = 0;
	size_t m_mask = 0;
	size_t m_capacity = 0;
	info_t m_info_offset = INITIAL_INFO_OFFSET;
	info_t m_info_hash_shift = INITIAL_INFO_HASH_SHIFT;
};

}

#endif // MINT_SYMBOLMAPPING_HPP
