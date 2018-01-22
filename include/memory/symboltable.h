#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "reference.h"

#include <string>
#include <map>

namespace mint {

class Class;

class MINT_EXPORT SymbolTable : public std::map<std::string, Reference> {
public:
	SymbolTable(Class *metadata = nullptr);

	Class *getMetadata() const;

	Reference &defaultResult();

private:
	Class *m_metadata;
	Reference m_defaultResult;
};

}

#endif // SYMBOL_TABLE_H
