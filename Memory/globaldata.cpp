#include "Memory/globaldata.h"
#include "Memory/class.h"
#include "System/error.h"

using namespace std;

ClassDescription::ClassDescription(Class *desc) : m_desc(desc) {}

string ClassDescription::name() const {
	return m_desc->name();
}

void ClassDescription::addParent(const std::string &name) {
	m_parents.push_back(name);
}

void ClassDescription::addMember(const std::string &name, SharedReference value) {
	m_members.push_back({name, value});
	/// \todo check override
}

Class *ClassDescription::generate() {

	if (!m_desc->members().empty()) {
		return m_desc;
	}

	for (string &name : m_parents) {
		Class *parent = GlobalData::instance().getClass(name);
		if (parent == nullptr) {
			/// \todo error
		}
		for (auto member : parent->members()) {
			Class::MemberInfo *info = new Class::MemberInfo;
			info->offset = m_desc->members().size();
			info->owner = member.second->owner;
			info->value.clone(member.second->value);
			m_desc->members().insert({member.first, info});
		}
	}

	for (auto member : m_members) {
		Class::MemberInfo *info = new Class::MemberInfo;
		info->offset = m_desc->members().size();
		info->owner = m_desc;
		info->value.clone(member.second);
		m_desc->members().insert({member.first, info});
	}

	return m_desc;
}

GlobalData &GlobalData::instance() {
	static GlobalData g_instance;
	return g_instance;
}

int GlobalData::createClass(const ClassDescription &desc) {

	size_t id = m_definedClasses.size();
	m_definedClasses.push_back(desc);
	return id;
}

void GlobalData::registerClass(int id) {

	ClassDescription &desc = m_definedClasses[id];
	if (m_registeredClasses.find(desc.name()) != m_registeredClasses.end()) {
		error("multiple definition of class '%s'", desc.name().c_str());
	}
	m_registeredClasses.insert({desc.name(), desc.generate()});
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
