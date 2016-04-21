#include "buildtool.h"
#include "AbstractSyntaxTree/modul.h"

using namespace std;

BuildContext::BuildContext(DataStream *stream, ModulContext node) :
	lexer(stream), data(node),
	m_defRecCpt(0) {
	m_currentModul = data.modul.second;
}

void BuildContext::startDef() {
	m_currentModul = data.defs.second;
	m_defRecCpt++;
}

void BuildContext::stopDef() {
	if ((--m_defRecCpt) == 0) {
		m_currentModul = data.modul.second;
	}
}

void BuildContext::startJumpForward() {

	m_jumpForward.push(m_currentModul->nextInstructionOffset());
	pushInstruction(0);
}

void BuildContext::shiftJumpForward() {

	auto firstLabel = m_jumpForward.top();
	m_jumpForward.pop();

	auto secondLabel = m_jumpForward.top();
	m_jumpForward.pop();

	m_jumpForward.push(firstLabel);
	m_jumpForward.push(secondLabel);
}

void BuildContext::resolveJumpForward() {

	Instruction instruction;
	instruction.parameter = m_currentModul->nextInstructionOffset();
	m_currentModul->replaceInstruction(m_jumpForward.top(), instruction);
	m_jumpForward.pop();
}

void BuildContext::startJumpBackward() {

	m_jumpBackward.push(m_currentModul->nextInstructionOffset());
}

void BuildContext::shiftJumpBackward() {

	auto firstLabel = m_jumpBackward.top();
	m_jumpBackward.pop();

	auto secondLabel = m_jumpBackward.top();
	m_jumpBackward.pop();

	m_jumpBackward.push(firstLabel);
	m_jumpBackward.push(secondLabel);
}

void BuildContext::resolveJumpBackward() {

	pushInstruction(m_jumpBackward.top());
	m_jumpBackward.pop();
}

void BuildContext::pushInstruction(Instruction::Command command) {

	Instruction instruction;
	instruction.command = command;
	m_currentModul->pushInstruction(instruction);
}

void BuildContext::pushInstruction(int parameter) {

	Instruction instruction;
	instruction.parameter = parameter;
	m_currentModul->pushInstruction(instruction);
}

void BuildContext::pushInstruction(const char *symbol) {

	Instruction instruction;
	instruction.symbol = m_currentModul->makeSymbol(symbol);
	m_currentModul->pushInstruction(instruction);
}

void BuildContext::pushInstruction(Data *constant) {

	Instruction instruction;
	instruction.constant = m_currentModul->makeConstant(constant);
	m_currentModul->pushInstruction(instruction);
}
