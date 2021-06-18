#include "memory/class.h"
#include "memory/object.h"
#include "system/error.h"

using namespace std;
using namespace mint;

Class::GlobalMembers::GlobalMembers(Class *metadata) :
	m_metadata(metadata) {

}

Class::GlobalMembers::~GlobalMembers() {

	for (auto &type : m_classes) {
		delete type.second;
	}

	for (auto &member : m_members) {
		delete member.second;
	}
}

void Class::GlobalMembers::registerClass(int id) {

	ClassDescription *desc = getClassDescription(id);
	Symbol symbol(desc->name());

	if (UNLIKELY(m_classes.find(desc->name()) != m_classes.end())) {
		error("multiple definition of class '%s'", desc->name().str().c_str());
	}

	TypeInfo *infos = new TypeInfo;
	infos->owner = m_metadata;
	infos->flags = desc->flags();
	infos->description = desc->generate();
	m_classes.emplace(desc->name(), infos);

	MemberInfo *member = new MemberInfo;
	member->offset = MemberInfo::InvalidOffset;
	member->owner = m_metadata;
	member->value = StrongReference(Reference::global | Reference::const_address | Reference::const_value | infos->flags, infos->description->makeInstance());
	m_members.emplace(symbol, member);
}

Class::TypeInfo *Class::GlobalMembers::getClass(const Symbol &name) {

	auto it = m_classes.find(name);
	if (it != m_classes.end()) {
		return it->second;
	}
	return nullptr;
}

void Class::GlobalMembers::cleanup() {

	for (auto &type : m_classes) {
		type.second->description->cleanup();
	}

	for (auto &member : m_members) {
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
	m_globals(this),
	m_copyable(true) {

}

Class::~Class() {

	for (auto &member : m_members) {
		delete member.second;
	}
}

Object *Class::makeInstance() {
	return Reference::alloc<Object>(this);
}

PackageData *Class::getPackage() const {
	return m_package;
}

const set<Class *> &Class::bases() const {
	return m_bases;
}

set<Class *> &Class::bases() {
	return m_bases;
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

bool Class::isCopyable() const {
	return m_copyable;
}

void Class::disableCopy() {
	m_copyable = false;
}

void Class::cleanup() {
	m_globals.cleanup();
}

void Class::createBuiltinMember(const Symbol &symbol, std::pair<int, Module::Handle *> member) {

	auto it = m_members.find(symbol);

	if (it != m_members.end()) {

		Function *data = it->second->value.data<Function>();
		data->mapping.emplace(member.first, member.second);
	}
	else {

		Function *data = Reference::alloc<Function>();
		data->mapping.emplace(member.first, member.second);

		m_members.emplace(symbol, new MemberInfo{
							  m_members.size(), this,
							  StrongReference(Reference::standard, data)
						  });
	}
}
