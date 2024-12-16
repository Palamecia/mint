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

#ifndef MINT_SYMBOLTABLE_H
#define MINT_SYMBOLTABLE_H

#include "mint/ast/symbol.h"
#include "mint/ast/symbolmapping.hpp"
#include "mint/memory/reference.h"

#include <vector>
#include <memory>

namespace mint {

class Class;
class Cursor;
struct Iterator;
class PackageData;

class MINT_EXPORT SymbolTable : public MemoryRoot {
public:
	using symbol_type = SymbolMapping<Reference>::value_type;
	using weak_symbol_type = SymbolMapping<WeakReference>::value_type;
	using strong_symbol_type = SymbolMapping<StrongReference>::value_type;

	using iterator = SymbolMapping<WeakReference>::iterator;
	using const_iterator = SymbolMapping<WeakReference>::const_iterator;

	explicit SymbolTable(Class *metadata = nullptr);
	~SymbolTable() override;

	SymbolTable(const SymbolTable &other) = delete;
	SymbolTable &operator =(const SymbolTable &other) = delete;

	Class *get_metadata() const;
	PackageData *get_package() const;

	inline void open_package(PackageData *package);
	inline void close_package();

	inline void reserve_fast(size_t count);
	inline WeakReference &setup_fast(const Symbol &name, size_t index, Reference::Flags flags = Reference::standard);
	inline WeakReference get_fast(const Symbol &name, size_t index);
	inline size_t erase_fast(const Symbol &name, size_t index);

	inline Reference &operator [](const Symbol &name);
	inline size_t size() const;
	inline bool empty() const;

	inline bool contains(const Symbol &name) const;
	inline const_iterator find(const Symbol &name) const;
	inline iterator find(const Symbol &name);
	inline const_iterator begin() const;
	inline const_iterator end() const;
	inline iterator begin();
	inline iterator end();

	inline std::pair<iterator, bool> emplace(const Symbol &name, Reference &&reference);
	inline std::pair<iterator, bool> emplace(const Symbol &name, Reference &reference);
	inline std::pair<iterator, bool> insert(const strong_symbol_type &symbol);
	inline std::pair<iterator, bool> insert(const weak_symbol_type &symbol);
	inline std::pair<iterator, bool> insert(const symbol_type &symbol);
	inline size_t erase(const Symbol &name);
	inline iterator erase(iterator position);
	inline void clear();

protected:
	void mark() override {
		for (auto it = m_symbols.begin(); it != m_symbols.end(); ++it) {
			it->second.data()->mark();
		}
	}

private:
	WeakReference &create_fast_reference(const Symbol &name, size_t index);
	WeakReference &create_fast_reference(Reference::Flags flags, const Symbol &name, size_t index);

	Class *m_metadata;
	std::vector<PackageData *> m_package;
	std::unique_ptr<WeakReference> *m_fasts;
	SymbolMapping<WeakReference> m_symbols;
};

void SymbolTable::open_package(PackageData *package) {
	m_package.emplace_back(package);
}

void SymbolTable::close_package() {
	assert(!m_package.empty());
	m_package.pop_back();
}

void SymbolTable::reserve_fast(size_t count) {
	m_fasts = new std::unique_ptr<WeakReference> [count];
}

WeakReference &SymbolTable::setup_fast(const Symbol &name, size_t index, Reference::Flags flags) {
	assert(m_fasts[index] == nullptr || m_fasts[index]->data()->format == Data::fmt_none);
	return create_fast_reference(flags, name, index);
}

WeakReference SymbolTable::get_fast(const Symbol &name, size_t index) {

	if (std::unique_ptr<WeakReference> &reference = m_fasts[index]) {
		return WeakReference::share(*reference);
	}

	return WeakReference::share(create_fast_reference(name, index));
}

size_t SymbolTable::erase_fast(const Symbol &name, size_t index) {
	// assert(m_fasts[index] != nullptr);
	m_fasts[index].reset();
	return erase(name);
}

Reference &SymbolTable::operator [](const Symbol &name) {
	return m_symbols[name];
}

size_t SymbolTable::size() const {
	return m_symbols.size();
}

bool SymbolTable::empty() const {
	return m_symbols.empty();
}

bool SymbolTable::contains(const Symbol &name) const {
	return m_symbols.contains(name);
}

SymbolTable::const_iterator SymbolTable::find(const Symbol &name) const {
	return m_symbols.find(name);
}

SymbolTable::iterator SymbolTable::find(const Symbol &name) {
	return m_symbols.find(name);
}

SymbolTable::const_iterator SymbolTable::begin() const {
	return m_symbols.begin();
}

SymbolTable::const_iterator SymbolTable::end() const {
	return m_symbols.end();
}

SymbolTable::iterator SymbolTable::begin() {
	return m_symbols.begin();
}

SymbolTable::iterator SymbolTable::end() {
	return m_symbols.end();
}

std::pair<SymbolTable::iterator, bool> SymbolTable::emplace(const Symbol &name, Reference &&reference) {
	return m_symbols.emplace(name, std::move(reference));
}

std::pair<SymbolTable::iterator, bool> SymbolTable::emplace(const Symbol &name, Reference &reference) {
	return m_symbols.emplace(name, WeakReference::share(reference));
}

std::pair<SymbolTable::iterator, bool> SymbolTable::insert(const strong_symbol_type &symbol) {
	return m_symbols.emplace(symbol.first, StrongReference::copy(symbol.second));
}

std::pair<SymbolTable::iterator, bool> SymbolTable::insert(const weak_symbol_type &symbol) {
	return m_symbols.emplace(symbol.first, StrongReference::copy(symbol.second));
}

std::pair<SymbolTable::iterator, bool> SymbolTable::insert(const symbol_type &symbol) {
	return m_symbols.emplace(symbol.first, StrongReference::copy(symbol.second));
}

size_t SymbolTable::erase(const Symbol &name) {
	return m_symbols.erase(name);
}

SymbolTable::iterator SymbolTable::erase(iterator position) {
	return m_symbols.erase(position);
}

void SymbolTable::clear() {
	m_symbols.clear();
	delete [] m_fasts;
	m_fasts = nullptr;
}

}

#endif // MINT_SYMBOLTABLE_H
