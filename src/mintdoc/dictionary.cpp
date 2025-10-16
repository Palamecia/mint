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

#include "dictionary.h"
#include "definition.h"
#include "docnode.h"

#include "generators/gollumgenerator.h"
#include "module.h"
#include "page.h"
#include "utils.h"

#include <algorithm>
#include <filesystem>
#include <functional>
#include <ranges>
#include <sstream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

Dictionary::Dictionary() :
    _generator(std::make_unique<GollumGenerator>()) {}

void Dictionary::open_module(const std::string& name) {

	if (_module) {
		_path.push(_module);
	}

	_module = _modules
	              .emplace_back(std::make_unique<Module>(Module {
	                  .type = Module::script,
	                  .name = name,
	              }))
	              .get();
}

void Dictionary::open_module_group(const std::string& name) {

	if (_module) {
		_path.push(_module);
	}

	_module = _modules
	              .emplace_back(std::make_unique<Module>(Module {
	                  .type = Module::group,
	                  .name = name,
	              }))
	              .get();
}

void Dictionary::close_module() {
	if (_path.empty()) {
		_module = nullptr;
	}
	else {
		_module = _path.top();
		_path.pop();
	}
}

void Dictionary::set_module_doc(const std::string& doc) {

	if (_module == nullptr) {
		open_module("main");
	}

	constexpr std::string_view license_tag = "@license";
	constexpr std::string_view module_tag = "@module";

	if (auto license = doc.find(license_tag); license == std::string::npos) {
		_module->doc = parse_doc(doc);
	}
	else if (auto module = doc.find(module_tag); module == std::string::npos) {
		_module->doc = parse_doc(doc.substr(0, license));
	}
	else {
		const auto offset = module + module_tag.length();
		const auto length = license > module ? license - module - module_tag.length() : std::string::npos;
		_module->doc = parse_doc(doc.substr(offset, length));
	}
}

void Dictionary::set_package_doc(const std::string& doc) {

	auto end = std::string::npos;
	auto stream = std::stringstream(doc);

	constexpr std::string_view package_tag = "@package";

	for (auto begin = doc.find(package_tag); begin != std::string::npos; begin = end) {

		std::string name;

		stream.seekg(static_cast<std::stringstream::off_type>(begin + package_tag.length()), std::stringstream::beg);
		stream >> name;
		begin = static_cast<decltype(begin)>(stream.tellg());
		end = doc.find(package_tag, begin);

		auto package = get_or_create_package(name);
		package->doc = parse_doc(doc.substr(begin, end != std::string::npos ? end - begin : end));
		_definitions.erase(name);
		insert_definition(package);
	}
}

void Dictionary::set_page_doc(const std::string& name, const std::string& doc) {
	_pages.push_back(std::make_unique<Page>(Page {
	    .name = name,
	    .doc = parse_doc(doc),
	}));
}

std::shared_ptr<Package> Dictionary::get_or_create_package(const std::string& name) const {
	if (auto i = _packages.find(name); i != _packages.end()) {
		return i->second;
	}
	return std::make_shared<Package>(name);
}

std::shared_ptr<Function> Dictionary::get_or_create_function(const std::string& name) const {
	if (auto i = _module->definitions.find(name); i != _module->definitions.end()) {
		switch (i->second->type) {
		case Definition::function_definition:
			return std::static_pointer_cast<Function>(i->second);
		default:
			return {};
		}
	}
	return std::make_shared<Function>(name);
}

void Dictionary::insert_definition(const std::shared_ptr<Definition>& definition) {

	_definitions.emplace(definition->name, _module);
	_module->definitions.emplace(definition->name, definition);

	switch (definition->type) {
	case Definition::package_definition:
		_module->elements[definition->type].emplace(definition->name, definition);
		_packages.emplace(definition->name, std::static_pointer_cast<Package>(definition));
		break;
	case Definition::constant_definition:
	case Definition::function_definition:
		if (!definition->name.contains('.')
		    || _module->definitions.at(definition->context())->type == Definition::package_definition) {
			_module->elements[definition->type].emplace(definition->name, definition);
		}
		break;
	default:
		_module->elements[definition->type].emplace(definition->name, definition);
		break;
	}
}

void Dictionary::generate(const std::filesystem::path& path) {

	std::ranges::sort(_modules, [](const auto& left, const auto& right) {
		return left->name < right->name;
	});

	for (Module& module : deref(_modules)) {
		switch (module.type) {
		case Module::script:
			break;
		case Module::group:
			for (const Module& script : child_modules(module)) {
				for (const auto& type : script.elements) {
					for (auto def : type.second) {
						module.elements[type.first].insert(def);
					}
				}
			}
			break;
		}
		_generator->setup_links(*this, module);
	}

	_generator->generate_page_list(*this, path, //
	    {std::from_range, std::views::transform(_pages, [](const auto& page) {
		     return std::cref(*page);
	     })});

	for (const Page& page : deref(_pages)) {
		_generator->generate_page(*this, path, page);
	}

	_generator->generate_module_list(*this, path, //
	    {std::from_range, std::views::transform(_modules, [](const auto& module) {
		     return std::cref(*module);
	     })});

	for (const Module& module : deref(_modules)) {
		_generator->generate_module(*this, path, module);
	}

	_generator->generate_package_list(*this, path, //
	    {std::from_range, std::views::transform(_packages, [](const auto& package) {
		     return std::cref(*package.second);
	     })});

	for (const Package& package : deref(std::views::values(_packages))) {
		_generator->generate_package(*this, path, package);
	}
}

Module* Dictionary::find_definition_module(const std::string& symbol) const {
	if (auto i = _definitions.find(symbol); i != _definitions.end()) {
		return i->second;
	}
	return nullptr;
}

std::vector<std::reference_wrapper<const Module>> Dictionary::child_modules(const Module& module) const {
	return {std::from_range, std::views::filter(_modules, [&module](const auto& script) {
		        return script->name.starts_with(module.name + ".");
	        }) | std::views::transform([](const auto& module) {
		        return std::cref(*module);
	        })};
}

std::vector<std::reference_wrapper<const Definition>> Dictionary::package_definitions(const Package& package) const {

	std::vector<std::reference_wrapper<const Definition>> definitions;
	definitions.reserve(package.members.size());

	for (const std::string& member : package.members) {
		if (auto module = _definitions.find(member); module != _definitions.end()) {
			if (auto def = module->second->definitions.find(member); def != module->second->definitions.end()) {
				definitions.push_back(std::cref(*def->second));
			}
		}
	}

	return definitions;
}

std::vector<std::reference_wrapper<const Definition>> Dictionary::enum_definitions(const Enum& instance) const {

	std::vector<std::reference_wrapper<const Definition>> definitions;

	if (auto module = _definitions.find(instance.name); module != _definitions.end()) {
		definitions.reserve(instance.members.size());
		for (const std::string& member : instance.members) {
			if (auto def = module->second->definitions.find(member); def != module->second->definitions.end()) {
				definitions.push_back(std::cref(*def->second));
			}
		}
	}

	return definitions;
}

std::vector<std::reference_wrapper<const Definition>> Dictionary::class_definitions(const Class& instance) const {

	std::vector<std::reference_wrapper<const Definition>> definitions;

	if (auto module = _definitions.find(instance.name); module != _definitions.end()) {
		definitions.reserve(instance.members.size());
		for (const std::string& member : instance.members) {
			if (auto def = module->second->definitions.find(member); def != module->second->definitions.end()) {
				definitions.push_back(std::cref(*def->second));
			}
		}
	}

	return definitions;
}
