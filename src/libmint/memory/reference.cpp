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
	m_flags(flags) {
	GarbageCollector::instance().use(m_data = data ? data : Reference::alloc<None>());
	m_infos = m_data->infos;
}

Reference::Reference(const Reference &other) :
	Reference(other.flags(), other.data()) {

}

Reference::~Reference() {

	if (!m_infos->collected) {

		assert(m_data);
		assert(m_infos == m_data->infos);

		GarbageCollector::instance().release(m_data);
	}
}

Reference &Reference::operator =(const Reference &other) {
	m_flags = other.flags();
	setData(other.data());
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
				array_append(data<Array>(), array_get_item(other.data<Array>(), static_cast<long>(i)));
			}
			break;
		case Class::hash:
			setData(alloc<Hash>());
			for (auto &item : other.data<Hash>()->values) {
				hash_insert(data<Hash>(), hash_get_key(other.data<Hash>(), item), hash_get_value(other.data<Hash>(), item));
			}
			break;
		case Class::iterator:
			setData(alloc<Iterator>());
			for (SharedReference &item : other.data<Iterator>()->ctx) {
				iterator_insert(data<Iterator>(), SharedReference::unique(new StrongReference(*item)));
			}
			break;
		case Class::library:
			setData(alloc<Library>());
			if (other.data<Library>()->plugin) {
				data<Library>()->plugin = new Plugin(other.data<Library>()->plugin->getPath());
			}
			break;
		case Class::libobject:
			setData(other.data());
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
			handler.generator = item.second.generator;
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
	setData(other.data());
}

Reference::Flags Reference::flags() const {
	return m_flags;
}

template<>
None *Reference::alloc<None>() {
	static StrongReference g_none(const_address | const_value, GarbageCollector::instance().registerData(new None));
	return g_none.data<None>();
}

template<>
Null *Reference::alloc<Null>() {
	static StrongReference g_null(const_address | const_value, GarbageCollector::instance().registerData(new Null));
	return g_null.data<Null>();
}

WeakReference::WeakReference(Flags flags, Data *data) :
	Reference(flags, data) {

}

WeakReference::WeakReference(const Reference &other) :
	Reference(other) {

}

WeakReference::~WeakReference() {

}

WeakReference *WeakReference::create(Data *data) {
	return new WeakReference(const_address | const_value, data);
}

StrongReference::StrongReference(Flags flags, Data *data) :
	Reference(flags, data) {
	GarbageCollector::instance().registerRoot(this);
}

StrongReference::StrongReference(const Reference &other) :
	Reference(other) {
	GarbageCollector::instance().registerRoot(this);
}

StrongReference::~StrongReference() {
	GarbageCollector::instance().unregisterRoot(this);
}

StrongReference *StrongReference::create(Data *data) {
	return new StrongReference(const_address | const_value, data);
}

ReferenceManager::ReferenceManager() {

}

ReferenceManager::~ReferenceManager() {
	while (!m_references.empty()) {
		SharedReference *reference = *m_references.begin();
		reference->makeUnique();
	}
}

ReferenceManager &ReferenceManager::operator=(const ReferenceManager &other) {
	m_references = other.m_references;
	return *this;
}

void ReferenceManager::link(SharedReference *reference) {
	m_references.insert(reference);
}

void ReferenceManager::unlink(SharedReference *reference) {
	m_references.erase(reference);
}

SharedReference::SharedReference() :
	SharedReference(new StrongReference(), true) {

}

SharedReference::SharedReference(nullptr_t) :
	SharedReference(nullptr, true) {

}

SharedReference::SharedReference(SharedReference &&other) {

	m_reference = other.m_reference;

	if ((m_unique = other.m_unique)) {
		other.m_reference = nullptr;
		m_linked = nullptr;
	}
	else if ((m_linked = other.m_linked)) {
		m_linked->link(this);
	}
}

SharedReference::SharedReference(Reference *reference, bool unique) :
	m_reference(reference),
	m_linked(nullptr),
	m_unique(unique) {

}

SharedReference::SharedReference(Reference *reference, ReferenceManager *manager) :
	m_reference(reference),
	m_linked(manager),
	m_unique(false) {
	manager->link(this);
}

SharedReference::~SharedReference() {

	if (m_unique) {
		delete m_reference;
		m_reference = nullptr;
	}
	else if (m_linked) {
		m_linked->unlink(this);
	}
}

SharedReference SharedReference::unsafe(Reference *reference) {
	return SharedReference(reference, false);
}

SharedReference SharedReference::unique(Reference *reference) {
	return SharedReference(reference, true);
}

SharedReference SharedReference::linked(ReferenceManager *manager, Reference *reference) {
	return SharedReference(reference, manager);
}

SharedReference &SharedReference::operator =(SharedReference &&other) {

	if (m_unique) {
		delete m_reference;
	}
	else if (m_linked) {
		m_linked->unlink(this);
	}

	m_reference = other.m_reference;

	if ((m_unique = other.m_unique)) {
		other.m_reference = nullptr;
		m_linked = nullptr;
	}
	else if ((m_linked = other.m_linked)) {
		m_linked->link(this);
	}

	return *this;
}

Reference &SharedReference::operator *() const {
	return *m_reference;
}

Reference *SharedReference::operator ->() const {
	return m_reference;
}

Reference *SharedReference::get() const {
	return m_reference;
}

SharedReference::operator bool() const {
	return m_reference != nullptr;
}

bool SharedReference::isUnique() const {
	return m_unique;
}

void SharedReference::makeUnique() {
	if (!m_unique) {
		if (m_linked) {
			m_linked->unlink(this);
			m_linked = nullptr;
		}
		m_reference = new StrongReference(*m_reference);
		m_unique = true;
	}
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
		delete ptr->infos;
		delete ptr;
	}
}

void Reference::setData(Data *data) {

	assert(data);
	assert(data->infos);

	Data *previous = m_data;
	GarbageCollector::instance().use(m_data = data);
	GarbageCollector::instance().release(previous);

	m_infos = data->infos;
}
