#ifndef ABSTRACTGENERATOR_H
#define ABSTRACTGENERATOR_H

#include "dictionnary.h"

class AbstractGenerator {
public:
	AbstractGenerator();
	virtual ~AbstractGenerator();

	virtual void setupLinks(Dictionnary *dictionnary, Module *module) = 0;

	virtual void generateModuleList(Dictionnary *dictionnary, const std::string &path, const std::vector<Module *> modules) = 0;
	virtual void generateModule(Dictionnary *dictionnary, const std::string &path, Module *module) = 0;

	virtual void generatePackageList(Dictionnary *dictionnary, const std::string &path, const std::vector<Package *> packages) = 0;
	virtual void generatePackage(Dictionnary *dictionnary, const std::string &path, Package *package) = 0;

	virtual void generatePageList(Dictionnary *dictionnary, const std::string &path, const std::vector<Page *> pages) = 0;
	virtual void generatePage(Dictionnary *dictionnary, const std::string &path, Page *page) = 0;
};

#endif // ABSTRACTGENERATOR_H
