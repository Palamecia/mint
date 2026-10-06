/**
 * Copyright (c) 2026 Gauvain CHÉRY.
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

#include "mint/program/symbol.h"
#include <cstddef>
#include <initializer_list>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace mint;

mint::SymbolPath::SymbolPath(const Symbol& symbol) :
    _symbols({symbol}) {}

mint::SymbolPath::SymbolPath(const std::string& path) :
    _symbols(std::from_range,
        std::views::split(path, std::string(".")) | std::views::transform([](const auto symbol) -> Symbol {
	        return std::string_view(symbol);
        })) {}

mint::SymbolPath::SymbolPath(std::initializer_list<Symbol> path) :
    _symbols(path) {}

mint::SymbolPath::SymbolPath(const SymbolPath& other, const Symbol& symbol) :
    _symbols(other._symbols) {
	_symbols.push_back(symbol);
}

size_t mint::SymbolPath::size() const noexcept {
	return _symbols.size();
}

bool mint::SymbolPath::empty() const noexcept {
	return _symbols.empty();
}

size_t mint::SymbolPath::capacity() const noexcept {
	return _symbols.capacity();
}

void mint::SymbolPath::reserve(size_t new_cap) {
	_symbols.reserve(new_cap);
}

void mint::SymbolPath::shrink_to_fit() {
	_symbols.shrink_to_fit();
}

void mint::SymbolPath::push_back(const Symbol& value) {
	_symbols.push_back(value);
}

void mint::SymbolPath::push_back(Symbol&& value) {
	_symbols.push_back(std::move(value));
}

void mint::SymbolPath::pop_back() {
	_symbols.pop_back();
}

void mint::SymbolPath::clear() noexcept {
	_symbols.clear();
}

Symbol& mint::SymbolPath::operator[](std::size_t index) noexcept {
	return _symbols[index];
}

const Symbol& mint::SymbolPath::operator[](std::size_t index) const noexcept {
	return _symbols[index];
}

Symbol& mint::SymbolPath::at(std::size_t index) {
	return _symbols.at(index);
}

const Symbol& mint::SymbolPath::at(std::size_t index) const {
	return _symbols.at(index);
}

Symbol& mint::SymbolPath::front() noexcept {
	return _symbols.front();
}

const Symbol& mint::SymbolPath::front() const noexcept {
	return _symbols.front();
}

Symbol& mint::SymbolPath::back() noexcept {
	return _symbols.back();
}

const Symbol& mint::SymbolPath::back() const noexcept {
	return _symbols.back();
}

mint::SymbolPath::iterator mint::SymbolPath::begin() noexcept {
	return _symbols.begin();
}

mint::SymbolPath::iterator mint::SymbolPath::end() noexcept {
	return _symbols.end();
}

mint::SymbolPath::const_iterator mint::SymbolPath::begin() const noexcept {
	return _symbols.begin();
}

mint::SymbolPath::const_iterator mint::SymbolPath::end() const noexcept {
	return _symbols.end();
}

mint::SymbolPath::const_iterator mint::SymbolPath::cbegin() const noexcept {
	return _symbols.cbegin();
}

mint::SymbolPath::const_iterator mint::SymbolPath::cend() const noexcept {
	return _symbols.cend();
}

std::string mint::SymbolPath::to_string() const {
	return std::views::transform(_symbols, &Symbol::str) | std::views::join_with(std::string("."))
	       | std::ranges::to<std::string>();
}
