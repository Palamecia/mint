#ifndef GLOBAL_DATA_H
#define GLOBAL_DATA_H

#include "symboltable.h"

#include <vector>
#include <string>
#include <list>

namespace mint {

class MINT_EXPORT ClassDescription {
public:
	ClassDescription(Class *desc);

	std::string name() const;

	void addParent(const std::string &name);
	void addSubClass(const ClassDescription &desc);

	bool createMember(const std::string &name, SharedReference value);
	bool updateMember(const std::string &name, SharedReference value);

	Class *generate();

	void clean();

private:
	Class *m_desc;
	std::list<std::string> m_parents;
	std::map<std::string, SharedReference> m_members;
	std::map<std::string, SharedReference> m_globals;
	std::vector<ClassDescription> m_subClasses;
	bool m_generated;
};

class MINT_EXPORT ClassRegister {
public:
	ClassRegister();
	virtual ~ClassRegister();

	int createClass(const ClassDescription &desc);
	void registerClass(int id);
	Class *getClass(const std::string &name);

private:
	std::vector<ClassDescription> m_definedClasses;
	std::map<std::string, Class *> m_registeredClasses;
};

class MINT_EXPORT GlobalData : public ClassRegister {
public:
	static GlobalData &instance();
	~GlobalData();

	SymbolTable &symbols();

protected:
	GlobalData();

private:
	SymbolTable m_symbols;
};

}

#endif // GLOBAL_DATA_H
