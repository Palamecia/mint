#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "reference.h"

#include <string>
#include <map>

class Class;

class SymbolTable : public std::map<std::string, Reference> {
public:
	SymbolTable();

	Class *metadata;
	Reference defaultResult;
};

#endif // SYMBOL_TABLE_H
