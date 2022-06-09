#include "dictionnary.h"
#include "definition.h"

#include "generators/gollumgenerator.h"

#include <algorithm>
#include <iterator>
#include <sstream>
#include <memory>

using namespace std;

Dictionnary::Dictionnary() :
	m_generator(new GollumGenerator) {

}

Dictionnary::~Dictionnary() {

	for_each(m_definitions.begin(), m_definitions.end(), [] (const pair<string, Module *> &item) {
		delete item.second->definitions.at(item.first);
	});

	for_each(m_modules.begin(), m_modules.end(), default_delete<Module>());
	for_each(m_pages.begin(), m_pages.end(), default_delete<Page>());
	delete m_generator;
}

void Dictionnary::openModule(const string &name) {

	if (m_module) {
		m_path.push(m_module);
	}

	m_module = new Module;
	m_module->type = Module::script;
	m_module->name = name;
	m_modules.push_back(m_module);
}

void Dictionnary::openModuleGroup(const string &name) {

	if (m_module) {
		m_path.push(m_module);
	}

	m_module = new Module;
	m_module->type = Module::group;
	m_module->name = name;
	m_modules.push_back(m_module);
}

void Dictionnary::closeModule() {

	if (m_path.empty()) {
		m_module = nullptr;
	}
	else {
		m_module = m_path.top();
		m_path.pop();
	}
}

void Dictionnary::setModuleDoc(const string &doc) {
	if (m_module == nullptr) {
		openModule("main");
	}
	m_module->doc = doc;
}

void Dictionnary::setPackageDoc(const string &doc) {

	auto end = string::npos;
	stringstream stream(doc);

	for (auto begin = doc.find("@package"); begin != string::npos; begin = end) {

		string name;

		stream.seekg(static_cast<stringstream::off_type>(begin + 8), stream.beg);
		stream >> name;
		begin = static_cast<decltype (begin)>(stream.tellg());
		end = doc.find("@package", begin);

		Package* package = getOrCreatePackage(name);
		package->doc = doc.substr(begin, end != string::npos ? end - begin : end);
		m_definitions.erase(name);
		insertDefinition(package);
	}
}

void Dictionnary::setPageDoc(const string &name, const string &doc) {
	Page *page = new Page;
	page->name = name;
	page->doc = doc;
	m_pages.push_back(page);
}

void Dictionnary::insertDefinition(Definition *definition) {

	m_definitions.emplace(definition->name, m_module);
	m_module->definitions.emplace(definition->name, definition);

	switch (definition->type) {
	case Definition::package_definition:
		m_module->elements[definition->type].emplace(definition->name, definition);
		m_packages.emplace(definition->name, static_cast<Package *>(definition));
		break;

	case Definition::constant_definition:
	case Definition::function_definition:
		if (definition->name.find('.') == string::npos) {
			m_module->elements[definition->type].emplace(definition->name, definition);
		}
		else {
			if (m_module->definitions.at(definition->context())->type == Definition::package_definition) {
				m_module->elements[definition->type].emplace(definition->name, definition);
			}
		}
		break;

	default:
		m_module->elements[definition->type].emplace(definition->name, definition);
		break;
	}
}

Package* Dictionnary::getOrCreatePackage(const string &name) const {

	auto i = m_packages.find(name);

	if (i != m_packages.end()) {
		return i->second;
	}

	return new Package(name);
}

Function *Dictionnary::getOrCreateFunction(const string &name) const {

	auto i = m_module->definitions.find(name);

	if (i != m_module->definitions.end()) {
		switch (i->second->type) {
		case Definition::function_definition:
			return static_cast<Function *>(i->second);

		default:
			return nullptr;
		}
	}

	return new Function(name);
}

void Dictionnary::generate(const string &path) {

	sort(m_modules.begin(), m_modules.end(), [] (Module *left, Module *right) {
		return left->name < right->name;
	});

	for (Module *module : m_modules) {
		m_generator->setupLinks(this, module);
	}

	m_generator->generatePageList(this, path, m_pages);

	for (Page* page : m_pages) {
		m_generator->generatePage(this, path, page);
	}

	m_generator->generateModuleList(this, path, m_modules);

	for (Module *module : m_modules) {
		m_generator->generateModule(this, path, module);
	}

	vector<Package *> packages;
	transform(m_packages.begin(), m_packages.end(), back_inserter(packages), [] (const pair<string, Package *> &package) {
		return package.second;
	});

	m_generator->generatePackageList(this, path, packages);

	for (Package *package : packages) {
		m_generator->generatePackage(this, path, package);
	}
}

Dictionnary::TagType Dictionnary::getTagType(const string &tag) const {

	static const map<string, TagType> g_tags = {
		{ "see", see_tag }
	};

	auto i = g_tags.find(tag);

	if (i != g_tags.end()) {
		return i->second;
	}

	return no_tag;
}

Module *Dictionnary::findDefinitionModule(const string &symbol) const {

	auto i = m_definitions.find(symbol);

	if (i != m_definitions.end()) {
		return i->second;
	}

	return nullptr;
}

vector<Module *> Dictionnary::childModules(Module *module) const {

	vector<Module *> children;

	for (Module *script : m_modules) {
		if (script->name.find(module->name + ".") == 0) {
			children.push_back(script);
		}
	}

	return children;
}

vector<Definition *> Dictionnary::packageDefinitions(Package *package) const {

	vector<Definition *> definitions;

	for (const string& member : package->members) {
		auto module = m_definitions.find(member);
		if (module != m_definitions.end()) {
			auto def = module->second->definitions.find(member);
			if (def != module->second->definitions.end()) {
				definitions.push_back(def->second);
			}
		}
	}

	return definitions;
}

vector<Definition *> Dictionnary::enumDefinitions(Enum *instance) const {

	vector<Definition *> definitions;

	auto module = m_definitions.find(instance->name);
	if (module != m_definitions.end()) {
		for (const string& member : instance->members) {
			auto def = module->second->definitions.find(member);
			if (def != module->second->definitions.end()) {
				definitions.push_back(def->second);
			}
		}
	}

	return definitions;
}

vector<Definition *> Dictionnary::classDefinitions(Class *instance) const {

	vector<Definition *> definitions;

	auto module = m_definitions.find(instance->name);
	if (module != m_definitions.end()) {
		for (const string& member : instance->members) {
			auto def = module->second->definitions.find(member);
			if (def != module->second->definitions.end()) {
				definitions.push_back(def->second);
			}
		}
	}

	return definitions;
}
