/**
 * Copyright (c) 2026 Gauvain CHÉRY.
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

#include "mint/compiler/descriptions.h"
#include "mint/compiler/compiler.h"
#include "mint/compiler/symbol_scope.h"
#include "mint/compiler/type_annotation.h"
#include "mint/program/symbol.h"
#include "mint/memory/class.h"
#include "mint/memory/global_data.h"
#include "mint/memory/memory_tools.h"
#include "mint/memory/object.h"
#include "mint/memory/reference.h"
#include <algorithm>
#include <cassert>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

using namespace mint;

mint::FunctionDescription::FunctionDescription(SymbolScope* owner) :
    SymbolScope(owner) {}

mint::FunctionDescription::FunctionDescription(Symbol name, SymbolScope* owner) :
    SymbolScope(owner),
    _name(std::move(name)) {}

mint::FunctionDescription::FunctionDescription(FunctionDescription&&) noexcept = default;

mint::FunctionDescription::~FunctionDescription() = default;

mint::FunctionDescription& FunctionDescription::operator=(FunctionDescription&&) noexcept = default;

std::optional<Symbol> mint::FunctionDescription::name() const {
	return _name;
}

Reference* mint::FunctionDescription::data() const {
	return _data;
}

void mint::FunctionDescription::set_data(Reference* data) {
	_data = data;
}

std::span<const ParameterDescription> mint::FunctionDescription::parameters() const {
	return _parameters;
}

void mint::FunctionDescription::add_parameter(ParameterDescription parameters) {
	_parameters.emplace_back(std::move(parameters));
}

TypeAnnotation mint::FunctionDescription::get_return_type() const {
	return _return_type;
}

void mint::FunctionDescription::set_return_type(TypeAnnotation return_type) {
	_return_type = std::move(return_type);
}

ClassDescription& mint::FunctionDescription::create_class(Symbol name) {
	return *_classes.emplace_back(std::make_unique<ClassDescription>(std::move(name), this));
}

FunctionDescription& mint::FunctionDescription::create_function(Symbol name) {
	return *_functions.emplace_back(std::make_unique<FunctionDescription>(std::move(name), this));
}

FunctionDescription& mint::FunctionDescription::create_function() {
	return *_functions.emplace_back(std::make_unique<FunctionDescription>(this));
}

SymbolDefinition mint::FunctionDescription::locate(const Symbol& symbol) const {
	if (const auto it = std::ranges::find(_classes, symbol, &ClassDescription::name); it != _classes.end()) {
		return **it;
	}
	if (const auto it = std::ranges::find(_functions, symbol, &FunctionDescription::name); it != _functions.end()) {
		return **it;
	}
	return {};
}

void mint::FunctionDescription::cleanup_memory() {
	SymbolScope::cleanup_memory();
	for (const auto& symbol_scope : _classes) {
		symbol_scope->cleanup_memory();
	}
	for (const auto& symbol_scope : _functions) {
		symbol_scope->cleanup_memory();
	}
}

void mint::FunctionDescription::cleanup_metadata() {
	SymbolScope::cleanup_metadata();
	for (const auto& symbol_scope : _classes) {
		symbol_scope->cleanup_metadata();
	}
	for (const auto& symbol_scope : _functions) {
		symbol_scope->cleanup_metadata();
	}
}

PackageDescription::PackageDescription(Program& program, Symbol name, PackageDescription* owner) :
    SymbolScope(owner),
    _name(std::move(name)),
    _data(std::make_unique<PackageData>(program, *this)) {}

PackageDescription::PackageDescription(std::unique_ptr<PackageData>&& data, Symbol name, PackageDescription* owner) :
    SymbolScope(owner),
    _name(std::move(name)),
    _data(std::move(data)) {}

PackageDescription::PackageDescription(PackageDescription&&) noexcept = default;

PackageDescription::~PackageDescription() = default;

PackageDescription& PackageDescription::operator=(PackageDescription&&) noexcept = default;

Symbol PackageDescription::name() const {
	return _name;
}

std::string PackageDescription::full_name() const {
	if (const auto* package = get_owner_package(); package && !package->is_global_data()) {
		return package->full_name() + "." + name().str();
	}
	return name().str();
}

SymbolPath PackageDescription::get_path() const {
	if (const auto* package = get_owner_package()) {
		return {package->get_path(), name()};
	}
	return {name()};
}

bool mint::PackageDescription::is_global_data() const {
	return false;
}

const PackageDescription* mint::PackageDescription::get_owner_package() const {
	return dynamic_cast<const PackageDescription*>(get_owner_scope());
}

PackageDescription* mint::PackageDescription::get_owner_package() {
	return dynamic_cast<PackageDescription*>(get_owner_scope());
}

PackageDescription* PackageDescription::find_package(const Symbol& name) const {
	if (const auto it = _packages.find(name); it != _packages.end()) {
		return it->second.get();
	}
	return nullptr;
}

PackageDescription& PackageDescription::get_or_create_package(Program& program, const Symbol& name) {
	auto it = _packages.find(name);
	if (it == _packages.end()) {
		constexpr auto flags = Reference::global | Reference::const_address | Reference::const_value;
		auto package = std::make_unique<PackageDescription>(program, name, this);
		data().symbols().emplace(name, make_reference<Package>(flags, package->data()));
		it = _packages.emplace(name, std::move(package)).first;
	}
	return *it->second;
}

ClassDescription* PackageDescription::find_class(const Symbol& name) const {
	if (const auto it = std::ranges::find(_defined_classes, name, &get_class_name); it != _defined_classes.end()) {
		return it->description.get();
	}
	return nullptr;
}

ClassDescription& PackageDescription::create_class(Symbol name, Reference::Flags flags) {
	return *_defined_classes
	            .emplace_back(ClassDescriptionEntry {
	                .description = std::make_unique<ClassDescription>(std::move(name), this),
	                .flags = flags,
	            })
	            .description;
}

FunctionDescription& mint::PackageDescription::create_function(Symbol name) {
	return *_functions.emplace_back(std::make_unique<FunctionDescription>(std::move(name), this));
}

FunctionDescription& mint::PackageDescription::create_function() {
	return *_functions.emplace_back(std::make_unique<FunctionDescription>(this));
}

PackageData& mint::PackageDescription::data() const {
	return *_data;
}

SymbolDefinition PackageDescription::locate(const Symbol& symbol) const {
	if (auto* description = find_class(symbol)) {
		return *description;
	}
	if (auto* description = find_package(symbol)) {
		return *description;
	}
	if (const auto it = std::ranges::find(_globals, symbol, &VariableDescription::name); it != _globals.end()) {
		return **it;
	}
	if (const auto it = std::ranges::find(_functions, symbol, &FunctionDescription::name); it != _functions.end()) {
		return **it;
	}
	return {};
}

void PackageDescription::cleanup_memory() {

	SymbolScope::cleanup_memory();

	for (const auto& package : _packages) {
		package.second->cleanup_memory();
	}

	for (auto symbol = _symbols.begin(); symbol != _symbols.end();) {
		if (is_class(symbol->second)) {
			symbol = next(symbol);
		}
		else {
			symbol = _symbols.erase(symbol);
		}
	}
}

void PackageDescription::cleanup_metadata() {

	SymbolScope::cleanup_metadata();

	_symbols.clear();

	for (const auto& package : _packages) {
		package.second->cleanup_metadata();
	}

	_packages.clear();
}

mint::GlobalDataDescription::GlobalDataDescription(Program& program) :
    PackageDescription(std::make_unique<GlobalData>(program, *this), "(default)") {}

bool mint::GlobalDataDescription::is_global_data() const {
	return true;
}

GlobalData& mint::GlobalDataDescription::global_data() const {
	return static_cast<GlobalData&>(data());
}

ClassDescription::ClassDescription(Symbol name, SymbolScope* owner) :
    SymbolScope(owner),
    _name(std::move(name)) {}

ClassDescription::ClassDescription(ClassDescription&&) noexcept = default;

ClassDescription::~ClassDescription() = default;

ClassDescription& ClassDescription::operator=(ClassDescription&&) noexcept = default;

Symbol ClassDescription::name() const {
	return _name;
}

std::string ClassDescription::full_name() const {
	if (const auto* owner = get_owner_class()) {
		return owner->full_name() + "." + name().str();
	}
	if (const auto* package = get_owner_package(); package && !package.is_global_data()) {
		return package->full_name() + "." + name().str();
	}
	return name().str();
}

SymbolPath ClassDescription::get_path() const {
	if (const auto* owner = get_owner_class()) {
		return {owner->get_path(), name()};
	}
	if (const auto* package = get_owner_package()) {
		return {package->get_path(), name()};
	}
	return {name()};
}

const PackageDescription* ClassDescription::get_owner_package() const {
	if (auto* package = dynamic_cast<const PackageDescription*>(get_owner_scope())) {
		return package;
	}
	if (auto* owner = dynamic_cast<const ClassDescription*>(get_owner_scope())) {
		return owner->get_owner_package();
	}
	return nullptr;
}

PackageDescription* ClassDescription::get_owner_package() {
	if (auto* package = dynamic_cast<PackageDescription*>(get_owner_scope())) {
		return package;
	}
	if (auto* owner = dynamic_cast<ClassDescription*>(get_owner_scope())) {
		return owner->get_owner_package();
	}
	return nullptr;
}

const ClassDescription* ClassDescription::get_owner_class() const {
	return dynamic_cast<const ClassDescription*>(get_owner_scope());
}

ClassDescription* ClassDescription::get_owner_class() {
	return dynamic_cast<ClassDescription*>(get_owner_scope());
}

std::span<const SymbolPath> mint::ClassDescription::bases() const {
	return _bases;
}

bool ClassDescription::is_same(const SymbolPath& other) const {
	return other == get_path();
}

bool ClassDescription::is_derived_from(const SymbolPath& other) const {
	return is_same(other) || std::ranges::any_of(_bases, [this, &other](const SymbolPath& base) {
		return base == other || get_root_scope().locate(base).is_derived_from(other);
	});
}

void ClassDescription::add_base(const SymbolPath& base) {
	_bases.push_back(base);
}

ClassDescription* ClassDescription::find_class(const Symbol& name) const {
	if (const auto it = std::ranges::find(_classes, name,
	        [](const auto& entry) {
		        return entry.description->name();
	        });
	    it != _classes.end()) {
		return it->description.get();
	}
	return nullptr;
}

std::span<const ClassDescriptionEntry> mint::ClassDescription::classes() const {
	return _classes;
}

ClassDescription& ClassDescription::create_class(Symbol name, Reference::Flags flags) {
	return *_classes
	            .emplace_back(ClassDescriptionEntry {
	                .description = std::make_unique<ClassDescription>(name, this),
	                .flags = flags,
	            })
	            .description;
}

bool mint::ClassDescription::has_member(const Symbol& name) const {
	return std::ranges::find(_members, name, &get_member_name) != _members.end();
}

const Reference* ClassDescription::find_member(const Symbol& name) const {
	if (const auto it = _members.find(name); it != _members.end()) {
		return &it->second;
	}
	for (const auto& base_path : _bases) {
		if (const auto* reference = get_root_scope().locate(base_path).find_member(name)) {
			if (reference->flags() & Reference::global) {
				return nullptr;
			}
			return reference;
		}
	}
	return nullptr;
}

std::span<const MemberDescriptionEntry> mint::ClassDescription::members() const {
	return _members;
}

VariableDescription& ClassDescription::create_attribute(Symbol name, TypeAnnotation type, Reference::Flags flags) {
	return *std::get<std::unique_ptr<VariableDescription>>(_members
	        .emplace_back(MemberDescriptionEntry {
	            .description = std::make_unique<VariableDescription>(VariableDescription {
	                .name = std::move(name),
	                .type = type,
	                .flags = flags,
	            }),
	            .flags = flags,
	        })
	        .description);
}

FunctionDescription& ClassDescription::create_method(Symbol name, Reference::Flags flags) {
	return *std::get<std::unique_ptr<FunctionDescription>>(_members
	        .emplace_back(MemberDescriptionEntry {
	            .description = std::make_unique<FunctionDescription>(name, this),
	            .flags = flags,
	        })
	        .description);
}

FunctionDescription& ClassDescription::update_method(Symbol name, Reference::Flags flags) {

	if (const auto it = _members.find(name); it != _members.end()) {

		Reference& member = it->second;

		if (member.flags() != value.flags()) {
			return false;
		}

		if ((member.data().format() == Data::Format::function) && (value.data().format() == Data::Format::function)) {
			return std::ranges::all_of(value.data<Function>().mapping, [&member](const auto& signature) {
				return member.data<Function>().mapping.insert(signature).second;
			});
		}
	}

	return _members.emplace(name, value).second;
}

std::span<std::reference_wrapper<Class>> ClassDescription::generated_bases() const {
	return _bases_metadata;
}

Class& ClassDescription::generate(const Compiler& compiler) const {
	if (!_metadata) {
		std::tie(_metadata, _bases_metadata) = compiler.make_class(*this);
	}
	return *_metadata;
}

void ClassDescription::cleanup_memory() {

	SymbolScope::cleanup_memory();

	if (_metadata) {
		_metadata->cleanup_memory();
	}

	_members.clear();
}

void ClassDescription::cleanup_metadata() {

	SymbolScope::cleanup_metadata();

	if (_metadata) {
		_metadata->cleanup_metadata();
	}
}

mint::ModuleDescription::ModuleDescription(SymbolPath path) :
    _path(std::move(path)) {}

[[nodiscard]] SymbolPath mint::ModuleDescription::get_path() const {
	return _path;
}

ClassDescription& mint::ModuleDescription::create_class(Symbol name) {
	return *_classes.emplace_back(std::make_unique<ClassDescription>(std::move(name), this));
}

FunctionDescription& mint::ModuleDescription::create_function(Symbol name) {
	return *_functions.emplace_back(std::make_unique<FunctionDescription>(std::move(name), this));
}

FunctionDescription& mint::ModuleDescription::create_function() {
	return *_functions.emplace_back(std::make_unique<FunctionDescription>(this));
}

SymbolDefinition mint::ModuleDescription::locate(const Symbol& symbol) const {
	if (const auto it = std::ranges::find(_classes, symbol, &ClassDescription::name); it != _classes.end()) {
		return **it;
	}
	if (const auto it = std::ranges::find(_functions, symbol, &FunctionDescription::name); it != _functions.end()) {
		return **it;
	}
	return {};
}

void ModuleDescription::cleanup_memory() {
	SymbolScope::cleanup_memory();
	for (const auto& symbol_scope : _classes) {
		symbol_scope->cleanup_memory();
	}
	for (const auto& symbol_scope : _functions) {
		symbol_scope->cleanup_memory();
	}
	_globals.clear();
	_functions.clear();
	_constants.clear();
}

void ModuleDescription::cleanup_metadata() {
	SymbolScope::cleanup_metadata();
	for (const auto& symbol_scope : _classes) {
		symbol_scope->cleanup_metadata();
	}
	for (const auto& symbol_scope : _functions) {
		symbol_scope->cleanup_metadata();
	}
	_classes.clear();
}

Symbol mint::get_class_name(const ClassDescriptionEntry& entry) {
	return entry.description->name();
}

Symbol mint::get_member_name(const MemberDescriptionEntry& entry) {
	return std::visit(Overloaded {
	                      [](const std::unique_ptr<VariableDescription>& description) {
		                      return description->name;
	                      },
	                      [](const std::unique_ptr<FunctionDescription>& description) {
		                      return description->name().value();
	                      },
	                  },
	    entry.description);
}

Reference* mint::get_member_data(const MemberDescriptionEntry& entry) {
	return std::visit(Overloaded {
	                      [](const std::unique_ptr<VariableDescription>& description) {
		                      return description->data;
	                      },
	                      [](const std::unique_ptr<FunctionDescription>& description) {
		                      return description->data();
	                      },
	                  },
	    entry.description);
}
