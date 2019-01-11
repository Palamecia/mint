#include "memory/reference.h"
#include "memory/memorytool.h"
#include "memory/builtin/string.h"
#include "memory/builtin/regex.h"
#include "memory/builtin/iterator.h"
#include "memory/builtin/library.h"
#include "scheduler/destructor.h"
#include "scheduler/scheduler.h"
#include "system/plugin.h"
#include "system/assert.h"

#include <cstring>

using namespace std;
using namespace mint;

Reference::Reference(Flags flags, Data *data) :
	m_flags(flags),
	m_data(nullptr) {
	GarbadgeCollector::instance().registerReference(this);
	setData(data ? data : Reference::alloc<None>());
}

Reference::Reference(const Reference &other) :
	Reference(other.flags(), const_cast<Data *>(other.data())) {

}

Reference::~Reference() {
	GarbadgeCollector::instance().release(m_data);
	GarbadgeCollector::instance().unregisterReference(this);
}

Reference &Reference::operator =(const Reference &other) {
	m_flags = other.flags();
	setData(const_cast<Data *>(other.data()));
	return *this;
}

void Reference::clone(const Reference &other) {
	m_flags = other.m_flags;
	copy(other);
}

void Reference::copy(const Reference &other) {
	switch (other.data()->format) {
	case Data::fmt_null:
		setData(alloc<Null>());
		break;
	case Data::fmt_none:
		setData(alloc<None>());
		break;
	case Data::fmt_number:
		setData(alloc<Number>());
		data<Number>()->value = other.data<Number>()->value;
		break;
	case Data::fmt_boolean:
		setData(alloc<Boolean>());
		data<Boolean>()->value = other.data<Boolean>()->value;
		break;
	case Data::fmt_object:
		switch (other.data<Object>()->metadata->metatype()) {
		case Class::object:
			setData(alloc<Object>(other.data<Object>()->metadata));
			break;
		case Class::string:
			setData(alloc<String>());
			data<String>()->str = other.data<String>()->str;
			break;
		case Class::regex:
			setData(alloc<Regex>());
			data<Regex>()->initializer = other.data<Regex>()->initializer;
			data<Regex>()->expr = other.data<Regex>()->expr;
			break;
		case Class::array:
			setData(alloc<Array>());
			for (size_t i = 0; i < other.data<Array>()->values.size(); ++i) {
				array_append(data<Array>(), array_get_item(other.data<Array>(), i));
			}
			break;
		case Class::hash:
			setData(alloc<Hash>());
			for (auto &item : other.data<Hash>()->values) {
				hash_insert(data<Hash>(), hash_get_key(item), hash_get_value(item));
			}
			break;
		case Class::iterator:
			setData(alloc<Iterator>());
			for (const SharedReference &item : other.data<Iterator>()->ctx) {
				iterator_insert(data<Iterator>(), item);
			}
			break;
		case Class::library:
			setData(alloc<Library>());
			if (other.data<Library>()->plugin) {
				data<Library>()->plugin = new Plugin(other.data<Library>()->plugin->getPath());
			}
			break;
		case Class::libobject:
			setData(const_cast<Data *>(other.data()));
			/// \todo safe ?
			return;
		}
		data<Object>()->construct(*other.data<Object>());
		break;
	case Data::fmt_package:
		setData(alloc<Package>(other.data<Package>()->data));
		break;
	case Data::fmt_function:
		setData(alloc<Function>());
		for (const Function::mapping_type::value_type &item : other.data<Function>()->mapping) {
			Function::Handler handler(item.second.package, item.second.module, item.second.offset);
			if (item.second.capture) {
				handler.capture.reset(new Function::Handler::Capture);
				*handler.capture = *item.second.capture;
			}
			data<Function>()->mapping.emplace(item.first, handler);
		}
		break;
	}
}

void Reference::move(const Reference &other) {
	setData(const_cast<Data *>(other.data()));
}

Reference *Reference::create(Data *data) {
	return new Reference(const_address | const_value, data);
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
	static Reference g_none(const_address | const_value, GarbadgeCollector::instance().registerData(new None));
	return g_none.data<None>();
}

template<>
Null *Reference::alloc<Null>() {
	static Reference g_null(const_address | const_value, GarbadgeCollector::instance().registerData(new Null));
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
			Object *object = static_cast<Object *>(ptr);
			if (is_object(object)) {
				scheduler->createDestructor(object);
				break;
			}
		}

	default:
		delete ptr;
	}
}

void Reference::setData(Data *data) {

	assert(data);

	Data *previous = m_data;
	GarbadgeCollector::instance().use(m_data = data);
	GarbadgeCollector::instance().release(previous);
}
