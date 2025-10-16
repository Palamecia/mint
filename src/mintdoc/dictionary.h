/**
 * Copyright (c) 2026 Gauvain CHERY.
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

#ifndef MINTDOC_DICTIONARY_H
#define MINTDOC_DICTIONARY_H

#include "definition.h"
#include "generators/abstractgenerator.h"
#include "module.h"
#include "page.h"

#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <stack>

class Dictionary {
public:
	Dictionary();

	void open_module(const std::string& name);
	void open_module_group(const std::string& name);
	void close_module();

	void set_module_doc(const std::string& doc);
	void set_package_doc(const std::string& doc);
	void set_page_doc(const std::string& name, const std::string& doc);

	[[nodiscard]] std::shared_ptr<Package> get_or_create_package(const std::string& name) const;
	[[nodiscard]] std::shared_ptr<Function> get_or_create_function(const std::string& name) const;
	void insert_definition(const std::shared_ptr<Definition>& definition);

	void generate(const std::filesystem::path& path);

	[[nodiscard]] Module* find_definition_module(const std::string& symbol) const;
	[[nodiscard]] std::vector<std::reference_wrapper<const Module>> child_modules(const Module& module) const;

	[[nodiscard]] std::vector<std::reference_wrapper<const Definition>> package_definitions(
	    const Package& package) const;
	[[nodiscard]] std::vector<std::reference_wrapper<const Definition>> enum_definitions(const Enum& instance) const;
	[[nodiscard]] std::vector<std::reference_wrapper<const Definition>> class_definitions(const Class& instance) const;

private:
	std::map<std::string, Module*> _definitions;
	std::map<std::string, std::shared_ptr<Package>> _packages;
	std::vector<std::unique_ptr<Module>> _modules;
	std::vector<std::unique_ptr<Page>> _pages;
	std::stack<Module*> _path;
	Module* _module = nullptr;

	std::unique_ptr<AbstractGenerator> _generator;
};

#endif // MINTDOC_DICTIONARY_H
