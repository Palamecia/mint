#ifndef MINT_ABSTRACTSYNTAXTREE_H
#define MINT_ABSTRACTSYNTAXTREE_H

#include "ast/module.h"
#include "memory/globaldata.h"
#include "system/assert.h"

#if 0
#include <functional>
#else
#include <type_traits>
#endif

#include <vector>
#include <mutex>

namespace mint {

class Cursor;
class Class;

class MINT_EXPORT AbstractSyntaxTree {
public:
	AbstractSyntaxTree();
	~AbstractSyntaxTree();

	AbstractSyntaxTree(const AbstractSyntaxTree &other) = delete;
	AbstractSyntaxTree &operator =(const AbstractSyntaxTree &other) = delete;

#if 0
	using BuiltinMethode = std::function<void(Cursor *)>;
#else
	using BuiltinMethode = std::add_pointer<void(Cursor *)>::type;
#endif

	static AbstractSyntaxTree *instance();

	std::pair<int, Module::Handle *> createBuiltinMethode(Class *type, int signature, BuiltinMethode methode);
	std::pair<int, Module::Handle *> createBuiltinMethode(Class *type, int signature, const std::string &methode);
	inline void callBuiltinMethode(size_t methode, Cursor *cursor);

	Cursor *createCursor(Cursor *parent = nullptr);
	Cursor *createCursor(Module::Id module, Cursor *parent = nullptr);

	Module::Infos createModule();
	Module::Infos loadModule(const std::string &module);
	Module::Infos main();

	inline Module *getModule(Module::Id id);
	inline DebugInfos *getDebugInfos(Module::Id id);
	Module::Id getModuleId(const Module *module);
	std::string getModuleName(const Module *module);

	inline GlobalData &globalData();

	void cleanupMemory();
	void cleanupModules();
	void cleanupMetadata();

protected:
	struct BuiltinModuleInfos : public Module::Infos {
		BuiltinModuleInfos(const Module::Infos &infos);
	};

	BuiltinModuleInfos &builtinModule(int module);

	void removeCursor(Cursor *cursor);

	friend class Cursor;

private:
	static AbstractSyntaxTree *g_instance;

	std::mutex m_mutex;
	std::set<Cursor *> m_cursors;
	std::vector<Module *> m_modules;
	std::vector<DebugInfos *> m_debugInfos;
	std::map<std::string, Module::Infos> m_cache;

	GlobalData m_globalData;
	std::vector<BuiltinModuleInfos> m_builtinModules;
	std::vector<BuiltinMethode> m_builtinMethodes;
};

void AbstractSyntaxTree::callBuiltinMethode(size_t methode, Cursor *cursor) {
	m_builtinMethodes[methode](cursor);
}

Module *AbstractSyntaxTree::getModule(Module::Id id) {
	assert(id < m_modules.size());
	return m_modules[id];
}

DebugInfos *AbstractSyntaxTree::getDebugInfos(Module::Id id) {
	return (id < m_debugInfos.size()) ? m_debugInfos[id] : nullptr;
}

GlobalData &AbstractSyntaxTree::globalData() {
	return m_globalData;
}

}

#endif // MINT_ABSTRACTSYNTAXTREE_H
