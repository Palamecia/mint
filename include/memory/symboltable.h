#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <ast/symbol.h>
#include <ast/symbolmapping.hpp>
#include <memory/reference.h>

#include <vector>

namespace mint {

class Class;
class Cursor;
struct Iterator;
class PackageData;

class MINT_EXPORT SymbolTable {
public:
	using symbol_type = SymbolMapping<Reference>::value_type;
	using weak_symbol_type = SymbolMapping<WeakReference>::value_type;
	using strong_symbol_type = SymbolMapping<StrongReference>::value_type;

	using iterator = SymbolMapping<StrongReference>::iterator;
	using const_iterator = SymbolMapping<StrongReference>::const_iterator;

	static_assert(SymbolMapping<StrongReference>::Optimizable, "unoptimized SymbolTable");

	SymbolTable(const SymbolTable &other) = delete;
	SymbolTable(Class *metadata = nullptr);
	~SymbolTable();

	SymbolTable &operator =(const SymbolTable &other) = delete;

	Class *getMetadata() const;
	PackageData *getPackage() const;

	Reference &defaultResult();

	inline void openPackage(PackageData *package);
	inline void closePackage();

	void setupGenerator(Cursor *cursor, size_t stack_size);
	Iterator *generator();

	inline void reserve_fast(size_t count);
	inline WeakReference &setup_fast(const Symbol &name, size_t index);
	inline WeakReference get_fast(const Symbol &name, size_t index);

	inline Reference &operator [](const Symbol &name);
	inline size_t size() const;
	inline bool empty() const;

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

private:
	WeakReference &createFastReference(const Symbol &name, size_t index);

	Class *m_metadata;
	std::vector<PackageData *> m_package;

	StrongReference m_defaultResult;
	std::unique_ptr<WeakReference> *m_fasts;
	SymbolMapping<StrongReference> m_symbols;
};

void SymbolTable::openPackage(PackageData *package) {
	m_package.emplace_back(package);
}

void SymbolTable::closePackage() {
	assert(!m_package.empty());
	m_package.pop_back();
}

void SymbolTable::reserve_fast(size_t count) {
	m_fasts = new std::unique_ptr<WeakReference> [count];
}

WeakReference &SymbolTable::setup_fast(const Symbol &name, size_t index) {
	assert(m_fasts[index] == nullptr);
	return createFastReference(name, index);
}

WeakReference SymbolTable::get_fast(const Symbol &name, size_t index) {

	if (std::unique_ptr<WeakReference> &reference = m_fasts[index]) {
		return WeakReference::share(*reference);
	}

	return WeakReference::share(createFastReference(name, index));
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
	return m_symbols.emplace(symbol.first, StrongReference(symbol.second.flags(), symbol.second.data()));
}

std::pair<SymbolTable::iterator, bool> SymbolTable::insert(const weak_symbol_type &symbol) {
	return m_symbols.emplace(symbol.first, StrongReference(symbol.second.flags(), symbol.second.data()));
}

std::pair<SymbolTable::iterator, bool> SymbolTable::insert(const symbol_type &symbol) {
	return m_symbols.emplace(symbol.first, StrongReference(symbol.second.flags(), symbol.second.data()));
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

#endif // SYMBOL_TABLE_H
