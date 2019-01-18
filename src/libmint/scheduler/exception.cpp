#include "scheduler/exception.h"
#include "ast/abstractsyntaxtree.h"
#include "memory/memorytool.h"
#include "memory/operatortool.h"
#include "system/error.h"

using namespace mint;

Exception::Exception(SharedReference reference, Process *process) :
	Process(AbstractSyntaxTree::instance().createCursor(process->cursor())),
	m_reference(reference),
	m_handled(false) {
	setThreadId(process->getThreadId());
}

void Exception::setup() {

	if (m_reference->data()->format == Data::fmt_object) {

		Object *object = m_reference->data<Object>();

		Class *metadata = object->metadata;

		if (Reference *data = object->data) {

			auto member = metadata->members().find("show");
			if (member != metadata->members().end()) {
				Reference *handler = data + member->second->offset;
				if (handler->data()->format == Data::fmt_function) {
					call_error_callbacks();
					cursor()->stack().push_back(m_reference);
					cursor()->waitingCalls().push(handler);
					cursor()->waitingCalls().top().setMetadata(member->second->owner);
					call_member_operator(cursor(), 0);
					m_handled = true;
				}
			}
		}
	}

}

void Exception::cleanup() {

	if (m_handled) {
		call_exit_callback();
	}
	else {
		error("exception : %s", to_string(*m_reference).c_str());
	}
}

bool mint::is_exception(Process *process) {
	return dynamic_cast<Exception *>(process);
}
