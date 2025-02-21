/**
 * Copyright (c) 2025 Gauvain CHERY.
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

#include "dictionary.h"
#include "definition.h"

#include "generators/gollumgenerator.h"

#include <algorithm>
#include <iterator>
#include <sstream>
#include <memory>

Dictionary::Dictionary() :
	m_generator(new GollumGenerator) {}

Dictionary::~Dictionary() {

	std::for_each(m_definitions.begin(), m_definitions.end(), [](const std::pair<std::string, Module *> &item) {
		delete item.second->definitions.at(item.first);
	});

	std::for_each(m_modules.begin(), m_modules.end(), std::default_delete<Module>());
	std::for_each(m_pages.begin(), m_pages.end(), std::default_delete<Page>());
	delete m_generator;
}

void Dictionary::open_module(const std::string &name) {

	if (m_module) {
		m_path.push(m_module);
	}

	m_module = new Module;
	m_module->type = Module::SCRIPT;
	m_module->name = name;
	m_modules.push_back(m_module);
}

void Dictionary::open_module_group(const std::string &name) {

	if (m_module) {
		m_path.push(m_module);
	}

	m_module = new Module;
	m_module->type = Module::GROUP;
	m_module->name = name;
	m_modules.push_back(m_module);
}

void Dictionary::close_module() {

	if (m_path.empty()) {
		m_module = nullptr;
	}
	else {
		m_module = m_path.top();
		m_path.pop();
	}
}

void Dictionary::set_module_doc(const std::string &doc) {
	if (m_module == nullptr) {
		open_module("main");
	}
	auto license = doc.find("@license");
	if (license == std::string::npos) {
		m_module->doc = doc;
	}
	else {
		auto module = doc.find("@module");
		if (module == std::string::npos) {
			m_module->doc = doc.substr(0, license);
		}
		else {
			m_module->doc = doc.substr(module + 7, license > module ? license - module - 7 : std::string::npos);
		}
	}
}

void Dictionary::set_package_doc(const std::string &doc) {

	auto end = std::string::npos;
	std::stringstream stream(doc);

	for (auto begin = doc.find("@package"); begin != std::string::npos; begin = end) {

		std::string name;

		stream.seekg(static_cast<std::stringstream::off_type>(begin + 8), std::stringstream::beg);
		stream >> name;
		begin = static_cast<decltype(begin)>(stream.tellg());
		end = doc.find("@package", begin);

		Package *package = get_or_create_package(name);
		package->doc = doc.substr(begin, end != std::string::npos ? end - begin : end);
		m_definitions.erase(name);
		insert_definition(package);
	}
}

void Dictionary::set_page_doc(const std::string &name, const std::string &doc) {
	Page *page = new Page;
	page->name = name;
	page->doc = doc;
	m_pages.push_back(page);
}

void Dictionary::insert_definition(Definition *definition) {

	m_definitions.emplace(definition->name, m_module);
	m_module->definitions.emplace(definition->name, definition);

	switch (definition->type) {
	case Definition::PACKAGE_DEFINITION:
		m_module->elements[definition->type].emplace(definition->name, definition);
		m_packages.emplace(definition->name, static_cast<Package *>(definition));
		break;

	case Definition::CONSTANT_DEFINITION:
	case Definition::FUNCTION_DEFINITION:
		if (definition->name.find('.') == std::string::npos) {
			m_module->elements[definition->type].emplace(definition->name, definition);
		}
		else {
			if (m_module->definitions.at(definition->context())->type == Definition::PACKAGE_DEFINITION) {
				m_module->elements[definition->type].emplace(definition->name, definition);
			}
		}
		break;

	default:
		m_module->elements[definition->type].emplace(definition->name, definition);
		break;
	}
}

Package *Dictionary::get_or_create_package(const std::string &name) const {

	auto i = m_packages.find(name);

	if (i != m_packages.end()) {
		return i->second;
	}

	return new Package(name);
}

Function *Dictionary::get_or_create_function(const std::string &name) const {

	auto i = m_module->definitions.find(name);

	if (i != m_module->definitions.end()) {
		switch (i->second->type) {
		case Definition::FUNCTION_DEFINITION:
			return static_cast<Function *>(i->second);

		default:
			return nullptr;
		}
	}

	return new Function(name);
}

void Dictionary::generate(const std::filesystem::path &path) {

	sort(m_modules.begin(), m_modules.end(), [](const Module *left, const Module *right) {
		return left->name < right->name;
	});

	for (Module *module : m_modules) {
		m_generator->setup_links(this, module);
	}

	m_generator->generate_page_list(this, path, m_pages);

	for (Page *page : m_pages) {
		m_generator->generate_page(this, path, page);
	}

	m_generator->generate_module_list(this, path, m_modules);

	for (Module *module : m_modules) {
		m_generator->generate_module(this, path, module);
	}

	std::vector<Package *> packages;
	transform(m_packages.begin(), m_packages.end(), back_inserter(packages),
			  [](const std::pair<std::string, Package *> &package) {
				  return package.second;
			  });

	m_generator->generate_package_list(this, path, packages);

	for (Package *package : packages) {
		m_generator->generate_package(this, path, package);
	}
}

Dictionary::TagType Dictionary::get_tag_type(const std::string &tag) const {

	static const std::map<std::string, TagType> g_tags = {
		{"module", MODULE_TAG},
		{"see", SEE_TAG},
	};

	auto i = g_tags.find(tag);

	if (i != g_tags.end()) {
		return i->second;
	}

	return NO_TAG;
}

Module *Dictionary::find_definition_module(const std::string &symbol) const {

	auto i = m_definitions.find(symbol);

	if (i != m_definitions.end()) {
		return i->second;
	}

	return nullptr;
}

std::vector<Module *> Dictionary::child_modules(const Module *module) const {

	std::vector<Module *> children;

	std::copy_if(m_modules.begin(), m_modules.end(), std::back_inserter(children), [module](Module *script) {
		return script->name.find(module->name + ".") == 0;
	});

	return children;
}

std::vector<Definition *> Dictionary::package_definitions(const Package *package) const {

	std::vector<Definition *> definitions;
	definitions.reserve(package->members.size());

	for (const std::string &member : package->members) {
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

std::vector<Definition *> Dictionary::enum_definitions(const Enum *instance) const {

	std::vector<Definition *> definitions;

	auto module = m_definitions.find(instance->name);
	if (module != m_definitions.end()) {
		definitions.reserve(instance->members.size());
		for (const std::string &member : instance->members) {
			auto def = module->second->definitions.find(member);
			if (def != module->second->definitions.end()) {
				definitions.push_back(def->second);
			}
		}
	}

	return definitions;
}

std::vector<Definition *> Dictionary::class_definitions(const Class *instance) const {

	std::vector<Definition *> definitions;

	auto module = m_definitions.find(instance->name);
	if (module != m_definitions.end()) {
		definitions.reserve(instance->members.size());
		for (const std::string &member : instance->members) {
			auto def = module->second->definitions.find(member);
			if (def != module->second->definitions.end()) {
				definitions.push_back(def->second);
			}
		}
	}

	return definitions;
}
