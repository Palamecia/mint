#include "Memory/globaldata.h"
#include "Memory/class.h"
#include "System/error.h"

using namespace std;

GlobalData &GlobalData::instance() {
	static GlobalData g_instance;
	return g_instance;
}

int GlobalData::createClass(Class *desc) {

	size_t id = m_definedClasses.size();
	m_definedClasses.push_back(desc);
	return id;
}

void GlobalData::registerClass(int id) {

	Class *desc = m_definedClasses[id];
	if (m_registeredClasses.find(desc->name()) != m_registeredClasses.end()) {
		error("multiple definition of class %s", desc->name().c_str());
	}
	m_registeredClasses.insert({desc->name(), desc});
}

Class *GlobalData::getClass(const string &name) {

	auto it = m_registeredClasses.find(name);
	if (it != m_registeredClasses.end()) {
		return it->second;
	}
	return nullptr;
}

SymbolTable &GlobalData::symbols() {
	return m_symbols;
}

GlobalData::GlobalData() {}
