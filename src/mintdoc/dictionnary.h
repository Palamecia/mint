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
		see_tag,
		module_tag
	};

	Dictionnary();
	~Dictionnary();

	Dictionnary(const Dictionnary &other) = delete;
	Dictionnary &operator =(const Dictionnary &other) = delete;

	void open_module(const std::string &name);
	void open_module_group(const std::string &name);
	void close_module();

	void set_module_doc(const std::string &doc);
	void set_package_doc(const std::string &doc);
	void set_page_doc(const std::string &name, const std::string &doc);

	void insert_definition(Definition *definition);

	Package* get_or_create_package(const std::string &name) const;
	Function *get_or_create_function(const std::string &name) const;

	void generate(const std::string &path);

	TagType get_tag_type(const std::string &tag) const;

	Module *find_definition_module(const std::string &symbol) const;
	std::vector<Module *> child_modules(const Module *module) const;

	std::vector<Definition *> package_definitions(const Package *package) const;
	std::vector<Definition *> enum_definitions(const Enum *instance) const;
	std::vector<Definition *> class_definitions(const Class *instance) const;

private:
	std::map<std::string, Module *> m_definitions;
	std::map<std::string, Package *> m_packages;
	std::vector<Module *> m_modules;
	std::vector<Page *> m_pages;
	std::stack<Module *> m_path;
	Module * m_module = nullptr;

	AbstractGenerator *m_generator;
};

#endif // DICTIONNARY_H
