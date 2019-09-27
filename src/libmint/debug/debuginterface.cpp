#include "debug/debuginterface.h"
#include "debug/cursordebugger.h"
#include "scheduler/scheduler.h"
#include "ast/cursor.h"

using namespace std;
using namespace mint;

DebugInterface::DebugInterface() :
	m_running(true) {

}

DebugInterface::~DebugInterface() {

}

void DebugInterface::declareThread(int id) {

	unique_lock<recursive_mutex> lock(m_mutex);

	ThreadContext *context = new ThreadContext;
	context->lineNumber = 0;
	context->callDepth = 0;
	context->state = debugger_run;
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
	ThreadContext *context = getThreadContext();

	if (context == nullptr) {
		return false;
	}

	size_t lineNumber = cursorDebugger.lineNumber();
	size_t callDepth = cursorDebugger.callDepth();

	if (context->lineNumber != lineNumber || context->callDepth != callDepth) {

		auto module = m_breackpoints.find(cursorDebugger.moduleName());
		if (module != m_breackpoints.end()) {
			auto line = module->second.find(lineNumber);
			if (line != module->second.end()) {
				context->state = debugger_pause;
			}
		}
	}

	switch (context->state) {
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
			context->state = debugger_pause;
		}
		break;

	case debugger_enter:
		if (context->lineNumber != lineNumber || context->callDepth < callDepth) {
			context->lineNumber = lineNumber;
			context->callDepth = callDepth;
			context->state = debugger_pause;
		}
		break;

	case debugger_return:
		if (context->lineNumber != lineNumber && context->callDepth > callDepth) {
			context->lineNumber = lineNumber;
			context->callDepth = callDepth;
			context->state = debugger_pause;
		}
		break;
	}

	while (context->state == debugger_pause) {
		if (!check(&cursorDebugger)) {
			m_running = false;
			return false;
		}
	}

	return true;
}

void DebugInterface::doRun() {
	if (ThreadContext *context = getThreadContext()) {
		context->state = debugger_run;
	}
}

void DebugInterface::doPause() {
	if (ThreadContext *context = getThreadContext()) {
		context->state = debugger_pause;
	}
}

void DebugInterface::doNext() {
	if (ThreadContext *context = getThreadContext()) {
		context->state = debugger_next;
	}
}

void DebugInterface::doEnter() {
	if (ThreadContext *context = getThreadContext()) {
		context->state = debugger_enter;
	}
}

void DebugInterface::doReturn() {
	if (ThreadContext *context = getThreadContext()) {
		context->state = debugger_return;
	}
}

void DebugInterface::createBreackpoint(const LineInfo &info) {
	m_breackpoints[info.moduleName()].insert(info.lineNumber());
}

void DebugInterface::removeBreackpoint(const LineInfo &info) {

	auto i = m_breackpoints.find(info.moduleName());

	if (i != m_breackpoints.end()) {
		i->second.erase(info.lineNumber());
		if (i->second.empty()) {
			m_breackpoints.erase(i);
		}
	}
}

LineInfoList DebugInterface::listBreakpoints() const {

	LineInfoList breackpoints;

	for (const auto &module : m_breackpoints) {
		for (const size_t &line : module.second) {
			breackpoints.push_back(LineInfo(module.first, line));
		}
	}

	return breackpoints;
}

DebugInterface::ThreadContext *DebugInterface::getThreadContext() const {

	auto it = m_threads.find(Scheduler::instance()->currentProcess()->getThreadId());

	if (it != m_threads.end()) {
		return it->second;
	}

	return nullptr;
}
