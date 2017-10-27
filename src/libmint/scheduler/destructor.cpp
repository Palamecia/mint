#include "scheduler/destructor.h"
#include "scheduler/scheduler.h"
#include "memory/operatortool.h"

Destructor::Destructor(Object *object) :
	Process(Scheduler::instance()->ast()->createCursor()),
	m_object(object) {

	Class * metadata = m_object->metadata;

	if (Reference *data = m_object->data) {

		auto member = metadata->members().find("delete");
		if (member != metadata->members().end()) {
			cursor()->stack().push_back(SharedReference::unique(new Reference(Reference::standard, m_object)));
			cursor()->waitingCalls().push(&data[member->second->offset]);
			call_member_operator(cursor(), 0);
		}
	}
}

Destructor::~Destructor() {
	delete m_object;
}
