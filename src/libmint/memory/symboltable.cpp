#include "memory/symboltable.h"
#include "memory/class.h"

using namespace mint;

SymbolTable::SymbolTable(Class *metadata) :
	m_metadata(metadata),
	m_defaultResult(Reference::const_address | Reference::const_value) {

}

Class *SymbolTable::getMetadata() const {
	return m_metadata;
}

Reference &SymbolTable::defaultResult() {
	return m_defaultResult;
}
