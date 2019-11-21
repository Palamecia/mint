#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "reference.h"

#include <string>
#include <stack>
#include <map>

namespace mint {

class Class;
class Iterator;
class PackageData;

class MINT_EXPORT SymbolTable : public std::map<std::string, Reference> {
public:
	SymbolTable(Class *metadata = nullptr);

	Class *getMetadata() const;
	PackageData *getPackage() const;

	Reference &defaultResult();
	ReferenceManager *referenceManager();

	void openPackage(PackageData *package);
	void closePackage();

	Iterator *generator();

private:
	Class *m_metadata;
	std::stack<PackageData *> m_package;
	ReferenceManager m_referenceManager;
	Reference m_defaultResult;
};

}

#endif // SYMBOL_TABLE_H
