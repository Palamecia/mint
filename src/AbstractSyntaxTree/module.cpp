#include "AbstractSyntaxTree/module.h"
#include "Compiler/compiler.h"
#include "System/filestream.h"
#include "System/filesystem.h"
#include "System/error.h"

#include <cstring>

using namespace std;

vector<Module *> Module::g_modules;
map<string, Module::Context> Module::cache;

Instruction &Module::at(uint idx) {
	return m_data[idx];
}

Module::Module() {}

Module::~Module() {

	for (auto symbol : m_symbols) {
		delete [] symbol;
	}

	for (auto constant : m_constants) {
		delete constant;
	}
}

char *Module::makeSymbol(const char *name) {

	for (char *symbol : m_symbols) {
		if (!strcmp(symbol, name)) {
			return symbol;
		}
	}

	char *symbol = new char [strlen(name) + 1];
	strcpy(symbol, name);
	m_symbols.push_back(symbol);
	return symbol;
}

Reference *Module::makeConstant(Data *data) {

	Reference *constant = new Reference(Reference::const_ref | Reference::const_value, data);
	m_constants.push_back(constant);
	return constant;
}

void Module::pushInstruction(const Instruction &instruction) {
	m_data.push_back(instruction);
}

void Module::replaceInstruction(size_t offset, const Instruction &instruction) {
	m_data[offset] = instruction;
}

size_t Module::nextInstructionOffset() const {
	return m_data.size();
}

Module *Module::get(size_t offset) {
	return g_modules[offset];
}

Module::Context Module::load(const std::string &module) {

	auto it = cache.find(module);

	if (it == cache.end()) {

		string path = FileSystem::instance().getModulePath(module);
		if (path.empty()) {
			error("module '%s' not found", module.c_str());
		}

		it = Module::cache.insert({module, create()}).first;

		FileStream stream(path);

		Compiler compiler;
		compiler.build(&stream, it->second);
	}

	return it->second;
}

Module::Context Module::create() {

	Context ctx;

	ctx.moduleId = g_modules.size();
	ctx.module = new Module;
	g_modules.push_back(ctx.module);

	return ctx;
}

Module::Context Module::main() {

	Module::Context ctx;

	ctx.moduleId = 0;
	ctx.module = g_modules.front();

	return ctx;
}

void Module::clearCache() {

	cache.clear();

	for (Module *module : g_modules) {
		delete module;
	}
	g_modules.clear();
}
