#include "memory/globaldata.h"
#include "memory/class.h"
#include "system/error.h"

#include <limits>

using namespace std;

Class::MemberInfo *get_member_infos(Class *desc, const string &member) {

	auto it = desc->members().find(member);
	if (it == desc->members().end()) {
		Class::MemberInfo *info = new Class::MemberInfo;
		info->offset = desc->members().size();
		it = desc->members().insert({member, info}).first;
	}

	return it->second;
}

ClassDescription::ClassDescription(Class *desc) : m_desc(desc), m_generated(false) {}

string ClassDescription::name() const {
	return m_desc->name();
}

void ClassDescription::addParent(const string &name) {
	m_parents.push_back(name);
}

void ClassDescription::addMember(const string &name, SharedReference value) {

	auto *context = (value->flags() & Reference::global) ? &m_globals: &m_members;
	auto it = context->find(name);

	if (it != context->end() &&
			(it->second->data()->format == Data::fmt_function) &&
			(value->data()->format == Data::fmt_function)) {
		for (auto def : ((Function *)value->data())->mapping) {
			((Function *)it->second->data())->mapping.insert(def);
		}
	}
	else {
		context->insert({name, value});
	}
}

void ClassDescription::addSubClass(const ClassDescription &desc) {
	m_subClasses.push_back(desc);
}

Class *ClassDescription::generate() {

	if (m_generated) {
		return m_desc;
	}

	for (string &name : m_parents) {
		Class *parent = GlobalData::instance().getClass(name);
		if (parent == nullptr) {
			error("class '%s' was not declared", name.c_str());
		}
		m_desc->parents().insert(parent);
		for (auto member : parent->members()) {
			Class::MemberInfo *info = new Class::MemberInfo;
			info->offset = m_desc->members().size();
			info->value.clone(member.second->value);
			info->owner = member.second->owner;
			if (!m_desc->members().insert({member.first, info}).second) {
				error("member '%s' is ambiguous for class '%s'", member.first.c_str(), m_desc->name().c_str());
			}
		}
	}

	for (auto member : m_members) {
		Class::MemberInfo *info = get_member_infos(m_desc, member.first);
		info->value.clone(*member.second);
		info->owner = m_desc;
	}

	for (auto member : m_globals) {
		Class::MemberInfo *info = new Class::MemberInfo;
		info->offset = numeric_limits<size_t>::max();
		info->value.clone(*member.second);
		info->owner = m_desc;
		if (!m_desc->globals().members().insert({member.first, info}).second) {
			error("global member '%s' cannot be overridden", member.first.c_str());
		}
	}

	for (auto sub : m_subClasses) {
		m_desc->globals().registerClass(m_desc->globals().createClass(sub));
	}

	m_generated = true;
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
}

GlobalData &GlobalData::instance() {
	static GlobalData g_instance;
	return g_instance;
}

SymbolTable &GlobalData::symbols() {
	return m_symbols;
}
