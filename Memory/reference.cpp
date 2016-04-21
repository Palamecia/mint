#include "reference.h"
#include "object.h"

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
		m_data = alloc<Data>();
		memcpy(m_data, other.m_data, sizeof(Data));
		break;
	case Data::fmt_none:
		m_data = alloc<Data>();
		memcpy(m_data, other.m_data, sizeof(Data));
		break;
	case Data::fmt_number:
		m_data = alloc<Number>();
		memcpy(m_data, other.m_data, sizeof(Number));
		break;
	case Data::fmt_object:

		break;
	case Data::fmt_function:
		m_data = alloc<Number>();
		memcpy(m_data, other.m_data, sizeof(Function));
		break;
	case Data::fmt_hash:

		break;
	case Data::fmt_array:

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
