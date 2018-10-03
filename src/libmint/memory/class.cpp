#include "memory/class.h"
#include "memory/object.h"
#include "system/error.h"

using namespace std;
using namespace mint;

Class::GlobalMembers::GlobalMembers(Class *metadata) :
	m_metadata(metadata) {

}

Class::GlobalMembers::~GlobalMembers() {

	for (auto type : m_classes) {
		delete type.second;
	}

	for (auto member : m_members) {
		delete member.second;
	}
}

void Class::GlobalMembers::registerClass(int id) {

	ClassDescription *desc = getDefinedClass(id);

	if (m_classes.find(desc->name()) != m_classes.end()) {
		error("multiple definition of class '%s'", desc->name().c_str());
	}

	TypeInfo *infos = new TypeInfo;
	infos->owner = m_metadata;
	infos->flags = desc->flags();
	infos->description = desc->generate();

	m_classes.emplace(desc->name(), infos);
}

Class::TypeInfo *Class::GlobalMembers::getClass(const std::string &name) {

	auto it = m_classes.find(name);
	if (it != m_classes.end()) {
		return it->second;
	}
	return nullptr;
}

Class::MembersMapping &Class::GlobalMembers::members() {
	return m_members;
}

void Class::GlobalMembers::clearGlobalReferences() {

	for (auto type : m_classes) {
		type.second->description->clearGlobalReferences();
	}

	for (auto member : m_members) {
		delete member.second;
	}

	m_members.clear();
}

Class::Class(const std::string &name, Metatype metatype) :
	Class(&GlobalData::instance(), name, metatype) {

}

Class::Class(PackageData *package, const std::string &name, Metatype metatype) :
	m_package(package),
	m_metatype(metatype),
	m_name(name),
	m_globals(this) {

}

Class::~Class() {

	for (auto member : m_members) {
		delete member.second;
	}
}

Object *Class::makeInstance() {
	return Reference::alloc<Object>(this);
}

PackageData *Class::getPackage() const {
	return m_package;
}

string Class::name() const {
	return m_name;
}

Class::Metatype Class::metatype() const {
	return m_metatype;
}

const set<Class *> &Class::bases() const {
	return m_bases;
}

set<Class *> &Class::bases() {
	return m_bases;
}

Class::MembersMapping &Class::members() {
	return m_members;
}

Class::GlobalMembers &Class::globals() {
	return m_globals;
}

size_t Class::size() const {
	return m_members.size();
}

bool Class::isBaseOf(const Class *other) const {
	if (other == nullptr) {
		return false;
	}
	for (const Class *base : other->bases()) {
		if (base == this) {
			return true;
		}
		if (isBaseOf(base)) {
			return true;
		}
	}
	return false;
}

bool Class::isBaseOrSame(const Class *other) const {
	if (other == this) {
		return true;
	}
	return isBaseOf(other);
}

void Class::clearGlobalReferences() {
	m_globals.clearGlobalReferences();
}

void Class::createBuiltinMember(const std::string &name, int signature, pair<int, int> offset) {

	auto it = m_members.find(name);

	if (it != m_members.end()) {

		Function *data = it->second->value.data<Function>();
		data->mapping.emplace(signature, Function::Handler(m_package, offset.first, offset.second));
	}
	else {

		Function *data = Reference::alloc<Function>();
		data->mapping.emplace(signature, Function::Handler(m_package, offset.first, offset.second));

		MemberInfo *infos = new MemberInfo;
		infos->offset = m_members.size();
		infos->owner = this;
		infos->value = Reference(Reference::standard, data);

		m_members.emplace(name, infos);
	}
}
