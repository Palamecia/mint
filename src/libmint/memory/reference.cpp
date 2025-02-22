/**
 * Copyright (c) 2025 Gauvain CHERY.
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

using namespace mint;

LocalPool<Reference::Info> Reference::g_pool;
GarbageCollector &Reference::g_garbage_collector = GarbageCollector::instance();

Reference::Reference(Flags flags, Data *data) :
	m_info(g_pool.alloc()) {
	m_info->flags = flags;
	g_garbage_collector.use(m_info->data = data ? data : g_garbage_collector.alloc<None>());
	m_info->refcount = 1;
	assert(m_info->data);
}

Reference::Reference(Reference &&other) noexcept :
	m_info(other.m_info) {
	++m_info->refcount;
	assert(m_info->data);
}

Reference::Reference(Info *infos) noexcept :
	m_info(infos) {
	++m_info->refcount;
	assert(m_info->data);
}

Reference::~Reference() {
	assert(m_info);
	if (!--m_info->refcount) {
		assert(m_info->data);
		g_garbage_collector.release(m_info->data);
		g_pool.free(m_info);
	}
}

Reference &Reference::operator=(Reference &&other) noexcept {
	std::swap(m_info, other.m_info);
	assert(m_info->data);
	return *this;
}

void Reference::copy_data(const Reference &other) {
	set_data(g_garbage_collector.copy(other.data()));
}

void Reference::move_data(const Reference &other) {
	set_data(other.data());
}

void Reference::set_data(Data *data) {

	assert(data);

	Data *previous = m_info->data;
	g_garbage_collector.use(m_info->data = data);
	g_garbage_collector.release(previous);
}

Reference::Info *Reference::info() {
	return m_info;
}

WeakReference::WeakReference(Flags flags, Data *data) :
	Reference(flags, data) {}

WeakReference::WeakReference(WeakReference &&other) noexcept :
	Reference(std::forward<WeakReference>(other)) {}

WeakReference::WeakReference(Reference &&other) noexcept :
	Reference(std::move(other)) {}

WeakReference::WeakReference(Info *infos) :
	Reference(infos) {}

WeakReference::~WeakReference() {}

WeakReference &WeakReference::operator=(WeakReference &&other) noexcept {
	Reference::operator=(std::forward<WeakReference>(other));
	return *this;
}

StrongReference::StrongReference(Flags flags, Data *data) :
	Reference(flags, data) {}

StrongReference::StrongReference(StrongReference &&other) noexcept :
	Reference(std::forward<StrongReference>(other)) {}

StrongReference::StrongReference(WeakReference &&other) noexcept :
	Reference(std::move(other)) {}

StrongReference::StrongReference(Reference &&other) noexcept :
	Reference(std::move(other)) {}

StrongReference::StrongReference(Info *infos) :
	Reference(infos) {}

StrongReference::~StrongReference() {}

StrongReference &StrongReference::operator=(StrongReference &&other) noexcept {
	Reference::operator=(std::move(other));
	return *this;
}

StrongReference &StrongReference::operator=(WeakReference &&other) noexcept {
	Reference::operator=(std::move(other));
	return *this;
}
