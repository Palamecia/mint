#include "memory/globaldata.h"
#include "memory/memorytool.h"
#include "memory/class.h"
#include "system/error.h"

#include <algorithm>
#include <limits>

using namespace std;
using namespace mint;

PackageData::PackageData(const string &name, PackageData *owner) :
	m_name(name),
	m_owner(owner) {

}

PackageData::~PackageData() {
	for (auto &package : m_packages) {
		delete package.second;
	}
}

PackageData *PackageData::getPackage() const {
	return m_owner;
}

PackageData *PackageData::getPackage(const Symbol &name) {
	auto it = m_packages.find(name);
	if (it == m_packages.end()) {
		PackageData *package = new PackageData(name.str(), this);
		m_symbols.emplace(name, WeakReference(Reference::global | Reference::const_address | Reference::const_value, Reference::alloc<Package>(package)));
		it = m_packages.emplace(name, package).first;
	}
	return it->second;
}

PackageData *PackageData::findPackage(const Symbol &name) const {
	auto it = m_packages.find(name);
	if (it != m_packages.end()) {
		return it->second;
	}
	return nullptr;
}

void PackageData::registerClass(ClassRegister::Id id) {

	ClassDescription *desc = getClassDescription(id);
	Symbol &&symbol = desc->name();

	if (UNLIKELY(m_symbols.find(symbol) != m_symbols.end())) {
		string symbol_str = symbol.str();
		error("multiple definition of class '%s'", symbol_str.c_str());
	}

	Class *type = desc->generate();
	m_symbols.emplace(symbol, WeakReference(Reference::global | Reference::const_address | Reference::const_value, type->makeInstance()));
}

Class *PackageData::getClass(const Symbol &name) {

	auto it = m_symbols.find(name);
	if (it != m_symbols.end() && it->second.data()->format == Data::fmt_object && is_class(it->second.data<Object>())) {
		return it->second.data<Object>()->metadata;
	}
	return nullptr;
}

string PackageData::name() const {
	return m_name;
}

string PackageData::fullName() const {

	if (m_owner && m_owner != GlobalData::instance()) {
		return m_owner->fullName() + "." + name();
	}

	return name();
}

void PackageData::cleanupMemory() {

	for (auto symbol = m_symbols.begin(); symbol != m_symbols.end();) {
		if (symbol->second.data()->format == Data::fmt_object && is_class(symbol->second.data<Object>())) {
			symbol->second.data<Object>()->metadata->cleanupMemory();
			++symbol;
		}
		else {
			symbol = m_symbols.erase(symbol);
		}
	}

	for (auto &package : m_packages) {
		package.second->cleanupMemory();
	}
}

void PackageData::cleanupMetadata() {

	for (auto &symbol : m_symbols) {
		if (symbol.second.data()->format == Data::fmt_object && is_class(symbol.second.data<Object>())) {
			symbol.second.data<Object>()->metadata->cleanupMetadata();
		}
	}

	m_symbols.clear();

	for (auto &package : m_packages) {
		package.second->cleanupMetadata();
		delete package.second;
	}

	m_packages.clear();
}

GlobalData *GlobalData::g_instance = nullptr;

GlobalData::GlobalData() : PackageData("(default)") {
	m_builtin.fill(nullptr);
	g_instance = this;
}

GlobalData::~GlobalData() {
	for_each(m_builtin.begin(), m_builtin.end(), default_delete<Class>());
	delete m_none;
	delete m_null;
	g_instance = nullptr;
}

GlobalData *GlobalData::instance() {
	return g_instance;
}

void GlobalData::cleanupBuiltin() {

	// cleanup builtin classes
	for_each(m_builtin.begin(), m_builtin.end(), default_delete<Class>());
	m_builtin.fill(nullptr);

	// cleanup builtin refs
	delete m_none;
	m_none = nullptr;

	delete m_null;
	m_null = nullptr;
}
