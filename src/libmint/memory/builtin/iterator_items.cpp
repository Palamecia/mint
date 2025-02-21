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

#include "iterator_items.h"
#include "iterator_p.h"
#include "memory/reference.h"
#include "mint/memory/functiontool.h"
#include "mint/memory/builtin/string.h"
#include "mint/system/utf8.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <limits>
#include <utility>

using namespace mint::internal;
using namespace mint;

ItemsIteratorData::ItemsIteratorData() :
	m_capacity(1) {
	m_data = static_cast<WeakReference *>(malloc(m_capacity * sizeof(WeakReference)));
}

ItemsIteratorData::ItemsIteratorData(size_t capacity) :
	m_capacity(capacity) {
	m_data = static_cast<WeakReference *>(malloc(m_capacity * sizeof(WeakReference)));
}

ItemsIteratorData::ItemsIteratorData(Reference &ref) {
	switch (ref.data()->format) {
	case Data::FMT_NONE:
		m_capacity = 1;
		m_data = static_cast<WeakReference *>(malloc(m_capacity * sizeof(WeakReference)));
		break;
	case Data::FMT_OBJECT:
		switch (ref.data<Object>()->metadata->metatype()) {
		case Class::STRING:
			m_capacity = ref.data<String>()->str.size();
			m_data = static_cast<WeakReference *>(malloc(m_capacity * sizeof(WeakReference)));
			for (utf8iterator i = ref.data<String>()->str.begin(); i != ref.data<String>()->str.end(); ++i) {
				new (m_data + m_size++) WeakReference(create_string(*i));
			}
			break;
		case Class::ARRAY:
			m_capacity = ref.data<Array>()->values.size();
			m_data = static_cast<WeakReference *>(malloc(m_capacity * sizeof(WeakReference)));
			for (auto &item : ref.data<Array>()->values) {
				new (m_data + m_size++) WeakReference(array_get_item(item));
			}
			break;
		case Class::HASH:
			m_capacity = ref.data<Hash>()->values.size();
			m_data = static_cast<WeakReference *>(malloc(m_capacity * sizeof(WeakReference)));
			for (auto &item : ref.data<Hash>()->values) {
				WeakReference element(Reference::CONST_ADDRESS | Reference::CONST_VALUE,
									  GarbageCollector::instance().alloc<Iterator>(2));
				element.data<Iterator>()->ctx.yield(hash_get_key(item));
				element.data<Iterator>()->ctx.yield(hash_get_value(item));
				element.data<Iterator>()->construct();
				new (m_data + m_size++) WeakReference(WeakReference::share(element));
			}
			break;
		case Class::ITERATOR:
			m_capacity = ref.data<Iterator>()->ctx.size();
			m_data = static_cast<WeakReference *>(malloc(m_capacity * sizeof(WeakReference)));
			for (Reference &item : ref.data<Iterator>()->ctx) {
				new (m_data + m_size++) WeakReference(WeakReference::share(item));
			}
			break;
		default:
			m_capacity = 1;
			m_data = static_cast<WeakReference *>(malloc(m_capacity * sizeof(WeakReference)));
			new (m_data + m_size++) WeakReference(std::forward<Reference>(ref));
			break;
		}
		break;
	default:
		m_capacity = 1;
		m_data = static_cast<WeakReference *>(malloc(m_capacity * sizeof(WeakReference)));
		new (m_data + m_size++) WeakReference(std::forward<Reference>(ref));
		break;
	}
}

ItemsIteratorData::ItemsIteratorData(Reference &&ref) {
	switch (ref.data()->format) {
	case Data::FMT_NONE:
		m_capacity = 1;
		m_data = static_cast<WeakReference *>(malloc(m_capacity * sizeof(WeakReference)));
		break;
	case Data::FMT_OBJECT:
		switch (ref.data<Object>()->metadata->metatype()) {
		case Class::STRING:
			m_capacity = ref.data<String>()->str.size();
			m_data = static_cast<WeakReference *>(malloc(m_capacity * sizeof(WeakReference)));
			for (utf8iterator i = ref.data<String>()->str.begin(); i != ref.data<String>()->str.end(); ++i) {
				new (m_data + m_size++) WeakReference(create_string(*i));
			}
			break;
		case Class::ARRAY:
			m_capacity = ref.data<Array>()->values.size();
			m_data = static_cast<WeakReference *>(malloc(m_capacity * sizeof(WeakReference)));
			for (auto &item : ref.data<Array>()->values) {
				new (m_data + m_size++) WeakReference(array_get_item(item));
			}
			break;
		case Class::HASH:
			m_capacity = ref.data<Hash>()->values.size();
			m_data = static_cast<WeakReference *>(malloc(m_capacity * sizeof(WeakReference)));
			for (auto &item : ref.data<Hash>()->values) {
				WeakReference element(Reference::CONST_ADDRESS | Reference::CONST_VALUE,
									  GarbageCollector::instance().alloc<Iterator>(2));
				element.data<Iterator>()->ctx.yield(hash_get_key(item));
				element.data<Iterator>()->ctx.yield(hash_get_value(item));
				element.data<Iterator>()->construct();
				new (m_data + m_size++) WeakReference(WeakReference::share(element));
			}
			break;
		case Class::ITERATOR:
			m_capacity = ref.data<Iterator>()->ctx.size();
			m_data = static_cast<WeakReference *>(malloc(m_capacity * sizeof(WeakReference)));
			for (Reference &item : ref.data<Iterator>()->ctx) {
				new (m_data + m_size++) WeakReference(WeakReference::share(item));
			}
			break;
		default:
			m_capacity = 1;
			m_data = static_cast<WeakReference *>(malloc(m_capacity * sizeof(WeakReference)));
			new (m_data + m_size++) WeakReference(std::move(ref));
			break;
		}
		break;
	default:
		m_capacity = 1;
		m_data = static_cast<WeakReference *>(malloc(m_capacity * sizeof(WeakReference)));
		new (m_data + m_size++) WeakReference(std::move(ref));
		break;
	}
}

ItemsIteratorData::ItemsIteratorData(ItemsIteratorData &&other) noexcept :
	m_data(other.m_data),
	m_capacity(other.m_capacity),
	m_size(other.m_size),
	m_pos(other.m_pos) {
	other.m_data = nullptr;
	other.m_size = 0;
}

ItemsIteratorData::ItemsIteratorData(const ItemsIteratorData &other) :
	m_data(static_cast<WeakReference *>(malloc(other.m_capacity * sizeof(WeakReference)))),
	m_capacity(other.m_capacity) {
	while (m_size < other.m_size) {
		new (m_data + m_size)
			WeakReference(WeakReference::share(other.m_data[(other.m_pos + m_size) % other.m_capacity]));
		++m_size;
	}
}

ItemsIteratorData::~ItemsIteratorData() {
	for (size_t i = 0; i < m_size; ++i) {
		m_data[(m_pos + i) % m_capacity].~WeakReference();
	}
	free(m_data);
}

mint::internal::IteratorData *ItemsIteratorData::copy() {
	return new ItemsIteratorData(*this);
}

void ItemsIteratorData::mark() {
	for (size_t i = 0; i < m_size; ++i) {
		m_data[(m_pos + i) % m_capacity].data()->mark();
	}
}

Iterator::Context::Type ItemsIteratorData::get_type() const {
	return Iterator::Context::ITEMS;
}

Iterator::Context::value_type &ItemsIteratorData::value() {
	return m_data[m_pos];
}

Iterator::Context::value_type &ItemsIteratorData::last() {
	return m_data[(m_pos + m_size - 1) % m_capacity];
}

size_t ItemsIteratorData::capacity() const {
	return m_capacity;
}

void ItemsIteratorData::reserve(size_t capacity) {
	if (m_capacity < capacity) {

		WeakReference *data = m_data;
		std::swap(m_capacity, capacity);
		m_data = static_cast<WeakReference *>(malloc(m_capacity * sizeof(WeakReference)));

		for (size_t i = 0; i < m_size; ++i) {
			WeakReference *item = data + ((m_pos + i) % capacity);
			new (m_data + i) WeakReference(std::move(*item));
			item->~WeakReference();
		}

		free(data);
		m_pos = 0;
	}
}

void ItemsIteratorData::yield(Iterator::Context::value_type &&value) {
	if (m_size >= m_capacity) {
		increase_size();
	}
	new (m_data + ((m_pos + m_size) % m_capacity)) WeakReference(std::move(value));
	++m_size;
}

void ItemsIteratorData::next() {
	assert(m_size != 0);
	m_data[m_pos].~WeakReference();
	m_pos = (m_pos + 1) % m_capacity;
	--m_size;
}

void ItemsIteratorData::finalize() {}

void ItemsIteratorData::clear() {
	for (size_t i = 0; i < m_size; ++i) {
		m_data[(m_pos + i) % m_capacity].~WeakReference();
	}
	m_size = m_pos = 0;
}

size_t ItemsIteratorData::size() const {
	return m_size;
}

bool ItemsIteratorData::empty() const {
	return m_size == 0;
}

void ItemsIteratorData::increase_size() {

	const size_t capacity = m_capacity;
	WeakReference *data = m_data;

	m_capacity = std::min(capacity * 2, std::numeric_limits<size_t>::max());
	m_data = static_cast<WeakReference *>(malloc(m_capacity * sizeof(WeakReference)));

	for (size_t i = 0; i < m_size; ++i) {
		WeakReference *item = data + ((m_pos + i) % capacity);
		new (m_data + i) WeakReference(std::move(*item));
		item->~WeakReference();
	}

	free(data);
	m_pos = 0;
}
