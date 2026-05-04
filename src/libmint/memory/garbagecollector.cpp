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
#include "mint/system/error.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <memory>
#include <utility>
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
LocalPool<Coroutine> Coroutine::g_pool;
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

	// if collection is deferred, mark that a collection is pending and return without collecting
	if (_defer_depth != 0) {
		_pending_collection = true;
		return 0;
	}

	// if already collecting, do not start another collection
	if (!_collecting.empty()) {
		return 0;
	}

	// mark roots
	for (auto* root = _roots.head; root != nullptr; root = root->_next) {
		root->mark();
	}

	// sweep
	for (auto* data = _memory.head; data != nullptr; data = data->_next) {
		if (data->_info.reachable) {
			data->_info.reachable = (data->_info.refcount == 0);
		}
		else {
			data->_info.collected = true;
			gc_list_remove_element(_memory, data);
			_collecting.emplace_back(data);
		}
	}

	// call destructors as possible
	if (auto* scheduler = Scheduler::instance()) {
		for (auto* data : _collecting) {
			if (auto* object = dynamic_cast<Object*>(data)) {
				if (auto* slots = object->data) {
					if (const auto* member = object->metadata.find_operator(Class::delete_operator)) {
						if (is_instance_of(Class::MemberInfo::get(*member, slots), Data::Format::function)) {
							scheduler->invoke(WeakReference(Reference::default_flags, *object), Class::delete_operator);
						}
					}
				}
			}
		}
	}

	// free memory
	for (auto* data : _collecting) {
		GarbageCollector::destroy(data);
	}

	_threshold = std::max(_threshold, ((_count / threshold_growth_factor) + 1) * threshold_growth_factor);

	auto collected = std::move(_collecting);
	_collecting.clear();
	return collected.size();
}

void GarbageCollector::clean() {

	// cleanup builtin refs
	_none.reset();
	_null.reset();

	assert(_roots.head == nullptr);

	while (collect() > 0) {
		;
	}

	assert(_memory.head == nullptr);
}

bool GarbageCollector::is_deferred() const noexcept {
	return _defer_depth != 0;
}

void GarbageCollector::defer() {
	++_defer_depth;
}

void GarbageCollector::resume() {

	assert(_defer_depth > 0);

	if (--_defer_depth == 0 && _pending_collection) {
		_pending_collection = false;
		collect();
	}
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
	case Data::Format::null:
		return alloc<Null>();
	case Data::Format::none:
		return alloc<None>();
	case Data::Format::number:
		return alloc<Number>(static_cast<const Number&>(other));
	case Data::Format::boolean:
		return alloc<Boolean>(static_cast<const Boolean&>(other));
	case Data::Format::object:
		{
			Object* data = nullptr;
			const auto& object = static_cast<const Object&>(other);
			switch (object.metadata.metatype()) {
			case Class::Metatype::object:
				data = alloc<Object>(object.metadata);
				break;
			case Class::Metatype::string:
				data = alloc<String>(static_cast<const String&>(other));
				break;
			case Class::Metatype::regex:
				data = alloc<Regex>(static_cast<const Regex&>(other));
				break;
			case Class::Metatype::array:
				data = alloc<Array>(static_cast<const Array&>(other));
				break;
			case Class::Metatype::hash:
				data = alloc<Hash>(static_cast<const Hash&>(other));
				break;
			case Class::Metatype::iterator:
			case Class::Metatype::async_iterator:
				data = alloc<Iterator>(static_cast<const Iterator&>(other));
				break;
			case Class::Metatype::library:
				data = alloc<Library>(static_cast<const Library&>(other));
				break;
			case Class::Metatype::libobject:
				return &const_cast<Data&>(other); // safe ?
			}
			data->construct(object);
			return data;
		}
	case Data::Format::package:
		return alloc<Package>(static_cast<const Package&>(other).data);
	case Data::Format::function:
		return alloc<Function>(static_cast<const Function&>(other));
	case Data::Format::coroutine:
		error("type 'coroutine' is not copyable");
	}

	return nullptr;
}

void GarbageCollector::free(Data* ptr) {
	switch (ptr->format()) {
	case Data::Format::none:
	case Data::Format::null:
		delete ptr;
		break;
	case Data::Format::number:
		Number::g_pool.free(static_cast<Number*>(ptr));
		break;
	case Data::Format::boolean:
		Boolean::g_pool.free(static_cast<Boolean*>(ptr));
		break;
	case Data::Format::object:
		if (Scheduler* scheduler = Scheduler::instance()) {
			auto* object = static_cast<Object*>(ptr);
			if (WeakReference* slots = object->data) {
				if (const auto* member = object->metadata.find_operator(Class::delete_operator)) {
					const auto& member_ref = Class::MemberInfo::get(*member, slots);
					if (member_ref.data().format() == Data::Format::function) {
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
	case Data::Format::package:
		Package::g_pool.free(static_cast<Package*>(ptr));
		break;
	case Data::Format::function:
		Function::g_pool.free(static_cast<Function*>(ptr));
		break;
	case Data::Format::coroutine:
		Coroutine::g_pool.free(static_cast<Coroutine*>(ptr));
		break;
	}
}

void GarbageCollector::destroy(Data* ptr) {
	switch (ptr->format()) {
	case Data::Format::none:
	case Data::Format::null:
		delete ptr;
		break;
	case Data::Format::number:
		Number::g_pool.free(static_cast<Number*>(ptr));
		break;
	case Data::Format::boolean:
		Boolean::g_pool.free(static_cast<Boolean*>(ptr));
		break;
	case Data::Format::object:
		destroy(static_cast<Object*>(ptr));
		break;
	case Data::Format::package:
		Package::g_pool.free(static_cast<Package*>(ptr));
		break;
	case Data::Format::function:
		Function::g_pool.free(static_cast<Function*>(ptr));
		break;
	case Data::Format::coroutine:
		Coroutine::g_pool.free(static_cast<Coroutine*>(ptr));
		break;
	}
}

void GarbageCollector::destroy(Object* ptr) {
	switch (ptr->metadata.metatype()) {
	case Class::Metatype::object:
		Object::g_pool.free(ptr);
		break;
	case Class::Metatype::string:
		String::g_pool.free(static_cast<String*>(ptr));
		break;
	case Class::Metatype::regex:
		Regex::g_pool.free(static_cast<Regex*>(ptr));
		break;
	case Class::Metatype::array:
		Array::g_pool.free(static_cast<Array*>(ptr));
		break;
	case Class::Metatype::hash:
		Hash::g_pool.free(static_cast<Hash*>(ptr));
		break;
	case Class::Metatype::iterator:
	case Class::Metatype::async_iterator:
		Iterator::g_pool.free(static_cast<Iterator*>(ptr));
		break;
	case Class::Metatype::library:
		Library::g_pool.free(static_cast<Library*>(ptr));
		break;
	case Class::Metatype::libobject:
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
