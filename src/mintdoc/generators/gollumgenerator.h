#ifndef GOLLUMGENERATOR_H
#define GOLLUMGENERATOR_H

#include "abstractgenerator.h"

class GollumGenerator : public AbstractGenerator {
public:
	GollumGenerator();

	void setupLinks(Dictionnary *dictionnary, Module *module) override;

	void generateModuleList(Dictionnary *dictionnary, const std::string &path, const std::vector<Module *> modules) override;
	void generateModule(Dictionnary *dictionnary, const std::string &path, Module *module) override;

	void generatePackageList(Dictionnary *dictionnary, const std::string &path, const std::vector<Package *> packages) override;
	void generatePackage(Dictionnary *dictionnary, const std::string &path, Package *package) override;

	void generatePageList(Dictionnary *dictionnary, const std::string &path, const std::vector<Page *> pages) override;
	void generatePage(Dictionnary *dictionnary, const std::string &path, Page *page) override;

private:
	std::string docFromMintdoc(Dictionnary *dictionnary, std::stringstream &stream, Definition *context = nullptr) const;
	std::string docFromMintdoc(Dictionnary *dictionnary, const std::string &doc, Definition *context = nullptr) const;
	std::string definitionBrief(Dictionnary *dictionnary, Definition *definition) const;

	void generateModule(Dictionnary *dictionnary, FILE *file, Module *module);
	void generateModuleGroup(Dictionnary *dictionnary, FILE *file, Module *module);
	void generatePackage(Dictionnary *dictionnary, FILE *file, Package *package);
};

#endif // GOLLUMGENERATOR_H
