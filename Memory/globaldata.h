#ifndef GLOBAL_DATA_H
#define GLOBAL_DATA_H

#include "symboltable.h"

#include <string>

class GlobalData {
public:
	static GlobalData &instance();

	void registerClass(const std::string &name, size_t id);
	void registerSymbol(const std::string &name, size_t id);

	SymbolTable &symbols();

protected:
	GlobalData();

private:
	SymbolTable m_symbols;
};

#endif // GLOBAL_DATA_H
