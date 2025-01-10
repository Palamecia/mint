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
#include "mint/memory/builtin/regex.h"
#include "mint/memory/builtin/string.h"
#include "mint/memory/memorytool.h"
#include "mint/memory/reference.h"
#include "mint/memory/object.h"
#include "mint/scheduler/scheduler.h"

#include <list>

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

	std::list<Data *> collected;

	// mark roots
	for (MemoryRoot *root = m_roots.head; root != nullptr; root = root->next) {
		root->mark();
	}

	// mark stacks
	for (const std::vector<WeakReference> *stack : m_stacks) {
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
			if (data->format == Data::FMT_OBJECT) {
				Object *object = static_cast<Object *>(data);
				if (WeakReference *slots = object->data) {
					if (Class::MemberInfo *member = object->metadata->find_operator(Class::DELETE_OPERATOR)) {
						if (is_instance_of(Class::MemberInfo::get(member, slots), Data::FMT_FUNCTION)) {
							WeakReference reference(Reference::DEFAULT, object);
							scheduler->invoke(reference, Class::DELETE_OPERATOR);
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

std::vector<WeakReference> *GarbageCollector::create_stack() {
	std::vector<WeakReference> *stack = new std::vector<WeakReference>;
	m_stacks.emplace(stack);
	stack->reserve(0x4000);
	return stack;
}

void GarbageCollector::remove_stack(std::vector<WeakReference> *stack) {
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
	case Data::FMT_NULL:
		return alloc<Null>();
	case Data::FMT_NONE:
		return alloc<None>();
	case Data::FMT_NUMBER:
		return alloc<Number>(*static_cast<const Number *>(other));
	case Data::FMT_BOOLEAN:
		return alloc<Boolean>(*static_cast<const Boolean *>(other));
	case Data::FMT_OBJECT:
	{
		Object *data = nullptr;
		const Object *object = static_cast<const Object *>(other);
		switch (object->metadata->metatype()) {
		case Class::OBJECT:
			data = alloc<Object>(object->metadata);
			break;
		case Class::STRING:
			data = alloc<String>(*static_cast<const String *>(other));
			break;
		case Class::REGEX:
			data = alloc<Regex>(*static_cast<const Regex *>(other));
			break;
		case Class::ARRAY:
			data = alloc<Array>(*static_cast<const Array *>(other));
			break;
		case Class::HASH:
			data = alloc<Hash>(*static_cast<const Hash *>(other));
			break;
		case Class::ITERATOR:
			data = alloc<Iterator>(*static_cast<const Iterator *>(other));
			break;
		case Class::LIBRARY:
			data = alloc<Library>(*static_cast<const Library *>(other));
			break;
		case Class::LIBOBJECT:
			/// \todo safe ?
			return const_cast<Data *>(other);
		}
		data->construct(*object);
		return data;
	}
	case Data::FMT_PACKAGE:
		return alloc<Package>(static_cast<const Package *>(other)->data);
	case Data::FMT_FUNCTION:
		return alloc<Function>(*static_cast<const Function *>(other));
	}

	return nullptr;
}

void GarbageCollector::free(Data *ptr) {
	switch (ptr->format) {
	case Data::FMT_NONE:
	case Data::FMT_NULL:
		delete ptr;
		break;
	case Data::FMT_NUMBER:
		Number::g_pool.free(static_cast<Number *>(ptr));
		break;
	case Data::FMT_BOOLEAN:
		Boolean::g_pool.free(static_cast<Boolean *>(ptr));
		break;
	case Data::FMT_OBJECT:
		if (Scheduler *scheduler = Scheduler::instance()) {
			Object *object = static_cast<Object *>(ptr);
			if (WeakReference *slots = object->data) {
				if (Class::MemberInfo *member = object->metadata->find_operator(Class::DELETE_OPERATOR)) {
					Reference &member_ref = Class::MemberInfo::get(member, slots);
					if (member_ref.data()->format == Data::FMT_FUNCTION) {
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
	case Data::FMT_PACKAGE:
		Package::g_pool.free(static_cast<Package *>(ptr));
		break;
	case Data::FMT_FUNCTION:
		Function::g_pool.free(static_cast<Function *>(ptr));
		break;
	}
}

void GarbageCollector::destroy(Data *ptr) {
	switch (ptr->format) {
	case Data::FMT_NONE:
	case Data::FMT_NULL:
		delete ptr;
		break;
	case Data::FMT_NUMBER:
		Number::g_pool.free(static_cast<Number *>(ptr));
		break;
	case Data::FMT_BOOLEAN:
		Boolean::g_pool.free(static_cast<Boolean *>(ptr));
		break;
	case Data::FMT_OBJECT:
		destroy(static_cast<Object *>(ptr));
		break;
	case Data::FMT_PACKAGE:
		Package::g_pool.free(static_cast<Package *>(ptr));
		break;
	case Data::FMT_FUNCTION:
		Function::g_pool.free(static_cast<Function *>(ptr));
		break;
	}
}

void GarbageCollector::destroy(Object *ptr) {
	switch (ptr->metadata->metatype()) {
	case Class::OBJECT:
		Object::g_pool.free(ptr);
		break;
	case Class::STRING:
		String::g_pool.free(static_cast<String *>(ptr));
		break;
	case Class::REGEX:
		Regex::g_pool.free(static_cast<Regex *>(ptr));
		break;
	case Class::ARRAY:
		Array::g_pool.free(static_cast<Array *>(ptr));
		break;
	case Class::HASH:
		Hash::g_pool.free(static_cast<Hash *>(ptr));
		break;
	case Class::ITERATOR:
		Iterator::g_pool.free(static_cast<Iterator *>(ptr));
		break;
	case Class::LIBRARY:
		Library::g_pool.free(static_cast<Library *>(ptr));
		break;
	case Class::LIBOBJECT:
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
