#include "scheduler/destructor.h"
#include "scheduler/scheduler.h"
#include "scheduler/processor.h"
#include "ast/abstractsyntaxtree.h"
#include "memory/operatortool.h"

using namespace mint;
using namespace std;

Destructor::Destructor(Object *object, Process *process) :
	Process(AbstractSyntaxTree::instance().createCursor(process ? process->cursor() : nullptr)),
	m_object(object) {
	if (process) {
		setThreadId(process->getThreadId());
	}
}

Destructor::~Destructor() {

}

void Destructor::setup() {

	Class *metadata = m_object->metadata;

	lock_processor();

	if (Reference *data = m_object->data) {

		auto member = metadata->members().find("delete");
		if (member != metadata->members().end()) {
			SharedReference destructor = SharedReference::unsafe(data + member->second->offset);
			if (destructor->data()->format == Data::fmt_function) {
				cursor()->stack().emplace_back(SharedReference::unique(new StrongReference(Reference::standard, m_object)));
				cursor()->waitingCalls().emplace(move(destructor));
				cursor()->waitingCalls().top().setMetadata(member->second->owner);
				call_member_operator(cursor(), 0);
			}
		}
	}

	unlock_processor();
}

void Destructor::cleanup() {
	lock_processor();
	delete m_object;
	unlock_processor();
}

bool mint::is_destructor(Process *process) {
	return dynamic_cast<Destructor *>(process);
}
