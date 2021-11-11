#include "memory/object.h"
#include "memory/class.h"
#include "memory/memorytool.h"
#include "memory/operatortool.h"
#include "memory/builtin/string.h"
#include "memory/builtin/regex.h"
#include "memory/builtin/library.h"
#include "scheduler/scheduler.h"
#include "scheduler/processor.h"
#include "ast/abstractsyntaxtree.h"
#include "ast/cursor.h"
#include "system/plugin.h"
#include "system/error.h"

#include <functional>

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

	data = static_cast<WeakReference *>(malloc(sizeof(WeakReference) * metadata->size()));

	for (auto &member : metadata->members()) {
		if ((member.second->value.flags() & (Reference::const_address | Reference::const_value)) != (Reference::const_address | Reference::const_value)) {
			new (data + member.second->offset) WeakReference(WeakReference::clone(member.second->value));
		}
		else {
			new (data + member.second->offset) WeakReference(WeakReference::share(member.second->value));
		}
	}
}

void Object::construct(const Object &other) {
	unordered_map<const Data *, Data *> memory_map;
	memory_map.emplace(&other, this);
	construct(other, memory_map);
}

void Object::construct(const Object &other, unordered_map<const Data *, Data *> &memory_map) {

	if (other.data) {

		if (UNLIKELY(!metadata->isCopyable())) {
			error("type '%s' is not copyable", metadata->name().c_str());
		}

		data = static_cast<WeakReference *>(malloc(sizeof(WeakReference) * metadata->size()));

		for (auto &member : metadata->members()) {

			Reference &target_ref = other.data[member.second->offset];
			Reference *member_ref = data + member.second->offset;
			auto i = memory_map.find(target_ref.data());

			if (i == memory_map.end()) {
				if ((target_ref.flags() & (Reference::const_address | Reference::const_value)) != (Reference::const_address | Reference::const_value)) {
					switch (target_ref.data()->format) {
					case Data::fmt_object:
						switch (target_ref.data<Object>()->metadata->metatype()) {
						case Class::object:
							member_ref = new (member_ref) WeakReference(target_ref.flags(), Reference::alloc<Object>(target_ref.data<Object>()->metadata));
							break;
						case Class::string:
							member_ref = new (member_ref) WeakReference(target_ref.flags(), Reference::alloc<String>(*target_ref.data<String>()));
							break;
						case Class::regex:
							member_ref = new (member_ref) WeakReference(target_ref.flags(), Reference::alloc<Regex>(*target_ref.data<Regex>()));
							break;
						case Class::array:
							member_ref = new (member_ref) WeakReference(target_ref.flags(), Reference::alloc<Array>(*target_ref.data<Array>()));
							break;
						case Class::hash:
							member_ref = new (member_ref) WeakReference(target_ref.flags(), Reference::alloc<Hash>(*target_ref.data<Hash>()));
							break;
						case Class::iterator:
							member_ref = new (member_ref) WeakReference(target_ref.flags(), Reference::alloc<Iterator>(*target_ref.data<Iterator>()));
							break;
						case Class::library:
							member_ref = new (member_ref) WeakReference(target_ref.flags(), Reference::alloc<Library>(*target_ref.data<Library>()));
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
	if (!markedBit()) {
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
	capture(move(other.capture)) {

}

Function::Signature::~Signature() {
	delete capture;
}

Function::mapping_type::mapping_type() :
	m_signatures(new map<int, Signature>),
	m_sharable(true),
	m_owned(true) {

}

Function::mapping_type::mapping_type(const mapping_type &other) :
	m_sharable(other.m_sharable) {
	if (m_sharable) {
		m_signatures = other.m_signatures;
		m_owned = false;
	}
	else {
		m_signatures.reset(new map<int, Signature>(*other.m_signatures));
		m_owned = true;
	}
}

Function::mapping_type::mapping_type(mapping_type &&other) noexcept :
	m_signatures(other.m_signatures),
	m_sharable(other.m_sharable),
	m_owned(other.m_owned) {

}

Function::mapping_type::~mapping_type() {

}

Function::mapping_type &Function::mapping_type::operator =(mapping_type &&other) noexcept {
	m_sharable = other.m_sharable;
	m_signatures = other.m_signatures;
	m_owned = other.m_owned;
	return *this;
}

Function::mapping_type &Function::mapping_type::operator =(const mapping_type &other) {
	m_sharable = other.m_sharable;
	if (m_sharable) {
		m_signatures = other.m_signatures;
		m_owned = false;
	}
	else {
		m_signatures.reset(new map<int, Signature>(*other.m_signatures));
		m_owned = true;
	}
	return *this;
}

pair<Function::mapping_type::iterator, bool> Function::mapping_type::emplace(int signature, const Signature &handle) {
	if (!m_owned) {
		m_signatures.reset(new map<int, Signature>(*m_signatures));
		m_owned = true;
	}
	if (handle.capture) {
		m_sharable = false;
	}
	return m_signatures->emplace(signature, handle);
}

pair<Function::mapping_type::iterator, bool> Function::mapping_type::insert(const pair<int, Signature> &signature) {
	if (!m_owned) {
		m_signatures.reset(new map<int, Signature>(*m_signatures));
		m_owned = true;
	}
	if (signature.second.capture) {
		m_sharable = false;
	}
	return m_signatures->insert(signature);
}

Function::mapping_type::iterator Function::mapping_type::lower_bound(int signature) const {
	return m_signatures->lower_bound(signature);
}

Function::mapping_type::iterator Function::mapping_type::find(int signature) const {
	return m_signatures->find(signature);
}

Function::mapping_type::const_iterator Function::mapping_type::cbegin() const {
	return m_signatures->cbegin();
}

Function::mapping_type::const_iterator Function::mapping_type::begin() const {
	return m_signatures->begin();
}

Function::mapping_type::iterator Function::mapping_type::begin() {
	return m_signatures->begin();
}

Function::mapping_type::const_iterator Function::mapping_type::cend() const {
	return m_signatures->cend();
}

Function::mapping_type::const_iterator Function::mapping_type::end() const {
	return m_signatures->end();
}

Function::mapping_type::iterator Function::mapping_type::end() {
	return m_signatures->end();
}

void Function::mark() {
	if (!markedBit()) {
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
