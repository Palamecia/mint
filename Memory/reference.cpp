#include "Memory/reference.h"
#include "Memory/object.h"
#include "Memory/class.h"

#include <cstring>

using namespace std;

Reference::Reference(Flags flags, Data *data) : m_flags(flags), m_data(data) {
	GarbadgeCollector::g_refs.insert(this);
}

Reference::Reference(const Reference &other) : Reference(other.flags(), (Data *)other.data()) {}

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
		((Number *)m_data)->value = ((Number *)other.m_data)->value;
		break;
	case Data::fmt_object:
		if (((Object *)other.m_data)->metadata == StringClass::instance()) {
			m_data = alloc<String>();
			((String *)m_data)->str = ((String *)other.m_data)->str;
		}
		else if (((Object *)other.m_data)->metadata == ArrayClass::instance()) {
			m_data = alloc<Array>();
			for (auto &item : ((Array *)other.data())->values) {
				((Array *)m_data)->values.push_back(Array::move_item(item));
			}
		}
		else if (((Object *)other.m_data)->metadata == HashClass::instance()) {
			m_data = alloc<Hash>();
			for (auto &item : ((Hash *)other.data())->values) {
				((Hash *)m_data)->values.insert(Hash::move_item(item));
			}
		}
		else {
			m_data = alloc<Object>(((Object *)other.data())->metadata);
		}
		((Object *)m_data)->construct();
		/// \todo copy members values
		break;
	case Data::fmt_function:
		m_data = alloc<Function>();
		((Function *)m_data)->mapping = ((Function *)other.m_data)->mapping;
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
