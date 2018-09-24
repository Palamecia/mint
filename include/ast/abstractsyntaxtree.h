#ifndef ABSTRACT_SYNTAX_TREE_H
#define ABSTRACT_SYNTAX_TREE_H

#include "module.h"

#include <functional>

namespace mint {

class Cursor;

class MINT_EXPORT AbstractSyntaxTree {
public:
	static AbstractSyntaxTree &instance();

	AbstractSyntaxTree &operator =(const AbstractSyntaxTree &other) = delete;

	typedef std::function<void(Cursor *)> Builtin;

	static std::pair<int, int> createBuiltinMethode(int type, Builtin methode);
	static std::pair<int, int> createBuiltinMethode(int type, const std::string &methode);
	void callBuiltinMethode(int module, int methode, Cursor *cursor);

	Cursor *createCursor();
	Cursor *createCursor(Module::Id module);

	Module::Infos createModule();
	Module::Infos loadModule(const std::string &module);
	Module::Infos main();

	Module *getModule(Module::Id id);
	DebugInfos *getDebugInfos(Module::Id id);
	Module::Id getModuleId(const Module *module);
	std::string getModuleName(const Module *module);

protected:
	AbstractSyntaxTree();
	~AbstractSyntaxTree();

	static std::map<int, Builtin> &builtinMembers(int builtinModule);
	static Module::Infos builtinModule(int module);

	void removeCursor(Cursor *cursor);

	friend class Cursor;

private:
	std::mutex m_mutex;
	std::set<Cursor *> m_cursors;
	std::vector<Module *> m_modules;
	std::vector<DebugInfos *> m_debugInfos;
	std::map<std::string, Module::Infos> m_cache;

	static std::map<int, Module::Infos> g_builtinModules;
	static std::map<int, std::map<int, Builtin>> g_builtinMembers;
};

}

#endif // ABSTRACT_SYNTAX_TREE_H
