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

#include "mint/memory/object.h"
#include "mint/memory/class.h"
#include "mint/memory/reference.h"
#include "mint/memory/builtin/array.h"
#include "mint/memory/builtin/hash.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/memory/builtin/library.h"
#include "mint/memory/builtin/regex.h"
#include "mint/memory/builtin/string.h"
#include "mint/system/error.h"

using namespace mint;

Number::Number(double value) :
	Data(FMT_NUMBER),
	value(value) {}

Number::Number(const Number &other) :
	Data(FMT_NUMBER),
	value(other.value) {}

Boolean::Boolean(bool value) :
	Data(FMT_BOOLEAN),
	value(value) {}

Boolean::Boolean(const Boolean &other) :
	Data(FMT_BOOLEAN),
	value(other.value) {}

Object::Object(Class *type) :
	Data(FMT_OBJECT),
	metadata(type),
	data(nullptr) {}

Object::~Object() {
	if (data) {
		for (size_t offset = 0; offset < metadata->size(); ++offset) {
			data[offset].~WeakReference();
		}
		free(data);
	}
}

void Object::construct() {

	data = static_cast<WeakReference *>(malloc(metadata->size() * sizeof(WeakReference)));

	for (const Class::MemberInfo *member : metadata->slots()) {
		new (data + member->offset) WeakReference(WeakReference::clone(member->value));
	}
}

void Object::construct(const Object &other) {
	std::unordered_map<const Data *, Data *> memory_map;
	memory_map.emplace(&other, this);
	construct(other, memory_map);
}

void Object::construct(const Object &other, std::unordered_map<const Data *, Data *> &memory_map) {

	if (other.data) {

		if (UNLIKELY(!metadata->is_copyable())) {
			error("type '%s' is not copyable", metadata->full_name().c_str());
		}

		data = static_cast<WeakReference *>(malloc(metadata->size() * sizeof(WeakReference)));

		for (auto &member : metadata->slots()) {

			Reference &target_ref = other.data[member->offset];
			Reference *member_ref = data + member->offset;
			auto i = memory_map.find(target_ref.data());

			if (i == memory_map.end()) {
				if ((target_ref.flags() & (Reference::CONST_ADDRESS | Reference::CONST_VALUE))
					!= (Reference::CONST_ADDRESS | Reference::CONST_VALUE)) {
					switch (target_ref.data()->format) {
					case Data::FMT_OBJECT:
						switch (target_ref.data<Object>()->metadata->metatype()) {
						case Class::OBJECT:
							member_ref = new (member_ref)
								WeakReference(target_ref.flags(), GarbageCollector::instance().alloc<Object>(
																	  target_ref.data<Object>()->metadata));
							break;
						case Class::STRING:
							member_ref = new (member_ref)
								WeakReference(target_ref.flags(),
											  GarbageCollector::instance().alloc<String>(*target_ref.data<String>()));
							break;
						case Class::REGEX:
							member_ref = new (member_ref)
								WeakReference(target_ref.flags(),
											  GarbageCollector::instance().alloc<Regex>(*target_ref.data<Regex>()));
							break;
						case Class::ARRAY:
							member_ref = new (member_ref)
								WeakReference(target_ref.flags(),
											  GarbageCollector::instance().alloc<Array>(*target_ref.data<Array>()));
							break;
						case Class::HASH:
							member_ref = new (member_ref)
								WeakReference(target_ref.flags(),
											  GarbageCollector::instance().alloc<Hash>(*target_ref.data<Hash>()));
							break;
						case Class::ITERATOR:
							member_ref = new (member_ref)
								WeakReference(target_ref.flags(), GarbageCollector::instance().alloc<Iterator>(
																	  *target_ref.data<Iterator>()));
							break;
						case Class::LIBRARY:
							member_ref = new (member_ref)
								WeakReference(target_ref.flags(),
											  GarbageCollector::instance().alloc<Library>(*target_ref.data<Library>()));
							break;
						case Class::LIBOBJECT:
							member_ref = new (member_ref) WeakReference(WeakReference::clone(target_ref));
							memory_map.emplace(target_ref.data(), member_ref->data());
							continue;
						}

						memory_map.emplace(target_ref.data(), member_ref->data());
						member_ref->data<Object>()->construct(*target_ref.data<Object>(), memory_map);
						break;

					default:
						member_ref = new (member_ref) WeakReference(WeakReference::clone(target_ref));
						memory_map.emplace(target_ref.data(), member_ref->data());
						break;
					}
				}
				else {
					member_ref = new (member_ref) WeakReference(WeakReference::share(target_ref));
					memory_map.emplace(target_ref.data(), member_ref->data());
				}
			}
			else {
				new (member_ref) WeakReference(target_ref.flags(), i->second);
			}
		}
	}
}

void Object::mark() {
	if (!marked_bit()) {
		Data::mark();
		if (data) {
			for (size_t offset = 0; offset < metadata->size(); ++offset) {
				data[offset].data()->mark();
			}
		}
	}
}

Package::Package(PackageData *package) :
	Data(FMT_PACKAGE),
	data(package) {}

Function::Function() :
	Data(FMT_FUNCTION) {}

Function::Function(const Function &other) :
	Data(FMT_FUNCTION),
	mapping(other.mapping) {}

Function::Signature::Signature(Module::Handle *handle, bool capture) :
	handle(handle),
	capture(capture ? new Capture : nullptr) {}

Function::Signature::Signature(Signature &&other) noexcept :
	handle(other.handle),
	capture(other.capture) {
	other.capture = nullptr;
}

Function::Signature::Signature(const Signature &other) :
	handle(other.handle),
	capture(other.capture ? new Function::Capture : nullptr) {
	if (capture) {
		for (auto &symbol : *other.capture) {
			capture->emplace(symbol.first, WeakReference::share(symbol.second));
		}
	}
}

Function::Signature::~Signature() {
	delete capture;
}

Function::Mapping::Mapping() :
	m_data(new SharedData) {}

Function::Mapping::Mapping(Mapping &&other) noexcept :
	m_data(other.m_data) {
	other.m_data = nullptr;
}

Function::Mapping::Mapping(const Mapping &other) {
	if (other.m_data->is_sharable()) {
		m_data = other.m_data->share();
	}
	else {
		m_data = other.m_data->detach();
	}
}

Function::Mapping::~Mapping() {
	if (m_data && !--m_data->refcount) {
		delete m_data;
	}
}

Function::Mapping &Function::Mapping::operator=(Mapping &&other) noexcept {
	std::swap(m_data, other.m_data);
	return *this;
}

Function::Mapping &Function::Mapping::operator=(const Mapping &other) {
	if (UNLIKELY(&other == this)) {
		return *this;
	}
	if (m_data && !--m_data->refcount) {
		delete m_data;
	}
	if (other.m_data->is_sharable()) {
		m_data = other.m_data->share();
	}
	else {
		m_data = other.m_data->detach();
	}
	return *this;
}

bool Function::Mapping::operator==(const Mapping &other) const {

	if (m_data->signatures.size() != other.m_data->signatures.size()) {
		return false;
	}

	for (auto it1 = m_data->signatures.begin(), it2 = other.m_data->signatures.begin();
		 it1 != m_data->signatures.end() && it2 != other.m_data->signatures.end(); ++it1, ++it2) {
		if (it1->first != it2->first || it1->second.handle != it2->second.handle) {
			return false;
		}
	}

	return true;
}

bool Function::Mapping::operator!=(const Mapping &other) const {

	if (m_data->signatures.size() != other.m_data->signatures.size()) {
		return true;
	}

	for (auto it1 = m_data->signatures.begin(), it2 = other.m_data->signatures.begin();
		 it1 != m_data->signatures.end() && it2 != other.m_data->signatures.end(); ++it1, ++it2) {
		if (it1->first != it2->first || it1->second.handle != it2->second.handle) {
			return true;
		}
	}

	return false;
}

std::pair<Function::Mapping::iterator, bool> Function::Mapping::emplace(int signature,
																				  const Signature &handle) {
	if (m_data->is_shared()) {
		m_data = m_data->detach();
	}
	if (handle.capture) {
		m_data->sharable = false;
	}
	return m_data->signatures.emplace(signature, handle);
}

std::pair<Function::Mapping::iterator, bool> Function::Mapping::insert(const std::pair<int, Signature> &signature) {
	if (m_data->is_shared()) {
		m_data = m_data->detach();
	}
	if (signature.second.capture) {
		m_data->sharable = false;
	}
	return m_data->signatures.insert(signature);
}

Function::Mapping::iterator Function::Mapping::lower_bound(int signature) const {
	return m_data->signatures.lower_bound(signature);
}

Function::Mapping::iterator Function::Mapping::find(int signature) const {
	return m_data->signatures.find(signature);
}

Function::Mapping::const_iterator Function::Mapping::cbegin() const {
	return m_data->signatures.cbegin();
}

Function::Mapping::const_iterator Function::Mapping::begin() const {
	return m_data->signatures.begin();
}

Function::Mapping::iterator Function::Mapping::begin() {
	return m_data->signatures.begin();
}

Function::Mapping::const_iterator Function::Mapping::cend() const {
	return m_data->signatures.cend();
}

Function::Mapping::const_iterator Function::Mapping::end() const {
	return m_data->signatures.end();
}

Function::Mapping::iterator Function::Mapping::end() {
	return m_data->signatures.end();
}

bool Function::Mapping::empty() const {
	return m_data->signatures.empty();
}

void Function::mark() {
	if (!marked_bit()) {
		Data::mark();
		for (const auto &signature : mapping) {
			if (const auto &capture = signature.second.capture) {
				for (const auto &reference : *capture) {
					reference.second.data()->mark();
				}
			}
		}
	}
}
