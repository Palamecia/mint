#include "class.h"
#include "object.h"

using namespace std;

Class::Class(const std::string &name) : m_name(name) {}

Object *Class::makeInstance() {
	Object *object = Reference::alloc<Object>(this);
	object->data = new Reference [m_members.size()];
	for (auto member : m_members) {
		object->data[member.second.offset].clone(member.second.value);
	}
	object->metadata = this;
	return object;
}

map<string, Class::MemberInfo> &Class::members() {
	return m_members;
}

size_t Class::size() const {
	return m_members.size();
}

StringClass::StringClass() : Class("string") {}

StringClass *StringClass::instance() {

	static StringClass *g_instance = new StringClass;

	return g_instance;
}

IteratorClass::IteratorClass() : Class("iterator") {}

IteratorClass *IteratorClass::instance() {

	static IteratorClass *g_instance = new IteratorClass;

	return g_instance;
}
