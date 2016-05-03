#include "Memory/reference.h"
#include "Memory/object.h"
#include "Memory/class.h"

#include <cstring>

using namespace std;

Reference::Reference(Flags flags, Data *data) : m_flags(flags), m_data(data) {
	GarbadgeCollector::g_refs.insert(this);
}

Reference::~Reference() {
	GarbadgeCollector::g_refs.erase(this);
}

void Reference::clone(const Reference &other) {
	m_flags = other.m_flags;
	copy(other);
}

void Reference::copy(const Reference &other) {
	switch (other.m_data->format) {
	case Data::fmt_null:
		m_data = alloc<Null>();
		break;
	case Data::fmt_none:
		m_data = alloc<None>();
		break;
	case Data::fmt_number:
		m_data = alloc<Number>();
		((Number *)m_data)->data = ((Number *)other.m_data)->data;
		break;
	case Data::fmt_object:
		if (((Object *)other.m_data)->metadata == StringClass::instance()) {
			m_data = alloc<String>();
			((String *)m_data)->str = ((String *)other.m_data)->str;
		}
		break;
	case Data::fmt_function:
		m_data = alloc<Function>();
		((Function *)m_data)->mapping = ((Function *)other.m_data)->mapping;
		break;
	case Data::fmt_hash:
		m_data = alloc<Hash>();
		((Hash *)m_data)->values = ((Hash *)other.data())->values;
		break;
	case Data::fmt_array:
		m_data = alloc<Array>();
		((Array *)m_data)->values = ((Array *)other.data())->values;
		break;
	}
}

void Reference::move(const Reference &other) {
	m_data = other.m_data;
}

Data *Reference::data() {
	return m_data;
}

const Data *Reference::data() const {
	return m_data;
}

Reference::Flags Reference::flags() const {
	return m_flags;
}
