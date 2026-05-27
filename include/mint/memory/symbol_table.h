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

#ifndef MINT_MEMORY_SYMBOLTABLE_H
#define MINT_MEMORY_SYMBOLTABLE_H

#include "mint/ast/symbol.h"
#include "mint/config.h"
#include "mint/memory/garbage_collector.h"
#include "mint/memory/reference.h"

#include <cassert>
#include <cstddef>
#include <functional>
#include <unordered_map>
#include <utility>
#include <vector>
#include <memory>

namespace mint {

class Class;
class GlobalData;
class Iterator;
class PackageData;

class MINT_EXPORT SymbolTable {
public:
	using symbol_type = std::unordered_map<Symbol, Reference>::value_type;

	using iterator = std::unordered_map<Symbol, Reference>::iterator;
	using const_iterator = std::unordered_map<Symbol, Reference>::const_iterator;

	explicit SymbolTable(GlobalData& global_data, Class* metadata = nullptr);
	SymbolTable(const SymbolTable&) = delete;
	SymbolTable(SymbolTable&&) = delete;
	~SymbolTable() = default;

	SymbolTable& operator=(const SymbolTable&) = delete;
	SymbolTable& operator=(SymbolTable&&) = delete;

	[[nodiscard]] Class* get_metadata() const;
	[[nodiscard]] PackageData& get_package() const;
	[[nodiscard]] GlobalData& get_global_data() const;

	inline void open_package(PackageData& package);
	inline void close_package();

	inline void reserve_fast(std::size_t count);
	inline Reference& setup_fast(const Symbol& name, std::size_t index,
	    Reference::Flags flags = Reference::default_flags);
	inline Reference get_fast(const Symbol& name, std::size_t index);
	inline std::size_t erase_fast(const Symbol& name, std::size_t index);

	inline Reference& operator[](const Symbol& name);
	[[nodiscard]] inline std::size_t size() const;
	[[nodiscard]] inline bool empty() const;

	[[nodiscard]] inline bool contains(const Symbol& name) const;
	[[nodiscard]] inline const_iterator find(const Symbol& name) const;
	[[nodiscard]] inline iterator find(const Symbol& name);
	[[nodiscard]] inline const_iterator begin() const;
	[[nodiscard]] inline const_iterator end() const;
	[[nodiscard]] inline iterator begin();
	[[nodiscard]] inline iterator end();

	inline std::pair<iterator, bool> emplace(const Symbol& name, const Reference& reference);
	inline std::pair<iterator, bool> emplace(const Symbol& name, Reference&& reference);
	inline std::pair<iterator, bool> insert(const symbol_type& symbol);
	inline std::size_t erase(const Symbol& name);
	inline iterator erase(iterator position);
	inline void clear();

	void mark() {
		for (auto& symbol : _symbols) {
			symbol.second.data().mark();
		}
	}

private:
	Reference& create_fast_reference(const Symbol& name, std::size_t index);
	Reference& create_fast_reference(Reference::Flags flags, const Symbol& name, std::size_t index);

	std::unique_ptr<std::unique_ptr<Reference>[]> _fasts;
	std::unordered_map<Symbol, Reference> _symbols;

	Class* _metadata;
	std::reference_wrapper<GlobalData> _global_data;
	std::vector<std::reference_wrapper<PackageData>> _package;
};

void SymbolTable::open_package(PackageData& package) {
	_package.emplace_back(std::ref(package));
}

void SymbolTable::close_package() {
	assert(!_package.empty());
	_package.pop_back();
}

void SymbolTable::reserve_fast(std::size_t count) {
	_fasts = std::make_unique<std::unique_ptr<Reference>[]>(count);
}

Reference& SymbolTable::setup_fast(const Symbol& name, std::size_t index, Reference::Flags flags) {
	assert(_fasts[index] == nullptr || _fasts[index]->data().format() == Data::Format::none);
	return create_fast_reference(flags, name, index);
}

Reference SymbolTable::get_fast(const Symbol& name, std::size_t index) {
	if (const auto& reference = _fasts[index]) {
		return *reference;
	}
	return create_fast_reference(name, index);
}

std::size_t SymbolTable::erase_fast(const Symbol& name, std::size_t index) {
	// assert(_fasts[index] != nullptr);
	_fasts[index].reset();
	return erase(name);
}

Reference& SymbolTable::operator[](const Symbol& name) {
	return _symbols[name];
}

std::size_t SymbolTable::size() const {
	return _symbols.size();
}

bool SymbolTable::empty() const {
	return _symbols.empty();
}

bool SymbolTable::contains(const Symbol& name) const {
	return _symbols.contains(name);
}

SymbolTable::const_iterator SymbolTable::find(const Symbol& name) const {
	return _symbols.find(name);
}

SymbolTable::iterator SymbolTable::find(const Symbol& name) {
	return _symbols.find(name);
}

SymbolTable::const_iterator SymbolTable::begin() const {
	return _symbols.begin();
}

SymbolTable::const_iterator SymbolTable::end() const {
	return _symbols.end();
}

SymbolTable::iterator SymbolTable::begin() {
	return _symbols.begin();
}

SymbolTable::iterator SymbolTable::end() {
	return _symbols.end();
}

std::pair<SymbolTable::iterator, bool> SymbolTable::emplace(const Symbol& name, const Reference& reference) {
	return _symbols.emplace(name, reference);
}

std::pair<SymbolTable::iterator, bool> SymbolTable::emplace(const Symbol& name, Reference&& reference) {
	return _symbols.emplace(name, std::move(reference));
}

std::pair<SymbolTable::iterator, bool> SymbolTable::insert(const symbol_type& symbol) {
	return _symbols.emplace(symbol.first, Reference(create_from, symbol.second));
}

std::size_t SymbolTable::erase(const Symbol& name) {
	return _symbols.erase(name);
}

SymbolTable::iterator SymbolTable::erase(iterator position) {
	return _symbols.erase(position);
}

void SymbolTable::clear() {
	_fasts.reset();
	_symbols.clear();
}

}

#endif // MINT_MEMORY_SYMBOLTABLE_H
