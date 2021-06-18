#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "ast/symbol.h"
#include "reference.h"

#include <string>
#include <stack>
#include <unordered_map>
#include <map>

namespace mint {

class Class;
struct Iterator;
class PackageData;

class MINT_EXPORT SymbolTable {
public:
	using symbol_type = std::pair<Symbol, Reference>;
	using weak_symbol_type = std::pair<Symbol, WeakReference>;
	using strong_symbol_type = std::pair<Symbol, StrongReference>;

	using iterator = std::unordered_map<Symbol, StrongReference>::iterator;
	using const_iterator = std::unordered_map<Symbol, StrongReference>::const_iterator;

	SymbolTable(Class *metadata = nullptr);
	~SymbolTable();

	Class *getMetadata() const;
	PackageData *getPackage() const;

	Reference &defaultResult();

	void openPackage(PackageData *package);
	void closePackage();

	Iterator *generator();

	Reference &operator [](const Symbol &name);
	size_t size() const;
	bool empty() const;

	const_iterator find(const Symbol &name) const;
	iterator find(const Symbol &name);
	const_iterator begin() const;
	const_iterator end() const;
	iterator begin();
	iterator end();

	std::pair<iterator, bool> emplace(const Symbol &name, Reference &&reference);
	std::pair<iterator, bool> emplace(const Symbol &name, Reference &reference);
	std::pair<iterator, bool> insert(const strong_symbol_type &symbol);
	std::pair<iterator, bool> insert(const weak_symbol_type &symbol);
	std::pair<iterator, bool> insert(const symbol_type &symbol);
	size_t erase(const Symbol &name);
	iterator erase(iterator position);
	void clear();

private:
	Class *m_metadata;
	std::stack<PackageData *> m_package;

	StrongReference m_defaultResult;
	std::unordered_map<Symbol, StrongReference> m_symbols;
};

}

#endif // SYMBOL_TABLE_H
