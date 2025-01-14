/**
 * Copyright (c) 2024 Gauvain CHERY.
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

#ifndef MINT_UTF8ITERATOR_H
#define MINT_UTF8ITERATOR_H

#include "mint/config.h"

#include <string_view>
#include <iterator>
#include <string>

namespace mint {

MINT_EXPORT bool utf8_begin_code_point(byte_t b);

MINT_EXPORT std::string_view::size_type utf8_code_point_length(byte_t b);
MINT_EXPORT std::string_view::size_type utf8_code_point_count(std::string_view str);

MINT_EXPORT std::string_view::size_type utf8_byte_index_to_code_point_index(
	std::string_view str, std::string_view::difference_type byte_index);
MINT_EXPORT std::string_view::size_type utf8_byte_index_to_code_point_index(std::string_view str,
																			std::string_view::size_type byte_index);
MINT_EXPORT std::string_view::size_type utf8_previous_code_point_byte_index(std::string_view str,
																			std::string_view::size_type byte_index);
MINT_EXPORT std::string_view::size_type utf8_next_code_point_byte_index(std::string_view str,
																		std::string_view::size_type byte_index);

MINT_EXPORT std::string_view::size_type utf8_code_point_index_to_byte_index(
	std::string_view str, std::string_view::size_type code_point_index);
MINT_EXPORT std::string_view::size_type utf8_substring_byte_count(std::string_view str,
																  std::string_view::size_type code_point_index,
																  std::string_view::size_type code_point_count);

MINT_EXPORT std::string_view::size_type utf8_grapheme_code_point_count(std::string_view str);

MINT_EXPORT int utf8_compare(std::string_view s1, std::string_view s2);
MINT_EXPORT int utf8_compare_substring(std::string_view s1, std::string_view s2,
									   std::string_view::size_type code_point_count);
MINT_EXPORT int utf8_compare_case_insensitive(std::string_view s1, std::string_view s2);
MINT_EXPORT int utf8_compare_substring_case_insensitive(std::string_view s1, std::string_view s2,
														std::string_view::size_type code_point_count);

MINT_EXPORT bool utf8_is_alnum(std::string_view str);
MINT_EXPORT bool utf8_is_alpha(std::string_view str);
MINT_EXPORT bool utf8_is_digit(std::string_view str);
MINT_EXPORT bool utf8_is_blank(std::string_view str);
MINT_EXPORT bool utf8_is_space(std::string_view str);
MINT_EXPORT bool utf8_is_cntrl(std::string_view str);
MINT_EXPORT bool utf8_is_graph(std::string_view str);
MINT_EXPORT bool utf8_is_print(std::string_view str);
MINT_EXPORT bool utf8_is_punct(std::string_view str);
MINT_EXPORT bool utf8_is_lower(std::string_view str);
MINT_EXPORT bool utf8_is_upper(std::string_view str);

MINT_EXPORT std::string utf8_to_lower(std::string_view str);
MINT_EXPORT std::string utf8_to_upper(std::string_view str);

template<class container_type, class iterator_type>
class basic_utf8iterator {
public:
	using iterator_category = std::random_access_iterator_tag;
	using difference_type = std::ptrdiff_t;
	using value_type = const container_type;
	using pointer = value_type *;
	using reference = value_type &;

	basic_utf8iterator(const iterator_type &it) :
		m_data(it) {}

	virtual ~basic_utf8iterator() = default;

	basic_utf8iterator<container_type, iterator_type> &operator=(const iterator_type &it) {
		m_data = it;
		return *this;
	}

	basic_utf8iterator<container_type, iterator_type> &operator++() {
		m_data += static_cast<difference_type>(utf8_code_point_length(static_cast<byte_t>(*m_data)));
		return *this;
	}

	basic_utf8iterator<container_type, iterator_type> &operator--() {
		do {
			m_data--;
		}
		while (!utf8_begin_code_point(*m_data));
	}

	auto operator++(int) -> decltype(*this) {

		decltype(*this) other(*this);
		operator++();
		return other;
	}

	auto operator--(int) -> decltype(*this) {

		decltype(*this) other(*this);
		operator--();
		return other;
	}

	auto operator+(size_t offset) -> decltype(*this) {

		decltype(*this) other(*this);
		for (size_t i = 0; i < offset; ++i) {
			other++;
		}
		return other;
	}

	auto operator-(size_t offset) -> decltype(*this) {

		decltype(*this) other(*this);
		for (size_t i = 0; i < offset; ++i) {
			other--;
		}
		return other;
	}

	difference_type operator-(const basic_utf8iterator &other) {
		const auto diff = m_data - other.m_data;
		difference_type offset = 0;
		if (diff < 0) {
			for (auto it = *this; it != other; ++it) {
				--offset;
			}
		}
		else if (diff > 0) {
			for (auto it = other; it != *this; ++it) {
				++offset;
			}
		}
		return offset;
	}

	bool operator!=(const basic_utf8iterator<container_type, iterator_type> &other) const {
		return m_data != other.m_data;
	}

	bool operator==(const basic_utf8iterator<container_type, iterator_type> &other) const {
		return m_data == other.m_data;
	}

	pointer operator->() const {
		if constexpr (std::is_pointer_v<iterator_type>) {
			m_buffer = value_type(m_data, utf8_code_point_length(static_cast<byte_t>(*m_data)));
		}
		else {
			m_buffer = value_type(m_data.operator->(), utf8_code_point_length(static_cast<byte_t>(*m_data)));
		}
		return &m_buffer;
	}

	reference operator*() const {
		if constexpr (std::is_pointer_v<iterator_type>) {
			m_buffer = value_type(m_data, utf8_code_point_length(static_cast<byte_t>(*m_data)));
		}
		else {
			m_buffer = value_type(m_data.operator->(), utf8_code_point_length(static_cast<byte_t>(*m_data)));
		}
		return m_buffer;
	}

private:
	mutable container_type m_buffer;
	iterator_type m_data;
};

typedef basic_utf8iterator<std::string, std::string::iterator> utf8iterator;
typedef basic_utf8iterator<std::string, std::string::const_iterator> const_utf8iterator;
typedef basic_utf8iterator<std::string_view, std::string_view::iterator> utf8view_iterator;
typedef basic_utf8iterator<std::string_view, std::string_view::const_iterator> const_utf8view_iterator;

}

#endif // MINT_UTF8ITERATOR_H
