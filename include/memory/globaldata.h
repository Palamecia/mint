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
	ClassDescription(Class *metadata);
	virtual ~ClassDescription();

	class Path : public std::list<std::string> {
	public:
		ClassDescription *locate(PackageData *package) const;
		std::string toString() const;
	};

	std::string name() const;

	void addParent(const Path &parent);
	void addSubClass(ClassDescription *desc);

	bool createMember(const std::string &name, SharedReference value);
	bool updateMember(const std::string &name, SharedReference value);

	ClassDescription *findSubClass(const std::string &name) const;

	Class *generate();

private:
	Class *m_metadata;
	std::list<Path> m_parents;
	std::list<ClassDescription *> m_subClasses;
	std::map<std::string, SharedReference> m_members;
	std::map<std::string, SharedReference> m_globals;
	bool m_generated;
};

class MINT_EXPORT ClassRegister {
	ClassRegister(const ClassRegister &) = delete;
	ClassRegister &operator =(const ClassRegister &) = delete;
public:
	ClassRegister();
	virtual ~ClassRegister();

	int createClass(ClassDescription *desc);
	void registerClass(int id);
	Class *getClass(const std::string &name);

	ClassDescription *findClassDescription(const std::string &name) const;

private:
	std::vector<ClassDescription *> m_definedClasses;
	std::map<std::string, Class *> m_registeredClasses;
};

class MINT_EXPORT PackageData : public ClassRegister {
public:
	PackageData *getPackage(const std::string &name);

	std::string name() const;
	SymbolTable &symbols();

protected:
	PackageData(const std::string &name);
	~PackageData();

private:
	std::string m_name;
	std::map<std::string, PackageData *> m_packages;
	SymbolTable m_symbols;
};

class MINT_EXPORT GlobalData : public PackageData {
public:
	static GlobalData &instance();

protected:
	GlobalData();
};

}

#endif // GLOBAL_DATA_H
