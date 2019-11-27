#ifndef CURSOR_DEBUGGER_H
#define CURSOR_DEBUGGER_H

#include "ast/node.h"
#include "ast/module.h"

#include <string>

namespace mint {

class Cursor;

class MINT_EXPORT CursorDebugger {
public:
	CursorDebugger(Cursor *cursor);

	Node::Command command() const;
	Cursor *cursor() const;

	std::string moduleName() const;
	Module::Id moduleId() const;
	size_t lineNumber() const;
	size_t callDepth() const;

private:
	Cursor *m_cursor;
};

}

#endif // CURSOR_DEBUGGER_H
