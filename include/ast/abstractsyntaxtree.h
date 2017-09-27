#ifndef ABSTRACT_SYNTAX_TREE_H
#define ABSTRACT_SYNTAX_TREE_H

#include "module.h"

#include <functional>

class Cursor;

class AbstractSyntaxTree {
public:
	AbstractSyntaxTree();
	~AbstractSyntaxTree();

	AbstractSyntaxTree &operator =(const AbstractSyntaxTree &other) = delete;

	typedef std::function<void(Cursor *)> Builtin;

	static std::pair<int, int> createBuiltinMethode(int type, Builtin methode);
	void callBuiltinMethode(int module, int methode, Cursor *cursor);

	Cursor *createCursor(size_t module = Module::MainId);

	Module::Infos createModule();
	Module *getModule(size_t offset);
	DebugInfos *getDebugInfos(size_t offset);
	Module::Infos loadModule(const std::string &module);
	Module::Infos main();

	size_t getModuleId(const Module *module);
	std::string getModuleName(const Module *module);

private:
	static std::map<int, std::map<int, Builtin>> g_builtinMembers;

	std::vector<Module *> m_modules;
	std::vector<DebugInfos *> m_debugInfos;
	std::map<std::string, Module::Infos> m_cache;
};

#endif // ABSTRACT_SYNTAX_TREE_H
