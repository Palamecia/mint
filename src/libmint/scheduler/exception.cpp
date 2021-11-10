#include "scheduler/exception.h"
#include "scheduler/processor.h"
#include "ast/abstractsyntaxtree.h"
#include "memory/memorytool.h"
#include "memory/operatortool.h"
#include "system/error.h"

using namespace mint;
using namespace std;

Exception::Exception(Reference &&reference, Process *process) :
	Process(AbstractSyntaxTree::instance().createCursor(process->cursor())),
	m_reference(move(reference)),
	m_handled(false) {
	setThreadId(process->getThreadId());

}

Exception::~Exception() {

}

void Exception::setup() {

	lock_processor();

	if (m_reference.data()->format == Data::fmt_object) {

		Object *object = m_reference.data<Object>();

		Class *metadata = object->metadata;

		if (Reference *data = object->data) {

			auto member = metadata->members().find(Symbol::Show);
			if (member != metadata->members().end()) {
				WeakReference handler = WeakReference::share(data[member->second->offset]);
				if (handler.data()->format == Data::fmt_function) {
					call_error_callbacks();
					cursor()->stack().emplace_back(move(m_reference));
					cursor()->waitingCalls().emplace(move(handler));
					cursor()->waitingCalls().top().setMetadata(member->second->owner);
					call_member_operator(cursor(), 0);
					m_handled = true;
				}
			}
		}
	}

	unlock_processor();
}

void Exception::cleanup() {

	if (m_handled) {
		call_exit_callback();
	}
	else {
		error("exception : %s", to_string(m_reference).c_str());
	}
}

bool mint::is_exception(Process *process) {
	return dynamic_cast<Exception *>(process);
}
