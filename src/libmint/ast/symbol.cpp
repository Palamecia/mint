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

#include "mint/ast/symbol.h"

using namespace mint;
using namespace std;

const Symbol Symbol::move_operator = Symbol("=");
const Symbol Symbol::copy_operator = Symbol(":=");
const Symbol Symbol::call_operator = Symbol("()");
const Symbol Symbol::add_operator = Symbol("+");
const Symbol Symbol::sub_operator = Symbol("-");
const Symbol Symbol::mul_operator = Symbol("*");
const Symbol Symbol::div_operator = Symbol("/");
const Symbol Symbol::pow_operator = Symbol("**");
const Symbol Symbol::mod_operator = Symbol("%");
const Symbol Symbol::in_operator = Symbol("in");
const Symbol Symbol::is_operator = Symbol("is");
const Symbol Symbol::eq_operator = Symbol("==");
const Symbol Symbol::ne_operator = Symbol("!=");
const Symbol Symbol::lt_operator = Symbol("<");
const Symbol Symbol::gt_operator = Symbol(">");
const Symbol Symbol::le_operator = Symbol("<=");
const Symbol Symbol::ge_operator = Symbol(">=");
const Symbol Symbol::and_operator = Symbol("&&");
const Symbol Symbol::or_operator = Symbol("||");
const Symbol Symbol::band_operator = Symbol("&");
const Symbol Symbol::bor_operator = Symbol("|");
const Symbol Symbol::xor_operator = Symbol("^");
const Symbol Symbol::inc_operator = Symbol("++");
const Symbol Symbol::dec_operator = Symbol("--");
const Symbol Symbol::not_operator = Symbol("!");
const Symbol Symbol::compl_operator = Symbol("~");
const Symbol Symbol::shift_left_operator = Symbol("<<");
const Symbol Symbol::shift_right_operator = Symbol(">>");
const Symbol Symbol::inclusive_range_operator = Symbol("..");
const Symbol Symbol::exclusive_range_operator = Symbol("...");
const Symbol Symbol::typeof_operator = Symbol("typeof");
const Symbol Symbol::membersof_operator = Symbol("membersof");
const Symbol Symbol::subscript_operator = Symbol("[]");
const Symbol Symbol::subscript_move_operator = Symbol("[]=");
const Symbol Symbol::regex_match_operator = Symbol("=~");
const Symbol Symbol::regex_unmatch_operator = Symbol("!~");
const Symbol Symbol::new_method = Symbol("new");
const Symbol Symbol::delete_method = Symbol("delete");
const Symbol Symbol::write_method = Symbol("write");
const Symbol Symbol::show_method = Symbol("show");

Symbol::Symbol(const string &symbol) :
	m_size(symbol.size()),
	m_hash(make_symbol_hash(symbol.data(), m_size)),
	m_symbol(strdup(symbol.data())) {

}

Symbol::Symbol(const Symbol &other) :
	m_size(other.m_size),
	m_hash(other.m_hash),
	m_symbol(strdup(other.m_symbol)) {

}

Symbol::Symbol(Symbol &&other) noexcept :
	m_size(other.m_size),
	m_hash(other.m_hash),
	m_symbol(other.m_symbol) {
	other.m_symbol = nullptr;
}

Symbol::~Symbol() {
	free(m_symbol);
}

Symbol &Symbol::operator =(const Symbol &other) {
	m_size = other.m_size;
	m_hash = other.m_hash;
	m_symbol = static_cast<char *>(realloc(m_symbol, other.m_size + 1));
	memcpy(m_symbol, other.m_symbol, other.m_size + 1);
	return *this;
}

Symbol &Symbol::operator =(Symbol &&other) noexcept {
	m_size = other.m_size;
	m_hash = other.m_hash;
	std::swap(m_symbol, other.m_symbol);
	return *this;
}

Symbol::hash_t Symbol::make_symbol_hash(const char *symbol, size_t length) {

	size_t hash = offset_basis;

	for (size_t i = 0; i < length; ++i) {
		hash = hash * fnv_prime;
		hash = hash ^ static_cast<hash_t>(symbol[i]);
	}

	return hash;
}
