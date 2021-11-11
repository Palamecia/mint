#ifndef ABSTRACT_SYNTAX_TREE_H
#define ABSTRACT_SYNTAX_TREE_H

#include "ast/module.h"
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

enum BuiltinOption {
	default_options = 0x00,
	finalize_self   = 0x01
};
using BuiltinOptions = int;

class MINT_EXPORT AbstractSyntaxTree {
public:
	static AbstractSyntaxTree &instance();

	AbstractSyntaxTree &operator =(const AbstractSyntaxTree &other) = delete;

#if 0
	using BuiltinMethode = std::function<void(Cursor *)>;
#else
	using BuiltinMethode = std::add_pointer<void(Cursor *)>::type;
#endif

	static std::pair<int, Module::Handle *> createBuiltinMethode(Class *type, int signature, BuiltinMethode methode, BuiltinOptions options = default_options);
	static std::pair<int, Module::Handle *> createBuiltinMethode(Class *type, int signature, const std::string &methode);
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

	void cleanupMemory();
	void cleanupMetadata();

protected:
	AbstractSyntaxTree();
	~AbstractSyntaxTree();

	struct BuiltinModuleInfos : public Module::Infos {
		BuiltinModuleInfos(const Module::Infos &infos);
	};

	static BuiltinModuleInfos &builtinModule(int module);

	void removeCursor(Cursor *cursor);

	friend class Cursor;

private:
	std::mutex m_mutex;
	std::set<Cursor *> m_cursors;
	std::vector<Module *> m_modules;
	std::vector<DebugInfos *> m_debugInfos;
	std::map<std::string, Module::Infos> m_cache;

	static std::vector<BuiltinModuleInfos> g_builtinModules;
	static std::vector<BuiltinMethode> g_builtinMethodes;
};

void AbstractSyntaxTree::callBuiltinMethode(size_t methode, Cursor *cursor) {
	g_builtinMethodes[methode](cursor);
}

Module *AbstractSyntaxTree::getModule(Module::Id id) {
	assert(id < m_modules.size());
	return m_modules[id];
}

DebugInfos *AbstractSyntaxTree::getDebugInfos(Module::Id id) {
	return (id < m_debugInfos.size()) ? m_debugInfos[id] : nullptr;
}

}

#endif // ABSTRACT_SYNTAX_TREE_H
