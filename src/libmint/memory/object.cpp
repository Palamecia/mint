#include "memory/object.h"
#include "memory/class.h"
#include "memory/operatortool.h"
#include "scheduler/scheduler.h"
#include "scheduler/processor.h"
#include "ast/abstractsyntaxtree.h"
#include "ast/cursor.h"

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
	data = new Reference [metadata->size()];
	for (auto member : metadata->members()) {
		data[member.second->offset].clone(member.second->value);
	}
}

void Object::construct(const Object &other) {

	if (other.data) {
		m_referenceManager = new ReferenceManager;
		data = new Reference [metadata->size()];
		for (auto member : metadata->members()) {
			data[member.second->offset].clone(other.data[member.second->offset]);
		}
	}
}

ReferenceManager *Object::referenceManager() {
	return m_referenceManager;
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
	package(package),
	capture(nullptr) {}
