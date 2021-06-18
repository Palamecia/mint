#include "memory/reference.h"
#include "memory/memorytool.h"
#include "memory/memorypool.hpp"
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

LocalPool<Number> Number::g_pool;
LocalPool<Boolean> Boolean::g_pool;
LocalPool<Object> Object::g_pool;
LocalPool<String> String::g_pool;
LocalPool<Regex> Regex::g_pool;
LocalPool<Array> Array::g_pool;
LocalPool<Hash> Hash::g_pool;
LocalPool<Iterator> Iterator::g_pool;
LocalPool<Library> Library::g_pool;
LocalPool<Package> Package::g_pool;
LocalPool<Function> Function::g_pool;

static LocalPool<ReferenceInfos> g_reference_info_pool;
static LocalPool<StrongReference> g_strong_reference_pool;
static LocalPool<WeakReference> g_weak_reference_pool;
static SystemPool<nullptr_t> g_null_reference_pool;

GarbageCollector &Reference::g_garbageCollector = GarbageCollector::instance();

Reference::Reference(Flags flags, Data *data) :
	m_infos(g_reference_info_pool.alloc()) {
	m_infos->flags = flags;
	g_garbageCollector.use(m_infos->data = data ? data : Reference::alloc<None>());
	m_infos->refcount = 1;
}

Reference::Reference(const Reference &other, copy_tag) :
	m_infos(g_reference_info_pool.alloc()) {
	m_infos->flags = other.flags();
	g_garbageCollector.use(m_infos->data = copy(other.data()));
	m_infos->refcount = 1;
}

Reference::Reference(Reference &&other) noexcept :
	m_infos(other.m_infos) {
	other.m_infos = nullptr;
}

Reference::Reference(const Reference &other) :
	Reference(other.flags(), other.data()) {

}

Reference::Reference(ReferenceInfos* infos) :
	m_infos(infos) {
	++m_infos->refcount;
}

Reference::~Reference() {
	if (m_infos && !--m_infos->refcount) {
		if (!m_infos->data->infos.collected) {
			assert(m_infos->data);
			g_garbageCollector.release(m_infos->data);
		}
		g_reference_info_pool.free(m_infos);
	}
}

Reference &Reference::operator =(const Reference &other) {
	m_infos->flags = other.flags();
	setData(other.data());
	return *this;
}

Reference &Reference::operator =(Reference &&other) noexcept {
	swap(m_infos, other.m_infos);
	return *this;
}

void Reference::clone(const Reference &other) {
	m_infos->flags = other.m_infos->flags;
	copy(other);
}

void Reference::copy(const Reference &other) {
	setData(copy(other.data()));
}

void Reference::move(const Reference &other) {
	setData(other.data());
}

Data *Reference::copy(const Data *other) {
	switch (other->format) {
	case Data::fmt_null:
		return alloc<Null>();
	case Data::fmt_none:
		return alloc<None>();
	case Data::fmt_number:
		return alloc<Number>(*static_cast<const Number *>(other));
	case Data::fmt_boolean:
		return alloc<Boolean>(*static_cast<const Boolean *>(other));
	case Data::fmt_object:
	{
		Object *data = nullptr;
		const Object *object = static_cast<const Object *>(other);
		switch (object->metadata->metatype()) {
		case Class::object:
			data = alloc<Object>(object->metadata);
			break;
		case Class::string:
			data = alloc<String>(*static_cast<const String *>(other));
			break;
		case Class::regex:
			data = alloc<Regex>(*static_cast<const Regex *>(other));
			break;
		case Class::array:
			data = alloc<Array>(*static_cast<const Array *>(other));
			break;
		case Class::hash:
			data = alloc<Hash>(*static_cast<const Hash *>(other));
			break;
		case Class::iterator:
			data = alloc<Iterator>(*static_cast<const Iterator *>(other));
			break;
		case Class::library:
			data = alloc<Library>(*static_cast<const Library *>(other));
			break;
		case Class::libobject:
			/// \todo safe ?
			return const_cast<Data *>(other);
		}
		data->construct(*object);
		return data;
	}
	case Data::fmt_package:
		return alloc<Package>(static_cast<const Package *>(other)->data);
	case Data::fmt_function:
		return alloc<Function>(*static_cast<const Function *>(other));
	}

	return nullptr;
}

void Reference::free(Data *ptr) {
	switch (ptr->format) {
	case Data::fmt_none:
	case Data::fmt_null:
		delete ptr;
		break;
	case Data::fmt_number:
		Number::g_pool.free(static_cast<Number *>(ptr));
		break;
	case Data::fmt_boolean:
		Boolean::g_pool.free(static_cast<Boolean *>(ptr));
		break;
	case Data::fmt_object:
	{
		Object *object = static_cast<Object *>(ptr);
		if (is_object(object)) {
			auto member = object->metadata->members().find(Symbol::Delete);
			if (member != object->metadata->members().end()) {
				Reference &member_ref = object->data[member->second->offset];
				if (member_ref.m_infos->data->format == Data::fmt_function) {
					if (Scheduler *scheduler = Scheduler::instance()) {
						scheduler->createDestructor(object, SharedReference::weak(member_ref), member->second->owner);
						break;
					}
				}
			}
		}
		destroy(object);
	}
		break;
	case Data::fmt_package:
		Package::g_pool.free(static_cast<Package *>(ptr));
		break;
	case Data::fmt_function:
		Function::g_pool.free(static_cast<Function *>(ptr));
		break;
	}
}

void Reference::destroy(Object *ptr) {
	switch (ptr->metadata->metatype()) {
	case Class::object:
		Object::g_pool.free(ptr);
		break;
	case Class::string:
		String::g_pool.free(static_cast<String *>(ptr));
		break;
	case Class::regex:
		Regex::g_pool.free(static_cast<Regex *>(ptr));
		break;
	case Class::array:
		Array::g_pool.free(static_cast<Array *>(ptr));
		break;
	case Class::hash:
		Hash::g_pool.free(static_cast<Hash *>(ptr));
		break;
	case Class::iterator:
		Iterator::g_pool.free(static_cast<Iterator *>(ptr));
		break;
	case Class::library:
		Library::g_pool.free(static_cast<Library *>(ptr));
		break;
	case Class::libobject:
		delete ptr;
		break;
	}
}

void Reference::setData(Data *data) {

	assert(data);

	Data *previous = m_infos->data;
	g_garbageCollector.use(m_infos->data = data);
	g_garbageCollector.release(previous);
}

ReferenceInfos *Reference::infos() {
	return m_infos;
}

template<>
None *Reference::alloc<None>() {
	static StrongReference g_none(const_address | const_value, new None);
	return g_none.data<None>();
}

template<>
Null *Reference::alloc<Null>() {
	static StrongReference g_null(const_address | const_value, new Null);
	return g_null.data<Null>();
}

WeakReference::WeakReference(Flags flags, Data *data) :
	Reference(flags, data) {

}

WeakReference::WeakReference(const Reference &other, copy_tag tag) :
	Reference (other, tag) {

}

WeakReference::WeakReference(WeakReference &&other) noexcept :
	Reference(std::move(other)) {

}

WeakReference::WeakReference(const WeakReference &other) :
	Reference(other) {

}

WeakReference::WeakReference(const Reference &other) :
	Reference(other) {

}

WeakReference::WeakReference(ReferenceInfos* infos) :
	Reference(infos) {

}

WeakReference::~WeakReference() {

}

WeakReference &WeakReference::operator =(const WeakReference &other) {
	Reference::operator=(other);
	return *this;
}

WeakReference &WeakReference::operator =(WeakReference &&other) noexcept {
	Reference::operator=(std::move(other));
	return *this;
}

StrongReference::StrongReference(Flags flags, Data *data) :
	Reference(flags, data) {
	g_garbageCollector.registerRoot(this);
}

StrongReference::StrongReference(const StrongReference &other) :
	Reference(other) {
	g_garbageCollector.registerRoot(this);
}

StrongReference::StrongReference(StrongReference &&other) noexcept :
	Reference(std::move(other)) {
	g_garbageCollector.registerRoot(this);
}

StrongReference::StrongReference(const WeakReference &other) :
	Reference(other) {
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

StrongReference &StrongReference::operator =(const WeakReference &other) {
	Reference::operator=(other);
	return *this;
}

StrongReference &StrongReference::operator =(const StrongReference &other) {
	Reference::operator=(other);
	return *this;
}

StrongReference &StrongReference::operator =(StrongReference &&other) noexcept {
	Reference::operator =(std::move(other));
	return *this;
}

SharedReference::SharedReference() :
	m_reference(nullptr),
	m_pool(&g_null_reference_pool) {

}

SharedReference::SharedReference(nullptr_t) :
	m_reference(nullptr),
	m_pool(&g_null_reference_pool) {

}

SharedReference::SharedReference(SharedReference &&other) noexcept :
	m_reference(other.m_reference),
	m_pool(other.m_pool) {
	other.m_reference = nullptr;
	other.m_pool = &g_null_reference_pool;
}

SharedReference::SharedReference(MemoryPool *pool, Reference *reference) :
	m_reference(reference),
	m_pool(pool) {

}

SharedReference::~SharedReference() {
	m_pool->free(m_reference);
}

SharedReference SharedReference::strong(Data *data) {
	return SharedReference(&g_strong_reference_pool, g_strong_reference_pool.alloc(Reference::const_address | Reference::const_value, data));
}

SharedReference SharedReference::strong(Reference::Flags flags, Data *data) {
	return SharedReference(&g_strong_reference_pool, g_strong_reference_pool.alloc(flags, data));
}

SharedReference SharedReference::strong(Reference &reference) {
	return SharedReference(&g_strong_reference_pool, g_strong_reference_pool.alloc(reference.infos()));
}

SharedReference SharedReference::weak(Reference &reference) {
	return SharedReference(&g_weak_reference_pool, g_weak_reference_pool.alloc(reference.infos()));
}

SharedReference &SharedReference::operator =(SharedReference &&other) noexcept {
	swap(m_reference, other.m_reference);
	swap(m_pool, other.m_pool);
	return *this;
}
