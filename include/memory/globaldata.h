#ifndef GLOBAL_DATA_H
#define GLOBAL_DATA_H

#include <ast/symbolmapping.hpp>
#include <memory/symboltable.h>

#include <vector>
#include <string>
#include <list>

namespace mint {

class MINT_EXPORT ClassDescription {
	ClassDescription(const ClassDescription &) = delete;
	ClassDescription &operator =(const ClassDescription &) = delete;
public:
	ClassDescription(PackageData *package, Reference::Flags flags, const std::string &name);
	virtual ~ClassDescription();

	class MINT_EXPORT Path {
	public:
		ClassDescription *locate(PackageData *package) const;
		std::string toString() const;

		void appendSymbol(const Symbol &symbol);
		void clear();

	private:
		std::list<Symbol> m_symbols;
	};

	Symbol name() const;
	std::string fullName() const;
	Reference::Flags flags() const;

	void addBase(const Path &base);
	void addSubClass(ClassDescription *desc);

	bool createMember(const Symbol &name, Reference &&value);
	bool updateMember(const Symbol &name, Reference &&value);

	ClassDescription *findSubClass(const Symbol &name) const;

	Class *generate();

private:
	ClassDescription *m_owner;
	PackageData *m_package;
	Reference::Flags m_flags;
	std::list<Path> m_bases;
	Symbol m_name;
	Class *m_metadata;
	std::list<ClassDescription *> m_subClasses;
	SymbolMapping<StrongReference> m_members;
	SymbolMapping<StrongReference> m_globals;
};

class MINT_EXPORT ClassRegister {
	ClassRegister(const ClassRegister &) = delete;
	ClassRegister &operator =(const ClassRegister &) = delete;
public:
	ClassRegister();
	virtual ~ClassRegister();

	int createClass(ClassDescription *desc);
	virtual void registerClass(int id) = 0;

	ClassDescription *findClassDescription(const Symbol &name) const;
	ClassDescription *getClassDescription(int id);

private:
	std::vector<ClassDescription *> m_definedClasses;
};

class MINT_EXPORT PackageData : public ClassRegister {
public:
	PackageData *getPackage(const Symbol &name);
	PackageData *findPackage(const Symbol &name) const;

	void registerClass(int id) override;
	Class *getClass(const Symbol &name);

	std::string name() const;
	std::string fullName() const;
	inline SymbolTable &symbols();

	void cleanup();

protected:
	PackageData(const std::string &name, PackageData *owner = nullptr);
	~PackageData();

private:
	std::string m_name;
	PackageData *m_owner;
	SymbolMapping<PackageData *> m_packages;
	SymbolMapping<Class *> m_classes;
	SymbolTable m_symbols;
};

class MINT_EXPORT GlobalData : public PackageData {
public:
	static GlobalData &instance();

protected:
	GlobalData();
	~GlobalData();
};

SymbolTable &PackageData::symbols() { return m_symbols; }

}

#endif // GLOBAL_DATA_H
