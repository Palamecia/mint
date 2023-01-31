#include "memory/reference.h"
#include "memory/memorytool.h"
#include "memory/builtin/array.h"
#include "memory/builtin/hash.h"
#include "memory/builtin/iterator.h"
#include "memory/builtin/library.h"
#include "memory/builtin/libobject.h"
#include "memory/builtin/regex.h"
#include "memory/builtin/string.h"
#include "scheduler/scheduler.h"
#include "system/malloc.h"
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

LocalPool<ReferenceInfos> Reference::g_pool;
GarbageCollector &Reference::g_garbageCollector = GarbageCollector::instance();

Reference::Reference(Flags flags, Data *data) :
	m_infos(g_pool.alloc()) {
	m_infos->flags = flags;
	g_garbageCollector.use(m_infos->data = data ? data : Reference::alloc<None>());
	m_infos->refcount = 1;
	assert(m_infos->data);
}

Reference::Reference(Reference &&other) noexcept :
	m_infos(other.m_infos) {
	++m_infos->refcount;
	assert(m_infos->data);
}

Reference::Reference(ReferenceInfos *infos) noexcept :
	m_infos(infos) {
	++m_infos->refcount;
	assert(m_infos->data);
}

Reference::~Reference() {
	assert(m_infos);
	if (!--m_infos->refcount) {
		if (!m_infos->data->infos.collected) {
			assert(m_infos->data);
			g_garbageCollector.release(m_infos->data);
		}
		g_pool.free(m_infos);
	}
}

Reference &Reference::operator =(Reference &&other) noexcept {
	swap(m_infos, other.m_infos);
	assert(m_infos->data);
	return *this;
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
		if (Scheduler *scheduler = Scheduler::instance()) {
			Object *object = static_cast<Object *>(ptr);
			if (Class::MemberInfo *member = object->metadata->findOperator(Class::delete_operator)) {
				if (is_object(object)) {
					Reference &member_ref = object->data[member->offset];
					if (member_ref.data()->format == Data::fmt_function) {
						scheduler->createDestructor(object, std::move(member_ref), member->owner);
						break;
					}
				}
			}
			destroy(object);
		}
		else {
			if (WeakReference *&members = static_cast<Object *>(ptr)->data) {
				const size_t members_count = mint::malloc_size(members) / sizeof(WeakReference);
				for (size_t offset = 0; offset < members_count; ++offset) {
					members[offset].~WeakReference();
				}
				::free(members);
				members = nullptr;
			}

			if (String *string_ptr = dynamic_cast<String *>(ptr)) {
				String::g_pool.free(string_ptr);
			}
			else if (Regex *regex_ptr = dynamic_cast<Regex *>(ptr)) {
				Regex::g_pool.free(regex_ptr);
			}
			else if (Array *array_ptr = dynamic_cast<Array *>(ptr)) {
				Array::g_pool.free(array_ptr);
			}
			else if (Hash *hash_ptr = dynamic_cast<Hash *>(ptr)) {
				Hash::g_pool.free(hash_ptr);
			}
			else if (Iterator *iterator_ptr = dynamic_cast<Iterator *>(ptr)) {
				Iterator::g_pool.free(iterator_ptr);
			}
			else if (Library *library_ptr = dynamic_cast<Library *>(ptr)) {
				Library::g_pool.free(library_ptr);
			}
			else if (LibObject<void> *lib_object_ptr = dynamic_cast<LibObject<void> *>(ptr)) {
				delete lib_object_ptr;
			}
			else {
				Object::g_pool.free(ptr);
			}
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
	return GlobalData::instance()->noneRef()->data<None>();
}

template<>
Null *Reference::alloc<Null>() {
	return GlobalData::instance()->nullRef()->data<Null>();
}

WeakReference::WeakReference(Flags flags, Data *data) :
	Reference(flags, data) {

}

WeakReference::WeakReference(WeakReference &&other) noexcept :
	Reference(std::forward<WeakReference>(other)) {

}

WeakReference::WeakReference(Reference &&other) noexcept :
	Reference(std::forward<Reference>(other)) {

}

WeakReference::WeakReference(ReferenceInfos *infos) :
	Reference(infos) {

}

WeakReference::~WeakReference() {

}

WeakReference &WeakReference::operator =(WeakReference &&other) noexcept {
	Reference::operator=(std::forward<WeakReference>(other));
	return *this;
}

StrongReference::StrongReference(Flags flags, Data *data) :
	Reference(flags, data) {

}

StrongReference::StrongReference(StrongReference &&other) noexcept :
	Reference(std::forward<StrongReference>(other)) {

}

StrongReference::StrongReference(WeakReference &&other) noexcept :
	Reference(std::forward<WeakReference>(other)) {

}

StrongReference::StrongReference(Reference &&other) noexcept :
	Reference(std::forward<Reference>(other)) {

}

StrongReference::StrongReference(ReferenceInfos *infos) :
	Reference(infos) {

}

StrongReference::~StrongReference() {

}

StrongReference &StrongReference::operator =(StrongReference &&other) noexcept {
	Reference::operator =(std::move(other));
	return *this;
}

StrongReference &StrongReference::operator =(WeakReference &&other) noexcept {
	Reference::operator=(std::move(other));
	return *this;
}
