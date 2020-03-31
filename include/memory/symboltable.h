#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "reference.h"

#include <string>
#include <stack>
#include <map>

namespace mint {

class Class;
struct Iterator;
class PackageData;

class MINT_EXPORT SymbolTable {
public:
	using iterator = std::map<std::string, StrongReference>::iterator;
	using const_iterator = std::map<std::string, StrongReference>::const_iterator;
	using strong_symbol_type = std::map<std::string, StrongReference>::value_type;
	using weak_symbol_type = std::map<std::string, WeakReference>::value_type;
	using symbol_type = std::map<std::string, Reference>::value_type;

	SymbolTable(Class *metadata = nullptr);
	~SymbolTable();

	Class *getMetadata() const;
	PackageData *getPackage() const;

	Reference &defaultResult();
	ReferenceManager *referenceManager();

	void openPackage(PackageData *package);
	void closePackage();

	Iterator *generator();

	Reference &operator [](const std::string &name);
	size_t size() const;
	bool empty() const;

	const_iterator find(const std::string &name) const;
	iterator find(const std::string &name);
	const_iterator begin() const;
	const_iterator end() const;
	iterator begin();
	iterator end();

	std::pair<iterator, bool> emplace(const std::string &name, Reference &&reference);
	std::pair<iterator, bool> emplace(const std::string &name, Reference &reference);
	std::pair<iterator, bool> insert(const strong_symbol_type &symbol);
	std::pair<iterator, bool> insert(const weak_symbol_type &symbol);
	std::pair<iterator, bool> insert(const symbol_type &symbol);
	size_t erase(const std::string &name);
	iterator erase(iterator position);
	void clear();

private:
	Class *m_metadata;
	std::stack<PackageData *> m_package;

	/* Do not change members declaration order. Members destructor will be called in reverse order
	 * of declataion. The ReferenceManager's destructor must be called first to unlink references
	 * before symbols and default result are deleted.
	 */
	StrongReference m_defaultResult;
	std::map<std::string, StrongReference> m_symbols;
	ReferenceManager m_referenceManager;
};

}

#endif // SYMBOL_TABLE_H
