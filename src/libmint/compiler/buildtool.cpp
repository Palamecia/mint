#include "compiler/buildtool.h"
#include "compiler/compiler.h"
#include "ast/module.h"
#include "memory/object.h"
#include "memory/class.h"
#include "system/assert.h"
#include "system/error.h"

using namespace std;
using namespace mint;

static SymbolMapping<Class::Operator> Operators = {
	{ Symbol::New, Class::new_operator },
	{ Symbol::Delete, Class::delete_operator },
};

BuildContext::BuildContext(DataStream *stream, Module::Infos node) :
	lexer(stream), data(node) {
	DEBUG_METADATA(this, "MODULE: %zu", data.id);
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
	m_tree.emplace_back(command);
}

void BuildContext::Branch::pushNode(int parameter) {
	m_tree.emplace_back(parameter);
}

void BuildContext::Branch::pushNode(const char *symbol) {
	m_tree.emplace_back(m_context->data.module->makeSymbol(symbol));
}

void BuildContext::Branch::pushNode(Data *constant) {
	m_tree.emplace_back(m_context->data.module->makeConstant(constant));
}

void BuildContext::Branch::build() {

	assert(m_jumpBackward.empty());
	assert(m_jumpForeward.empty());

	size_t offset = m_context->data.module->nextNodeOffset();

	for (size_t label : m_labels) {
		m_tree[label].parameter += static_cast<int>(offset);
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

BuildContext::Block::Block(Type type) :
	type(type) {

}

int BuildContext::fastSymbolIndex(const std::string &symbol) {

	if (currentContext()->packages > 0) {
		return -1;
	}

	Symbol *s = data.module->makeSymbol(symbol.c_str());

	if (Definition *def = currentDefinition()) {

		auto i = def->fastSymbolIndexes.find(*s);
		if (i != def->fastSymbolIndexes.end()) {
			return i->second;
		}

		int index = static_cast<int>(def->fastSymbolIndexes.size());
		return def->fastSymbolIndexes[*s] = index;
	}

	return -1;
}

int BuildContext::fastSymbolIndex(Symbol *symbol) {

	if (Definition *def = currentDefinition()) {

		auto i = def->fastSymbolIndexes.find(*symbol);
		if (i != def->fastSymbolIndexes.end()) {
			return i->second;
		}

		int index = static_cast<int>(def->fastSymbolIndexes.size());
		return def->fastSymbolIndexes[*symbol] = index;
	}

	return -1;
}

void BuildContext::openBloc(Block::Type type) {

	Block *block = new Block(type);

	switch (block->type) {
	case Block::conditional_loop_type:
	case Block::custom_range_loop_type:
	case Block::range_loop_type:
		block->backward = &m_jumpBackward.top();
		block->foreward = &m_jumpForeward.top();
		break;

	case Block::switch_type:
		block->case_table = new CaseTable(this);
		pushNode(Node::jump);
		block->case_table->origin = data.module->nextNodeOffset();
		pushNode(0);
		m_jumpForeward.push({});
		block->foreward = &m_jumpForeward.top();
		break;
	}

	currentContext()->blocks.push_back(block);
}

void BuildContext::closeBloc() {

	Block *block = currentBlock();

	if (CaseTable *case_table = block->case_table) {
		delete case_table->default_label;
		delete case_table;
		resolveJumpForward();
	}

	currentContext()->blocks.pop_back();
	delete block;
}

bool BuildContext::isInLoop() const {
	if (const Block *block = currentBlock()) {
		return block->type != Block::switch_type;
	}
	return false;
}

bool BuildContext::isInSwitch() const {
	if (const Block *block = currentBlock()) {
		return block->type == Block::switch_type;
	}
	return false;
}

bool BuildContext::isInRangeLoop() const {
	if (const Block *block = currentBlock()) {
		return block->type == Block::range_loop_type;
	}
	return false;
}

bool BuildContext::isInFunction() const {
	return !m_definitions.empty();
}

bool BuildContext::isInGenerator() const {
	if (const Definition *def = currentDefinition()) {
		return def->generator;
	}
	return false;
}

void BuildContext::prepareContinue() {

	if (Block *block = currentBlock()) {
		for (size_t i = 0; i < block->retrievePointCount; ++i) {
			pushNode(Node::unset_retrieve_point);
		}
	}
}

void BuildContext::prepareBreak() {

	if (Block *block = currentBlock()) {

		switch (block->type) {
		case Block::range_loop_type:
			// unload range
			pushNode(Node::unload_reference);
			// unload target
			pushNode(Node::unload_reference);
			break;

		default:
			break;
		}

		for (size_t i = 0; i < block->retrievePointCount; ++i) {
			pushNode(Node::unset_retrieve_point);
		}
	}
}

void BuildContext::prepareReturn() {

	if (Definition *def = currentDefinition()) {

		for (const Block *block : def->blocks) {
			switch (block->type) {
			case Block::range_loop_type:
				// unload range
				pushNode(Node::unload_reference);
				// unload target
				pushNode(Node::unload_reference);
				break;

			default:
				break;
			}
		}

		for (size_t i = 0; i < def->retrievePointCount; ++i) {
			pushNode(Node::unset_retrieve_point);
		}
	}
}

void BuildContext::registerRetrievePoint() {

	if (Definition *definition = currentDefinition()) {
		definition->retrievePointCount++;
	}
	if (Block *block = currentBlock()) {
		block->retrievePointCount++;
	}
}

void BuildContext::unregisterRetrievePoint() {

	if (Definition *definition = currentDefinition()) {
		definition->retrievePointCount--;
	}
	if (Block *block = currentBlock()) {
		block->retrievePointCount--;
	}
}

const char *token_to_constant(const char *token) {
	return (*token == '-') ? token + 1 : token;
}

void BuildContext::addInclusiveRangeCaseLabel(const string &begin, const string &end) {

	if (CaseTable *case_table = currentBlock()->case_table) {
		const char *begin_ptr = token_to_constant(begin.c_str());
		if (Data *beginData = Compiler::makeData(begin_ptr)) {
			const char *end_ptr = token_to_constant(end.c_str());
			if (Data *endData = Compiler::makeData(end_ptr)) {
				case_table->current_token = begin + ".." + end;
				case_table->current_label.offset = data.module->nextNodeOffset();
				case_table->current_label.condition.pushNode(Node::load_constant);
				case_table->current_label.condition.pushNode(beginData);
				if (begin_ptr != begin.c_str()) {
					case_table->current_label.condition.pushNode(Node::neg_op);
				}
				case_table->current_label.condition.pushNode(Node::load_constant);
				case_table->current_label.condition.pushNode(endData);
				if (end_ptr != end.c_str()) {
					case_table->current_label.condition.pushNode(Node::neg_op);
				}
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

	if (CaseTable *case_table = currentBlock()->case_table) {
		const char *begin_ptr = token_to_constant(begin.c_str());
		if (Data *beginData = Compiler::makeData(begin_ptr)) {
			const char *end_ptr = token_to_constant(end.c_str());
			if (Data *endData = Compiler::makeData(end_ptr)) {
				case_table->current_token = begin + "..." + end;
				case_table->current_label.offset = data.module->nextNodeOffset();
				case_table->current_label.condition.pushNode(Node::load_constant);
				case_table->current_label.condition.pushNode(beginData);
				if (begin_ptr != begin.c_str()) {
					case_table->current_label.condition.pushNode(Node::neg_op);
				}
				case_table->current_label.condition.pushNode(Node::load_constant);
				case_table->current_label.condition.pushNode(endData);
				if (end_ptr != end.c_str()) {
					case_table->current_label.condition.pushNode(Node::neg_op);
				}
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

	if (CaseTable *case_table = currentBlock()->case_table) {
		const char *token_ptr = token_to_constant(token.c_str());
		if (Data *constant = Compiler::makeData(token_ptr)) {
			case_table->current_token = token;
			case_table->current_label.offset = data.module->nextNodeOffset();
			case_table->current_label.condition.pushNode(Node::load_constant);
			case_table->current_label.condition.pushNode(constant);
			if (token_ptr != token.c_str()) {
				case_table->current_label.condition.pushNode(Node::neg_op);
			}
		}
		else {
			parse_error(string("token '" + token + "' is not a valid constant").c_str());
		}
	}
}

void BuildContext::addSymbolCaseLabel(const string &token) {

	if (CaseTable *case_table = currentBlock()->case_table) {
		case_table->current_token = token;
		case_table->current_label.offset = data.module->nextNodeOffset();
		case_table->current_label.condition.pushNode(Node::load_symbol);
		case_table->current_label.condition.pushNode(token.c_str());
	}
}

void BuildContext::addMemberCaseLabel(const string &token) {

	if (CaseTable *case_table = currentBlock()->case_table) {
		case_table->current_token += "." + token;
		case_table->current_label.offset = data.module->nextNodeOffset();
		case_table->current_label.condition.pushNode(Node::load_member);
		case_table->current_label.condition.pushNode(token.c_str());
	}
}

void BuildContext::resolveEqCaseLabel() {

	if (CaseTable *case_table = currentBlock()->case_table) {
		case_table->current_label.condition.pushNode(Node::eq_op);
	}
}

void BuildContext::resolveIsCaseLabel() {

	if (CaseTable *case_table = currentBlock()->case_table) {
		case_table->current_label.condition.pushNode(Node::is_op);
	}
}

void BuildContext::setCaseLabel() {
	if (CaseTable *case_table = currentBlock()->case_table) {
		if (!case_table->labels.emplace(case_table->current_token, case_table->current_label).second) {
			parse_error("duplicate case value");
		}
		case_table->current_label.condition = Branch(this);
	}
}

void BuildContext::setDefaultLabel() {
	if (CaseTable *case_table = currentBlock()->case_table) {
		if (case_table->default_label) {
			parse_error("multiple default labels in one switch");
		}
		case_table->default_label = new size_t(data.module->nextNodeOffset());
	}
}

void BuildContext::buildCaseTable() {

	if (CaseTable *case_table = currentBlock()->case_table) {

		data.module->replaceNode(case_table->origin, static_cast<int>(data.module->nextNodeOffset()));

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
	currentBlock()->foreward->push_back(data.module->nextNodeOffset());
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

	Node node(static_cast<int>(data.module->nextNodeOffset()));
	for (size_t offset : m_jumpForeward.top()) {
		data.module->replaceNode(offset, node);
	}
	m_jumpForeward.pop();
}

void BuildContext::startJumpBackward() {
	m_jumpBackward.push(data.module->nextNodeOffset());
}

void BuildContext::blocJumpBackward() {
	pushNode(static_cast<int>(*currentBlock()->backward));
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

	pushNode(static_cast<int>(m_jumpBackward.top()));
	m_jumpBackward.pop();
}

void BuildContext::startDefinition() {
	Definition *def = new Definition;
	def->function = data.module->makeConstant(Reference::alloc<Function>());
	def->beginOffset = data.module->nextNodeOffset();
	m_definitions.push(def);
}

bool BuildContext::addParameter(const string &symbol) {

	Definition *def = currentDefinition();
	if (def->variadic) {
		parse_error("unexpected parameter after '...' token");
		return false;
	}

	Symbol *s = data.module->makeSymbol(symbol.c_str());
	int index = static_cast<int>(def->fastSymbolIndexes.size());
	def->fastSymbolIndexes.emplace(*s, index);
	def->parameters.push(s);
	return true;
}

bool BuildContext::setVariadic() {

	Definition *def = currentDefinition();
	if (def->variadic) {
		parse_error("unexpected parameter after '...' token");
		return false;
	}

	Symbol *s = data.module->makeSymbol("va_args");
	int index = static_cast<int>(def->fastSymbolIndexes.size());
	def->fastSymbolIndexes.emplace(*s, index);
	def->parameters.push(s);
	def->variadic = true;
	return true;
}

void BuildContext::setGenerator() {
	currentDefinition()->generator = true;
}

bool BuildContext::saveParameters() {

	Definition *def = currentDefinition();
	if (def->variadic && def->parameters.empty()) {
		parse_error("expected parameter before '...' token");
		return false;
	}

	int count = static_cast<int>(def->parameters.size());
	int signature = def->variadic ? -(count - 1) : count;
	Module::Handle *handle = data.module->makeHandle(currentPackage(), data.id, def->beginOffset);
	def->function->data<Function>()->mapping.emplace(signature, Function::Signature(handle, !def->capture.empty() || def->capture_all));

	while (!def->parameters.empty()) {
		pushNode(Node::init_param);
		pushNode(def->parameters.top());
		pushNode(fastSymbolIndex(def->parameters.top()));
		def->parameters.pop();
	}

	return true;
}

bool BuildContext::addDefinitionSignature() {

	Definition *def = currentDefinition();
	if (def->variadic) {
		parse_error("unexpected parameter after '...' token");
	}

	int signature = static_cast<int>(def->parameters.size());
	Module::Handle *handle = data.module->makeHandle(currentPackage(), data.id, def->beginOffset);
	def->function->data<Function>()->mapping.emplace(signature, Function::Signature(handle, !def->capture.empty() || def->capture_all));
	def->beginOffset = data.module->nextNodeOffset();
	return true;
}

void BuildContext::saveDefinition() {

	Definition *def = currentDefinition();

	for (auto &signature : def->function->data<Function>()->mapping) {
		signature.second.handle->fastCount = def->fastSymbolIndexes.size();
		signature.second.handle->generator = def->generator;
	}

	pushNode(Node::load_constant);
	data.module->pushNode(def->function);

	if (def->capture_all) {
		DEBUG_STACK(this, "CAPTURE_ALL");
		pushNode(Node::capture_all);
	}
	else for (Symbol *symbol : def->capture) {
		DEBUG_STACK(this, "CAPTURE %s", symbol->str().c_str());
		pushNode(Node::capture_symbol);
		pushNode(symbol);
	}

	m_definitions.pop();
	delete def;
}

Data *BuildContext::retrieveDefinition() {

	Definition *def = currentDefinition();
	Data *data = def->function->data();

	for (auto &signature : def->function->data<Function>()->mapping) {
		signature.second.handle->fastCount = def->fastSymbolIndexes.size();
		signature.second.handle->generator = def->generator;
	}

	m_definitions.pop();
	delete def;

	return data;
}

PackageData *BuildContext::currentPackage() const {
	if (m_packages.empty()) {
		return &GlobalData::instance();
	}

	return m_packages.top();
}

void BuildContext::openPackage(const string &name) {
	PackageData *package = currentPackage()->getPackage(Symbol(name));
	DEBUG_STACK(this, "OPEN PACKAGE %s", name.c_str());
	pushNode(Node::open_package);
	pushNode(Reference::alloc<Package>(package));
	++currentContext()->packages;
	m_packages.push(package);
}

void BuildContext::closePackage() {
	assert(!m_packages.empty());
	DEBUG_STACK(this, "CLOSE PACKAGE");
	pushNode(Node::close_package);
	--currentContext()->packages;
	m_packages.pop();
}

void BuildContext::startClassDescription(const string &name, Reference::Flags flags) {
	m_classBase.clear();
	m_classDescription.push(new ClassDescription(currentPackage(), flags, name));
}

void BuildContext::appendSymbolToBaseClassPath(const string &symbol) {
	m_classBase.appendSymbol(Symbol(symbol));
}

void BuildContext::saveBaseClassPath() {
	DEBUG_STACK(this, "INHERITE %s", m_classBase.toString().c_str());
	m_classDescription.top()->addBase(m_classBase);
	m_classBase.clear();
}

bool BuildContext::createMember(Reference::Flags flags, Class::Operator op, Data *value) {

	if (value == nullptr) {
		string error_message = get_operator_symbol(op).str() + ": member value is not a valid constant";
		parse_error(error_message.c_str());
		return false;
	}

	if (!m_classDescription.top()->createMember(op, WeakReference(flags, value))) {
		string error_message = get_operator_symbol(op).str() + ": member was already defined";
		parse_error(error_message.c_str());
		return false;
	}

	return true;
}

bool BuildContext::createMember(Reference::Flags flags, const string &name, Data *value) {

	Symbol symbol(name);
	auto i = Operators.find(symbol);

	if (i == Operators.end()) {
		if (value == nullptr) {
			string error_message = name + ": member value is not a valid constant";
			parse_error(error_message.c_str());
			return false;
		}

		if (!m_classDescription.top()->createMember(symbol, WeakReference(flags, value))) {
			string error_message = name + ": member was already defined";
			parse_error(error_message.c_str());
			return false;
		}

		return true;
	}

	return createMember(flags, i->second, value);
}

bool BuildContext::updateMember(Reference::Flags flags, Class::Operator op, Data *value) {

	if (!m_classDescription.top()->updateMember(op, WeakReference(flags, value))) {
		string error_message = get_operator_symbol(op).str() + ": member was already defined";
		parse_error(error_message.c_str());
		return false;
	}

	return true;
}

bool BuildContext::updateMember(Reference::Flags flags, const string &name, Data *value) {

	Symbol symbol(name);
	auto i = Operators.find(symbol);

	if (i == Operators.end()) {
		if (!m_classDescription.top()->updateMember(symbol, WeakReference(flags, value))) {
			string error_message = name + ": member was already defined";
			parse_error(error_message.c_str());
			return false;
		}

		return true;
	}

	return updateMember(flags, i->second, value);
}

void BuildContext::resolveClassDescription() {

	ClassDescription *desc = m_classDescription.top();
	m_classDescription.pop();

	if (m_classDescription.empty()) {
		pushNode(Node::register_class);
		pushNode(static_cast<int>(currentPackage()->createClass(desc)));
	}
	else {
		m_classDescription.top()->createClass(desc);
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
	m_calls.push(new Call);
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
	Definition *def = currentDefinition();
	def->capture.emplace_back(data.module->makeSymbol(symbol.c_str()));
}

void BuildContext::captureAll() {
	Definition *def = currentDefinition();
	def->capture_all = true;
}

bool BuildContext::hasPrinter() const {
	return currentContext()->printers > 0;
}

void BuildContext::openPrinter() {
	DEBUG_STACK(this, "OPEN PRINTER");
	pushNode(Node::open_printer);
	++currentContext()->printers;
}

void BuildContext::closePrinter() {
	DEBUG_STACK(this, "CLOSE PRINTER");
	pushNode(Node::close_printer);
	--currentContext()->printers;
}

void BuildContext::forcePrinter() {
	++currentContext()->printers;
}

void BuildContext::pushNode(Node::Command command) {
	data.module->pushNode(command);
}

void BuildContext::pushNode(int parameter) {
	data.module->pushNode(parameter);
}

void BuildContext::pushNode(const char *symbol) {
	data.module->pushNode(data.module->makeSymbol(symbol));
}

void BuildContext::pushNode(Symbol *symbol) {
	data.module->pushNode(symbol);
}

void BuildContext::pushNode(Data *constant) {
	data.module->pushNode(data.module->makeConstant(constant));
}

void BuildContext::setOperator(Class::Operator op) {
	m_operator = op;
}

Class::Operator BuildContext::getOperator() const {
	return m_operator;
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

BuildContext::Block *BuildContext::currentBlock() {
	auto &current_stack = currentContext()->blocks;
	if (current_stack.empty()) {
		return nullptr;
	}
	return current_stack.back();
}

const BuildContext::Block *BuildContext::currentBlock() const {
	auto &current_stack = currentContext()->blocks;
	if (current_stack.empty()) {
		return nullptr;
	}
	return current_stack.back();
}

BuildContext::Context *BuildContext::currentContext() {
	if (m_definitions.empty()) {
		return &m_moduleContext;
	}
	return m_definitions.top();
}

const BuildContext::Context *BuildContext::currentContext() const {
	if (m_definitions.empty()) {
		return &m_moduleContext;
	}
	return m_definitions.top();
}

BuildContext::Definition *BuildContext::currentDefinition() {
	if (m_definitions.empty()) {
		return nullptr;
	}
	return m_definitions.top();
}

const BuildContext::Definition *BuildContext::currentDefinition() const {
	if (m_definitions.empty()) {
		return nullptr;
	}
	return m_definitions.top();
}
