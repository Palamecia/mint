#ifndef GLOBAL_DATA_H
#define GLOBAL_DATA_H

#include "symboltable.h"

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

	class Path : public std::list<std::string> {
	public:
		ClassDescription *locate(PackageData *package) const;
		std::string toString() const;
	};

	std::string name() const;
	std::string fullName() const;
	Reference::Flags flags() const;

	void addBase(const Path &base);
	void addSubClass(ClassDescription *desc);

	bool createMember(const std::string &name, SharedReference value);
	bool updateMember(const std::string &name, SharedReference value);

	ClassDescription *findSubClass(const std::string &name) const;

	Class *generate();

private:
	ClassDescription *m_owner;
	PackageData *m_package;
	Reference::Flags m_flags;
	std::list<Path> m_bases;
	std::string m_name;
	Class *m_metadata;
	std::list<ClassDescription *> m_subClasses;
	std::map<std::string, SharedReference> m_members;
	std::map<std::string, SharedReference> m_globals;
};

class MINT_EXPORT ClassRegister {
	ClassRegister(const ClassRegister &) = delete;
	ClassRegister &operator =(const ClassRegister &) = delete;
public:
	ClassRegister();
	virtual ~ClassRegister();

	int createClass(ClassDescription *desc);
	virtual void registerClass(int id) = 0;

	ClassDescription *findClassDescription(const std::string &name) const;

protected:
	ClassDescription *getDefinedClass(int id);

private:
	std::vector<ClassDescription *> m_definedClasses;
};

class MINT_EXPORT PackageData : public ClassRegister {
public:
	PackageData *getPackage(const std::string &name);
	PackageData *findPackage(const std::string &name) const;

	void registerClass(int id) override;
	Class *getClass(const std::string &name);

	std::string name() const;
	std::string fullName() const;
	SymbolTable &symbols();

	void clearGlobalReferences();

protected:
	PackageData(const std::string &name, PackageData *owner = nullptr);
	~PackageData();

private:
	std::string m_name;
	PackageData *m_owner;
	std::map<std::string, PackageData *> m_packages;
	std::map<std::string, Class *> m_classes;
	SymbolTable m_symbols;
};

class MINT_EXPORT GlobalData : public PackageData {
public:
	static GlobalData &instance();

protected:
	GlobalData();
	~GlobalData();
};

}

#endif // GLOBAL_DATA_H
