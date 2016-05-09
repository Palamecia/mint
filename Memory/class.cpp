#include "class.h"
#include "object.h"

using namespace std;

Class::Class(const std::string &name) : m_name(name) {}

Object *Class::makeInstance() {
	return Reference::alloc<Object>(this);
}

string Class::name() const {
	return m_name;
}

std::map<string, Class::MemberInfo *> &Class::members() {
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
