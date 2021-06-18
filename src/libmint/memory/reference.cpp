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

GarbageCollector &Reference::g_garbageCollector = GarbageCollector::instance();

Reference::Reference(Flags flags, Data *data) :
	m_infos(new ReferenceInfos) {
	m_infos->flags = flags;
	g_garbageCollector.use(m_infos->data = data ? data : Reference::alloc<None>());
	m_infos->infos = m_infos->data->infos;
	m_infos->refcount = 1;
}

Reference::Reference(const Reference &other) :
	Reference(other.flags(), other.data()) {

}

Reference::Reference(ReferenceInfos* infos) :
	m_infos(infos) {
	++m_infos->refcount;
}

Reference::~Reference() {

	if (!--m_infos->refcount) {
		if (!m_infos->infos->collected) {
			assert(m_infos->data);
			assert(m_infos->infos == m_infos->data->infos);
			g_garbageCollector.release(m_infos->data);
		}
		delete m_infos;
	}
}

Reference &Reference::operator =(const Reference &other) {
	m_infos->flags = other.flags();
	setData(other.data());
	return *this;
}

void Reference::clone(const Reference &other) {
	m_infos->flags = other.m_infos->flags;
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
				array_append(data<Array>(), array_get_item(other.data<Array>(), static_cast<intmax_t>(i)));
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
			for (SharedReference &item : other.data<Iterator>()->ctx) {
				iterator_insert(data<Iterator>(), SharedReference::strong(*item));
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
	return m_infos->flags;
}

template<>
None *Reference::alloc<None>() {
	static StrongReference g_none(const_address | const_value, g_garbageCollector.registerData(new None));
	return g_none.data<None>();
}

template<>
Null *Reference::alloc<Null>() {
	static StrongReference g_null(const_address | const_value, g_garbageCollector.registerData(new Null));
	return g_null.data<Null>();
}

WeakReference::WeakReference(Flags flags, Data *data) :
	Reference(flags, data) {

}

WeakReference::WeakReference(const Reference &other) :
	Reference(other) {

}

WeakReference::WeakReference(ReferenceInfos* infos) :
	Reference(infos) {

}

WeakReference::~WeakReference() {

}

StrongReference::StrongReference(Flags flags, Data *data) :
	Reference(flags, data) {
	g_garbageCollector.registerRoot(this);
}

StrongReference::StrongReference(const Reference &other) :
	Reference(other) {
	g_garbageCollector.registerRoot(this);
}

StrongReference::StrongReference(ReferenceInfos* infos) :
	Reference(infos) {
	g_garbageCollector.registerRoot(this);
}

StrongReference::~StrongReference() {
	g_garbageCollector.unregisterRoot(this);
}

StrongReference &StrongReference::operator =(const StrongReference &other) {
	Reference::operator=(other);
	return *this;
}

SharedReference::SharedReference(nullptr_t) :
	m_reference(nullptr) {

}

SharedReference::SharedReference(SharedReference &&other) :
	m_reference(other.m_reference) {
	other.m_reference = nullptr;
}

SharedReference::SharedReference(Reference *reference) :
	m_reference(reference) {

}

SharedReference::~SharedReference() {
	delete m_reference;
}

SharedReference SharedReference::strong(Data *data) {
	return SharedReference(new StrongReference(Reference::const_address | Reference::const_value, data));
}

SharedReference SharedReference::strong(Reference::Flags flags, Data *data) {
	return SharedReference(new StrongReference(flags, data));
}

SharedReference SharedReference::strong(Reference &reference) {
	return SharedReference(new StrongReference(reference.infos()));
}

SharedReference SharedReference::weak(Reference &reference) {
	return SharedReference(new WeakReference(reference.infos()));
}

SharedReference &SharedReference::operator =(SharedReference &&other) {
	swap(m_reference, other.m_reference);
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

void Reference::destroy(Data *ptr) {
	delete ptr->infos;
	delete ptr;
}

void Reference::setData(Data *data) {

	assert(data);
	assert(data->infos);

	Data *previous = m_infos->data;
	g_garbageCollector.use(m_infos->data = data);
	g_garbageCollector.release(previous);

	m_infos->infos = data->infos;
}

ReferenceInfos *Reference::infos() {
	return m_infos;
}
