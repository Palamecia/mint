#include "memory/reference.h"
#include "memory/memorytool.h"
#include "memory/builtin/string.h"
#include "memory/builtin/iterator.h"
#include "memory/builtin/library.h"
#include "scheduler/destructor.h"
#include "scheduler/scheduler.h"
#include "system/plugin.h"

#include <cstring>

using namespace std;
using namespace mint;

Reference::Reference(Flags flags, Data *data) :
	m_flags(flags),
	m_data(data ? data : Reference::alloc<None>()) {
	GarbadgeCollector::instance().m_references.insert(this);
}

Reference::Reference(const Reference &other) :
	Reference(other.flags(), const_cast<Data *>(other.data())) {

}

Reference::~Reference() {
	GarbadgeCollector::instance().m_references.erase(this);
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
		((Number *)m_data)->value = other.data<Number>()->value;
		break;
	case Data::fmt_boolean:
		m_data = alloc<Boolean>();
		((Boolean *)m_data)->value = other.data<Boolean>()->value;
		break;
	case Data::fmt_object:
		switch (other.data<Object>()->metadata->metatype()) {
		case Class::object:
			m_data = alloc<Object>(other.data<Object>()->metadata);
			break;
		case Class::string:
			m_data = alloc<String>();
			((String *)m_data)->str = other.data<String>()->str;
			break;
		case Class::array:
			m_data = alloc<Array>();
			for (size_t i = 0; i < other.data<Array>()->values.size(); ++i) {
				array_append((Array *)m_data, array_get_item(other.data<Array>(), i));
			}
			break;
		case Class::hash:
			m_data = alloc<Hash>();
			for (auto &item : other.data<Hash>()->values) {
				hash_insert((Hash *)m_data, hash_get_key(item), hash_get_value(item));
			}
			break;
		case Class::iterator:
			m_data = alloc<Iterator>();
			while (SharedReference item = iterator_next(const_cast<Iterator *>(other.data<Iterator>()))) {
				iterator_insert((Iterator *)m_data, item);
			}
			break;
		case Class::library:
			m_data = alloc<Library>();
			if (other.data<Library>()->plugin) {
				((Library *)m_data)->plugin = new Plugin(other.data<Library>()->plugin->getPath());
			}
			break;
		case Class::libobject:
			m_data = other.m_data;
			/// \todo safe ?
			return;
		}
		((Object *)m_data)->construct(*other.data<Object>());
		break;
	case Data::fmt_function:
		m_data = alloc<Function>();
		for (const Function::mapping_type::value_type &item : other.data<Function>()->mapping) {
			Function::Handler handler(item.second.module, item.second.offset);
			if (item.second.capture) {
				handler.capture.reset(new Function::Handler::Capture);
				*handler.capture = *item.second.capture;
			}
			((Function *)m_data)->mapping.emplace(item.first, handler);
		}
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

template<>
None *Reference::alloc<None>() {
	static Reference g_none(const_ref | const_value,
							GarbadgeCollector::instance().m_memory.emplace(new None, false).first->first);
	return g_none.data<None>();
}

template<>
Null *Reference::alloc<Null>() {
	static Reference g_null(const_ref | const_value,
							GarbadgeCollector::instance().m_memory.emplace(new Null, false).first->first);
	return g_null.data<Null>();
}

SharedReference::SharedReference() :
	SharedReference(new Reference(), true) {

}

SharedReference::SharedReference(Reference *ref) :
	SharedReference(ref, false) {

}

SharedReference::SharedReference(const SharedReference &other) {

	m_ref = other.m_ref;

	if ((m_unique = other.m_unique)) {
		other.m_ref = nullptr;
	}
}

SharedReference::SharedReference(Reference *ref, bool unique) :
	m_ref(ref),
	m_unique(unique) {

}

SharedReference::~SharedReference() {

	if (m_unique) {
		delete m_ref;
		m_ref = nullptr;
	}
}

SharedReference SharedReference::unique(Reference *ref) {
	return SharedReference(ref, true);
}

SharedReference &SharedReference::operator =(const SharedReference &other) {

	if (m_unique) {
		delete m_ref;
	}

	m_ref = other.m_ref;

	if ((m_unique = other.m_unique)) {
		other.m_ref = nullptr;
	}

	return *this;
}

Reference &SharedReference::operator *() const {
	return *m_ref;
}

Reference *SharedReference::operator ->() const {
	return m_ref;
}

Reference *SharedReference::get() const {
	return m_ref;
}

SharedReference::operator bool() const {
	return m_ref != nullptr;
}

bool SharedReference::isUnique() const {
	return m_unique;
}

void Reference::free(Data *ptr) {

	switch (ptr->format) {
	case Data::fmt_object:
		if (Scheduler *scheduler = Scheduler::instance()) {
			scheduler->createThread(new Destructor((Object *)ptr));
			break;
		}

	default:
		delete ptr;
		break;
	}
}
