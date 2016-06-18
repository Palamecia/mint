#ifndef GLOBAL_DATA_H
#define GLOBAL_DATA_H

#include "symboltable.h"

#include <vector>
#include <string>
#include <list>

class ClassDescription {
public:
	ClassDescription(Class *desc);

	std::string name() const;

	void addParent(const std::string &name);
	void addMember(const std::string &name, SharedReference value);

	Class *generate();

	void clean();

private:
	Class *m_desc;
	std::list<std::string> m_parents;
	std::map<std::string, SharedReference> m_members;
};

class GlobalData {
public:
	static GlobalData &instance();
	~GlobalData();

	int createClass(const ClassDescription &desc);
	void registerClass(int id);
	Class *getClass(const std::string &name);

	SymbolTable &symbols();

protected:
	GlobalData();

private:
	SymbolTable m_symbols;
	std::vector<ClassDescription> m_definedClasses;
	std::map<std::string, Class *> m_registeredClasses;
};

#endif // GLOBAL_DATA_H
