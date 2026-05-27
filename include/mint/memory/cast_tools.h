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

#ifndef MINT_MEMORY_CAST_TOOLS_H
#define MINT_MEMORY_CAST_TOOLS_H

#include "mint/config.h"
#include "mint/memory/garbage_collector.h"
#include "mint/memory/builtin/array.h"
#include "mint/memory/builtin/hash.h"

#include <concepts>
#include <cstdint>
#include <regex>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace mint {

class Cursor;

MINT_EXPORT double to_number(Cursor& cursor, const Reference& ref);
MINT_EXPORT double to_number(Cursor& cursor, Reference&& ref);
MINT_EXPORT std::intmax_t to_signed_integer(Cursor& cursor, const Reference& ref);
MINT_EXPORT std::intmax_t to_signed_integer(Cursor& cursor, Reference&& ref);
MINT_EXPORT std::uintmax_t to_unsigned_integer(Cursor& cursor, const Reference& ref);
MINT_EXPORT std::uintmax_t to_unsigned_integer(Cursor& cursor, Reference&& ref);
MINT_EXPORT bool to_boolean(const Reference& ref);
MINT_EXPORT std::string to_char(const Reference& ref);
MINT_EXPORT std::string to_string(const Reference& ref);
MINT_EXPORT std::regex to_regex(const Reference& ref);
MINT_EXPORT Array::values_type to_array(const Reference& ref);
MINT_EXPORT Hash::values_type to_hash(const Reference& ref);

MINT_EXPORT double to_unsigned_number(std::string_view str);
MINT_EXPORT double to_signed_number(std::string_view str);

MINT_EXPORT std::uintmax_t to_unsigned_integer(std::string_view str);
MINT_EXPORT std::intmax_t to_signed_integer(std::string_view str);

MINT_EXPORT double to_signed_number(std::intmax_t value);
MINT_EXPORT double to_unsigned_number(std::uintmax_t value);

MINT_EXPORT std::intmax_t to_signed_integer(double value);
MINT_EXPORT std::uintmax_t to_unsigned_integer(double value);

template<std::integral T>
T to_integer(Cursor& cursor, const Reference& ref) {
	if constexpr (std::is_signed_v<T>) {
		return static_cast<T>(to_signed_integer(cursor, ref));
	}
	else {
		return static_cast<T>(to_unsigned_integer(cursor, ref));
	}
}

template<std::integral T>
T to_integer(Cursor& cursor, Reference&& ref) {
	if constexpr (std::is_signed_v<T>) {
		return static_cast<T>(to_signed_integer(cursor, std::move(ref)));
	}
	else {
		return static_cast<T>(to_unsigned_integer(cursor, std::move(ref)));
	}
}

template<std::integral T>
T to_integer(std::string_view str) {
	if constexpr (std::is_signed_v<T>) {
		return static_cast<T>(to_signed_integer(str));
	}
	else {
		return static_cast<T>(to_unsigned_integer(str));
	}
}

template<std::integral T>
double to_number(T value) {
	if constexpr (std::is_signed_v<T>) {
		return to_signed_number(value);
	}
	else {
		return to_unsigned_number(value);
	}
}

template<std::integral T>
T to_integer(double value) {
	if constexpr (std::is_signed_v<T>) {
		return static_cast<T>(to_signed_integer(value));
	}
	else {
		return static_cast<T>(to_unsigned_integer(value));
	}
}

}

#endif // MINT_MEMORY_CAST_TOOLS_H
