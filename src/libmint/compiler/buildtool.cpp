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

BuildContext::Branch::Branch(BuildContext *context) :
	m_context(context) {

}

void BuildContext::Branch::startJumpForward() {
	m_jumpForeward.push({m_tree.size()});
	m_labels.insert(m_tree.size());
	pushNode(0);
}

void BuildContext::Branch::resolveJumpForward() {

	for (size_t offset : m_jumpForeward.top()) {
		m_tree[offset].parameter = static_cast<int>(m_tree.size());
	}

	m_jumpForeward.pop();
}

void BuildContext::Branch::startJumpBackward() {

	m_jumpBackward.push(m_tree.size());
}

void BuildContext::Branch::resolveJumpBackward() {

	m_labels.insert(m_tree.size());
	pushNode(static_cast<int>(m_jumpBackward.top()));
	m_jumpBackward.pop();
}

void BuildContext::Branch::pushNode(Node::Command command) {

	Node node;
	node.command = command;
	m_tree.push_back(node);
}

void BuildContext::Branch::pushNode(int parameter) {

	Node node;
	node.parameter = parameter;
	m_tree.push_back(node);
}

void BuildContext::Branch::pushNode(const char *symbol) {

	Node node;
	node.symbol = m_context->data.module->makeSymbol(symbol);
	m_tree.push_back(node);
}

void BuildContext::Branch::pushNode(Data *constant) {

	Node node;
	node.constant = m_context->data.module->makeConstant(constant);
	m_tree.push_back(node);
}

void BuildContext::Branch::build() {

	assert(m_jumpBackward.empty());
	assert(m_jumpForeward.empty());

	size_t offset = m_context->data.module->nextNodeOffset();

	for (size_t label : m_labels) {
		m_tree[label].parameter += offset;
	}

	for (const Node &node : m_tree) {
		m_context->data.module->pushNode(node);
	}

	m_tree.clear();
	m_labels.clear();
}

BuildContext::CaseTable::CaseTable(BuildContext *context) :
	current_label(context) {

}

BuildContext::CaseTable::Label::Label(BuildContext *context) :
	condition(context) {

}

void BuildContext::openBloc(Bloc::Type type) {

	Bloc bloc;

	bloc.type = type;
	bloc.foreward = nullptr;
	bloc.backward = nullptr;
	bloc.case_table = nullptr;

	if (bloc.type == Bloc::switch_type) {
		bloc.case_table = new CaseTable(this);
		pushNode(Node::jump);
		bloc.case_table->origin = data.module->nextNodeOffset();
		bloc.case_table->default_label = nullptr;
		pushNode(0);
		m_jumpForeward.push({});
	}

	bloc.backward = &m_jumpBackward.top();
	bloc.foreward = &m_jumpForeward.top();

	blocs().push_back(bloc);
}

void BuildContext::closeBloc() {

	Bloc &bloc = blocs().back();

	if (CaseTable *case_table = bloc.case_table) {
		delete case_table->default_label;
		delete case_table;
		resolveJumpForward();
	}

	blocs().pop_back();
}

bool BuildContext::isInLoop() const {
	if (!blocs().empty()) {
		return blocs().back().type != Bloc::switch_type;
	}
	return false;
}

bool BuildContext::isInSwitch() const {
	if (!blocs().empty()) {
		return blocs().back().type == Bloc::switch_type;
	}
	return false;
}

void BuildContext::prepareReturn() {

	for (const Bloc &bloc : blocs()) {
		switch (bloc.type) {
		case Bloc::range_loop_type:
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

void BuildContext::addInclusiveRangeCaseLabel(const string &begin, const string &end) {

	if (CaseTable *case_table = blocs().back().case_table) {
		if (Data *beginData = Compiler::makeData(begin)) {
			if (Data *endData = Compiler::makeData(end)) {
				case_table->current_token = begin + ".." + end;
				case_table->current_label.offset = data.module->nextNodeOffset();
				case_table->current_label.condition.pushNode(Node::load_constant);
				case_table->current_label.condition.pushNode(beginData);
				case_table->current_label.condition.pushNode(Node::load_constant);
				case_table->current_label.condition.pushNode(endData);
				case_table->current_label.condition.pushNode(Node::inclusive_range_op);
				case_table->current_label.condition.startJumpBackward();
				case_table->current_label.condition.pushNode(Node::find_next);
				case_table->current_label.condition.pushNode(Node::find_check);
				case_table->current_label.condition.startJumpForward();
				case_table->current_label.condition.pushNode(Node::jump);
				case_table->current_label.condition.resolveJumpBackward();
				case_table->current_label.condition.resolveJumpForward();
			}
			else {
				parse_error(string("token '" + end + "' is not a valid number").c_str());
			}
		}
		else {
			parse_error(string("token '" + begin + "' is not a valid number").c_str());
		}
	}
}

void BuildContext::addExclusiveRangeCaseLabel(const string &begin, const string &end) {

	if (CaseTable *case_table = blocs().back().case_table) {
		if (Data *beginData = Compiler::makeData(begin)) {
			if (Data *endData = Compiler::makeData(end)) {
				case_table->current_token = begin + "..." + end;
				case_table->current_label.offset = data.module->nextNodeOffset();
				case_table->current_label.condition.pushNode(Node::load_constant);
				case_table->current_label.condition.pushNode(beginData);
				case_table->current_label.condition.pushNode(Node::load_constant);
				case_table->current_label.condition.pushNode(endData);
				case_table->current_label.condition.pushNode(Node::exclusive_range_op);
				case_table->current_label.condition.startJumpBackward();
				case_table->current_label.condition.pushNode(Node::find_next);
				case_table->current_label.condition.pushNode(Node::find_check);
				case_table->current_label.condition.startJumpForward();
				case_table->current_label.condition.pushNode(Node::jump);
				case_table->current_label.condition.resolveJumpBackward();
				case_table->current_label.condition.resolveJumpForward();
			}
			else {
				parse_error(string("token '" + end + "' is not a valid number").c_str());
			}
		}
		else {
			parse_error(string("token '" + begin + "' is not a valid number").c_str());
		}
	}
}

void BuildContext::addConstantCaseLabel(const string &token) {

	if (CaseTable *case_table = blocs().back().case_table) {
		if (Data *constant = Compiler::makeData(token)) {
			case_table->current_token = token;
			case_table->current_label.offset = data.module->nextNodeOffset();
			case_table->current_label.condition.pushNode(Node::load_constant);
			case_table->current_label.condition.pushNode(constant);
		}
		else {
			parse_error(string("token '" + token + "' is not a valid constant").c_str());
		}
	}
}

void BuildContext::addSymbolCaseLabel(const string &token) {

	if (CaseTable *case_table = blocs().back().case_table) {
		case_table->current_token = token;
		case_table->current_label.offset = data.module->nextNodeOffset();
		case_table->current_label.condition.pushNode(Node::load_symbol);
		case_table->current_label.condition.pushNode(token.c_str());
	}
}

void BuildContext::addMemberCaseLabel(const string &token) {

	if (CaseTable *case_table = blocs().back().case_table) {
		case_table->current_token += "." + token;
		case_table->current_label.offset = data.module->nextNodeOffset();
		case_table->current_label.condition.pushNode(Node::load_member);
		case_table->current_label.condition.pushNode(token.c_str());
	}
}

void BuildContext::resolveEqCaseLabel() {

	if (CaseTable *case_table = blocs().back().case_table) {
		case_table->current_label.condition.pushNode(Node::eq_op);
	}
}

void BuildContext::resolveIsCaseLabel() {

	if (CaseTable *case_table = blocs().back().case_table) {
		case_table->current_label.condition.pushNode(Node::is_op);
	}
}

void BuildContext::setCaseLabel() {
	if (CaseTable *case_table = blocs().back().case_table) {
		if (!case_table->labels.emplace(case_table->current_token, case_table->current_label).second) {
			parse_error("duplicate case value");
		}
		case_table->current_label.condition = Branch(this);
	}
}

void BuildContext::setDefaultLabel() {
	if (CaseTable *case_table = blocs().back().case_table) {
		if (case_table->default_label) {
			parse_error("multiple default labels in one switch");
		}
		case_table->default_label = new size_t(data.module->nextNodeOffset());
	}
}

void BuildContext::buildCaseTable() {

	Node node;

	if (CaseTable *case_table = blocs().back().case_table) {

		node.parameter = static_cast<int>(data.module->nextNodeOffset());
		data.module->replaceNode(case_table->origin, node);

		for (auto &label : case_table->labels) {
			DEBUG_STACK(this, "RELOAD");
			pushNode(Node::reload_reference);
			DEBUG_STACK(this, "CASE LOAD %s", label.first.c_str());
			label.second.condition.build();
			DEBUG_STACK(this, "CASE JMP %s", label.first.c_str());
			pushNode(Node::case_jump);
			pushNode(static_cast<int>(label.second.offset));
		}

		if (case_table->default_label) {
			DEBUG_STACK(this, "PUSH true");
			pushNode(Node::load_constant);
			pushNode(Compiler::makeData("true"));
			DEBUG_STACK(this, "CASE JMP DEFAULT");
			pushNode(Node::case_jump);
			pushNode(static_cast<int>(*case_table->default_label));
		}
		else {
			DEBUG_STACK(this, "POP");
			pushNode(Node::unload_reference);
		}
	}
}

void BuildContext::startJumpForward() {

	m_jumpForeward.push({data.module->nextNodeOffset()});
	pushNode(0);
}

void BuildContext::blocJumpForward() {
	blocs().back().foreward->push_back(data.module->nextNodeOffset());
	pushNode(0);
}

void BuildContext::shiftJumpForward() {

	auto firstLabel = m_jumpForeward.top();
	m_jumpForeward.pop();

	auto secondLabel = m_jumpForeward.top();
	m_jumpForeward.pop();

	m_jumpForeward.push(firstLabel);
	m_jumpForeward.push(secondLabel);
}

void BuildContext::resolveJumpForward() {

	Node node;
	node.parameter = data.module->nextNodeOffset();
	for (size_t offset : m_jumpForeward.top()) {
		data.module->replaceNode(offset, node);
	}
	m_jumpForeward.pop();
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
