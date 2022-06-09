#include "debug/cursordebugger.h"
#include "ast/abstractsyntaxtree.h"
#include "ast/cursor.h"

using namespace std;
using namespace mint;

CursorDebugger::CursorDebugger(Cursor *cursor) :
	m_cursor(cursor) {

}

Node::Command CursorDebugger::command() const {
	return m_cursor->m_currentContext->module->at(m_cursor->m_currentContext->iptr).command;
}

Cursor *CursorDebugger::cursor() const {
	return m_cursor;
}

string CursorDebugger::moduleName() const {
	return m_cursor->ast()->getModuleName(m_cursor->m_currentContext->module);
}

Module::Id CursorDebugger::moduleId() const {
	return m_cursor->ast()->getModuleId(m_cursor->m_currentContext->module);
}

size_t CursorDebugger::lineNumber() const {

	if (DebugInfos *infos = m_cursor->ast()->getDebugInfos(moduleId())) {
		return infos->lineNumber(m_cursor->m_currentContext->iptr);
	}

	return 0;
}

size_t CursorDebugger::callDepth() const {

	size_t depth = 0;

	for (Cursor *cursor = m_cursor; cursor; cursor = cursor->m_parent) {

		depth += cursor->m_callStack.size();

		if (cursor->parent()) {
			depth += 1;
		}
	}

	return depth;
}
