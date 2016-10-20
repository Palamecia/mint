#include "Memory/class.h"
#include "Memory/object.h"

using namespace std;

Class::GlobalMembers::GlobalMembers() {}

Class::GlobalMembers::~GlobalMembers() {
	m_members.clear();
}

Class::MembersMapping &Class::GlobalMembers::members() {
	return m_members;
}

Class::Class(const std::string &name) : m_name(name) {}

Class::~Class() {

	for (auto member : m_members) {
		delete member.second;
	}
}

Object *Class::makeInstance() {
	return Reference::alloc<Object>(this);
}

string Class::name() const {
	return m_name;
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

void Class::createBuiltinMember(const string &name, int signature, pair<int, int> offset) {

	auto it = m_members.find(name);

	if (it != m_members.end()) {

		Function *data = (Function *)it->second->value.data();
		data->mapping.insert({signature, offset});
	}
	else {

		Function *data = Reference::alloc<Function>();
		data->mapping.insert({signature, offset});

		MemberInfo *infos = new MemberInfo;
		infos->offset = m_members.size();
		infos->owner = this;
		infos->value = Reference(Reference::standard, data);

		m_members.insert({name, infos});
	}
}

StringClass *StringClass::instance() {

	static StringClass *g_instance = new StringClass;

	return g_instance;
}

ArrayClass *ArrayClass::instance() {

	static ArrayClass *g_instance = new ArrayClass;

	return g_instance;
}

HashClass *HashClass::instance() {

	static HashClass *g_instance = new HashClass;

	return g_instance;
}

IteratorClass *IteratorClass::instance() {

	static IteratorClass *g_instance = new IteratorClass;

	return g_instance;
}

LibraryClass *LibraryClass::instance() {

	static LibraryClass *g_instance = new LibraryClass;

	return g_instance;
}