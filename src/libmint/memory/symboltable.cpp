#include "memory/symboltable.h"
#include "memory/memorytool.h"
#include "memory/globaldata.h"
#include "memory/class.h"
#include "system/assert.h"

using namespace std;
using namespace mint;

SymbolTable::SymbolTable(Class *metadata) :
	m_metadata(metadata),
	m_fasts(nullptr) {

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

WeakReference &SymbolTable::createFastReference(const Symbol &name, size_t index) {
	return *(m_fasts[index] = std::unique_ptr<WeakReference>(new WeakReference(get_symbol_reference(this, name))));
}
