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

#ifndef MINT_AST_SYMBOL_H
#define MINT_AST_SYMBOL_H

#include "mint/config.h"

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <functional>
#include <new>
#include <string_view>
#include <cstring>
#include <string>
#include <gsl/pointers>

namespace mint {

class MINT_EXPORT Symbol {
public:
	using hash_t = std::size_t;

	constexpr Symbol(const char* symbol) noexcept :
	    Symbol(std::string_view(symbol)) {}

	constexpr Symbol(std::string_view symbol) noexcept :
	    _hash(make_symbol_hash(symbol)),
	    _size(symbol.length()) {
		try {
			_symbol = new char[_size + 1];
			copy_symbol_data(_symbol, symbol.data(), symbol.length());
		}
		catch (const std::bad_alloc&) {
			_symbol = nullptr;
		}
	}

	constexpr Symbol(Symbol&& other) noexcept :
	    _hash(other._hash),
	    _size(other._size),
	    _symbol(other._symbol) {
		other._symbol = nullptr;
	}

	Symbol(const Symbol& other) :
	    _hash(other._hash),
	    _size(other._size),
	    _symbol(new char[_size + 1]) {
		copy_symbol_data(_symbol, other._symbol, other._size);
	}

	~Symbol() noexcept {
		delete[] _symbol;
	}

	constexpr Symbol& operator=(Symbol&& other) noexcept {
		_size = other._size;
		_hash = other._hash;
		std::swap(_symbol, other._symbol);
		return *this;
	}

	Symbol& operator=(const Symbol& other) {
		_size = other._size;
		_hash = other._hash;
		delete[] _symbol;
		_symbol = new char[_size + 1];
		copy_symbol_data(_symbol, other._symbol, other._size);
		return *this;
	}

	constexpr bool operator==(const Symbol& other) const noexcept {
		return (_size == other._size) && memcmp(_symbol, other._symbol, _size) == 0;
	}

	constexpr bool operator!=(const Symbol& other) const noexcept {
		return (_size != other._size) || memcmp(_symbol, other._symbol, _size) != 0;
	}

	[[nodiscard]] constexpr hash_t hash() const noexcept {
		return _hash;
	}

	[[nodiscard]] std::string str() const noexcept {
		return {_symbol, _size};
	}

private:
#if !defined(__x86_64__) && !defined(_WIN64)
	static constexpr const hash_t fnv_prime = 16777619u;
	static constexpr const hash_t offset_basis = 2166136261u;
#else
	static constexpr const hash_t fnv_prime = 1099511628211u;
	static constexpr const hash_t offset_basis = 14695981039346656037u;
#endif

	static constexpr hash_t make_symbol_hash(std::string_view symbol) noexcept {
		std::size_t hash = offset_basis;
		for (std::size_t i = 0; i < symbol.length(); ++i) {
			hash = hash * fnv_prime;
			hash = hash ^ static_cast<hash_t>(symbol[i]);
		}
		return hash;
	}

	static constexpr char* copy_symbol_data(char* copy, const char* data, std::size_t length) noexcept {
		if consteval {
			std::copy(data, data + length, copy);
		}
		else {
			std::memcpy(copy, data, length);
		}
		copy[length] = '\0';
		return copy;
	}

	hash_t _hash;
	std::size_t _size;
	gsl::owner<char*> _symbol {nullptr};
};

namespace builtin_symbols {

static const Symbol move_operator("=");
static const Symbol copy_operator(":=");
static const Symbol call_operator("()");
static const Symbol add_operator("+");
static const Symbol sub_operator("-");
static const Symbol mul_operator("*");
static const Symbol div_operator("/");
static const Symbol pow_operator("**");
static const Symbol mod_operator("%");
static const Symbol in_operator("in");
static const Symbol is_operator("is");
static const Symbol eq_operator("==");
static const Symbol ne_operator("!=");
static const Symbol lt_operator("<");
static const Symbol gt_operator(">");
static const Symbol le_operator("<=");
static const Symbol ge_operator(">=");
static const Symbol and_operator("&&");
static const Symbol or_operator("||");
static const Symbol band_operator("&");
static const Symbol bor_operator("|");
static const Symbol xor_operator("^");
static const Symbol inc_operator("++");
static const Symbol dec_operator("--");
static const Symbol not_operator("!");
static const Symbol compl_operator("~");
static const Symbol shift_left_operator("<<");
static const Symbol shift_right_operator(">>");
static const Symbol inclusive_range_operator("..");
static const Symbol exclusive_range_operator("...");
static const Symbol typeof_operator("typeof");
static const Symbol membersof_operator("membersof");
static const Symbol subscript_operator("[]");
static const Symbol subscript_move_operator("[]=");
static const Symbol regex_match_operator("=~");
static const Symbol regex_unmatch_operator("!~");
static const Symbol new_method("new");
static const Symbol delete_method("delete");
static const Symbol clone_method("clone");
static const Symbol write_method("write");
static const Symbol show_method("show");
static const Symbol await_method("await");
static const Symbol to_number("toNumber");
static const Symbol to_boolean("toBoolean");
static const Symbol to_string("toString");
static const Symbol to_regex("toRegex");
static const Symbol to_array("toArray");
static const Symbol to_hash("toHash");

}

}

template<>
struct std::hash<mint::Symbol> {
	std::size_t operator()(const mint::Symbol& k) const {
		return k.hash();
	}
};

#endif // MINT_AST_SYMBOL_H
