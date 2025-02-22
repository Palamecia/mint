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

#include "mint/ast/symbol.h"
#include "mint/config.h"

using namespace mint;

Symbol::Symbol(Symbol &&other) noexcept :
	m_size(other.m_size),
	m_hash(other.m_hash),
	m_symbol(other.m_symbol) {
	other.m_symbol = nullptr;
}

Symbol::Symbol(const Symbol &other) :
	m_size(other.m_size),
	m_hash(other.m_hash),
	m_symbol(strdup(other.m_symbol)) {}

Symbol::~Symbol() {
	free(m_symbol);
}

Symbol &Symbol::operator=(const Symbol &other) {
	if (UNLIKELY(this == &other)) {
		return *this;
	}
	m_size = other.m_size;
	m_hash = other.m_hash;
	m_symbol = static_cast<char *>(realloc(m_symbol, other.m_size + 1));
	memcpy(m_symbol, other.m_symbol, other.m_size + 1);
	return *this;
}

Symbol &Symbol::operator=(Symbol &&other) noexcept {
	m_size = other.m_size;
	m_hash = other.m_hash;
	std::swap(m_symbol, other.m_symbol);
	return *this;
}
