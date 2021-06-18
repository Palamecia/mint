#include "scheduler/destructor.h"
#include "scheduler/scheduler.h"
#include "scheduler/processor.h"
#include "ast/abstractsyntaxtree.h"
#include "memory/operatortool.h"
#include "system/assert.h"

using namespace mint;
using namespace std;

Destructor::Destructor(Object *object, SharedReference &&member, Class *owner, Process *process) :
	Process(AbstractSyntaxTree::instance().createCursor(process ? process->cursor() : nullptr)),
	m_owner(owner),
	m_object(object),
	m_member(move(member)) {
	if (process) {
		setThreadId(process->getThreadId());
	}
}

Destructor::~Destructor() {

}

void Destructor::setup() {
	lock_processor();
	assert(m_member->data()->format == Data::fmt_function);
	cursor()->stack().emplace_back(SharedReference::strong(Reference::standard, m_object));
	cursor()->waitingCalls().emplace(move(m_member));
	cursor()->waitingCalls().top().setMetadata(m_owner);
	call_member_operator(cursor(), 0);
	unlock_processor();
}

void Destructor::cleanup() {
	lock_processor();
	Reference::destroy(m_object);
	unlock_processor();
}

bool mint::is_destructor(Process *process) {
	return dynamic_cast<Destructor *>(process);
}
