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

void BuildContext::beginLoop() {

	Loop ctx;

	ctx.backward = &m_jumpBackward.top();
	ctx.forward = &m_jumpForward.top();

	m_loops.push(ctx);
}

void BuildContext::endLoop() {
	m_loops.pop();
}

bool BuildContext::isInLoop() const {
	return !m_loops.empty();
}

void BuildContext::startJumpForward() {

	m_jumpForward.push({data.module->nextNodeOffset()});
	pushNode(0);
}

void BuildContext::loopJumpForward() {
	m_loops.top().forward->push_back(data.module->nextNodeOffset());
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

void BuildContext::loopJumpBackward() {
	pushNode(*m_loops.top().backward);
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
		DEBUG_STACK("CAPTURE_ALL");
		pushNode(Node::capture_all);
	}
	else for (const string &symbol : def->capture) {
		DEBUG_STACK("CAPTURE %s", symbol.c_str());
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

void BuildContext::startClassDescription(const string &name) {
	m_classParent.clear();
	m_classDescription.push(new ClassDescription(new Class(currentPackage(), name)));
}

void BuildContext::appendSymbolToClassParent(const string &symbol) {
	m_classParent.push_back(symbol);
}

void BuildContext::saveClassParent() {
	DEBUG_STACK("INHERITE %s", m_classParent.toString().c_str());
	m_classDescription.top()->addParent(m_classParent);
	m_classParent.clear();
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

void BuildContext::startEnumDescription(const string &name) {
	startClassDescription(name);
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
	m_calls.push(0);
}

void BuildContext::addToCall() {
	m_calls.top()++;
}

void BuildContext::resolveCall() {
	pushNode(m_calls.top());
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
