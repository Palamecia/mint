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

#include <cmath>

using namespace std;
using namespace mint;

Number::Number() : Data(fmt_number) {

}

Number::Number(double value) : Data(fmt_number),
	value(value) {

}

Number::Number(const Number &other) : Data(fmt_number),
	value(other.value) {

}

double mint::to_unsigned_number(const string &str, bool *error) {

	const char *value = str.c_str();
	intmax_t intpart = 0;

	if (value[0] == '0') {
		switch (value[1]) {
		case 'b':
		case 'B':
			for (const char *cptr = value + 2; *cptr != '\0'; ++cptr) {
				switch (*cptr) {
				case '0':
					intpart = intpart << 1;
					break;
				case '1':
					intpart = (intpart << 1) + 1;
					break;
				default:
					if (error) {
						*error = true;
					}
					return 0;
				}
			}
			if (error) {
				*error = false;
			}
			return intpart;
		case 'o':
		case 'O':
			for (const char *cptr = value + 2; *cptr != '\0'; ++cptr) {
				if ('0' <= *cptr && *cptr < '8') {
					intpart = intpart * 8 + (*cptr - '0');
				}
				else {
					if (error) {
						*error = true;
					}
					return 0;
				}
			}
			if (error) {
				*error = false;
			}
			return intpart;

		case 'x':
		case 'X':
			for (const char *cptr = value + 2; *cptr != '\0'; ++cptr) {
				if (*cptr >= 'A') {
					const int digit = ((*cptr - 'A') & (~('a' ^ 'A'))) + 10;
					if (digit < 16) {
						intpart = intpart * 16 + digit;
					}
					else {
						if (error) {
							*error = true;
						}
						return 0;
					}
				}
				else if (isdigit(*cptr)) {
					intpart = intpart * 16 + (*cptr - '0');
				}
				else {
					if (error) {
						*error = true;
					}
					return 0;
				}
			}
			if (error) {
				*error = false;
			}
			return intpart;

		default:
			break;
		}
	}

	bool decimals = false;
	bool exponant = false;
	double fracpart = 0.;
	intmax_t fracexp = 0;
	intmax_t exppart = 0;
	intmax_t expsign = 0;

	for (const char *cptr = value; *cptr != '\0'; ++cptr) {
		switch (*cptr) {
		case '.':
			if (decimals || exponant) {
				if (error) {
					*error = true;
				}
				return 0;
			}
			decimals = true;
			break;
		case 'e':
		case 'E':
			if (exponant) {
				if (error) {
					*error = true;
				}
				return 0;
			}
			exponant = true;
			switch (cptr[1]) {
			case '+':
				expsign = +1;
				++cptr;
				break;
			case '-':
				expsign = -1;
				++cptr;
				break;
			default:
				break;
			}
			break;
		default:
			if (isdigit(*cptr)) {
				if (exponant) {
					exppart = exppart * 10 + (*cptr - '0');
				}
				else if (decimals) {
					fracpart = fracpart * 10. + (*cptr - '0');
					--fracexp;
				}
				else {
					intpart = intpart * 10 + (*cptr - '0');
				}
			}
			else {
				if (error) {
					*error = true;
				}
				return 0;
			}
		}
	}

	if (error) {
		*error = false;
	}

	if (exponant) {
		return (fracpart * pow(10, fracexp) + intpart) * pow(10, copysign(exppart, expsign));
	}

	if (decimals) {
		return fracpart * pow(10, fracexp) + intpart;
	}

	return intpart;
}

double mint::to_signed_number(const string &str, bool *error) {
	const char *data = str.data();
	return *data == '-' ? -to_unsigned_number(data + 1, error) : to_unsigned_number(str, error);
}

Boolean::Boolean() : Data(fmt_boolean) {

}

Boolean::Boolean(bool value) : Data(fmt_boolean),
	value(value) {

}

Boolean::Boolean(const Boolean &other) : Data(fmt_boolean),
	value(other.value) {

}

Object::Object(Class *type) :
	Data(fmt_object),
	metadata(type),
	data(nullptr) {

}

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

	for (auto &member : metadata->slots()) {
		new (data + member->offset) WeakReference(WeakReference::clone(member->value));
	}
}

void Object::construct(const Object &other) {
	unordered_map<const Data *, Data *> memory_map;
	memory_map.emplace(&other, this);
	construct(other, memory_map);
}

void Object::construct(const Object &other, unordered_map<const Data *, Data *> &memory_map) {

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
				if ((target_ref.flags() & (Reference::const_address | Reference::const_value)) != (Reference::const_address | Reference::const_value)) {
					switch (target_ref.data()->format) {
					case Data::fmt_object:
						switch (target_ref.data<Object>()->metadata->metatype()) {
						case Class::object:
							member_ref = new (member_ref) WeakReference(target_ref.flags(), GarbageCollector::instance().alloc<Object>(target_ref.data<Object>()->metadata));
							break;
						case Class::string:
							member_ref = new (member_ref) WeakReference(target_ref.flags(), GarbageCollector::instance().alloc<String>(*target_ref.data<String>()));
							break;
						case Class::regex:
							member_ref = new (member_ref) WeakReference(target_ref.flags(), GarbageCollector::instance().alloc<Regex>(*target_ref.data<Regex>()));
							break;
						case Class::array:
							member_ref = new (member_ref) WeakReference(target_ref.flags(), GarbageCollector::instance().alloc<Array>(*target_ref.data<Array>()));
							break;
						case Class::hash:
							member_ref = new (member_ref) WeakReference(target_ref.flags(), GarbageCollector::instance().alloc<Hash>(*target_ref.data<Hash>()));
							break;
						case Class::iterator:
							member_ref = new (member_ref) WeakReference(target_ref.flags(), GarbageCollector::instance().alloc<Iterator>(*target_ref.data<Iterator>()));
							break;
						case Class::library:
							member_ref = new (member_ref) WeakReference(target_ref.flags(), GarbageCollector::instance().alloc<Library>(*target_ref.data<Library>()));
							break;
						case Class::libobject:
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
	Data(fmt_package),
	data(package) {

}

Function::Function() : Data(fmt_function) {

}

Function::Function(const Function &other) : Data(fmt_function),
	mapping(other.mapping) {

}

Function::Signature::Signature(Module::Handle *handle, bool capture) :
	handle(handle),
	capture(capture ? new Capture : nullptr) {

}

Function::Signature::Signature(const Signature &other) :
	handle(other.handle),
	capture(other.capture ? new Function::Capture : nullptr) {
	if (capture) {
		for (const auto &symbol : *other.capture) {
			capture->emplace(symbol.first, WeakReference(symbol.second.flags(), symbol.second.data()));
		}
	}
}

Function::Signature::Signature(Signature &&other) :
	handle(other.handle),
	capture(std::forward<Capture *const>(other.capture)) {

}

Function::Signature::~Signature() {
	delete capture;
}

Function::mapping_type::mapping_type() :
	m_data(new shared_data_t) {

}

Function::mapping_type::mapping_type(const mapping_type &other) {
	if (other.m_data->is_sharable()) {
		m_data = other.m_data->share();
	}
	else {
		m_data = other.m_data->detach();
	}
}

Function::mapping_type::mapping_type(mapping_type &&other) noexcept :
	m_data(other.m_data) {
	other.m_data = nullptr;
}

Function::mapping_type::~mapping_type() {
	if (m_data && !--m_data->refcount) {
		delete m_data;
	}
}

Function::mapping_type &Function::mapping_type::operator =(mapping_type &&other) noexcept {
	swap(m_data, other.m_data);
	return *this;
}

Function::mapping_type &Function::mapping_type::operator =(const mapping_type &other) {
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

bool Function::mapping_type::operator ==(const mapping_type &other) const {

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

bool Function::mapping_type::operator !=(const mapping_type &other) const {

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

pair<Function::mapping_type::iterator, bool> Function::mapping_type::emplace(int signature, const Signature &handle) {
	if (m_data->is_shared()) {
		m_data = m_data->detach();
	}
	if (handle.capture) {
		m_data->sharable = false;
	}
	return m_data->signatures.emplace(signature, handle);
}

pair<Function::mapping_type::iterator, bool> Function::mapping_type::insert(const pair<int, Signature> &signature) {
	if (m_data->is_shared()) {
		m_data = m_data->detach();
	}
	if (signature.second.capture) {
		m_data->sharable = false;
	}
	return m_data->signatures.insert(signature);
}

Function::mapping_type::iterator Function::mapping_type::lower_bound(int signature) const {
	return m_data->signatures.lower_bound(signature);
}

Function::mapping_type::iterator Function::mapping_type::find(int signature) const {
	return m_data->signatures.find(signature);
}

Function::mapping_type::const_iterator Function::mapping_type::cbegin() const {
	return m_data->signatures.cbegin();
}

Function::mapping_type::const_iterator Function::mapping_type::begin() const {
	return m_data->signatures.begin();
}

Function::mapping_type::iterator Function::mapping_type::begin() {
	return m_data->signatures.begin();
}

Function::mapping_type::const_iterator Function::mapping_type::cend() const {
	return m_data->signatures.cend();
}

Function::mapping_type::const_iterator Function::mapping_type::end() const {
	return m_data->signatures.end();
}

Function::mapping_type::iterator Function::mapping_type::end() {
	return m_data->signatures.end();
}

bool Function::mapping_type::empty() const {
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
