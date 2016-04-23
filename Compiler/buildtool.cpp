#include "buildtool.h"
#include "AbstractSyntaxTree/modul.h"

using namespace std;

BuildContext::BuildContext(DataStream *stream, Modul::Context node) :
	lexer(stream), data(node),
	m_defRecCpt(0) {}

void BuildContext::startJumpForward() {

	m_jumpForward.push(data.modul->nextInstructionOffset());
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
	instruction.parameter = data.modul->nextInstructionOffset();
	data.modul->replaceInstruction(m_jumpForward.top(), instruction);
	m_jumpForward.pop();
}

void BuildContext::startJumpBackward() {

	m_jumpBackward.push(data.modul->nextInstructionOffset());
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
	data.modul->pushInstruction(instruction);
}

void BuildContext::pushInstruction(int parameter) {

	Instruction instruction;
	instruction.parameter = parameter;
	data.modul->pushInstruction(instruction);
}

void BuildContext::pushInstruction(const char *symbol) {

	Instruction instruction;
	instruction.symbol = data.modul->makeSymbol(symbol);
	data.modul->pushInstruction(instruction);
}

void BuildContext::pushInstruction(Data *constant) {

	Instruction instruction;
	instruction.constant = data.modul->makeConstant(constant);
	data.modul->pushInstruction(instruction);
}

void BuildContext::setModifiers(Reference::Flags flags) {
	m_modifiers = flags;
}

Reference::Flags BuildContext::getModifiers() const {
	return m_modifiers;
}
