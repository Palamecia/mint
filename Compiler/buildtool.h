#ifndef BUILD_TOOL_H
#define BUILD_TOOL_H

#include "Compiler/lexer.h"
#include "AbstractSyntaxTree/abstractsyntaxtree.h"

#include <string>

class BuildContext {
public:
	BuildContext(DataStream *stream, ModulContext data);

	Lexer lexer;
	ModulContext data;

	void startDef();
	void stopDef();

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

private:
	Modul *m_currentModul;
	size_t m_defRecCpt;

	std::stack<size_t> m_jumpForward;
	std::stack<size_t> m_jumpBackward;
};

#endif // BUILD_TOOL_H
