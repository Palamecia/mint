#include "Memory/globaldata.h"
#include "Memory/class.h"
#include "System/error.h"

#include <limits>

using namespace std;

ClassDescription::ClassDescription(Class *desc) : m_desc(desc) {}

string ClassDescription::name() const {
	return m_desc->name();
}

void ClassDescription::addParent(const std::string &name) {
	m_parents.push_back(name);
}

void ClassDescription::addMember(const std::string &name, SharedReference value) {

	auto it = m_members.find(name);

	if (it != m_members.end() &&
			(it->second.get().data()->format == Data::fmt_function) &&
			(value.get().data()->format == Data::fmt_function)) {
		for (auto def : ((Function *)value.get().data())->mapping) {
			((Function *)it->second.get().data())->mapping.insert(def);
		}
	}
	else {
		m_members.insert({name, value});
	}
}

void ClassDescription::addGlobalMember(const std::string &name, SharedReference value) {
	m_globals.insert({name, value});
}

void ClassDescription::addMemberClass(const ClassDescription &desc) {
	m_subClasses.push_back(desc);
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
			/// \todo check override
			info->value.clone(member.second->value);
			m_desc->members().insert({member.first, info});
		}
	}

	for (auto member : m_members) {
		Class::MemberInfo *info = new Class::MemberInfo;
		info->offset = m_desc->members().size();
		info->owner = m_desc;
		/// \todo check override
		info->value.clone(member.second);
		m_desc->members().insert({member.first, info});
	}

	for (auto member : m_globals) {
		Class::MemberInfo *info = new Class::MemberInfo;
		info->offset = numeric_limits<size_t>::max();
		info->owner = m_desc;
		/// \todo check override
		info->value.clone(member.second);
		m_desc->globals().members().insert({member.first, info});
	}

	for (auto sub : m_subClasses) {
		m_desc->globals().registerClass(m_desc->globals().createClass(sub));
	}

	return m_desc;
}

void ClassDescription::clean() {

	m_parents.clear();
	m_members.clear();

	delete m_desc;
}

ClassRegister::ClassRegister() {}

ClassRegister::~ClassRegister() {

	for (ClassDescription &desc : m_definedClasses) {
		desc.clean();
	}

	m_registeredClasses.clear();
	m_definedClasses.clear();
}

int ClassRegister::createClass(const ClassDescription &desc) {

	size_t id = m_definedClasses.size();
	m_definedClasses.push_back(desc);
	return id;
}

void ClassRegister::registerClass(int id) {

	ClassDescription &desc = m_definedClasses[id];
	if (m_registeredClasses.find(desc.name()) != m_registeredClasses.end()) {
		error("multiple definition of class '%s'", desc.name().c_str());
	}
	m_registeredClasses.insert({desc.name(), desc.generate()});
}

Class *ClassRegister::getClass(const string &name) {

	auto it = m_registeredClasses.find(name);
	if (it != m_registeredClasses.end()) {
		return it->second;
	}
	return nullptr;
}

GlobalData::GlobalData() {}

GlobalData::~GlobalData() {

	m_symbols.clear();

	GarbadgeCollector::free();
}

GlobalData &GlobalData::instance() {
	static GlobalData g_instance;
	return g_instance;
}

SymbolTable &GlobalData::symbols() {
	return m_symbols;
}
