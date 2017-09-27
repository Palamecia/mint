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

	static constexpr size_t MainId = 0;

	struct Infos {
		size_t id;
		Module *module;
		DebugInfos *debugInfos;
	};

	Instruction &at(size_t idx);
	size_t end() const;
	char *makeSymbol(const char *name);
	Reference *makeConstant(Data *data);

protected:
	void pushInstruction(const Instruction &instruction);
	void replaceInstruction(size_t offset, const Instruction &instruction);
	size_t nextInstructionOffset() const;
	friend class DebugInfos;
	friend class BuildContext;
	friend class yy::parser;

private:
	std::vector<Instruction> m_data;
	std::vector<char *> m_symbols;
	std::vector<Reference *> m_constants;
};

#endif // MODULE_H
