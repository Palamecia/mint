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

Boolean::Boolean() : Data(fmt_boolean) {

}

Object::Object(Class *type) :
	Data(fmt_object),
	metadata(type),
	data(nullptr),
	m_referenceManager(nullptr) {

}

Object::~Object() {
	delete m_referenceManager;
	delete [] data;
}

void Object::construct() {

	m_referenceManager = new ReferenceManager;
	data = new WeakReference [metadata->size()];

	for (auto member : metadata->members()) {
		data[member.second->offset].clone(member.second->value);
	}
}

void Object::construct(const Object &other) {
	map<const Data *, Data *> memory_map;
	memory_map.emplace(&other, this);
	construct(other, memory_map);
}

void Object::construct(const Object &other, map<const Data *, Data *> &memory_map) {

	if (other.data) {

		if (!metadata->isCopyable()) {
			error("type '%s' is not copyable", metadata->name().c_str());
		}

		m_referenceManager = new ReferenceManager;
		data = new WeakReference [metadata->size()];

		for (auto member : metadata->members()) {

			Reference &member_ref = data[member.second->offset];
			const Reference &target_ref = other.data[member.second->offset];
			auto i = memory_map.find(target_ref.data());

			if (i == memory_map.end()) {
				switch (target_ref.data()->format) {
				case Data::fmt_object:
					switch (target_ref.data<Object>()->metadata->metatype()) {
					case Class::object:
						member_ref = WeakReference(target_ref.flags(), Reference::alloc<Object>(target_ref.data<Object>()->metadata));
						break;
					case Class::string:
						member_ref = WeakReference(target_ref.flags(), Reference::alloc<String>());
						member_ref.data<String>()->str = target_ref.data<String>()->str;
						break;
					case Class::regex:
						member_ref = WeakReference(target_ref.flags(), Reference::alloc<Regex>());
						member_ref.data<Regex>()->initializer = target_ref.data<Regex>()->initializer;
						member_ref.data<Regex>()->expr = target_ref.data<Regex>()->expr;
						break;
					case Class::array:
						member_ref = WeakReference(target_ref.flags(), Reference::alloc<Array>());
						for (size_t i = 0; i < target_ref.data<Array>()->values.size(); ++i) {
							array_append(member_ref.data<Array>(), array_get_item(target_ref.data<Array>(), static_cast<intmax_t>(i)));
						}
						break;
					case Class::hash:
						member_ref = WeakReference(target_ref.flags(), Reference::alloc<Hash>());
						for (auto &item : target_ref.data<Hash>()->values) {
							hash_insert(member_ref.data<Hash>(), hash_get_key(target_ref.data<Hash>(), item), hash_get_value(target_ref.data<Hash>(), item));
						}
						break;
					case Class::iterator:
						member_ref = WeakReference(target_ref.flags(), Reference::alloc<Iterator>());
						for (SharedReference &item : target_ref.data<Iterator>()->ctx) {
							iterator_insert(member_ref.data<Iterator>(), SharedReference::unique(new StrongReference(*item)));
						}
						break;
					case Class::library:
						member_ref = WeakReference(target_ref.flags(), Reference::alloc<Library>());
						if (target_ref.data<Library>()->plugin) {
							member_ref.data<Library>()->plugin = new Plugin(target_ref.data<Library>()->plugin->getPath());
						}
						break;
					case Class::libobject:
						member_ref.clone(target_ref);
						memory_map.emplace(target_ref.data(), member_ref.data());
						continue;
					}

					memory_map.emplace(target_ref.data(), member_ref.data());
					member_ref.data<Object>()->construct(*target_ref.data<Object>(), memory_map);
					break;

				default:
					member_ref.clone(target_ref);
					memory_map.emplace(target_ref.data(), member_ref.data());
					break;
				}
			}
			else {
				member_ref = WeakReference(target_ref.flags(), i->second);
			}
		}
	}
}

ReferenceManager *Object::referenceManager() {
	return m_referenceManager;
}

void Object::mark() {
	if (!markedBit()) {
		Data::mark();
		if (data) {
			for (const auto &member : metadata->members()) {
				data[member.second->offset].data()->mark();
			}
		}
	}
}

void Object::invalidateReferenceManager() {
	delete m_referenceManager;
	m_referenceManager = nullptr;
}

Package::Package(PackageData *package) :
	Data(fmt_package),
	data(package) {

}

Function::Function() : Data(fmt_function) {

}

Function::Handler::Handler(PackageData *package, int module, int offset) :
	module(module),
	offset(offset),
	generator(false),
	package(package),
	capture(nullptr) {}

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
