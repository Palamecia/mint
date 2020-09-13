#ifndef DICTIONNARY_H
#define DICTIONNARY_H

#include "module.h"
#include "page.h"

#include <vector>
#include <stack>

class AbstractGenerator;

class Dictionnary {
public:
	enum TagType {
		no_tag,
		see_tag
	};

	Dictionnary();
	~Dictionnary();

	void openModule(const std::string &name);
	void openModuleGroup(const std::string &name);
	void closeModule();

	void setModuleDoc(const std::string &doc);
	void setPackageDoc(const std::string &doc);
	void setPageDoc(const std::string &name, const std::string &doc);

	void insertDefinition(Definition *definition);

	Package* getOrCreatePackage(const std::string &name) const;
	Function *getOrCreateFunction(const std::string &name) const;

	void generate(const std::string &path);

	TagType getTagType(const std::string &tag) const;

	Module *findDefinitionModule(const std::string &symbol) const;
	std::vector<Module *> childModules(Module *module) const;

	std::vector<Definition *> packageDefinitions(Package *package) const;
	std::vector<Definition *> enumDefinitions(Enum *instance) const;
	std::vector<Definition *> classDefinitions(Class *instance) const;

private:
	std::map<std::string, Module *> m_definitions;
	std::map<std::string, Package *> m_packages;
	std::vector<Module *> m_modules;
	std::vector<Page *> m_pages;
	std::stack<Module *> m_path;
	Module * m_module;

	AbstractGenerator *m_generator;
};

#endif // DICTIONNARY_H
