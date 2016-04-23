#ifndef BUILD_TOOL_H
#define BUILD_TOOL_H

#include "Compiler/lexer.h"
#include "AbstractSyntaxTree/abstractsyntaxtree.h"

#include <string>

class BuildContext {
public:
	BuildContext(DataStream *stream, Modul::Context data);

	Lexer lexer;
	Modul::Context data;

	void startJumpForward();
	void shiftJumpForward();
	void resolveJumpForward();

	void startJumpBackward();
	void shiftJumpBackward();
	void resolveJumpBackward();

	void pushInstruction(Instruction::Command command);
	void pushInstruction(int parameter);
	void pushInstruction(const char *symbol);
	void pushInstruction(Data *constant);

	void setModifiers(Reference::Flags flags);
	Reference::Flags getModifiers() const;

private:
	size_t m_defRecCpt;

	std::stack<size_t> m_jumpForward;
	std::stack<size_t> m_jumpBackward;

	Reference::Flags m_modifiers;
};

#endif // BUILD_TOOL_H
