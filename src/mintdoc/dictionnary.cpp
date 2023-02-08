/**
 * Copyright (c) 2024 Gauvain CHERY.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

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

void Dictionnary::open_module(const string &name) {

	if (m_module) {
		m_path.push(m_module);
	}

	m_module = new Module;
	m_module->type = Module::script;
	m_module->name = name;
	m_modules.push_back(m_module);
}

void Dictionnary::open_module_group(const string &name) {

	if (m_module) {
		m_path.push(m_module);
	}

	m_module = new Module;
	m_module->type = Module::group;
	m_module->name = name;
	m_modules.push_back(m_module);
}

void Dictionnary::close_module() {

	if (m_path.empty()) {
		m_module = nullptr;
	}
	else {
		m_module = m_path.top();
		m_path.pop();
	}
}

void Dictionnary::set_module_doc(const string &doc) {
	if (m_module == nullptr) {
		open_module("main");
	}
	auto license = doc.find("@license");
	if (license == string::npos) {
		m_module->doc = doc;
	}
	else {
		auto module = doc.find("@module");
		if (module == string::npos) {
			m_module->doc = doc.substr(0, license);
		}
		else {
			m_module->doc = doc.substr(module + 7, license > module ? license - module - 7 : string::npos);
		}
	}
}

void Dictionnary::set_package_doc(const string &doc) {

	auto end = string::npos;
	stringstream stream(doc);

	for (auto begin = doc.find("@package"); begin != string::npos; begin = end) {

		string name;

		stream.seekg(static_cast<stringstream::off_type>(begin + 8), stream.beg);
		stream >> name;
		begin = static_cast<decltype (begin)>(stream.tellg());
		end = doc.find("@package", begin);

		Package* package = get_or_create_package(name);
		package->doc = doc.substr(begin, end != string::npos ? end - begin : end);
		m_definitions.erase(name);
		insert_definition(package);
	}
}

void Dictionnary::set_page_doc(const string &name, const string &doc) {
	Page *page = new Page;
	page->name = name;
	page->doc = doc;
	m_pages.push_back(page);
}

void Dictionnary::insert_definition(Definition *definition) {

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

Package* Dictionnary::get_or_create_package(const string &name) const {

	auto i = m_packages.find(name);

	if (i != m_packages.end()) {
		return i->second;
	}

	return new Package(name);
}

Function *Dictionnary::get_or_create_function(const string &name) const {

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
		m_generator->setup_links(this, module);
	}
	
	m_generator->generate_page_list(this, path, m_pages);

	for (Page* page : m_pages) {
		m_generator->generate_page(this, path, page);
	}
	
	m_generator->generate_module_list(this, path, m_modules);

	for (Module *module : m_modules) {
		m_generator->generate_module(this, path, module);
	}

	vector<Package *> packages;
	transform(m_packages.begin(), m_packages.end(), back_inserter(packages), [] (const pair<string, Package *> &package) {
		return package.second;
	});

	m_generator->generate_package_list(this, path, packages);

	for (Package *package : packages) {
		m_generator->generate_package(this, path, package);
	}
}

Dictionnary::TagType Dictionnary::get_tag_type(const string &tag) const {

	static const map<string, TagType> g_tags = {
		{ "module", module_tag },
		{ "see", see_tag }
	};

	auto i = g_tags.find(tag);

	if (i != g_tags.end()) {
		return i->second;
	}

	return no_tag;
}

Module *Dictionnary::find_definition_module(const string &symbol) const {

	auto i = m_definitions.find(symbol);

	if (i != m_definitions.end()) {
		return i->second;
	}

	return nullptr;
}

vector<Module *> Dictionnary::child_modules(Module *module) const {

	vector<Module *> children;

	for (Module *script : m_modules) {
		if (script->name.find(module->name + ".") == 0) {
			children.push_back(script);
		}
	}

	return children;
}

vector<Definition *> Dictionnary::package_definitions(Package *package) const {

	vector<Definition *> definitions;
	definitions.reserve(package->members.size());

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

vector<Definition *> Dictionnary::enum_definitions(Enum *instance) const {

	vector<Definition *> definitions;

	auto module = m_definitions.find(instance->name);
	if (module != m_definitions.end()) {
		definitions.reserve(instance->members.size());
		for (const string& member : instance->members) {
			auto def = module->second->definitions.find(member);
			if (def != module->second->definitions.end()) {
				definitions.push_back(def->second);
			}
		}
	}

	return definitions;
}

vector<Definition *> Dictionnary::class_definitions(Class *instance) const {

	vector<Definition *> definitions;

	auto module = m_definitions.find(instance->name);
	if (module != m_definitions.end()) {
		definitions.reserve(instance->members.size());
		for (const string& member : instance->members) {
			auto def = module->second->definitions.find(member);
			if (def != module->second->definitions.end()) {
				definitions.push_back(def->second);
			}
		}
	}

	return definitions;
}
