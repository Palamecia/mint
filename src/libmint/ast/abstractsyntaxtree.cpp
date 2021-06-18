#include "ast/abstractsyntaxtree.h"
#include "ast/cursor.h"
#include "memory/class.h"
#include "compiler/compiler.h"
#include "system/filestream.h"
#include "system/filesystem.h"
#include "system/bufferstream.h"
#include "system/error.h"
#include "threadentrypoint.h"

#include <memory>
#include <algorithm>

using namespace std;
using namespace mint;

ThreadEntryPoint ThreadEntryPoint::g_instance;
vector<AbstractSyntaxTree::BuiltinModuleInfos> AbstractSyntaxTree::g_builtinModules;
vector<AbstractSyntaxTree::BuiltinMethode> AbstractSyntaxTree::g_builtinMethodes;

AbstractSyntaxTree::BuiltinModuleInfos::BuiltinModuleInfos(const Module::Infos &infos) {
	id = infos.id;
	module = infos.module;
	debugInfos = infos.debugInfos;
	loaded = infos.loaded;
}

AbstractSyntaxTree::AbstractSyntaxTree() {

	// setup global data
	GlobalData::instance();

	g_builtinModules.reserve(8);
}

AbstractSyntaxTree::~AbstractSyntaxTree() {
	cleanup();
}

void AbstractSyntaxTree::cleanup() {

	while (!m_cursors.empty()) {
		delete *m_cursors.begin();
	}

	// cleanup global data
	GlobalData::instance().cleanup();

	for_each(m_modules.begin(), m_modules.end(), default_delete<Module>());
	m_modules.clear();

	for_each(m_debugInfos.begin(), m_debugInfos.end(), default_delete<DebugInfos>());
	m_debugInfos.clear();

	g_builtinModules.clear();
	m_cache.clear();

	// leaked destructors are ignored
	while (!m_cursors.empty()) {
		delete *m_cursors.begin();
	}
}

AbstractSyntaxTree &AbstractSyntaxTree::instance() {
	static AbstractSyntaxTree g_instance;
	return g_instance;
}

pair<int, Module::Handle *> AbstractSyntaxTree::createBuiltinMethode(Class *type, int signature, BuiltinMethode methode, BuiltinOptions options) {

	BuiltinModuleInfos &module = builtinModule(-type->metatype());

	const size_t offset = module.module->nextNodeOffset() + 2;
	const size_t index = g_builtinMethodes.size();
	g_builtinMethodes.emplace_back(methode);

	module.module->pushNodes({
								 Node::jump, static_cast<int>(offset) + 6,
								 Node::finalize_iterators, signature, (options & finalize_self) ? 0 : 1,
								 Node::call_builtin, static_cast<int>(index),
								 Node::exit_call, Node::module_end
							 });

	return make_pair(signature, module.module->makeBuiltinHandle(type->getPackage(), module.id, offset));
}

pair<int, Module::Handle *> AbstractSyntaxTree::createBuiltinMethode(Class *type, int signature, const string &methode) {

	BuiltinModuleInfos &module = builtinModule(-type->metatype());
	BufferStream stream(methode);
	const size_t offset = module.module->end() + 3;

	Compiler compiler;
	compiler.build(&stream, module);

	return make_pair(signature, module.module->findHandle(module.id, offset));
}

Cursor *AbstractSyntaxTree::createCursor(Cursor *parent) {
	unique_lock<mutex> lock(m_mutex);
	return *m_cursors.insert(new Cursor(ThreadEntryPoint::instance(), parent)).first;
}

Cursor *AbstractSyntaxTree::createCursor(Module::Id module, Cursor *parent) {
	unique_lock<mutex> lock(m_mutex);
	return *m_cursors.insert(new Cursor(getModule(module), parent)).first;
}

Module::Infos AbstractSyntaxTree::createModule() {

	Module::Infos infos;

	infos.id = m_modules.size();
	infos.module = new Module;
	infos.debugInfos = new DebugInfos;

	m_modules.push_back(infos.module);
	m_debugInfos.push_back(infos.debugInfos);

	return infos;
}

Module::Infos AbstractSyntaxTree::loadModule(const string &module) {

	auto it = m_cache.find(module);

	if (it == m_cache.end()) {

		string path = FileSystem::instance().getModulePath(module);
		if (UNLIKELY(path.empty())) {
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

	return Module::invalid_id;
}

AbstractSyntaxTree::BuiltinModuleInfos &AbstractSyntaxTree::builtinModule(int module) {

	size_t index = static_cast<size_t>(~module);

	for (size_t i = g_builtinModules.size(); i <= index; ++i) {
		g_builtinModules.emplace_back(instance().createModule());
	}

	return g_builtinModules[index];
}

void AbstractSyntaxTree::removeCursor(Cursor *cursor) {
	unique_lock<mutex> lock(m_mutex);
	m_cursors.erase(cursor);
}
