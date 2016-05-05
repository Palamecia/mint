#ifndef GLOBAL_DATA_H
#define GLOBAL_DATA_H

#include "symboltable.h"

#include <vector>
#include <string>

class GlobalData {
public:
	static GlobalData &instance();

	int createClass(Class *desc);
	void registerClass(int id);
	Class *getClass(const std::string &name);

	SymbolTable &symbols();

protected:
	GlobalData();

private:
	SymbolTable m_symbols;
	std::vector<Class *> m_definedClasses;
	std::map<std::string, Class *> m_registeredClasses;
};

#endif // GLOBAL_DATA_H
