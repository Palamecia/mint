#include "compiler/buildtool.h"
#include "compiler/compiler.h"
#include "ast/module.h"
#include "memory/globaldata.h"
#include "memory/object.h"
#include "memory/class.h"
#include "system/assert.h"
#include "system/error.h"
#include "catchcontext.h"
#include "casetable.h"
#include "context.h"
#include "branch.h"
#include "block.h"

#include <iterator>

using namespace std;
using namespace mint;

static SymbolMapping<Class::Operator> Operators = {
	{ Symbol::New, Class::new_operator },
	{ Symbol::Delete, Class::delete_operator },
};

BuildContext::BuildContext(DataStream *stream, Module::Infos node) :
	lexer(stream), data(node),
	m_moduleContext(new Context),
	m_branch(new MainBranch(this)) {
	stream->setNewLineCallback(bind(&DebugInfos::newLine, node.debugInfos, node.module, placeholders::_1));
}

BuildContext::~BuildContext() {
	assert(m_modifiers.empty());
	assert(m_branches.empty());
	m_branch->build();
	delete m_branch;
}

int BuildContext::fastScopedSymbolIndex(const std::string &symbol) {

	Symbol *s = nullptr;

	if (Context *context = currentContext()) {
		if (context->condition_scoped_symbols) {
			s = data.module->makeSymbol(symbol.c_str());
			context->condition_scoped_symbols->push_back(s);
		}
		else if (!context->blocks.empty()) {
			Block *block = context->blocks.back();
			s = data.module->makeSymbol(symbol.c_str());
			block->block_scoped_symbols.push_back(s);
		}
	}

	if (Definition *def = currentDefinition()) {
		if (def->with_fast && def->packages == 0) {
			if (s == nullptr) {
				s = data.module->makeSymbol(symbol.c_str());
			}
			return fast_symbol_index(def, s);
		}
	}

	return -1;
}

int BuildContext::fastSymbolIndex(const std::string &symbol) {

	if (Definition *def = currentDefinition()) {
		if (def->with_fast && def->packages == 0) {
			return fast_symbol_index(def, data.module->makeSymbol(symbol.c_str()));
		}
	}

	return -1;
}

bool BuildContext::hasReturned() const {
	if (const Definition *def = currentDefinition()) {
		return def->returned;
	}
	return false;
}

void BuildContext::openBlock(BlockType type) {

	Context *context = currentContext();
	Block *block = new Block(type);

	switch (type) {
	case conditional_loop_type:
	case custom_range_loop_type:
	case range_loop_type:
		block->backward = m_branch->nextJumpBackward();
		block->forward = m_branch->nextJumpForward();
		break;

	case switch_type:
		block->case_table = new CaseTable;
		pushNode(Node::jump);
		block->case_table->origin = m_branch->nextNodeOffset();
		pushNode(0);
		block->forward = m_branch->startEmptyJumpForward();
		break;

	case catch_type:
		block->catch_context = new CatchContext;
		break;

	default:
		break;
	}

	if (context->condition_scoped_symbols) {
		move(context->condition_scoped_symbols->begin(), context->condition_scoped_symbols->end(), back_inserter(block->block_scoped_symbols));
		block->condition_scoped_symbols = context->condition_scoped_symbols.release();
	}

	context->blocks.emplace_back(block);
}

void BuildContext::resetScopedSymbols() {
	Context *context = currentContext();
	resetScopedSymbols(&context->blocks.back()->block_scoped_symbols);
}

void BuildContext::closeBlock() {

	Context *context = currentContext();
	Block *block = context->blocks.back();

	switch (block->type) {
	case switch_type:
		delete block->case_table;
		resolveJumpForward();
		break;

	case catch_type:
		delete block->catch_context;
		break;

	default:
		break;
	}

	if (block->condition_scoped_symbols) {
		resetScopedSymbols(block->condition_scoped_symbols);
		delete block->condition_scoped_symbols;
	}

	context->blocks.pop_back();
	delete block;
}

bool BuildContext::isInLoop() const {
	if (const Block *block = currentContinuableBlock()) {
		switch (block->type) {
		case conditional_loop_type:
		case custom_range_loop_type:
		case range_loop_type:
			return true;
		default:
			break;
		}
	}
	return false;
}

bool BuildContext::isInSwitch() const {
	if (const Block *block = currentBreakableBlock()) {
		return block->type == switch_type;
	}
	return false;
}

bool BuildContext::isInRangeLoop() const {
	if (const Block *block = currentContinuableBlock()) {
		return block->type == range_loop_type;
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

	if (Block *block = currentBreakableBlock()) {

		for (size_t i = 0; i < block->retrievePointCount; ++i) {
			pushNode(Node::unset_retrieve_point);
		}

		Context *context = currentContext();
		auto &children = context->blocks;

		for (auto child = children.rbegin(); child != children.rend() && *child != block; ++child) {
			resetScopedSymbols(&(*child)->block_scoped_symbols);
		}

		resetScopedSymbols(&block->block_scoped_symbols);
	}
}

void BuildContext::prepareBreak() {

	if (Block *block = currentBreakableBlock()) {

		switch (block->type) {
		case range_loop_type:
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

		Context *context = currentContext();
		auto &children = context->blocks;

		for (auto child = children.rbegin(); child != children.rend() && *child != block; ++child) {
			resetScopedSymbols(&(*child)->block_scoped_symbols);
		}

		resetScopedSymbols(&block->block_scoped_symbols);
	}
}

void BuildContext::prepareReturn() {

	if (Definition *def = currentDefinition()) {

		for (const Block *block : def->blocks) {
			switch (block->type) {
			case range_loop_type:
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

		if (def->blocks.empty()) {
			def->returned = true;
		}
	}
}

void BuildContext::registerRetrievePoint() {

	if (Definition *definition = currentDefinition()) {
		definition->retrievePointCount++;
	}
	if (Block *block = currentBreakableBlock()) {
		block->retrievePointCount++;
	}
}

void BuildContext::unregisterRetrievePoint() {

	if (Definition *definition = currentDefinition()) {
		definition->retrievePointCount--;
	}
	if (Block *block = currentBreakableBlock()) {
		block->retrievePointCount--;
	}
}

void BuildContext::setExceptionSymbol(const string &symbol) {

	Context *context = currentContext();
	Block *block = context->blocks.back();

	if (CatchContext *catch_context = block->catch_context) {
		catch_context->symbol = data.module->makeSymbol(symbol.c_str());
	}
}

void BuildContext::resetException() {

	Context *context = currentContext();
	Block *block = context->blocks.back();

	if (CatchContext *catch_context = block->catch_context) {
		pushNode(Node::reset_exception);
		pushNode(catch_context->symbol);
	}
}

void BuildContext::startCaseLabel() {
	if (CaseTable *case_table = currentBreakableBlock()->case_table) {
		case_table->current_label = new CaseTable::Label(m_branch);
		case_table->current_label->offset = m_branch->nextNodeOffset();
		pushBranch(case_table->current_label->condition.get());
	}
}

void BuildContext::resolveCaseLabel(const string &label) {
	if (CaseTable *case_table = currentBreakableBlock()->case_table) {
		if (!case_table->labels.emplace(label, case_table->current_label).second) {
			parse_error("duplicate case value");
		}
		popBranch();
	}
}

void BuildContext::setDefaultLabel() {
	if (CaseTable *case_table = currentBreakableBlock()->case_table) {
		if (case_table->default_label) {
			parse_error("multiple default labels in one switch");
		}
		case_table->default_label = new size_t(m_branch->nextNodeOffset());
	}
}

void BuildContext::buildCaseTable() {

	if (CaseTable *case_table = currentBreakableBlock()->case_table) {

		m_branch->replaceNode(case_table->origin, static_cast<int>(m_branch->nextNodeOffset()));

		for (auto &label : case_table->labels) {
			pushNode(Node::reload_reference);
			label.second->condition->build();
			pushNode(Node::case_jump);
			pushNode(static_cast<int>(label.second->offset));
			delete label.second;
		}

		if (case_table->default_label) {
			pushNode(Node::load_constant);
			pushNode(Compiler::makeData("true"));
			pushNode(Node::case_jump);
			pushNode(static_cast<int>(*case_table->default_label));
			delete case_table->default_label;
		}
		else {
			pushNode(Node::unload_reference);
		}
	}
}

void BuildContext::startJumpForward() {
	m_branch->startJumpForward();
}

void BuildContext::blocJumpForward() {
	Block *block = currentBreakableBlock();
	assert(block && block->forward);
	block->forward->push_back(m_branch->nextNodeOffset());
	pushNode(0);
}

void BuildContext::shiftJumpForward() {
	m_branch->shiftJumpForward();
}

void BuildContext::resolveJumpForward() {
	m_branch->resolveJumpForward();
}

void BuildContext::startJumpBackward() {
	m_branch->startJumpBackward();
}

void BuildContext::blocJumpBackward() {
	Block *block = currentContinuableBlock();
	assert(block && block->backward);
	pushNode(static_cast<int>(*block->backward));
}

void BuildContext::shiftJumpBackward() {
	m_branch->shiftJumpBackward();
}

void BuildContext::resolveJumpBackward() {
	m_branch->resolveJumpBackward();
}

void BuildContext::startDefinition() {
	Definition *def = new Definition;
	def->function = data.module->makeConstant(Reference::alloc<Function>());
	def->beginOffset = m_branch->nextNodeOffset();
	m_definitions.push(def);
}

bool BuildContext::addParameter(const string &symbol, Reference::Flags flags) {

	Definition *def = currentDefinition();
	if (def->variadic) {
		parse_error("unexpected parameter after '...' token");
		return false;
	}

	Symbol *s = data.module->makeSymbol(symbol.c_str());
	int index = static_cast<int>(def->fastSymbolIndexes.size());
	def->fastSymbolIndexes.emplace(*s, index);
	def->parameters.push({flags, s});
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
	def->parameters.push({Reference::standard, s});
	def->variadic = true;
	return true;
}

void BuildContext::setGenerator() {

	Definition *def = currentDefinition();

	for (auto exit_point : def->exitPoints) {
		m_branch->replaceNode(exit_point, Node::yield_exit_generator);
	}

	def->generator = true;
}

void BuildContext::setExitPoint() {
	currentDefinition()->exitPoints.emplace_back(m_branch->nextNodeOffset());
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
	def->function->data<Function>()->mapping.emplace(signature, Function::Signature(handle, def->capture != nullptr));

	while (!def->parameters.empty()) {
		Parameter &param = def->parameters.top();
		pushNode(Node::init_param);
		pushNode(param.symbol);
		pushNode(param.flags);
		pushNode(fast_symbol_index(def, param.symbol));
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
	def->function->data<Function>()->mapping.emplace(signature, Function::Signature(handle, def->capture != nullptr));
	def->beginOffset = m_branch->nextNodeOffset();
	return true;
}

void BuildContext::saveDefinition() {

	Definition *def = currentDefinition();

	for (auto &signature : def->function->data<Function>()->mapping) {
		signature.second.handle->fastCount = def->fastSymbolIndexes.size();
		signature.second.handle->generator = def->generator;
	}

	pushNode(Node::load_constant);
	pushNode(def->function);

	if (def->capture) {
		def->capture->build();
		delete def->capture;
	}

	assert(def->blocks.empty());
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

	assert(def->blocks.empty());
	m_definitions.pop();
	delete def;

	return data;
}

PackageData *BuildContext::currentPackage() const {
	if (m_packages.empty()) {
		return GlobalData::instance();
	}
	return m_packages.top();
}

void BuildContext::openPackage(const string &name) {
	PackageData *package = currentPackage()->getPackage(Symbol(name));
	pushNode(Node::open_package);
	pushNode(Reference::alloc<Package>(package));
	++currentContext()->packages;
	m_packages.push(package);
}

void BuildContext::closePackage() {
	assert(!m_packages.empty());
	pushNode(Node::close_package);
	--currentContext()->packages;
	m_packages.pop();
}

void BuildContext::startClassDescription(const string &name, Reference::Flags flags) {
	m_classBase.clear();
	currentContext()->classes.push(new ClassDescription(currentPackage(), flags, name));
}

void BuildContext::appendSymbolToBaseClassPath(const string &symbol) {
	m_classBase.appendSymbol(Symbol(symbol));
}

void BuildContext::saveBaseClassPath() {
	currentContext()->classes.top()->addBase(m_classBase);
	m_classBase.clear();
}

bool BuildContext::createMember(Reference::Flags flags, Class::Operator op, Data *value) {

	if (value == nullptr) {
		string error_message = get_operator_symbol(op).str() + ": member value is not a valid constant";
		parse_error(error_message.c_str());
		return false;
	}

	if (!currentContext()->classes.top()->createMember(op, WeakReference(flags, value))) {
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

		if (!currentContext()->classes.top()->createMember(symbol, WeakReference(flags, value))) {
			string error_message = name + ": member was already defined";
			parse_error(error_message.c_str());
			return false;
		}

		return true;
	}

	return createMember(flags, i->second, value);
}

bool BuildContext::updateMember(Reference::Flags flags, Class::Operator op, Data *value) {

	if (!currentContext()->classes.top()->updateMember(op, WeakReference(flags, value))) {
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
		if (!currentContext()->classes.top()->updateMember(symbol, WeakReference(flags, value))) {
			string error_message = name + ": member was already defined";
			parse_error(error_message.c_str());
			return false;
		}

		return true;
	}

	return updateMember(flags, i->second, value);
}

void BuildContext::resolveClassDescription() {

	Context *context = currentContext();

	ClassDescription *desc = context->classes.top();
	context->classes.pop();

	if (context->classes.empty()) {
		pushNode(Node::register_class);
		pushNode(static_cast<int>(currentPackage()->createClass(desc)));
	}
	else {
		context->classes.top()->createClass(desc);
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


void BuildContext::startCapture() {
	Definition *def = currentDefinition();
	def->capture = new SubBranch(m_branch);
	def->with_fast = false;
	pushBranch(def->capture);
}

void BuildContext::resolveCapture() {
	Definition *def = currentDefinition();
	def->with_fast = true;
	popBranch();
}

bool BuildContext::captureAs(const string &symbol) {

	Definition *def = currentDefinition();

	if (def->capture_all) {
		parse_error("unexpected parameter after '...' token");
		delete def->capture;
		return false;
	}

	pushNode(Node::capture_as);
	pushNode(symbol.c_str());
	return true;
}

bool BuildContext::capture(const string &symbol) {

	Definition *def = currentDefinition();

	if (def->capture_all) {
		parse_error("unexpected parameter after '...' token");
		delete def->capture;
		return false;
	}

	pushNode(Node::capture_symbol);
	pushNode(symbol.c_str());
	return true;
}

bool BuildContext::captureAll() {

	Definition *def = currentDefinition();

	if (def->capture_all) {
		parse_error("unexpected parameter after '...' token");
		delete def->capture;
		return false;
	}

	pushNode(Node::capture_all);
	def->capture_all = true;
	return true;
}

bool BuildContext::hasPrinter() const {
	return currentContext()->printers > 0;
}

void BuildContext::openPrinter() {
	pushNode(Node::open_printer);
	++currentContext()->printers;
}

void BuildContext::closePrinter() {
	pushNode(Node::close_printer);
	--currentContext()->printers;
}

void BuildContext::forcePrinter() {
	++currentContext()->printers;
}

void BuildContext::startCondition() {
	Context *context = currentContext();
	context->condition_scoped_symbols.reset(new vector<Symbol *>);
}

void BuildContext::resolveCondition() {

}

void BuildContext::pushNode(Node::Command command) {
	m_branch->pushNode(command);
}

void BuildContext::pushNode(int parameter) {
	m_branch->pushNode(parameter);
}

void BuildContext::pushNode(const char *symbol) {
	m_branch->pushNode(data.module->makeSymbol(symbol));
}

void BuildContext::pushNode(Symbol *symbol) {
	m_branch->pushNode(symbol);
}

void BuildContext::pushNode(Data *constant) {
	m_branch->pushNode(data.module->makeConstant(constant));
}

void BuildContext::pushNode(Reference *constant) {
	m_branch->pushNode(constant);
}

void BuildContext::pushBranch(Branch *branch) {
	m_branches.push(m_branch);
	m_branch = branch;
}

void BuildContext::popBranch() {
	m_branch = m_branches.top();
	m_branches.pop();
}

void BuildContext::setOperator(Class::Operator op) {
	m_operator = op;
}

Class::Operator BuildContext::getOperator() const {
	return m_operator;
}

void BuildContext::startModifiers(Reference::Flags flags) {
	m_modifiers.push(flags);
}

void BuildContext::addModifiers(Reference::Flags flags) {
	assert(!m_modifiers.empty());
	m_modifiers.top() |= flags;
}

Reference::Flags BuildContext::retrieveModifiers() {
	assert(!m_modifiers.empty());
	Reference::Flags flags = m_modifiers.top();
	m_modifiers.pop();
	return flags;
}

void BuildContext::parse_error(const char *error_msg) {
	string error_str = lexer.formatError(error_msg);
	fflush(stdout);
	error("%s", error_str.c_str());
}

Block *BuildContext::currentBreakableBlock() {
	auto &current_stack = currentContext()->blocks;
	for (auto block = current_stack.rbegin(); block != current_stack.rend(); ++block) {
		if ((*block)->is_breakable()) {
			return *block;
		}
	}
	return nullptr;
}

const Block *BuildContext::currentBreakableBlock() const {
	auto &current_stack = currentContext()->blocks;
	for (auto block = current_stack.rbegin(); block != current_stack.rend(); ++block) {
		if ((*block)->is_breakable()) {
			return *block;
		}
	}
	return nullptr;
}

Block *BuildContext::currentContinuableBlock() {
	auto &current_stack = currentContext()->blocks;
	for (auto block = current_stack.rbegin(); block != current_stack.rend(); ++block) {
		if ((*block)->is_continuable()) {
			return *block;
		}
	}
	return nullptr;
}

const Block *BuildContext::currentContinuableBlock() const {
	auto &current_stack = currentContext()->blocks;
	for (auto block = current_stack.rbegin(); block != current_stack.rend(); ++block) {
		if ((*block)->is_continuable()) {
			return *block;
		}
	}
	return nullptr;
}

Context *BuildContext::currentContext() {
	if (m_definitions.empty()) {
		return m_moduleContext.get();
	}
	return m_definitions.top();
}

const Context *BuildContext::currentContext() const {
	if (m_definitions.empty()) {
		return m_moduleContext.get();
	}
	return m_definitions.top();
}

Definition *BuildContext::currentDefinition() {
	if (m_definitions.empty()) {
		return nullptr;
	}
	return m_definitions.top();
}

const Definition *BuildContext::currentDefinition() const {
	if (m_definitions.empty()) {
		return nullptr;
	}
	return m_definitions.top();
}

int BuildContext::findFastSymbolIndex(Symbol *symbol) {
	if (Definition *def = currentDefinition()) {
		if (def->with_fast && def->packages == 0) {
			return find_fast_symbol_index(def, symbol);
		}
	}
	return -1;
}

void BuildContext::resetScopedSymbols(const vector<Symbol *> *symbols) {
	for (auto symbol = symbols->rbegin(); symbol != symbols->rend(); ++symbol) {
		int index = findFastSymbolIndex(*symbol);
		if (index != -1) {
			pushNode(Node::reset_fast);
			pushNode(*symbol);
			pushNode(index);
		}
		else {
			pushNode(Node::reset_symbol);
			pushNode(*symbol);
		}
	}
}
