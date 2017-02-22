#include "ast/module.h"
#include "compiler/compiler.h"
#include "system/filestream.h"
#include "system/filesystem.h"
#include "system/error.h"

#include <cstring>
#include <algorithm>

using namespace std;

vector<Module *> Module::g_modules;
vector<DebugInfos *> Module::g_debugInfos;
map<string, Module::Context> Module::g_cache;

Instruction &Module::at(size_t idx) {
	return m_data[idx];
}

size_t Module::end() const {
	return m_data.size() - 1;
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

DebugInfos *Module::debug(size_t offset) {
	return g_debugInfos[offset];
}

Module::Context Module::load(const std::string &module) {

	auto it = g_cache.find(module);

	if (it == g_cache.end()) {

		string path = FileSystem::instance().getModulePath(module);
		if (path.empty()) {
			error("module '%s' not found", module.c_str());
		}

		it = Module::g_cache.insert({module, create()}).first;

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
	ctx.debugInfos = new DebugInfos;

	g_modules.push_back(ctx.module);
	g_debugInfos.push_back(ctx.debugInfos);

	return ctx;
}

Module::Context Module::main() {

	Module::Context ctx;

	ctx.moduleId = 0;
	ctx.module = g_modules.front();
	ctx.debugInfos = g_debugInfos.front();

	return ctx;
}

void Module::clearCache() {

	g_cache.clear();

	for_each(g_modules.begin(), g_modules.end(), [](Module *module) {delete module; });
	g_modules.clear();

	for_each(g_debugInfos.begin(), g_debugInfos.end(), [](DebugInfos *infos) { delete infos; });
	g_debugInfos.clear();
}

string Module::name(const Module *module) {

	if (module == Module::main().module) {
		return "main";
	}

	for (auto data : g_cache) {
		if (module == data.second.module) {
			return data.first;
		}
	}

	return "unknown";
}

size_t Module::id(const Module *module) {

	for (auto data : g_cache) {
		if (module == data.second.module) {
			return data.second.moduleId;
		}
	}

	if (module == Module::main().module) {
		return 0;
	}

	return -1;
}
