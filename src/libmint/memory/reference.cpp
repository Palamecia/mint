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

#include "mint/memory/reference.h"
#include "mint/memory/data.h"
#include "mint/memory/garbagecollector.h"
#include <cassert>
#include <memory>
#include <utility>

using namespace mint;

Reference::Info::Info(Flags flags, Data* data) :
    _flags(flags),
    _data(data) {
	assert(_data);
	static GarbageCollector& g_garbage_collector = GarbageCollector::instance();
	g_garbage_collector.use(_data);
}

Reference::Info::~Info() {
	static GarbageCollector& g_garbage_collector = GarbageCollector::instance();
	g_garbage_collector.release(_data);
}

void Reference::Info::set_data(Data* data) {

	static GarbageCollector& g_garbage_collector = GarbageCollector::instance();

	assert(data != nullptr);

	std::swap(_data, data);
	g_garbage_collector.use(_data);
	g_garbage_collector.release(data);
}

Reference::Reference(Flags flags, Data* data) :
    _info(std::make_shared<Info>(flags, data)) {}

void Reference::copy_data(const Reference& other) {
	_info->set_data(Info::copy(other.data()));
}

void Reference::move_data(const Reference& other) {
	_info->set_data(&other.data());
}

std::shared_ptr<Reference::Info> Reference::info() const {
	return _info;
}

WeakReference::WeakReference() :
    Reference(default_flags, Info::alloc<None>()) {}

WeakReference::WeakReference(Flags flags) :
    Reference(flags, Info::alloc<None>()) {}

WeakReference::WeakReference(Flags flags, Data& data) :
    Reference(flags, &data) {}

WeakReference::WeakReference(const Reference& other) :
    Reference(other) {}

WeakReference::WeakReference(Reference&& other) noexcept :
    Reference(std::move(other)) {}

WeakReference::~WeakReference() {}

StrongReference::StrongReference() :
    Reference(default_flags, Info::alloc<None>()) {
	register_root();
}

StrongReference::StrongReference(Flags flags) :
    Reference(flags, Info::alloc<None>()) {
	register_root();
}

StrongReference::StrongReference(Flags flags, Data& data) :
    Reference(flags, &data) {
	register_root();
}

StrongReference::StrongReference(const StrongReference& other) :
    Reference(other) {
	register_root();
}

StrongReference::StrongReference(const WeakReference& other) :
    Reference(other) {
	register_root();
}

StrongReference::StrongReference(const Reference& other) :
    Reference(other) {
	register_root();
}

StrongReference::StrongReference(StrongReference&& other) noexcept :
    Reference(std::move(other)) {
	register_root();
}

StrongReference::StrongReference(WeakReference&& other) noexcept :
    Reference(std::move(other)) {
	register_root();
}

StrongReference::StrongReference(Reference&& other) noexcept :
    Reference(std::move(other)) {
	register_root();
}

StrongReference::~StrongReference() {
	unregister_root();
}

StrongReference& StrongReference::operator=(WeakReference&& other) noexcept {
	Reference::operator=(std::move(other));
	return *this;
}

StrongReference& StrongReference::operator=(const WeakReference& other) {
	Reference::operator=(other);
	return *this;
}
