#ifndef MODUL_H
#define MODUL_H

#include "instruction.h"

#include "System/datastream.h"

#include <string>
#include <vector>
#include <map>

class Modul {
public:
	Modul();
	~Modul();

	Instruction &at(uint idx);
	char *makeSymbol(const char *name);
	Reference *makeConstant(Data *data);

	static std::map<std::string, Modul> cache;

protected:
	void pushInstruction(const Instruction &instruction);
	void replaceInstruction(size_t offset, const Instruction &instruction);
	size_t nextInstructionOffset() const;
	friend class BuildContext;

private:
	std::vector<Instruction> m_data;
	std::vector<char *> m_symbols;
	std::vector<Reference *> m_constants;
};

#endif // MODUL_H
