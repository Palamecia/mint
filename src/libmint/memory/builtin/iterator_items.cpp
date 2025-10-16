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

#include "iterator_items.h"
#include "iterator_p.h"
#include "mint/ast/classregister.h"
#include "mint/memory/builtin/array.h"
#include "mint/memory/builtin/hash.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/memory/builtin/string.h"
#include "mint/memory/class.h"
#include "mint/memory/data.h"
#include "mint/memory/functiontool.h"
#include "mint/memory/reference.h"
#include "mint/system/utf8.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <limits>
#include <memory>
#include <utility>

using namespace mint::internal;
using namespace mint;

std::allocator<WeakReference> ItemsIteratorData::g_allocator;

ItemsIteratorViewData::ItemsIteratorViewData(mint::WeakReference* data, std::size_t capacity, std::size_t size,
    std::size_t pos) :
    _data(data),
    _capacity(capacity),
    _size(size),
    _pos(pos) {}

mint::Iterator::Context::reference ItemsIteratorViewData::front() {
	return _data[_pos];
}

mint::Iterator::Context::reference ItemsIteratorViewData::back() {
	return _data[(_pos + _size - 1) % _capacity];
}

mint::Iterator::Context::reference ItemsIteratorViewData::get() {
	return _data[(_pos + _cur) % _capacity];
}

bool ItemsIteratorViewData::empty() const {
	return _cur == _size;
}

void ItemsIteratorViewData::prev() {
	--_cur;
}

void ItemsIteratorViewData::next() {
	++_cur;
}

ItemsIteratorData::ItemsIteratorData() :
    _capacity(1) {
	_data = g_allocator.allocate(_capacity);
}

ItemsIteratorData::ItemsIteratorData(std::size_t capacity) :
    _capacity(capacity) {
	_data = g_allocator.allocate(_capacity);
}

ItemsIteratorData::ItemsIteratorData(AbstractSyntaxTree& ast, const Reference& ref) {
	switch (ref.data().format()) {
	case Data::none_format:
		_capacity = 1;
		_data = g_allocator.allocate(_capacity);
		break;
	case Data::object_format:
		switch (ref.data<Object>().metadata.metatype()) {
		case Class::string:
			_capacity = ref.data<String>().str.size();
			_data = g_allocator.allocate(_capacity);
			for (const auto& item : views::utf8(ref.data<String>().str)) {
				std::construct_at(_data + _size++, create_string(ast, item));
			}
			break;
		case Class::array:
			_capacity = ref.data<Array>().values.size();
			_data = g_allocator.allocate(_capacity);
			for (auto& item : ref.data<Array>().values) {
				std::construct_at(_data + _size++, array_get_item(item));
			}
			break;
		case Class::hash:
			_capacity = ref.data<Hash>().values.size();
			_data = g_allocator.allocate(_capacity);
			for (auto& item : ref.data<Hash>().values) {
				auto element = make_weak_reference<Iterator>(Reference::const_address | Reference::const_value, ast, 2);
				element.data<Iterator>().ctx.yield(hash_get_key(item));
				element.data<Iterator>().ctx.yield(hash_get_value(item));
				element.data<Iterator>().construct();
				std::construct_at(_data + _size++, element);
			}
			break;
		case Class::iterator:
			_capacity = ref.data<Iterator>().ctx.size();
			_data = g_allocator.allocate(_capacity);
			for (const Reference& item : ref.data<Iterator>().ctx) {
				std::construct_at(_data + _size++, item);
			}
			break;
		default:
			_capacity = 1;
			_data = g_allocator.allocate(_capacity);
			std::construct_at(_data + _size++, ref);
			break;
		}
		break;
	default:
		_capacity = 1;
		_data = g_allocator.allocate(_capacity);
		std::construct_at(_data + _size++, ref);
		break;
	}
}

ItemsIteratorData::ItemsIteratorData(AbstractSyntaxTree& ast, Reference&& ref) {
	switch (ref.data().format()) {
	case Data::none_format:
		_capacity = 1;
		_data = g_allocator.allocate(_capacity);
		break;
	case Data::object_format:
		switch (ref.data<Object>().metadata.metatype()) {
		case Class::string:
			_capacity = ref.data<String>().str.size();
			_data = g_allocator.allocate(_capacity);
			for (const auto& item : views::utf8(ref.data<String>().str)) {
				std::construct_at(_data + _size++, create_string(ast, item));
			}
			break;
		case Class::array:
			_capacity = ref.data<Array>().values.size();
			_data = g_allocator.allocate(_capacity);
			for (auto& item : ref.data<Array>().values) {
				std::construct_at(_data + _size++, array_get_item(item));
			}
			break;
		case Class::hash:
			_capacity = ref.data<Hash>().values.size();
			_data = g_allocator.allocate(_capacity);
			for (auto& item : ref.data<Hash>().values) {
				auto element = make_weak_reference<Iterator>(Reference::const_address | Reference::const_value, ast, 2);
				element.data<Iterator>().ctx.yield(hash_get_key(item));
				element.data<Iterator>().ctx.yield(hash_get_value(item));
				element.data<Iterator>().construct();
				std::construct_at(_data + _size++, element);
			}
			break;
		case Class::iterator:
			_capacity = ref.data<Iterator>().ctx.size();
			_data = g_allocator.allocate(_capacity);
			for (const Reference& item : ref.data<Iterator>().ctx) {
				std::construct_at(_data + _size++, item);
			}
			break;
		default:
			_capacity = 1;
			_data = g_allocator.allocate(_capacity);
			std::construct_at(_data + _size++, std::move(ref));
			break;
		}
		break;
	default:
		_capacity = 1;
		_data = g_allocator.allocate(_capacity);
		std::construct_at(_data + _size++, std::move(ref));
		break;
	}
}

ItemsIteratorData::ItemsIteratorData(ItemsIteratorData&& other) noexcept :
    _data(other._data),
    _capacity(other._capacity),
    _size(other._size),
    _pos(other._pos) {
	other._data = nullptr;
	other._size = 0;
}

ItemsIteratorData::ItemsIteratorData(const ItemsIteratorData& other) :
    _data(g_allocator.allocate(other._capacity)),
    _capacity(other._capacity) {
	while (_size < other._size) {
		std::construct_at(_data + _size, other._data[(other._pos + _size) % other._capacity]);
		++_size;
	}
}

ItemsIteratorData::~ItemsIteratorData() {
	for (std::size_t i = 0; i < _size; ++i) {
		std::destroy_at(_data + ((_pos + i) % _capacity));
	}
	g_allocator.deallocate(_data, _capacity);
}

std::unique_ptr<IteratorViewData> ItemsIteratorData::view() {
	return std::make_unique<ItemsIteratorViewData>(_data, _capacity, _size, _pos);
}

std::unique_ptr<IteratorData> ItemsIteratorData::copy() {
	return std::make_unique<ItemsIteratorData>(*this);
}

void ItemsIteratorData::mark() {
	for (std::size_t i = 0; i < _size; ++i) {
		_data[(_pos + i) % _capacity].data().mark();
	}
}

Iterator::Context::Type ItemsIteratorData::get_type() const {
	return Iterator::Context::items;
}

Iterator::Context::value_type& ItemsIteratorData::get() {
	return _data[_pos];
}

std::size_t ItemsIteratorData::capacity() const {
	return _capacity;
}

void ItemsIteratorData::reserve(std::size_t capacity) {
	if (_capacity < capacity) {

		WeakReference* data = _data;
		std::swap(_capacity, capacity);
		_data = g_allocator.allocate(_capacity);

		for (std::size_t i = 0; i < _size; ++i) {
			WeakReference* item = data + ((_pos + i) % capacity);
			std::construct_at(_data + i, std::move(*item));
			item->~WeakReference();
		}

		g_allocator.deallocate(data, capacity);
		_pos = 0;
	}
}

void ItemsIteratorData::yield(Iterator::Context::value_type&& value) {
	if (_size >= _capacity) {
		increase_size();
	}
	std::construct_at(_data + ((_pos + _size) % _capacity), std::move(value));
	++_size;
}

void ItemsIteratorData::next() {
	assert(_size != 0);
	std::destroy_at(_data + _pos);
	_pos = (_pos + 1) % _capacity;
	--_size;
}

void ItemsIteratorData::finalize() {}

void ItemsIteratorData::clear() {
	for (std::size_t i = 0; i < _size; ++i) {
		std::destroy_at(_data + ((_pos + i) % _capacity));
	}
	_size = _pos = 0;
}

std::size_t ItemsIteratorData::size() const {
	return _size;
}

bool ItemsIteratorData::empty() const {
	return _size == 0;
}

void ItemsIteratorData::increase_size() {

	const std::size_t capacity = _capacity;
	WeakReference* data = _data;

	_capacity = std::min(capacity * 2, std::numeric_limits<std::size_t>::max());
	_data = g_allocator.allocate(_capacity);

	for (std::size_t i = 0; i < _size; ++i) {
		WeakReference* item = data + ((_pos + i) % capacity);
		std::construct_at(_data + i, std::move(*item));
		std::destroy_at(item);
	}

	g_allocator.deallocate(data, capacity);
	_pos = 0;
}
