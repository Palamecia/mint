#include "memory/symboltable.h"
#include "memory/builtin/iterator.h"
#include "memory/class.h"
#include "system/assert.h"

using namespace std;
using namespace mint;

SymbolTable::SymbolTable(Class *metadata) :
	m_metadata(metadata),
	m_defaultResult(Reference::const_address | Reference::const_value) {

}

SymbolTable::~SymbolTable() {

}

Class *SymbolTable::getMetadata() const {
	return m_metadata;
}

PackageData *SymbolTable::getPackage() const {

	if (m_package.empty()) {
		return &GlobalData::instance();
	}

	return m_package.top();
}

Reference &SymbolTable::defaultResult() {
	return m_defaultResult;
}

ReferenceManager *SymbolTable::referenceManager() {
	return &m_referenceManager;
}

void SymbolTable::openPackage(PackageData *package) {
	m_package.push(package);
}

void SymbolTable::closePackage() {
	assert(!m_package.empty());
	m_package.pop();
}

Iterator *SymbolTable::generator() {

	if (m_defaultResult.data()->format == Data::fmt_object
			&& m_defaultResult.data<Object>()->metadata->metatype() == Class::iterator) {
		return m_defaultResult.data<Iterator>();
	}

	return nullptr;
}

Reference &SymbolTable::operator [](const string &name) {
	return m_symbols[name];
}

size_t SymbolTable::size() const {
	return m_symbols.size();
}

bool SymbolTable::empty() const {
	return m_symbols.empty();
}

SymbolTable::const_iterator SymbolTable::find(const string &name) const {
	return m_symbols.find(name);
}

SymbolTable::iterator SymbolTable::find(const string &name) {
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

pair<SymbolTable::iterator, bool> SymbolTable::emplace(const string &name, Reference &&reference) {
	return m_symbols.emplace(name, reference);
}

pair<SymbolTable::iterator, bool> SymbolTable::emplace(const string &name, Reference &reference) {
	return m_symbols.emplace(name, reference);
}

pair<SymbolTable::iterator, bool> SymbolTable::insert(const strong_symbol_type &symbol) {
	return m_symbols.insert(symbol);
}

pair<SymbolTable::iterator, bool> SymbolTable::insert(const weak_symbol_type &symbol) {
	return m_symbols.insert(symbol);
}

pair<SymbolTable::iterator, bool> SymbolTable::insert(const symbol_type &symbol) {
	return m_symbols.insert(symbol);
}

size_t SymbolTable::erase(const string &name) {
	return m_symbols.erase(name);
}

SymbolTable::iterator SymbolTable::erase(iterator position) {
	return m_symbols.erase(position);
}

void SymbolTable::clear() {
	m_symbols.clear();
}
