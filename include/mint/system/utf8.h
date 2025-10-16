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

#ifndef MINT_SYSTEM_UTF8_H
#define MINT_SYSTEM_UTF8_H

#include "mint/config.h"

#include <compare>
#include <cstddef>
#include <string_view>
#include <iterator>
#include <string>
#include <type_traits>
#include <utility>

namespace mint {

constexpr inline std::size_t utf8_code_point_length_max = 4;

MINT_EXPORT bool utf8_begin_code_point(byte_t b);

MINT_EXPORT std::string_view::size_type utf8_code_point_length(byte_t b);
MINT_EXPORT std::string_view::size_type utf8_code_point_count(std::string_view str);

MINT_EXPORT std::string_view::size_type utf8_byte_index_to_code_point_index(std::string_view str,
    std::string_view::difference_type byte_index);
MINT_EXPORT std::string_view::size_type utf8_byte_index_to_code_point_index(std::string_view str,
    std::string_view::size_type byte_index);
MINT_EXPORT std::string_view::size_type utf8_previous_code_point_byte_index(std::string_view str,
    std::string_view::size_type byte_index);
MINT_EXPORT std::string_view::size_type utf8_next_code_point_byte_index(std::string_view str,
    std::string_view::size_type byte_index);

MINT_EXPORT std::string_view::size_type utf8_code_point_index_to_byte_index(std::string_view str,
    std::string_view::size_type code_point_index);
MINT_EXPORT std::string_view::size_type utf8_substring_byte_count(std::string_view str,
    std::string_view::size_type code_point_index, std::string_view::size_type code_point_count);

MINT_EXPORT std::string_view::size_type utf8_grapheme_code_point_count(std::string_view str);

MINT_EXPORT std::strong_ordering utf8_compare(std::string_view s1, std::string_view s2);
MINT_EXPORT std::strong_ordering utf8_compare_substring(std::string_view s1, std::string_view s2,
    std::string_view::size_type code_point_count);
MINT_EXPORT std::strong_ordering utf8_compare_case_insensitive(std::string_view s1, std::string_view s2);
MINT_EXPORT std::strong_ordering utf8_compare_substring_case_insensitive(std::string_view s1, std::string_view s2,
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
class Utf8Iterator {
public:
	using iterator_category = std::forward_iterator_tag;
	using difference_type = std::ptrdiff_t;
	using value_type = const container_type;
	using pointer = value_type*;
	using reference = value_type&;
	using const_reference = value_type&;
	using const_pointer = value_type*;

	Utf8Iterator(const iterator_type& it) :
	    _data(it) {}

	Utf8Iterator() = default;
	Utf8Iterator(const Utf8Iterator&) = default;
	Utf8Iterator(Utf8Iterator&&) = default;
	virtual ~Utf8Iterator() = default;

	Utf8Iterator& operator=(const Utf8Iterator&) = default;
	Utf8Iterator& operator=(Utf8Iterator&&) = default;

	Utf8Iterator<container_type, iterator_type>& operator=(const iterator_type& it) {
		_data = it;
		return *this;
	}

	Utf8Iterator<container_type, iterator_type>& operator++() {
		_data += static_cast<difference_type>(utf8_code_point_length(static_cast<byte_t>(*_data)));
		return *this;
	}

	auto operator++(int) -> std::remove_reference_t<decltype(*this)> {
		decltype(*this) other(*this);
		operator++();
		return other;
	}

	Utf8Iterator<container_type, iterator_type>& operator--() {
		do {
			_data--;
		}
		while (!utf8_begin_code_point(*_data));
	}

	auto operator--(int) -> std::remove_reference_t<decltype(*this)> {
		decltype(*this) other(*this);
		operator--();
		return other;
	}

	auto operator+(std::size_t offset) -> std::remove_reference_t<decltype(*this)> {
		decltype(*this) other(*this);
		for (std::size_t i = 0; i < offset; ++i) {
			other++;
		}
		return other;
	}

	auto operator-(std::size_t offset) -> std::remove_reference_t<decltype(*this)> {
		decltype(*this) other(*this);
		for (std::size_t i = 0; i < offset; ++i) {
			other--;
		}
		return other;
	}

	difference_type operator-(const Utf8Iterator& other) {
		const auto diff = _data - other._data;
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

	bool operator!=(const Utf8Iterator<container_type, iterator_type>& other) const {
		return _data != other._data;
	}

	bool operator==(const Utf8Iterator<container_type, iterator_type>& other) const {
		return _data == other._data;
	}

	pointer operator->() const {
		if constexpr (std::is_pointer_v<iterator_type>) {
			_buffer = value_type(_data, utf8_code_point_length(static_cast<byte_t>(*_data)));
		}
		else {
			_buffer = value_type(_data.operator->(), utf8_code_point_length(static_cast<byte_t>(*_data)));
		}
		return &_buffer;
	}

	reference operator*() const {
		if constexpr (std::is_pointer_v<iterator_type>) {
			_buffer = value_type(_data, utf8_code_point_length(static_cast<byte_t>(*_data)));
		}
		else {
			_buffer = value_type(_data.operator->(), utf8_code_point_length(static_cast<byte_t>(*_data)));
		}
		return _buffer;
	}

private:
	mutable container_type _buffer;
	iterator_type _data;
};

using utf8iterator = Utf8Iterator<std::string, std::string::iterator>;
using const_utf8iterator = Utf8Iterator<std::string, std::string::const_iterator>;
using utf8view_iterator = Utf8Iterator<std::string_view, std::string_view::iterator>;
using const_utf8view_iterator = Utf8Iterator<std::string_view, std::string_view::const_iterator>;

namespace views {

template<class iterator_type>
class Utf8View {
	iterator_type _begin;
	iterator_type _end;
public:
	Utf8View(iterator_type&& begin, iterator_type&& end) :
	    _begin(std::move(begin)),
	    _end(std::move(end)) {}

	iterator_type begin() {
		return _begin;
	}

	iterator_type end() {
		return _end;
	}
};

auto utf8(auto& str) {
	using container_type = std::remove_reference_t<decltype(str)>;
	using iterator_type = Utf8Iterator<container_type, typename container_type::iterator>;
	return Utf8View(iterator_type(str.begin()), iterator_type(str.end()));
}

auto utf8(const auto& str) {
	using container_type = std::remove_reference_t<decltype(str)>;
	using iterator_type = Utf8Iterator<std::remove_const_t<container_type>, typename container_type::const_iterator>;
	return Utf8View(iterator_type(str.begin()), iterator_type(str.end()));
}

}

}

static_assert(std::forward_iterator<mint::utf8iterator>);
static_assert(std::forward_iterator<mint::const_utf8iterator>);
static_assert(std::forward_iterator<mint::utf8view_iterator>);
static_assert(std::forward_iterator<mint::const_utf8view_iterator>);

#endif // MINT_SYSTEM_UTF8_H
