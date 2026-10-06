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
#include "mint/compiler/class_builder.h"
#include "mint/compiler/symbol_scope.h"
#include "mint/compiler/type_annotation.h"
#include "mint/config.h"
#include "mint/memory/data.h"
#include "mint/program/symbol.h"
#include "mint/memory/class.h"
#include "mint/memory/global_data.h"
#include "mint/memory/memory_tools.h"
#include "mint/memory/object.h"
#include "mint/memory/reference.h"
#include <algorithm>
#include <cassert>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <span>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

using namespace mint;

mint::FunctionDescription::FunctionDescription(TypeAnnotation return_type, SymbolScope* owner) :
    SymbolScope(owner),
    _return_type(std::move(return_type)) {}

mint::FunctionDescription::FunctionDescription(Symbol name, TypeAnnotation return_type, SymbolScope* owner) :
    SymbolScope(owner),
    _name(std::move(name)),
    _return_type(std::move(return_type)) {}

mint::FunctionDescription::FunctionDescription(FunctionDescription&&) noexcept = default;

mint::FunctionDescription::~FunctionDescription() = default;

mint::FunctionDescription& FunctionDescription::operator=(FunctionDescription&&) noexcept = default;

Symbol mint::FunctionDescription::get_name() const {
	return _name.value_or("<unknown>");
}

SymbolPath mint::FunctionDescription::get_path() const {
	if (const auto* owner = get_owner_class()) {
		return {owner->get_path(), get_name()};
	}
	if (const auto* package = get_owner_package()) {
		return {package->get_path(), get_name()};
	}
	return {get_name()};
}

TypeAnnotation mint::FunctionDescription::get_return_type() const {
	return _return_type;
}

std::span<const ParameterDescription> mint::FunctionDescription::parameters() const {
	return _parameters;
}

void mint::FunctionDescription::add_parameter(ParameterDescription parameters) {
	_parameters.emplace_back(std::move(parameters));
}

const ClassDescription* mint::FunctionDescription::find_class(const Symbol& name) const {
	if (const auto it = std::ranges::find(_classes, name, &ClassDescription::get_name); it != _classes.end()) {
		return it->get();
	}
	return nullptr;
}

ClassDescription& mint::FunctionDescription::create_class(Symbol name) {
	return *_classes.emplace_back(std::make_unique<ClassDescription>(std::move(name), this));
}

const FunctionDescription* mint::FunctionDescription::find_function(const Symbol& name) const {
	if (const auto it = std::ranges::find(_functions, name, &FunctionDescription::get_name); it != _functions.end()) {
		return it->get();
	}
	return nullptr;
}

FunctionDescription& mint::FunctionDescription::create_function(Symbol name, TypeAnnotation return_type) {
	return *_functions.emplace_back(
	    std::make_unique<FunctionDescription>(std::move(name), std::move(return_type), this));
}

FunctionDescription& mint::FunctionDescription::create_function(TypeAnnotation return_type) {
	return *_functions.emplace_back(std::make_unique<FunctionDescription>(std::move(return_type), this));
}

const VariableDescription* mint::FunctionDescription::find_variable(const Symbol& name) const {
	if (const auto it = std::ranges::find(_variables, name, &VariableDescription::name); it != _variables.end()) {
		return it->get();
	}
	return nullptr;
}

VariableDescription& mint::FunctionDescription::create_variable(Symbol name, TypeAnnotation type) {
	return *_variables.emplace_back(std::make_unique<VariableDescription>(VariableDescription {
	    .name = std::move(name),
	    .type = std::move(type),
	}));
}

SymbolDefinition mint::FunctionDescription::locate(const Symbol& symbol) const {
	if (const auto* description = find_class(symbol)) {
		return *description;
	}
	if (const auto* description = find_function(symbol)) {
		return *description;
	}
	if (const auto* description = find_variable(symbol)) {
		return *description;
	}
	return {};
}

void mint::FunctionDescription::cleanup_memory() {
	for (const auto& symbol_scope : _classes) {
		symbol_scope->cleanup_memory();
	}
	for (const auto& symbol_scope : _functions) {
		symbol_scope->cleanup_memory();
	}
}

void mint::FunctionDescription::cleanup_metadata() {
	for (const auto& symbol_scope : _classes) {
		symbol_scope->cleanup_metadata();
	}
	for (const auto& symbol_scope : _functions) {
		symbol_scope->cleanup_metadata();
	}
}

Symbol mint::MemberDescriptionEntry::get_name() const {
	return std::visit(Overloaded {
	                      [](const VariableDescription& description) {
		                      return description.name;
	                      },
	                      [](const FunctionDescription& description) {
		                      return description.get_name();
	                      },
	                  },
	    description);
}

ClassDescription::ClassDescription(Symbol name, SymbolScope* owner) :
    SymbolScope(owner),
    _name(std::move(name)) {}

ClassDescription::ClassDescription(ClassDescription&&) noexcept = default;

ClassDescription::~ClassDescription() = default;

ClassDescription& ClassDescription::operator=(ClassDescription&&) noexcept = default;

Symbol ClassDescription::get_name() const {
	return _name;
}

SymbolPath ClassDescription::get_path() const {
	if (const auto* owner = get_owner_class()) {
		return {owner->get_path(), get_name()};
	}
	if (const auto* package = get_owner_package()) {
		return {package->get_path(), get_name()};
	}
	return {get_name()};
}

std::span<const SymbolPath> mint::ClassDescription::bases() const {
	return _bases;
}

bool ClassDescription::is_same(const SymbolPath& other) const {
	return other == get_path();
}

bool ClassDescription::is_derived_from(const SymbolPath& other) const {
	return is_same(other) || std::ranges::any_of(base_classes(), [&other](const ClassDescription& base) -> bool {
		return base.is_derived_from(other);
	});
}

void ClassDescription::add_base(const SymbolPath& base) {
	_bases.push_back(base);
}

const ClassDescription* ClassDescription::find_class(const Symbol& name) const {
	if (const auto it = std::ranges::find(_classes, name, &ClassDescriptionEntry::get_name); it != _classes.end()) {
		return &(*it)->description;
	}
	return nullptr;
}

ClassDescription& ClassDescription::create_class(Symbol name, Reference::Flags flags) {
	return _classes
	    .emplace_back(std::make_unique<ClassDescriptionEntry>(ClassDescriptionEntry {
	        .description = ClassDescription(std::move(name), this),
	        .flags = flags,
	    }))
	    ->description;
}

bool mint::ClassDescription::has_member(const Symbol& name) const {
	return std::ranges::find(_members, name, &MemberDescriptionEntry::get_name) != _members.end();
}

const MemberDescriptionEntry::Description* ClassDescription::find_member(const Symbol& name) const {
	if (const auto it = std::ranges::find(_members, name, &MemberDescriptionEntry::get_name); it != _members.end()) {
		return &(*it)->description;
	}
	return nullptr;
}

VariableDescription& ClassDescription::create_attribute(Symbol name, TypeAnnotation type, const Reference* value) {
	return std::get<VariableDescription>(_members
	        .emplace_back(std::make_unique<MemberDescriptionEntry>(MemberDescriptionEntry {
	            .description = VariableDescription(VariableDescription {
	                .name = std::move(name),
	                .type = std::move(type),
	            }),
	            .value = value,
	        }))
	        ->description);
}

FunctionDescription& ClassDescription::create_method(Symbol name, TypeAnnotation return_type, const Reference* value) {
	return std::get<FunctionDescription>(_members
	        .emplace_back(std::make_unique<MemberDescriptionEntry>(MemberDescriptionEntry {
	            .description = FunctionDescription(std::move(name), std::move(return_type), this),
	            .value = value,
	        }))
	        ->description);
}

Class& ClassDescription::generate(const ClassBuilder& builder) const {
	if (!_metadata) {
		_metadata = builder.make_class(*this);
	}
	return *_metadata;
}

SymbolDefinition mint::ClassDescription::locate(const Symbol& symbol) const {
	if (const auto* description = find_class(symbol)) {
		return *description;
	}
	if (const auto* description = find_member(symbol)) {
		return std::visit(Overloaded {
		                      [](const VariableDescription& description) -> SymbolDefinition {
			                      return description;
		                      },
		                      [](const FunctionDescription& description) -> SymbolDefinition {
			                      return description;
		                      },
		                  },
		    *description);
	}
	for (const auto& base : base_classes()) {
		if (auto definition = base.locate(symbol); !std::holds_alternative<std::monostate>(definition)) {
			return definition;
		}
	}
	return {};
}

void ClassDescription::cleanup_memory() {

	if (_metadata) {
		_metadata->cleanup_memory();
	}

	_members.clear();
}

void ClassDescription::cleanup_metadata() {
	if (_metadata) {
		_metadata->cleanup_metadata();
	}
}

Symbol mint::ClassDescriptionEntry::get_name() const {
	return description.get_name();
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

Symbol PackageDescription::get_name() const {
	return _name;
}

SymbolPath PackageDescription::get_path() const {
	if (is_global_data()) {
		return {};
	}
	if (const auto* owner = get_owner_package()) {
		return {owner->get_path(), get_name()};
	}
	return {get_name()};
}

bool mint::PackageDescription::is_global_data() const {
	return false;
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

const ClassDescription* PackageDescription::find_class(const Symbol& name) const {
	if (const auto it = std::ranges::find(_classes, name, &ClassDescriptionEntry::get_name); it != _classes.end()) {
		return &(*it)->description;
	}
	return nullptr;
}

ClassDescription& PackageDescription::create_class(Symbol name, Reference::Flags flags) {
	return _classes
	    .emplace_back(std::make_unique<ClassDescriptionEntry>(ClassDescriptionEntry {
	        .description = ClassDescription(std::move(name), this),
	        .flags = flags,
	    }))
	    ->description;
}

const FunctionDescription* mint::PackageDescription::find_function(const Symbol& name) const {
	if (const auto it = std::ranges::find(_functions, name, &FunctionDescription::get_name); it != _functions.end()) {
		return it->get();
	}
	return nullptr;
}

FunctionDescription& mint::PackageDescription::create_function(Symbol name, TypeAnnotation return_type) {
	return *_functions.emplace_back(
	    std::make_unique<FunctionDescription>(std::move(name), std::move(return_type), this));
}

FunctionDescription& mint::PackageDescription::create_function(TypeAnnotation return_type) {
	return *_functions.emplace_back(std::make_unique<FunctionDescription>(std::move(return_type), this));
}

const VariableDescription* mint::PackageDescription::find_variable(const Symbol& name) const {
	if (const auto it = std::ranges::find(_variables, name, &VariableDescription::name); it != _variables.end()) {
		return it->get();
	}
	return nullptr;
}

VariableDescription& mint::PackageDescription::create_variable(Symbol name, TypeAnnotation type) {
	return *_variables.emplace_back(std::make_unique<VariableDescription>(VariableDescription {
	    .name = std::move(name),
	    .type = std::move(type),
	}));
}

PackageData& mint::PackageDescription::data() const {
	return *_data;
}

SymbolDefinition PackageDescription::locate(const Symbol& symbol) const {
	if (const auto* description = find_class(symbol)) {
		return *description;
	}
	if (const auto* description = find_package(symbol)) {
		return *description;
	}
	if (const auto* description = find_function(symbol)) {
		return *description;
	}
	if (const auto* description = find_variable(symbol)) {
		return *description;
	}
	return {};
}

void PackageDescription::cleanup_memory() {

	for (const auto& package : _packages) {
		package.second->cleanup_memory();
	}

	auto& symbols = _data->symbols();
	auto symbol = symbols.begin();
	while (symbol != symbols.end()) {
		if (is_class(symbol->second) || is_instance_of(symbol->second, Data::Format::package)) {
			symbol = std::next(symbol);
		}
		else {
			symbol = symbols.erase(symbol);
		}
	}
}

void PackageDescription::cleanup_metadata() {

	_data->symbols().clear();

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

mint::ModuleDescription::ModuleDescription(SymbolPath path) :
    _path(std::move(path)) {}

[[nodiscard]] SymbolPath mint::ModuleDescription::get_path() const {
	return _path;
}

const ClassDescription* mint::ModuleDescription::find_class(const Symbol& name) const {
	if (const auto it = std::ranges::find(_classes, name, &ClassDescription::get_name); it != _classes.end()) {
		return it->get();
	}
	return nullptr;
}

ClassDescription& mint::ModuleDescription::create_class(Symbol name) {
	return *_classes.emplace_back(std::make_unique<ClassDescription>(std::move(name), this));
}

const FunctionDescription* mint::ModuleDescription::find_function(const Symbol& name) const {
	if (const auto it = std::ranges::find(_functions, name, &FunctionDescription::get_name); it != _functions.end()) {
		return it->get();
	}
	return nullptr;
}

FunctionDescription& mint::ModuleDescription::create_function(Symbol name, TypeAnnotation return_type) {
	return *_functions.emplace_back(
	    std::make_unique<FunctionDescription>(std::move(name), std::move(return_type), this));
}

FunctionDescription& mint::ModuleDescription::create_function(TypeAnnotation return_type) {
	return *_functions.emplace_back(std::make_unique<FunctionDescription>(std::move(return_type), this));
}

const VariableDescription* mint::ModuleDescription::find_variable(const Symbol& name) const {
	if (const auto it = std::ranges::find(_variables, name, &VariableDescription::name); it != _variables.end()) {
		return it->get();
	}
	return nullptr;
}

VariableDescription& mint::ModuleDescription::create_variable(Symbol name, TypeAnnotation type) {
	return *_variables.emplace_back(std::make_unique<VariableDescription>(VariableDescription {
	    .name = std::move(name),
	    .type = std::move(type),
	}));
}

SymbolDefinition mint::ModuleDescription::locate(const Symbol& symbol) const {
	if (const auto* description = find_class(symbol)) {
		return *description;
	}
	if (const auto* description = find_function(symbol)) {
		return *description;
	}
	if (const auto* description = find_variable(symbol)) {
		return *description;
	}
	return {};
}

void ModuleDescription::cleanup_memory() {
	for (const auto& symbol_scope : _classes) {
		symbol_scope->cleanup_memory();
	}
	for (const auto& symbol_scope : _functions) {
		symbol_scope->cleanup_memory();
	}
	_variables.clear();
	_functions.clear();
}

void ModuleDescription::cleanup_metadata() {
	for (const auto& symbol_scope : _classes) {
		symbol_scope->cleanup_metadata();
	}
	for (const auto& symbol_scope : _functions) {
		symbol_scope->cleanup_metadata();
	}
	_classes.clear();
}
