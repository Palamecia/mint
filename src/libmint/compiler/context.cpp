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

#include "context.h"
#include "mint/ast/symbol.h"
#include "mint/compiler/build_tools.h"
#include <cstddef>

using namespace mint;

std::size_t mint::find_fast_symbol_index(const Definition& def, const Symbol& symbol) {
	if (auto i = def.fast_symbol_indexes.find(symbol); i != def.fast_symbol_indexes.end()) {
		return i->second;
	}
	return invalid_index;
}

std::size_t mint::create_fast_symbol_index(Definition& def, const Symbol& symbol) {
	const auto index = def.fast_symbol_count++;
	return def.fast_symbol_indexes[symbol] = index;
}

std::size_t mint::fast_symbol_index(Definition& def, const Symbol& symbol) {
	if (auto i = def.fast_symbol_indexes.find(symbol); i != def.fast_symbol_indexes.end()) {
		return i->second;
	}
	const auto index = def.fast_symbol_count++;
	return def.fast_symbol_indexes[symbol] = index;
}
