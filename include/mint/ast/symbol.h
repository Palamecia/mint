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
		m_symbol(strdup(symbol.data())) {}

	Symbol(const char *symbol) noexcept :
		Symbol(std::string_view(symbol)) {}

	Symbol(Symbol &&other) noexcept;
	Symbol(const Symbol &other);
	~Symbol();

	Symbol &operator=(const Symbol &other);
	Symbol &operator=(Symbol &&other) noexcept;

	inline bool operator==(const Symbol &other) const;
	inline bool operator!=(const Symbol &other) const;

	inline hash_t hash() const;
	inline std::string str() const;

private:
#if !defined(__x86_64__) && !defined(_WIN64)
	static constexpr const hash_t FNV_PRIME = 16777619u;
	static constexpr const hash_t OFFSET_BASIS = 2166136261u;
#else
	static constexpr const hash_t FNV_PRIME = 1099511628211u;
	static constexpr const hash_t OFFSET_BASIS = 14695981039346656037u;
#endif

	static constexpr hash_t make_symbol_hash(std::string_view symbol) {
		return make_symbol_hash_next(symbol.data(), symbol.length(), OFFSET_BASIS, 0);
	}

	static constexpr hash_t make_symbol_hash_next(const char *symbol, std::size_t length, hash_t hash, std::size_t i) {
		return (i < length)
				   ? make_symbol_hash_next(symbol, length, (hash * FNV_PRIME) ^ static_cast<hash_t>(symbol[i]), i + 1)
				   : hash;
	}

	size_t m_size;
	hash_t m_hash;
	char *m_symbol;
};

namespace builtin_symbols {

static const Symbol MOVE_OPERATOR("=");
static const Symbol COPY_OPERATOR(":=");
static const Symbol CALL_OPERATOR("()");
static const Symbol ADD_OPERATOR("+");
static const Symbol SUB_OPERATOR("-");
static const Symbol MUL_OPERATOR("*");
static const Symbol DIV_OPERATOR("/");
static const Symbol POW_OPERATOR("**");
static const Symbol MOD_OPERATOR("%");
static const Symbol IN_OPERATOR("in");
static const Symbol IS_OPERATOR("is");
static const Symbol EQ_OPERATOR("==");
static const Symbol NE_OPERATOR("!=");
static const Symbol LT_OPERATOR("<");
static const Symbol GT_OPERATOR(">");
static const Symbol LE_OPERATOR("<=");
static const Symbol GE_OPERATOR(">=");
static const Symbol AND_OPERATOR("&&");
static const Symbol OR_OPERATOR("||");
static const Symbol BAND_OPERATOR("&");
static const Symbol BOR_OPERATOR("|");
static const Symbol XOR_OPERATOR("^");
static const Symbol INC_OPERATOR("++");
static const Symbol DEC_OPERATOR("--");
static const Symbol NOT_OPERATOR("!");
static const Symbol COMPL_OPERATOR("~");
static const Symbol SHIFT_LEFT_OPERATOR("<<");
static const Symbol SHIFT_RIGHT_OPERATOR(">>");
static const Symbol INCLUSIVE_RANGE_OPERATOR("..");
static const Symbol EXCLUSIVE_RANGE_OPERATOR("...");
static const Symbol TYPEOF_OPERATOR("typeof");
static const Symbol MEMBERSOF_OPERATOR("membersof");
static const Symbol SUBSCRIPT_OPERATOR("[]");
static const Symbol SUBSCRIPT_MOVE_OPERATOR("[]=");
static const Symbol REGEX_MATCH_OPERATOR("=~");
static const Symbol REGEX_UNMATCH_OPERATOR("!~");
static const Symbol NEW_METHOD("new");
static const Symbol DELETE_METHOD("delete");
static const Symbol CLONE_METHOD("clone");
static const Symbol WRITE_METHOD("write");
static const Symbol SHOW_METHOD("show");

}

Symbol::hash_t Symbol::hash() const {
	return m_hash;
}

std::string Symbol::str() const {
	return std::string(m_symbol, m_size);
}

bool Symbol::operator==(const Symbol &other) const {
	return LIKELY((m_size == other.m_size) && !memcmp(m_symbol, other.m_symbol, m_size));
}

bool Symbol::operator!=(const Symbol &other) const {
	return UNLIKELY((m_size != other.m_size) || memcmp(m_symbol, other.m_symbol, m_size));
}

}

template<>
struct std::hash<mint::Symbol> {
	std::size_t operator()(const mint::Symbol &k) const {
		return k.hash();
	}
};

#endif // MINT_SYMBOL_H
