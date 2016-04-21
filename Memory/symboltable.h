#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "reference.h"

#include <string>
#include <map>

class SymbolTable : public std::map<std::string, Reference> {
public:

};

#endif // SYMBOL_TABLE_H
