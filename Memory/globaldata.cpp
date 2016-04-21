#include "globaldata.h"

GlobalData &GlobalData::instance() {
	static GlobalData g_instance;
	return g_instance;
}

void GlobalData::registerClass(const std::string &name, size_t id) {
	/// \todo à implémenter
}

void GlobalData::registerSymbol(const std::string &name, size_t id) {
	/// \todo à implémenter
}

SymbolTable &GlobalData::symbols() {
	return m_symbols;
}

GlobalData::GlobalData() {}
