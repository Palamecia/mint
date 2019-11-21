#include "memory/symboltable.h"
#include "memory/builtin/iterator.h"
#include "memory/class.h"
#include "system/assert.h"

using namespace mint;

SymbolTable::SymbolTable(Class *metadata) :
	m_metadata(metadata),
	m_defaultResult(Reference::const_address | Reference::const_value) {

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
