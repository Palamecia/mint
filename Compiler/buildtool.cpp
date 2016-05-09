#include "Compiler/buildtool.h"
#include "AbstractSyntaxTree/modul.h"
#include "Memory/object.h"
#include "Memory/class.h"

using namespace std;

BuildContext::BuildContext(DataStream *stream, Modul::Context node) :
	lexer(stream), data(node) {}

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

void BuildContext::startDefinition() {
	Definition *def = new Definition;
	def->function = data.modul->makeConstant(Reference::alloc<Function>());
	def->beginOffset = data.modul->nextInstructionOffset();
	m_definitions.push(def);
}

void BuildContext::addParameter(const string &symbol) {

	Definition *def = m_definitions.top();
	def->parameters.push(symbol);
}

void BuildContext::saveParameters() {

	Definition *def = m_definitions.top();
	((Function *)def->function->data())->mapping.insert({def->parameters.size(), {data.modulId, def->beginOffset}});

	while (!def->parameters.empty()) {
		pushInstruction(Instruction::init_param);
		pushInstruction(def->parameters.top().c_str());
		def->parameters.pop();
	}
}

void BuildContext::addDefinitionFormat() {

	Definition *def = m_definitions.top();
	((Function *)def->function->data())->mapping.insert({def->parameters.size(), {data.modulId, def->beginOffset}});
	def->beginOffset = data.modul->nextInstructionOffset();
}

void BuildContext::saveDefinition() {

	Instruction instruction;
	Definition *def = m_definitions.top();

	instruction.constant = def->function;
	pushInstruction(Instruction::load_constant);
	data.modul->pushInstruction(instruction);
	m_definitions.pop();
	delete def;
}

Data *BuildContext::retriveDefinition() {

	Definition *def = m_definitions.top();
	Data *data = def->function->data();

	m_definitions.pop();
	delete def;

	return data;
}

void BuildContext::startClassDescription(const string &name) {
	m_classDescription.push(new Class(name));
}

void BuildContext::classInheritance(const string &parent) {
	m_classDescription.top().addParent(parent);
}

void BuildContext::addMember(Reference::Flags flags, const string &name, Data *value) {
	m_classDescription.top().addMember(name, SharedReference::unique(new Reference(flags, value)));
}

void BuildContext::resolveClassDescription() {
	pushInstruction(GlobalData::instance().createClass(m_classDescription.top()));
	m_classDescription.pop();
}

void BuildContext::startCall() {
	m_calls.push(0);
}

void BuildContext::addToCall() {
	m_calls.top()++;
}

void BuildContext::resolveCall() {
	pushInstruction(m_calls.top());
	m_calls.pop();
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
