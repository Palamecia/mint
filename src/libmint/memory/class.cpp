#include "memory/class.h"
#include "memory/object.h"

using namespace std;
using namespace mint;

Class::GlobalMembers::GlobalMembers() {}

Class::GlobalMembers::~GlobalMembers() {

	for (auto member : m_members) {
		delete member.second;
	}
}

Class::MembersMapping &Class::GlobalMembers::members() {
	return m_members;
}

Class::Class(const std::string &name, Metatype metatype) :
	Class(&GlobalData::instance(), name, metatype) {

}

Class::Class(PackageData *package, const std::string &name, Metatype metatype) :
	m_package(package),
	m_metatype(metatype),
	m_name(name) {

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

const set<Class *> &Class::parents() const {
	return m_parents;
}

set<Class *> &Class::parents() {
	return m_parents;
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

bool Class::isParentOf(const Class *other) const {
	if (other == nullptr) {
		return false;
	}
	for (const Class *parent : other->parents()) {
		if (parent == this) {
			return true;
		}
		if (isParentOf(parent)) {
			return true;
		}
	}
	return false;
}

bool Class::isParentOrSameOf(const Class *other) const {
	if (other == this) {
		return true;
	}
	return isParentOf(other);
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
