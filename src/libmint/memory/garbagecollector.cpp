/**
 * Copyright (c) 2024 Gauvain CHERY.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "mint/memory/garbagecollector.h"
#include "mint/memory/builtin/array.h"
#include "mint/memory/builtin/hash.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/memory/builtin/library.h"
#include "mint/memory/builtin/libobject.h"
#include "mint/memory/builtin/regex.h"
#include "mint/memory/builtin/string.h"
#include "mint/memory/memorytool.h"
#include "mint/memory/reference.h"
#include "mint/memory/object.h"
#include "mint/scheduler/scheduler.h"
#include "mint/system/malloc.h"
#include "mint/system/assert.h"

#include <list>

using namespace std;
using namespace mint;

#define gc_list_insert_element(list, node) \
	if (list.tail) { \
		list.tail->next = node; \
		node->prev = list.tail; \
		list.tail = node; \
	} \
	else { \
		list.head = list.tail = node; \
	}

#define gc_list_remove_element(list, node) \
	if (node->prev) { \
		node->prev->next = node->next; \
	} \
	else { \
		list.head = node->next; \
	} \
	if (node->next) { \
		node->next->prev = node->prev; \
	} \
	else { \
		list.tail = node->prev; \
	}

GarbageCollector &MemoryRoot::g_garbage_collector = GarbageCollector::instance();

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

GarbageCollector::GarbageCollector() {

}

GarbageCollector::~GarbageCollector() {
	clean();
}

GarbageCollector &GarbageCollector::instance() {

	static GarbageCollector g_instance;
	return g_instance;
}

size_t GarbageCollector::collect() {

	list<Data *> collected;

	// mark roots
	for (MemoryRoot *root = m_roots.head; root != nullptr; root = root->next) {
		root->mark();
	}

	// mark stacks
	for (const vector<WeakReference> *stack : m_stacks) {
		for (const WeakReference &reference : *stack) {
			reference.data()->mark();
		}
	}

	// sweep
	for (Data *data = m_memory.head; data != nullptr; data = data->next) {
		if (data->infos.reachable) {
			data->infos.reachable = (data->infos.refcount == 0);
		}
		else {
			data->infos.collected = true;
			gc_list_remove_element(m_memory, data);
			collected.emplace_back(data);
		}
	}

	// call destructors as possible
	if (Scheduler *scheduler = Scheduler::instance()) {
		for (Data *data : collected) {
			if (data->format == Data::fmt_object) {
				Object *object = static_cast<Object *>(data);
				if (WeakReference *slots = object->data) {
					if (Class::MemberInfo *member = object->metadata->find_operator(Class::delete_operator)) {
						if (is_instance_of(Class::MemberInfo::get(member, slots), Data::fmt_function)) {
							WeakReference reference(Reference::standard, object);
							scheduler->invoke(reference, Class::delete_operator);
						}
					}
				}
			}
		}
	}

	// free memory
	for (Data *data : collected) {
		GarbageCollector::destroy(data);
	}

	return collected.size();
}

void GarbageCollector::clean() {

	assert(m_stacks.empty());
	assert(m_roots.head == nullptr);

	while (collect() > 0);

	assert(m_memory.head == nullptr);
}

void GarbageCollector::register_data(Data *data) {
	gc_list_insert_element(m_memory, data);
}

void GarbageCollector::unregister_data(Data *data) {
	gc_list_remove_element(m_memory, data);
}

void GarbageCollector::register_root(MemoryRoot *reference) {
	assert(m_roots.head == nullptr || m_roots.head->prev == nullptr);
	assert(m_roots.tail == nullptr || m_roots.tail->next == nullptr);
	gc_list_insert_element(m_roots, reference);
	assert(m_roots.head->prev == nullptr);
	assert(m_roots.tail->next == nullptr);
}

void GarbageCollector::unregister_root(MemoryRoot *reference) {
	assert(m_roots.head->prev == nullptr);
	assert(m_roots.tail->next == nullptr);
	gc_list_remove_element(m_roots, reference);
	assert(m_roots.head == nullptr || m_roots.head->prev == nullptr);
	assert(m_roots.tail == nullptr || m_roots.tail->next == nullptr);
}

vector<WeakReference> *GarbageCollector::create_stack() {
	vector<WeakReference> *stack = new vector<WeakReference>;
	m_stacks.emplace(stack);
	stack->reserve(0x4000);
	return stack;
}

void GarbageCollector::remove_stack(vector<WeakReference> *stack) {
	m_stacks.erase(stack);
	delete stack;
}

template<>
None *GarbageCollector::alloc<None>() {
	return GlobalData::instance()->none_ref()->data<None>();
}

template<>
Null *GarbageCollector::alloc<Null>() {
	return GlobalData::instance()->null_ref()->data<Null>();
}

Data *GarbageCollector::copy(const Data *other) {
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

void GarbageCollector::free(Data *ptr) {
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
			if (WeakReference *slots = object->data) {
				if (Class::MemberInfo *member = object->metadata->find_operator(Class::delete_operator)) {
					Reference &member_ref = Class::MemberInfo::get(member, slots);
					if (member_ref.data()->format == Data::fmt_function) {
						scheduler->create_destructor(object, std::move(member_ref), member->owner);
						break;
					}
				}
			}
			destroy(object);
		}
		else {
			destroy(static_cast<Object *>(ptr));
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

void GarbageCollector::destroy(Data *ptr) {
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
		destroy(static_cast<Object *>(ptr));
		break;
	case Data::fmt_package:
		Package::g_pool.free(static_cast<Package *>(ptr));
		break;
	case Data::fmt_function:
		Function::g_pool.free(static_cast<Function *>(ptr));
		break;
	}
}

void GarbageCollector::destroy(Object *ptr) {
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

MemoryRoot::MemoryRoot() {
	g_garbage_collector.register_root(this);
}

MemoryRoot::~MemoryRoot() {
	g_garbage_collector.unregister_root(this);
}
