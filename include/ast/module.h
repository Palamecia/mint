#ifndef MODULE_H
#define MODULE_H

#include "ast/instruction.h"
#include "ast/debuginfos.h"

#include "system/datastream.h"

#include <string>
#include <vector>
#include <map>

namespace yy {
class parser;
}

class Module {
public:
	Module();
	~Module();

	struct Context {
		size_t moduleId;
		Module *module;
		DebugInfos *debugInfos;
	};

	Instruction &at(size_t idx);
	char *makeSymbol(const char *name);
	Reference *makeConstant(Data *data);

	static Module *get(size_t offset);
	static DebugInfos *debug(size_t offset);
	static Context load(const std::string &module);
	static Context create();
	static Context main();
	static void clearCache();

	static size_t id(const Module *module);
	static std::string name(const Module *module);

protected:
	void pushInstruction(const Instruction &instruction);
	void replaceInstruction(size_t offset, const Instruction &instruction);
	size_t nextInstructionOffset() const;
	friend class DebugInfos;
	friend class BuildContext;
	friend class yy::parser;

private:
	static std::vector<Module *> g_modules;
	static std::vector<DebugInfos *> g_debugInfos;
	static std::map<std::string, Context> g_cache;

	std::vector<Instruction> m_data;
	std::vector<char *> m_symbols;
	std::vector<Reference *> m_constants;
};

#endif // MODULE_H
