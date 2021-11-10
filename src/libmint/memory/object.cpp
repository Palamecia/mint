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

		size_t offset = metadata->size();

		while (offset) {
			data[--offset].~WeakReference();
		}

		free(data);
	}
}

void Object::construct() {

	data = static_cast<WeakReference *>(malloc(sizeof(WeakReference) * metadata->size()));

	for (auto &member : metadata->members()) {
		new (data + member.second->offset) WeakReference(WeakReference::clone(member.second->value));
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

			const Reference &target_ref = other.data[member.second->offset];
			Reference *member_ref = data + member.second->offset;
			auto i = memory_map.find(target_ref.data());

			if (i == memory_map.end()) {
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
				new (member_ref) WeakReference(target_ref.flags(), i->second);
			}
		}
	}
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

Package::Package(PackageData *package) :
	Data(fmt_package),
	data(package) {

}

Function::Function() : Data(fmt_function) {

}

Function::Function(const Function &other) : Data(fmt_function),
	mapping(other.mapping) {

}

Function::Signature::Signature(Module::Handle *handle) :
	handle(handle),
	capture(nullptr) {

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
