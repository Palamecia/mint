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

#ifndef MINT_SYMBOL_H
#define MINT_SYMBOL_H

#include "mint/config.h"

#include <string_view>
#include <cstring>
#include <string>

namespace mint {

class MINT_EXPORT Symbol {
public:
	using hash_t = std::size_t;

	Symbol(std::string_view symbol) noexcept :
		m_size(symbol.length()),
		m_hash(make_symbol_hash(symbol)),
		m_symbol(strdup(symbol.data())) {

	}

	Symbol(const char *symbol) noexcept :
		Symbol(std::string_view(symbol)) {

	}

	Symbol(Symbol &&other) noexcept;
	Symbol(const Symbol &other);
	~Symbol();

	Symbol &operator =(const Symbol &other);
	Symbol &operator =(Symbol &&other) noexcept;

	inline bool operator ==(const Symbol &other) const;
	inline bool operator !=(const Symbol &other) const;

	inline hash_t hash() const;
	inline std::string str() const;

private:
#if !defined (__x86_64__) && !defined (_WIN64)
	static constexpr const hash_t fnv_prime = 16777619u;
	static constexpr const hash_t offset_basis = 2166136261u;
#else
	static constexpr const hash_t fnv_prime = 1099511628211u;
	static constexpr const hash_t offset_basis = 14695981039346656037u;
#endif

	static constexpr hash_t make_symbol_hash(std::string_view symbol) {
		return make_symbol_hash_next(symbol.data(), symbol.length(), offset_basis, 0);
	}

	static constexpr hash_t make_symbol_hash_next(const char *symbol, std::size_t length, hash_t hash, std::size_t i) {
		return (i < length) ? make_symbol_hash_next(symbol, length, (hash * fnv_prime) ^ static_cast<hash_t>(symbol[i]), i + 1) : hash;
	}

	size_t m_size;
	hash_t m_hash;
	char *m_symbol;
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

}

Symbol::hash_t Symbol::hash() const {
	return m_hash;
}

std::string Symbol::str() const {
	return std::string(m_symbol, m_size);
}

bool Symbol::operator ==(const Symbol &other) const {
	return LIKELY((m_size == other.m_size) && !memcmp(m_symbol, other.m_symbol, m_size));
}

bool Symbol::operator !=(const Symbol &other) const {
	return UNLIKELY((m_size != other.m_size) || memcmp(m_symbol, other.m_symbol, m_size));
}

}

template <>
struct std::hash<mint::Symbol> {
	std::size_t operator()(const mint::Symbol &k) const {
		return k.hash();
	}
};

#endif // MINT_SYMBOL_H
