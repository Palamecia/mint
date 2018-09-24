#include "ast/abstractsyntaxtree.h"
#include "ast/cursor.h"
#include "compiler/compiler.h"
#include "system/filestream.h"
#include "system/filesystem.h"
#include "system/bufferstream.h"
#include "system/assert.h"
#include "system/error.h"
#include "threadentrypoint.h"

#include <memory>
#include <algorithm>

using namespace std;
using namespace mint;

ThreadEntryPoint ThreadEntryPoint::g_instance;
map<int, Module::Infos> AbstractSyntaxTree::g_builtinModules;
map<int, map<int, AbstractSyntaxTree::Builtin>> AbstractSyntaxTree::g_builtinMembers;

AbstractSyntaxTree::AbstractSyntaxTree() {

}

AbstractSyntaxTree::~AbstractSyntaxTree() {

	while (!m_cursors.empty()) {
		delete *m_cursors.begin();
	}

	GlobalData::instance().clearGlobalReferences();

	g_builtinModules.clear();
	g_builtinMembers.clear();
	m_cache.clear();

	for_each(m_modules.begin(), m_modules.end(), default_delete<Module>());
	m_modules.clear();

	for_each(m_debugInfos.begin(), m_debugInfos.end(), default_delete<DebugInfos>());
	m_debugInfos.clear();

	while (!m_cursors.empty()) {
		delete *m_cursors.begin();
	}
}

AbstractSyntaxTree &AbstractSyntaxTree::instance() {
	static AbstractSyntaxTree g_instance;
	return g_instance;
}

pair<int, int> AbstractSyntaxTree::createBuiltinMethode(int type, Builtin methode) {

	auto &methodes = builtinMembers(-type);
	int offset = methodes.size();

	methodes[offset] = methode;

	return pair<int, int>(-type, offset);
}

pair<int, int> AbstractSyntaxTree::createBuiltinMethode(int type, const string &methode) {

	Module::Infos module = builtinModule(type);
	BufferStream stream(methode);

	auto offset = module.module->end() + 3;

	Compiler compiler;
	compiler.build(&stream, module);

	return {module.id, offset};
}

void AbstractSyntaxTree::callBuiltinMethode(int module, int methode, Cursor *cursor) {
	builtinMembers(module)[methode](cursor);
}

Cursor *AbstractSyntaxTree::createCursor() {
	unique_lock<mutex> lock(m_mutex);
	return *m_cursors.insert(new Cursor(ThreadEntryPoint::instance())).first;
}

Cursor *AbstractSyntaxTree::createCursor(Module::Id module) {
	unique_lock<mutex> lock(m_mutex);
	return *m_cursors.insert(new Cursor(getModule(module))).first;
}

Module::Infos AbstractSyntaxTree::createModule() {

	Module::Infos infos;

	infos.id = m_modules.size();
	infos.module = new Module;
	infos.debugInfos = new DebugInfos;
	infos.loaded = false;

	m_modules.push_back(infos.module);
	m_debugInfos.push_back(infos.debugInfos);

	return infos;
}

Module::Infos AbstractSyntaxTree::loadModule(const string &module) {

	auto it = m_cache.find(module);

	if (it == m_cache.end()) {

		string path = FileSystem::instance().getModulePath(module);
		if (path.empty()) {
			error("module '%s' not found", module.c_str());
		}

		it = m_cache.emplace(module, createModule()).first;

		FileStream stream(path);

		Compiler compiler;
		compiler.build(&stream, it->second);
	}

	auto infos = it->second;
	it->second.loaded = true;
	return infos;
}

Module::Infos AbstractSyntaxTree::main() {

	if (m_modules.empty()) {
		return m_cache.emplace("main", createModule()).first->second;
	}

	Module::Infos infos;

	infos.id = Module::main_id;
	infos.module = m_modules.front();
	infos.debugInfos = m_debugInfos.front();
	infos.loaded = true;

	return infos;
}

Module *AbstractSyntaxTree::getModule(Module::Id id) {
	assert(id < m_modules.size());
	return m_modules[id];
}

DebugInfos *AbstractSyntaxTree::getDebugInfos(Module::Id id) {
	assert(id < m_debugInfos.size());
	return m_debugInfos[id];
}

string AbstractSyntaxTree::getModuleName(const Module *module) {

	if (module == main().module) {
		return "main";
	}

	for (auto data : m_cache) {
		if (module == data.second.module) {
			return data.first;
		}
	}

	return "unknown";
}

Module::Id AbstractSyntaxTree::getModuleId(const Module *module) {

	for (auto data : m_cache) {
		if (module == data.second.module) {
			return data.second.id;
		}
	}

	if (module == main().module) {
		return Module::main_id;
	}

	return -1;
}

map<int, AbstractSyntaxTree::Builtin> &AbstractSyntaxTree::builtinMembers(int builtinModule) {
	return g_builtinMembers[builtinModule];
}

Module::Infos AbstractSyntaxTree::builtinModule(int module) {

	auto i = g_builtinModules.find(module);

	if (i == g_builtinModules.end()) {
		i = g_builtinModules.emplace(module, instance().createModule()).first;
	}

	return i->second;
}

void AbstractSyntaxTree::removeCursor(Cursor *cursor) {
	unique_lock<mutex> lock(m_mutex);
	m_cursors.erase(cursor);
}
