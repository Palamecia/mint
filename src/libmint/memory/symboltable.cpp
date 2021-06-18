#include "memory/symboltable.h"
#include "memory/memorytool.h"
#include "memory/class.h"
#include "system/assert.h"

using namespace std;
using namespace mint;

SymbolTable::SymbolTable(Class *metadata) :
	m_metadata(metadata),
	m_fasts(nullptr),
	m_defaultResult(Reference::const_address | Reference::const_value) {

}

SymbolTable::~SymbolTable() {
	delete [] m_fasts;
}

Class *SymbolTable::getMetadata() const {
	return m_metadata;
}

PackageData *SymbolTable::getPackage() const {

	if (m_package.empty()) {
		return &GlobalData::instance();
	}

	return m_package.back();
}

Reference &SymbolTable::defaultResult() {
	return m_defaultResult;
}

void SymbolTable::openPackage(PackageData *package) {
	m_package.emplace_back(package);
}

void SymbolTable::closePackage() {
	assert(!m_package.empty());
	m_package.pop_back();
}

void SymbolTable::setupGenerator(Cursor *cursor, size_t stack_size) {
	m_defaultResult = StrongReference(Reference::standard, Reference::alloc<Iterator>(cursor, stack_size));
	m_defaultResult.data<Iterator>()->construct();
}

Iterator *SymbolTable::generator() {

	if (m_defaultResult.data()->format == Data::fmt_object && m_defaultResult.data<Object>()->metadata->metatype() == Class::iterator) {
		return m_defaultResult.data<Iterator>();
	}

	return nullptr;
}

SharedReference &SymbolTable::createFastReference(const Symbol &name, size_t index) {
	return m_fasts[index] = get_symbol_reference(this, name);
}
