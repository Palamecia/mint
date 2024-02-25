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

#include "mint/memory/reference.h"

using namespace std;
using namespace mint;

LocalPool<ReferenceInfos> Reference::g_pool;
GarbageCollector &Reference::g_garbage_collector = GarbageCollector::instance();

Reference::Reference(Flags flags, Data *data) :
	m_infos(g_pool.alloc()) {
	m_infos->flags = flags;
	g_garbage_collector.use(m_infos->data = data ? data : g_garbage_collector.alloc<None>());
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
		assert(m_infos->data);
		g_garbage_collector.release(m_infos->data);
		g_pool.free(m_infos);
	}
}

Reference &Reference::operator =(Reference &&other) noexcept {
	swap(m_infos, other.m_infos);
	assert(m_infos->data);
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

	Data *previous = m_infos->data;
	g_garbage_collector.use(m_infos->data = data);
	g_garbage_collector.release(previous);
}

ReferenceInfos *Reference::infos() {
	return m_infos;
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
