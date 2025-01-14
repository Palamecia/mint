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

#include "mint/memory/functiontool.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/memory/builtin/string.h"
#include "mint/memory/operatortool.h"
#include "mint/memory/globaldata.h"
#include "mint/scheduler/scheduler.h"
#include "mint/ast/cursor.h"

using namespace mint;

ReferenceHelper::ReferenceHelper(const FunctionHelper *function, Reference &&reference) :
	m_function(function),
	m_reference(std::forward<Reference>(reference)) {}

ReferenceHelper ReferenceHelper::operator[](const Symbol &symbol) const {
	return m_function->member(m_reference, symbol);
}

ReferenceHelper ReferenceHelper::member(const Symbol &symbol) const {
	return m_function->member(m_reference, symbol);
}

ReferenceHelper::operator Reference &() {
	return m_reference;
}

ReferenceHelper::operator Reference &&() {
	return std::move(m_reference);
}

const Reference &ReferenceHelper::operator*() const {
	return m_reference;
}

const Reference *ReferenceHelper::operator->() const {
	return &m_reference;
}

const Reference *ReferenceHelper::get() const {
	return &m_reference;
}

FunctionHelper::FunctionHelper(Cursor *cursor, size_t argc) :
	m_cursor(cursor),
	m_base(static_cast<ssize_t>(get_stack_base(cursor))),
	m_value_returned(false) {

	m_top = m_base - static_cast<ssize_t>(argc);
}

FunctionHelper::~FunctionHelper() {

	if (!m_value_returned) {
		return_value(WeakReference::create<None>());
	}
}

Reference &FunctionHelper::pop_parameter() {

	assert(m_base > m_top);
	return load_from_stack(m_cursor, static_cast<size_t>(m_base--));
}

ReferenceHelper FunctionHelper::reference(const Symbol &symbol) const {
	GlobalData *globalData = GlobalData::instance();
	auto it = globalData->symbols().find(symbol);
	if (it != globalData->symbols().end()) {
		return ReferenceHelper(this, WeakReference::share(it->second));
	}
	return ReferenceHelper(this, WeakReference::create<None>());
}

ReferenceHelper FunctionHelper::member(const Reference &object, const Symbol &symbol) const {
	return ReferenceHelper(this, get_member(m_cursor, object, symbol));
}

void FunctionHelper::return_value(Reference &&value) {

	assert(m_value_returned == false);

	while (static_cast<ssize_t>(get_stack_base(m_cursor)) > m_top) {
		m_cursor->stack().pop_back();
	}

	m_cursor->stack().emplace_back(std::forward<Reference>(value));
	m_value_returned = true;
}

WeakReference mint::create_number(double value) {
	return WeakReference::create<Number>(value);
}

WeakReference mint::create_boolean(bool value) {
	return WeakReference::create<Boolean>(value);
}

WeakReference mint::create_string(const char *value) {
	WeakReference ref = WeakReference::create<String>(value);
	ref.data<String>()->construct();
	return ref;
}

WeakReference mint::create_string(const std::string &value) {
	WeakReference ref = WeakReference::create<String>(value);
	ref.data<String>()->construct();
	return ref;
}

WeakReference mint::create_string(std::string_view value) {
	WeakReference ref = WeakReference::create<String>(value);
	ref.data<String>()->construct();
	return ref;
}

WeakReference mint::create_array(Array::values_type &&values) {
	WeakReference ref = WeakReference::create<Array>();
	ref.data<Array>()->values.swap(values);
	ref.data<Array>()->construct();
	return ref;
}

WeakReference mint::create_array(std::initializer_list<WeakReference> items) {
	WeakReference ref = WeakReference::create<Array>();
	for (auto i = items.begin(); i != items.end(); ++i) {
		array_append(ref.data<Array>(), array_item(*i));
	}
	ref.data<Array>()->construct();
	return ref;
}

WeakReference mint::create_hash(Hash::values_type &&values) {
	WeakReference ref = WeakReference::create<Hash>();
	ref.data<Hash>()->values.swap(values);
	ref.data<Hash>()->construct();
	return ref;
}

WeakReference mint::create_hash(std::initializer_list<std::pair<WeakReference, WeakReference>> items) {
	WeakReference ref = WeakReference::create<Hash>();
	for (auto i = items.begin(); i != items.end(); ++i) {
		hash_insert(ref.data<Hash>(), i->first, i->second);
	}
	ref.data<Hash>()->construct();
	return ref;
}

WeakReference mint::create_array() {
	WeakReference ref = WeakReference::create<Array>();
	ref.data<Array>()->construct();
	return ref;
}

WeakReference mint::create_hash() {
	WeakReference ref = WeakReference::create<Hash>();
	ref.data<Hash>()->construct();
	return ref;
}

WeakReference mint::create_iterator() {
	WeakReference ref = WeakReference::create<Iterator>();
	ref.data<Iterator>()->construct();
	return ref;
}

#ifdef OS_WINDOWS
WeakReference mint::create_handle(mint::handle_t handle) {
	WeakReference ref = WeakReference::create<LibObject<std::remove_pointer<HANDLE>::type>>();
	ref.data<LibObject<std::remove_pointer<HANDLE>::type>>()->impl = handle;
	ref.data<LibObject<std::remove_pointer<HANDLE>::type>>()->construct();
	return ref;
}

mint::handle_t mint::to_handle(const Reference &reference) {
	return reference.data<LibObject<std::remove_pointer<HANDLE>::type>>()->impl;
}

mint::handle_t *mint::to_handle_ptr(const Reference &reference) {
	return &reference.data<LibObject<std::remove_pointer<HANDLE>::type>>()->impl;
}
#else
WeakReference mint::create_handle(mint::handle_t handle) {
	WeakReference ref = WeakReference::create<LibObject<void>>();
	ref.data<LibObject<void>>()->construct();
	ref.data<LibObject<void>>()->impl = reinterpret_cast<void *>(handle);
	return ref;
}

mint::handle_t mint::to_handle(const Reference &reference) {
	return static_cast<handle_t>(reinterpret_cast<intptr_t>(reference.data<LibObject<void>>()->impl));
}

mint::handle_t *mint::to_handle_ptr(const Reference &reference) {
	return reinterpret_cast<handle_t *>(&reference.data<LibObject<void>>()->impl);
}
#endif

WeakReference mint::get_member_ignore_visibility(Reference &reference, const Symbol &member) {

	switch (reference.data()->format) {
	case Data::FMT_PACKAGE:
		for (PackageData *package_data = reference.data<Package>()->data; package_data != nullptr;
			 package_data = package_data->get_package()) {
			if (auto it = package_data->symbols().find(member); it != package_data->symbols().end()) {
				return WeakReference::share(it->second);
			}
		}
		break;

	case Data::FMT_OBJECT:
		if (Object *object = reference.data<Object>()) {

			if (auto it = object->metadata->members().find(member); it != object->metadata->members().end()) {
				if (is_object(object)) {
					return WeakReference::share(Class::MemberInfo::get(it->second, object));
				}
				else {
					return WeakReference(Reference::CONST_ADDRESS | Reference::CONST_VALUE | Reference::GLOBAL,
										 it->second->value.data());
				}
			}

			if (auto it = object->metadata->globals().find(member); it != object->metadata->globals().end()) {
				return WeakReference::share(it->second->value);
			}

			for (PackageData *package = object->metadata->get_package(); package != nullptr;
				 package = package->get_package()) {
				if (auto it = package->symbols().find(member); it != package->symbols().end()) {
					return WeakReference(Reference::CONST_ADDRESS | Reference::CONST_VALUE, it->second.data());
				}
			}
		}
		break;

	default:
		GlobalData *externals = GlobalData::instance();
		if (auto it = externals->symbols().find(member); it != externals->symbols().end()) {
			return WeakReference(Reference::CONST_ADDRESS | Reference::CONST_VALUE, it->second.data());
		}
	}

	return {};
}

WeakReference mint::get_member_ignore_visibility(PackageData *package, const Symbol &member) {
	for (PackageData *package_data = package; package_data != nullptr; package_data = package_data->get_package()) {
		if (auto it = package_data->symbols().find(member); it != package_data->symbols().end()) {
			return WeakReference::share(it->second);
		}
	}
	return {};
}

WeakReference mint::get_member_ignore_visibility(Object *object, const Symbol &member) {
	auto it = object->metadata->members().find(member);
	if (it != object->metadata->members().end()) {
		return WeakReference::share(Class::MemberInfo::get(it->second, object));
	}
	return {};
}

WeakReference mint::get_global_ignore_visibility(Object *object, const Symbol &global) {
	auto it = object->metadata->globals().find(global);
	if (it != object->metadata->globals().end()) {
		return WeakReference::share(it->second->value);
	}
	return {};
}

WeakReference mint::find_enum_value(Object *object, double value) {
	for (auto [symbol, info] : object->metadata->globals()) {
		if (is_instance_of(info->value, Data::FMT_NUMBER) && info->value.data<Number>()->value == value) {
			return WeakReference::share(info->value);
		}
	}
	return {};
}
