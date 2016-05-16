#ifndef MODUL_H
#define MODUL_H

#include "instruction.h"

#include "System/datastream.h"

#include <string>
#include <vector>
#include <map>

namespace yy {
class parser;
}

class Modul {
public:
	Modul();
	~Modul();

	struct Context {
		size_t modulId;
		Modul *modul;
	};

	Instruction &at(uint idx);
	char *makeSymbol(const char *name);
	Reference *makeConstant(Data *data);

	static std::map<std::string, Context> cache;

protected:
	void pushInstruction(const Instruction &instruction);
	void replaceInstruction(size_t offset, const Instruction &instruction);
	size_t nextInstructionOffset() const;
	friend class BuildContext;
	friend class yy::parser;

private:
	std::vector<Instruction> m_data;
	std::vector<char *> m_symbols;
	std::vector<Reference *> m_constants;
};

#endif // MODUL_H
