/**
 * Copyright (c) 2026 Gauvain CHERY.
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
#include "mint/memory/data.h"
#include "mint/memory/memorytool.h"
#include "mint/memory/reference.h"
#include "mint/memory/object.h"
#include "mint/scheduler/scheduler.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <list>
#include <memory>
#include <vector>

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
LocalPool<Reference::Info> Reference::g_pool;

GarbageCollector::GarbageCollector() = default;

GarbageCollector::~GarbageCollector() {

	for (Data* data = _memory.head; data != nullptr; data = data->_next) {
		data->_info.reachable = false;
	}

	clean();
}

GarbageCollector& GarbageCollector::instance() {
	static GarbageCollector g_instance;
	return g_instance;
}

std::size_t GarbageCollector::collect() {

	std::list<Data*> collected;

	// mark roots
	for (MemoryRoot* root = _roots.head; root != nullptr; root = root->_next) {
		root->mark();
	}

	// mark stacks
	for (const std::vector<WeakReference>* stack : _stacks) {
		for (const WeakReference& reference : *stack) {
			reference.data().mark();
		}
	}

	// sweep
	for (Data* data = _memory.head; data != nullptr; data = data->_next) {
		if (data->_info.reachable) {
			data->_info.reachable = (data->_info.refcount == 0);
		}
		else {
			data->_info.collected = true;
			gc_list_remove_element(_memory, data);
			collected.emplace_back(data);
		}
	}

	// call destructors as possible
	if (Scheduler* scheduler = Scheduler::instance()) {
		for (Data* data : collected) {
			if (data->format() == Data::object_format) {
				auto* object = static_cast<Object*>(data);
				if (WeakReference* slots = object->data) {
					if (Class::MemberInfo* member = object->metadata.find_operator(Class::delete_operator)) {
						if (is_instance_of(Class::MemberInfo::get(*member, slots), Data::function_format)) {
							scheduler->invoke(WeakReference(Reference::default_flags, *object), Class::delete_operator);
						}
					}
				}
			}
		}
	}

	// free memory
	for (Data* data : collected) {
		GarbageCollector::destroy(data);
	}

	_threshold = std::max(_threshold, ((_count / threshold_growth_factor) + 1) * threshold_growth_factor);

	return collected.size();
}

void GarbageCollector::clean() {

	// cleanup builtin refs
	_none.reset();
	_null.reset();

	assert(_stacks.empty());
	assert(_roots.head == nullptr);

	while (collect() > 0) {
		;
	}

	assert(_memory.head == nullptr);
}

void GarbageCollector::register_data(Data* data) {
	gc_list_insert_element(_memory, data);
	++_count;
}

void GarbageCollector::unregister_data(Data* data) {
	gc_list_remove_element(_memory, data);
	--_count;
}

void GarbageCollector::register_root(MemoryRoot* root) {
	assert(_roots.head == nullptr || _roots.head->_prev == nullptr);
	assert(_roots.tail == nullptr || _roots.tail->_next == nullptr);
	gc_list_insert_element(_roots, root);
	assert(_roots.head->_prev == nullptr);
	assert(_roots.tail->_next == nullptr);
}

void GarbageCollector::unregister_root(MemoryRoot* root) {
	assert(_roots.head->_prev == nullptr);
	assert(_roots.tail->_next == nullptr);
	gc_list_remove_element(_roots, root);
	assert(_roots.head == nullptr || _roots.head->_prev == nullptr);
	assert(_roots.tail == nullptr || _roots.tail->_next == nullptr);
}

std::vector<WeakReference>* GarbageCollector::create_stack() {
	auto* stack = new std::vector<WeakReference>();
	stack->reserve(default_stack_capacity);
	return *_stacks.emplace(stack).first;
}

void GarbageCollector::remove_stack(std::vector<WeakReference>* stack) {
	_stacks.erase(stack);
	delete stack;
}

Reference& GarbageCollector::none_ref() {
	return _none ? *_none
	             : *(_none = std::make_unique<StrongReference>(Reference::const_address | Reference::const_value,
	                     *new None));
}

Reference& GarbageCollector::null_ref() {
	return _null ? *_null
	             : *(_null = std::make_unique<StrongReference>(Reference::const_address | Reference::const_value,
	                     *new Null));
}

std::size_t GarbageCollector::get_threshold() const {
	return _threshold;
}

void GarbageCollector::set_threshold(std::size_t threshold) {
	_threshold = threshold;
}

std::size_t GarbageCollector::get_refcount(const Data& data) {
	return data._info.refcount;
}

std::size_t GarbageCollector::get_count() const {
	return _count;
}

template<>
None* GarbageCollector::alloc<None>() {
	return &none_ref().data<None>();
}

template<>
Null* GarbageCollector::alloc<Null>() {
	return &null_ref().data<Null>();
}

Data* GarbageCollector::copy(const Data& other) {
	switch (other.format()) {
	case Data::null_format:
		return alloc<Null>();
	case Data::none_format:
		return alloc<None>();
	case Data::number_format:
		return alloc<Number>(static_cast<const Number&>(other));
	case Data::boolean_format:
		return alloc<Boolean>(static_cast<const Boolean&>(other));
	case Data::object_format:
		{
			Object* data = nullptr;
			const auto& object = static_cast<const Object&>(other);
			switch (object.metadata.metatype()) {
			case Class::object:
				data = alloc<Object>(object.metadata);
				break;
			case Class::string:
				data = alloc<String>(static_cast<const String&>(other));
				break;
			case Class::regex:
				data = alloc<Regex>(static_cast<const Regex&>(other));
				break;
			case Class::array:
				data = alloc<Array>(static_cast<const Array&>(other));
				break;
			case Class::hash:
				data = alloc<Hash>(static_cast<const Hash&>(other));
				break;
			case Class::iterator:
				data = alloc<Iterator>(static_cast<const Iterator&>(other));
				break;
			case Class::library:
				data = alloc<Library>(static_cast<const Library&>(other));
				break;
			case Class::libobject:
				return &const_cast<Data&>(other); // safe ?
			}
			data->construct(object);
			return data;
		}
	case Data::package_format:
		return alloc<Package>(static_cast<const Package&>(other).data);
	case Data::function_format:
		return alloc<Function>(static_cast<const Function&>(other));
	}

	return nullptr;
}

void GarbageCollector::free(Data* ptr) {
	switch (ptr->format()) {
	case Data::none_format:
	case Data::null_format:
		delete ptr;
		break;
	case Data::number_format:
		Number::g_pool.free(static_cast<Number*>(ptr));
		break;
	case Data::boolean_format:
		Boolean::g_pool.free(static_cast<Boolean*>(ptr));
		break;
	case Data::object_format:
		if (Scheduler* scheduler = Scheduler::instance()) {
			auto* object = static_cast<Object*>(ptr);
			if (WeakReference* slots = object->data) {
				if (Class::MemberInfo* member = object->metadata.find_operator(Class::delete_operator)) {
					const auto& member_ref = Class::MemberInfo::get(*member, slots);
					if (member_ref.data().format() == Data::function_format) {
						scheduler->create_destructor(object, member_ref, member->owner);
						break;
					}
				}
			}
			destroy(object);
		}
		else {
			destroy(static_cast<Object*>(ptr));
		}
		break;
	case Data::package_format:
		Package::g_pool.free(static_cast<Package*>(ptr));
		break;
	case Data::function_format:
		Function::g_pool.free(static_cast<Function*>(ptr));
		break;
	}
}

void GarbageCollector::destroy(Data* ptr) {
	switch (ptr->format()) {
	case Data::none_format:
	case Data::null_format:
		delete ptr;
		break;
	case Data::number_format:
		Number::g_pool.free(static_cast<Number*>(ptr));
		break;
	case Data::boolean_format:
		Boolean::g_pool.free(static_cast<Boolean*>(ptr));
		break;
	case Data::object_format:
		destroy(static_cast<Object*>(ptr));
		break;
	case Data::package_format:
		Package::g_pool.free(static_cast<Package*>(ptr));
		break;
	case Data::function_format:
		Function::g_pool.free(static_cast<Function*>(ptr));
		break;
	}
}

void GarbageCollector::destroy(Object* ptr) {
	switch (ptr->metadata.metatype()) {
	case Class::object:
		Object::g_pool.free(ptr);
		break;
	case Class::string:
		String::g_pool.free(static_cast<String*>(ptr));
		break;
	case Class::regex:
		Regex::g_pool.free(static_cast<Regex*>(ptr));
		break;
	case Class::array:
		Array::g_pool.free(static_cast<Array*>(ptr));
		break;
	case Class::hash:
		Hash::g_pool.free(static_cast<Hash*>(ptr));
		break;
	case Class::iterator:
		Iterator::g_pool.free(static_cast<Iterator*>(ptr));
		break;
	case Class::library:
		Library::g_pool.free(static_cast<Library*>(ptr));
		break;
	case Class::libobject:
		delete ptr;
		break;
	}
}

MemoryRoot::MemoryRoot() = default;

MemoryRoot::MemoryRoot(MemoryRoot&& /*other*/) noexcept {}

MemoryRoot::MemoryRoot(const MemoryRoot& /*other*/) {}

MemoryRoot::~MemoryRoot() {
#ifdef MINT_BUILD_TYPE_DEBUG
	assert(!_registered);
#endif
}

MemoryRoot& MemoryRoot::operator=(MemoryRoot&& /*other*/) noexcept {
	return *this;
}

MemoryRoot& MemoryRoot::operator=(const MemoryRoot& /*other*/) {
	return *this;
}

void MemoryRoot::register_root() {
#ifdef MINT_BUILD_TYPE_DEBUG
	assert(!_registered);
#endif
	static GarbageCollector& g_garbage_collector = GarbageCollector::instance();
	g_garbage_collector.register_root(this);
#ifdef MINT_BUILD_TYPE_DEBUG
	_registered = true;
#endif
}

void MemoryRoot::unregister_root() {
#ifdef MINT_BUILD_TYPE_DEBUG
	assert(_registered);
#endif
	static GarbageCollector& g_garbage_collector = GarbageCollector::instance();
	g_garbage_collector.unregister_root(this);
#ifdef MINT_BUILD_TYPE_DEBUG
	_registered = false;
#endif
}
