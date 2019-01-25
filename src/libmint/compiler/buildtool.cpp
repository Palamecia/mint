#include "compiler/buildtool.h"
#include "compiler/compiler.h"
#include "ast/module.h"
#include "memory/object.h"
#include "memory/class.h"
#include "system/assert.h"
#include "system/error.h"

using namespace std;
using namespace mint;

BuildContext::BuildContext(DataStream *stream, Module::Infos node) :
	lexer(stream), data(node) {
	stream->setLineEndCallback(bind(&DebugInfos::newLine, node.debugInfos, node.module, placeholders::_1));
}

void BuildContext::openBloc(BlocType type) {

	if (type == switch_case) {
		CaseTable table;
		pushNode(Node::jump);
		table.origin = data.module->nextNodeOffset();
		table.default_label = nullptr;
		pushNode(0);
		m_jumpForward.push({});
		caseTables().push_back(table);
	}

	Bloc bloc;

	bloc.type = type;
	bloc.backward = &m_jumpBackward.top();
	bloc.forward = &m_jumpForward.top();

	blocs().push_back(bloc);
}

void BuildContext::closeBloc() {
	if (isInSwitch()) {
		delete caseTables().back().default_label;
		caseTables().pop_back();
		resolveJumpForward();
	}
	blocs().pop_back();
}

bool BuildContext::isInLoop() const {
	if (!blocs().empty()) {
		return blocs().back().type != switch_case;
	}
	return false;
}

bool BuildContext::isInSwitch() const {
	if (!blocs().empty()) {
		return blocs().back().type == switch_case;
	}
	return false;
}

void BuildContext::prepareReturn() {

	for (const Bloc &loop : blocs()) {
		switch (loop.type) {
		case range_loop:
			// unload range
			pushNode(Node::unload_reference);
			// unload target
			pushNode(Node::unload_reference);
			break;

		default:
			break;
		}
	}
}

void BuildContext::setCaseLabel(const string &token) {
	if (!caseTables().back().labels.emplace(token, data.module->nextNodeOffset()).second) {
		parse_error("duplicate case value");
	}
}

void BuildContext::setDefaultLabel() {
	if (caseTables().back().default_label) {
		parse_error("multiple default labels in one switch");
	}
	caseTables().back().default_label = new size_t(data.module->nextNodeOffset());
}

void BuildContext::buildCaseTable() {

	Node node;
	const CaseTable &case_table = caseTables().back();

	node.parameter = data.module->nextNodeOffset();
	data.module->replaceNode(case_table.origin, node);

	for (const auto &label : case_table.labels) {
		DEBUG_STACK(this, "RELOAD");
		pushNode(Node::reload_reference);
		DEBUG_STACK(this, "PUSH %s", label.first.c_str());
		pushNode(Node::load_constant);
		if (Data *data = Compiler::makeData(label.first.c_str())) {
			pushNode(data);
		}
		else {
			parse_error(string("token '" + label.first + "' is not a valid constant").c_str());
		}
		DEBUG_STACK(this, "EQ");
		pushNode(Node::eq_op);
		DEBUG_STACK(this, "CASE JMP %s", label.first.c_str());
		pushNode(Node::case_jump);
		pushNode(label.second);
	}

	if (case_table.default_label) {
		DEBUG_STACK(this, "PUSH true");
		pushNode(Node::load_constant);
		pushNode(Compiler::makeData("true"));
		DEBUG_STACK(this, "CASE JMP DEFAULT");
		pushNode(Node::case_jump);
		pushNode(*case_table.default_label);
	}
	else {
		DEBUG_STACK(this, "POP");
		pushNode(Node::unload_reference);
	}
}

void BuildContext::startJumpForward() {

	m_jumpForward.push({data.module->nextNodeOffset()});
	pushNode(0);
}

void BuildContext::blocJumpForward() {
	blocs().back().forward->push_back(data.module->nextNodeOffset());
	pushNode(0);
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

	Node node;
	node.parameter = data.module->nextNodeOffset();
	for (size_t offset : m_jumpForward.top()) {
		data.module->replaceNode(offset, node);
	}
	m_jumpForward.pop();
}

void BuildContext::startJumpBackward() {

	m_jumpBackward.push(data.module->nextNodeOffset());
}

void BuildContext::blocJumpBackward() {
	pushNode(*blocs().back().backward);
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

	pushNode(m_jumpBackward.top());
	m_jumpBackward.pop();
}

void BuildContext::startDefinition() {
	Definition *def = new Definition;
	def->function = data.module->makeConstant(Reference::alloc<Function>());
	def->beginOffset = data.module->nextNodeOffset();
	def->variadic = false;
	def->capture_all = false;
	m_definitions.push(def);
}

bool BuildContext::addParameter(const string &symbol) {

	Definition *def = m_definitions.top();
	if (def->variadic) {
		parse_error("unexpected parameter after '...' token");
		return false;
	}

	def->parameters.push(symbol);
	return true;
}

bool BuildContext::setVariadic() {

	Definition *def = m_definitions.top();
	if (def->variadic) {
		parse_error("unexpected parameter after '...' token");
		return false;
	}

	def->parameters.push("va_args");
	def->variadic = true;
	return true;
}

bool BuildContext::saveParameters() {

	Definition *def = m_definitions.top();
	if (def->variadic && def->parameters.empty()) {
		parse_error("expected parameter before '...' token");
		return false;
	}

	int signature = def->variadic ? -(def->parameters.size() - 1) : def->parameters.size();
	Function::Handler handler(currentPackage(), data.id, def->beginOffset);
	if (!def->capture.empty() || def->capture_all) {
		handler.capture.reset(new Function::Handler::Capture);
	}

	def->function->data<Function>()->mapping.emplace(signature, handler);
	while (!def->parameters.empty()) {
		pushNode(Node::init_param);
		pushNode(def->parameters.top().c_str());
		def->parameters.pop();
	}

	return true;
}

bool BuildContext::addDefinitionSignature() {

	Definition *def = m_definitions.top();
	if (def->variadic) {
		parse_error("unexpected parameter after '...' token");
		return false;
	}

	int signature = def->parameters.size();
	Function::Handler handler(currentPackage(), data.id, def->beginOffset);
	if (!def->capture.empty() || def->capture_all) {
		handler.capture.reset(new Function::Handler::Capture);
	}

	def->function->data<Function>()->mapping.emplace(signature, handler);
	def->beginOffset = data.module->nextNodeOffset();
	return true;
}

void BuildContext::saveDefinition() {

	Node node;
	Definition *def = m_definitions.top();

	node.constant = def->function;
	pushNode(Node::load_constant);
	data.module->pushNode(node);
	if (def->capture_all) {
		DEBUG_STACK(this, "CAPTURE_ALL");
		pushNode(Node::capture_all);
	}
	else for (const string &symbol : def->capture) {
		DEBUG_STACK(this, "CAPTURE %s", symbol.c_str());
		pushNode(Node::capture_symbol);
		pushNode(symbol.c_str());
	}
	m_definitions.pop();
	delete def;
}

Data *BuildContext::retrieveDefinition() {

	Definition *def = m_definitions.top();
	Data *data = def->function->data();

	m_definitions.pop();
	delete def;

	return data;
}

void BuildContext::openPackage(const string &name) {
	PackageData *package = currentPackage()->getPackage(name);
	pushNode(Node::open_package);
	pushNode(Reference::alloc<Package>(package));
	m_packages.push(package);
}

void BuildContext::closePackage() {
	assert(!m_packages.empty());
	pushNode(Node::close_package);
	m_packages.pop();
}

PackageData *BuildContext::currentPackage() const {
	if (m_packages.empty()) {
		return &GlobalData::instance();
	}

	return m_packages.top();
}

void BuildContext::startClassDescription(const string &name, Reference::Flags flags) {
	m_classBase.clear();
	m_classDescription.push(new ClassDescription(currentPackage(), flags, name));
}

void BuildContext::appendSymbolToBaseClassPath(const string &symbol) {
	m_classBase.push_back(symbol);
}

void BuildContext::saveBaseClassPath() {
	DEBUG_STACK(this, "INHERITE %s", m_classBase.toString().c_str());
	m_classDescription.top()->addBase(m_classBase);
	m_classBase.clear();
}

bool BuildContext::createMember(Reference::Flags flags, const string &name, Data *value) {

	if (value == nullptr) {
		string error_message = name + ": member value is not a valid constant";
		parse_error(error_message.c_str());
		return false;
	}

	if (!m_classDescription.top()->createMember(name, SharedReference::unique(new Reference(flags, value)))) {
		string error_message = name + ": member was already defined";
		parse_error(error_message.c_str());
		return false;
	}

	return true;
}

bool BuildContext::updateMember(Reference::Flags flags, const string &name, Data *value) {

	if (!m_classDescription.top()->updateMember(name, SharedReference::unique(new Reference(flags, value)))) {
		string error_message = name + ": member was already defined";
		parse_error(error_message.c_str());
		return false;
	}

	return true;
}

void BuildContext::resolveClassDescription() {

	ClassDescription *desc = m_classDescription.top();
	m_classDescription.pop();

	if (m_classDescription.empty()) {
		pushNode(Node::register_class);
		pushNode(currentPackage()->createClass(desc));
	}
	else {
		m_classDescription.top()->addSubClass(desc);
	}
}

void BuildContext::startEnumDescription(const string &name, Reference::Flags flags) {
	startClassDescription(name, flags);
	m_nextEnumValue = 0;
}

void BuildContext::setCurrentEnumValue(int value) {
	m_nextEnumValue = value + 1;
}

int BuildContext::nextEnumValue() {
	return m_nextEnumValue++;
}

void BuildContext::resolveEnumDescription() {
	resolveClassDescription();
}

void BuildContext::startCall() {
	Call *call = new Call;
	call->argc = 0;
	m_calls.push(call);
}

void BuildContext::addToCall() {
	m_calls.top()->argc++;
}

void BuildContext::resolveCall() {
	pushNode(m_calls.top()->argc);
	delete m_calls.top();
	m_calls.pop();
}

void BuildContext::capture(const string &symbol) {
	Definition *def = m_definitions.top();
	def->capture.push_back(symbol);
}

void BuildContext::captureAll() {
	Definition *def = m_definitions.top();
	def->capture_all = true;
}

void BuildContext::pushNode(Node::Command command) {

	Node node;
	node.command = command;
	data.module->pushNode(node);
}

void BuildContext::pushNode(int parameter) {

	Node node;
	node.parameter = parameter;
	data.module->pushNode(node);
}

void BuildContext::pushNode(const char *symbol) {

	Node node;
	node.symbol = data.module->makeSymbol(symbol);
	data.module->pushNode(node);
}

void BuildContext::pushNode(Data *constant) {

	Node node;
	node.constant = data.module->makeConstant(constant);
	data.module->pushNode(node);
}

void BuildContext::setModifiers(Reference::Flags flags) {
	m_modifiers = flags;
}

Reference::Flags BuildContext::getModifiers() const {
	return m_modifiers;
}

void BuildContext::parse_error(const char *error_msg) {

	fflush(stdout);
	error("%s", lexer.formatError(error_msg).c_str());
}

list<BuildContext::Bloc> &BuildContext::blocs() {
	if (m_definitions.empty()) {
		return m_blocs;
	}
	return m_definitions.top()->blocs;
}

const list<BuildContext::Bloc> &BuildContext::blocs() const {
	if (m_definitions.empty()) {
		return m_blocs;
	}
	return m_definitions.top()->blocs;
}

list<BuildContext::CaseTable> &BuildContext::caseTables() {
	if (m_definitions.empty()) {
		return m_caseTables;
	}
	return m_definitions.top()->case_tables;
}

const list<BuildContext::CaseTable> &BuildContext::caseTables() const {
	if (m_definitions.empty()) {
		return m_caseTables;
	}
	return m_definitions.top()->case_tables;
}
