#include "memory/object.h"
#include "memory/class.h"
#include "memory/operatortool.h"
#include "scheduler/scheduler.h"
#include "scheduler/processor.h"
#include "ast/abstractsyntaxtree.h"
#include "ast/cursor.h"

#include <functional>

Null::Null()
{ format = fmt_null; }

None::None()
{ format = fmt_none; }

Number::Number()
{ format = fmt_number; }

Boolean::Boolean()
{ format = fmt_boolean; }

Object::Object(Class *type) : metadata(type), data(nullptr)
{ format = fmt_object; }

Object::~Object() {

	if (data) {

		auto destructor = metadata->members().find("delete");
		if (destructor != metadata->members().end()) {

			if (AbstractSyntaxTree *ast = Scheduler::instance()->ast()) {

				if (Cursor *cursor = ast->createCursor()) {
					cursor->stack().push_back(SharedReference::unique(new Reference(Reference::standard, this)));
					cursor->waitingCalls().push(&data[destructor->second->offset]);

					call_member_operator(cursor, 0);
					while (cursor->callInProgress()) {
						run_step(cursor);
					}
				}
			}
		}

		delete [] data;
	}
}

void Object::construct() {

	data = new Reference [metadata->size()];
	for (auto member : metadata->members()) {
		data[member.second->offset].clone(member.second->value);
	}
}

void Object::construct(const Object &other) {

	data = new Reference [metadata->size()];
	for (auto member : metadata->members()) {
		data[member.second->offset].clone(other.data[member.second->offset]);
	}
}

Function::Function()
{ format = fmt_function; }

Function::Handler::Handler(int module, int offset) :
	module(module),
	offset(offset),
	capture(nullptr) {}
