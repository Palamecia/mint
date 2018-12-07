#include "scheduler/destructor.h"
#include "scheduler/scheduler.h"
#include "ast/abstractsyntaxtree.h"
#include "memory/operatortool.h"

using namespace mint;

Destructor::Destructor(Object *object, Process *process) :
	Process(AbstractSyntaxTree::instance().createCursor(process->cursor())),
	m_object(object) {

}

Destructor::~Destructor() {
	delete m_object;
}

void Destructor::setup() {

	Class *metadata = m_object->metadata;

	if (Reference *data = m_object->data) {

		auto member = metadata->members().find("delete");
		if (member != metadata->members().end()) {
			Reference *destructor = data + member->second->offset;
			if (destructor->data()->format == Data::fmt_function) {
				cursor()->stack().push_back(SharedReference::unique(new Reference(Reference::standard, m_object)));
				cursor()->waitingCalls().push(destructor);
				cursor()->waitingCalls().top().setMetadata(member->second->owner);
				call_member_operator(cursor(), 0);
			}
		}
	}
}

bool mint::is_destructor(Process *process) {
	return dynamic_cast<Destructor *>(process);
}
