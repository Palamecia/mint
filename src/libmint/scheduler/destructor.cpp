#include "scheduler/destructor.h"
#include "scheduler/scheduler.h"
#include "ast/abstractsyntaxtree.h"
#include "memory/operatortool.h"

using namespace mint;

Destructor::Destructor(Object *object) :
	Process(AbstractSyntaxTree::instance().createCursor()),
	m_object(object) {

	Class *metadata = m_object->metadata;

	if (Reference *data = m_object->data) {

		auto member = metadata->members().find("delete");
		if (member != metadata->members().end()) {
			cursor()->stack().push_back(SharedReference::unique(new Reference(Reference::standard, m_object)));
			cursor()->waitingCalls().push(data + member->second->offset);
			cursor()->waitingCalls().top().setMetadata(member->second->owner);
			call_member_operator(cursor(), 0);
		}
	}
}

Destructor::~Destructor() {
	delete m_object;
}

bool mint::is_destructor(Process *process) {
	return dynamic_cast<Destructor *>(process);
}
