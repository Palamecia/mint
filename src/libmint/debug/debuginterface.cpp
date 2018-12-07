#include "debug/debuginterface.h"
#include "debug/cursordebugger.h"
#include "scheduler/scheduler.h"
#include "ast/cursor.h"

using namespace std;
using namespace mint;

DebugInterface::DebugInterface() :
	m_running(true),
	m_state(debugger_run) {

}

DebugInterface::~DebugInterface() {

}

void DebugInterface::declareThread(int id) {

	unique_lock<recursive_mutex> lock(m_mutex);

	ThreadContext *context = new ThreadContext;
	context->lineNumber = 0;
	context->callDepth = 0;
	m_threads.emplace(id, context);
}

void DebugInterface::removeThread(int id) {

	unique_lock<recursive_mutex> lock(m_mutex);

	auto it = m_threads.find(id);

	if (it == m_threads.end()) {
		delete it->second;
		m_threads.erase(it);
	}
}

bool DebugInterface::debug(Cursor *cursor) {

	if (m_running == false) {
		return false;
	}

	unique_lock<recursive_mutex> lock(m_mutex);

	CursorDebugger cursorDebugger(cursor);

	auto it = m_threads.find(Scheduler::instance()->currentProcess()->getThreadId());

	if (it == m_threads.end()) {
		return false;
	}

	ThreadContext *context = it->second;
	size_t lineNumber = cursorDebugger.lineNumber();
	size_t callDepth = cursorDebugger.callDepth();

	if (context->lineNumber != lineNumber || context->callDepth != callDepth) {

		auto module = m_breackpoints.find(cursorDebugger.moduleName());
		if (module != m_breackpoints.end()) {
			auto line = module->second.find(lineNumber);
			if (line != module->second.end()) {
				m_state = debugger_pause;
			}
		}
	}

	switch (m_state) {
	case debugger_run:
	case debugger_pause:
		if (context->lineNumber != lineNumber || context->callDepth != callDepth) {
			context->lineNumber = lineNumber;
			context->callDepth = callDepth;
		}
		break;

	case debugger_next:
		if (context->lineNumber != lineNumber && context->callDepth >= callDepth) {
			context->lineNumber = lineNumber;
			context->callDepth = callDepth;
			m_state = debugger_pause;
		}
		break;

	case debugger_enter:
		if (context->lineNumber != lineNumber || context->callDepth < callDepth) {
			context->lineNumber = lineNumber;
			context->callDepth = callDepth;
			m_state = debugger_pause;
		}
		break;

	case debugger_return:
		if (context->lineNumber != lineNumber && context->callDepth > callDepth) {
			context->lineNumber = lineNumber;
			context->callDepth = callDepth;
			m_state = debugger_pause;
		}
		break;
	}

	while (m_state == debugger_pause) {
		if (!check(&cursorDebugger)) {
			m_running = false;
			return false;
		}
	}

	return true;
}

void DebugInterface::doRun() {
	m_state = debugger_run;
}

void DebugInterface::doPause() {
	m_state = debugger_pause;
}

void DebugInterface::doNext() {
	m_state = debugger_next;
}

void DebugInterface::doEnter() {
	m_state = debugger_enter;
}

void DebugInterface::doReturn() {
	m_state = debugger_return;
}

void DebugInterface::createBreackpoint(const string &module, size_t line) {
	m_breackpoints[module].insert(line);
}

void DebugInterface::removeBreackpoint(const string &module, size_t line) {

	auto i = m_breackpoints.find(module);

	if (i != m_breackpoints.end()) {
		i->second.erase(line);
		if (i->second.empty()) {
			m_breackpoints.erase(i);
		}
	}
}
