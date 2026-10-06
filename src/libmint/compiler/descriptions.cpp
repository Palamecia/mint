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
#include "mint/memory/builtin/array.h"
#include "mint/memory/builtin/hash.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/memory/builtin/libobject.h"
#include "mint/memory/builtin/library.h"
#include "mint/memory/builtin/regex.h"
#include "mint/memory/builtin/string.h"
#include "mint/memory/data.h"
#include "mint/program/symbol.h"
#include "mint/memory/class.h"
#include "mint/memory/global_data.h"
#include "mint/memory/memory_tools.h"
#include "mint/memory/object.h"
#include "mint/memory/reference.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
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

mint::FunctionDescription::FunctionDescription(ModuleId module_id, TypeAnnotation return_type, Reference::Flags flags,
    SymbolScope* owner) :
    SymbolScope(owner),
    _return_type(std::move(return_type)),
    _flags(flags),
    _module_id(module_id) {}

mint::FunctionDescription::FunctionDescription(ModuleId module_id, Symbol name, TypeAnnotation return_type,
    Reference::Flags flags, SymbolScope* owner) :
    SymbolScope(owner),
    _name(std::move(name)),
    _return_type(std::move(return_type)),
    _flags(flags),
    _module_id(module_id) {}

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

ModuleId mint::FunctionDescription::get_module_id() const {
	return _module_id;
}

TypeAnnotation mint::FunctionDescription::get_return_type() const {
	return _return_type;
}

Reference::Flags mint::FunctionDescription::get_flags() const {
	return _flags;
}

std::span<const ParameterDescription> mint::FunctionDescription::parameters() const {
	return _parameters;
}

const ParameterDescription* mint::FunctionDescription::find_parameter(const Symbol& name) const {
	if (const auto it = std::ranges::find(_parameters, name, &ParameterDescription::name); it != _parameters.end()) {
		return std::to_address(it);
	}
	return nullptr;
}

FunctionDescription& mint::FunctionDescription::add_parameter(ParameterDescription parameters) {
	_parameters.emplace_back(std::move(parameters));
	return *this;
}

std::size_t mint::FunctionDescription::get_required_parameter_count() const {
	return _required_parameter_count;
}

FunctionDescription& mint::FunctionDescription::set_required_parameter_count(std::size_t count) {
	assert(count <= _parameters.size());
	_required_parameter_count = count;
	return *this;
}

bool mint::FunctionDescription::is_variadic() const {
	return _variadic;
}

FunctionDescription& mint::FunctionDescription::set_variadic(bool variadic) {
	_variadic = variadic;
	return *this;
}

const ClassDescription* mint::FunctionDescription::find_class(const Symbol& name) const {
	if (const auto it = std::ranges::find(_classes, name, &ClassDescription::get_name); it != _classes.end()) {
		return it->get();
	}
	return nullptr;
}

const ClassDescription* mint::FunctionDescription::find_any_class(const Symbol& name) const {
	if (const auto* description = find_class(name)) {
		return description;
	}
	if (const auto* description = find_alias(name)) {
		if (const auto* type = std::get_if<ClassDefinition>(&description->target)) {
			return &type->get();
		}
	}
	return {};
}

ClassDescription& mint::FunctionDescription::create_class(Symbol name) {
	return *_classes.emplace_back(std::make_unique<ClassDescription>(_module_id, std::move(name), this));
}

FunctionDefinition mint::FunctionDescription::find_function(const Symbol& name) const {
	auto functions = FunctionDefinition();
	for (const auto& function : _functions) {
		if (name == function->get_name()) {
			functions.emplace_back(std::ref(*function));
		}
	}
	return functions;
}

FunctionDefinition mint::FunctionDescription::find_any_function(const Symbol& name) const {
	if (auto description = find_function(name); !description.empty()) {
		return description;
	}
	if (const auto* description = find_alias(name)) {
		if (const auto* function = std::get_if<FunctionDefinition>(&description->target)) {
			return *function;
		}
	}
	return {};
}

FunctionDescription& mint::FunctionDescription::create_function(Symbol name, TypeAnnotation return_type,
    Reference::Flags flags) {
	return *_functions.emplace_back(
	    std::make_unique<FunctionDescription>(_module_id, std::move(name), std::move(return_type), flags, this));
}

FunctionDescription& mint::FunctionDescription::create_function(TypeAnnotation return_type, Reference::Flags flags) {
	return *_functions.emplace_back(
	    std::make_unique<FunctionDescription>(_module_id, std::move(return_type), flags, this));
}

const VariableDescription* mint::FunctionDescription::find_variable(const Symbol& name) const {
	if (const auto it = std::ranges::find(_variables, name, &VariableDescription::name); it != _variables.end()) {
		return it->get();
	}
	return nullptr;
}

VariableDescription& mint::FunctionDescription::create_variable(VariableDescription variable) {
	return *_variables.emplace_back(std::make_unique<VariableDescription>(std::move(variable)));
}

VariableDescription& mint::FunctionDescription::create_variable(Symbol name, TypeAnnotation type,
    Reference::Flags flags) {
	return *_variables.emplace_back(std::make_unique<VariableDescription>(VariableDescription {
	    .name = std::move(name),
	    .type = std::move(type),
	    .flags = flags,
	}));
}

VariableDescription& mint::FunctionDescription::update_variable(const VariableDescription& variable,
    TypeAnnotation type) {
	const auto it = std::ranges::find(_variables, &variable, &std::unique_ptr<VariableDescription>::get);
	assert(it != _variables.end());
	(*it)->type = std::move(type);
	return **it;
}

const AliasDescription* mint::FunctionDescription::find_alias(const Symbol& name) const {
	if (const auto it = std::ranges::find(_aliases, name, &AliasDescription::name); it != _aliases.end()) {
		return it->get();
	}
	return nullptr;
}

AliasDescription& mint::FunctionDescription::create_alias(const VariableDescription& variable,
    const PackageDescription& value) {
	auto& alias = *_aliases.emplace_back(std::make_unique<AliasDescription>(AliasDescription {
	    .name = variable.name,
	    .target = value,
	}));
	_variables.erase(std::ranges::find(_variables, &variable, &std::unique_ptr<VariableDescription>::get));
	return alias;
}

AliasDescription& mint::FunctionDescription::create_alias(const VariableDescription& variable,
    const ClassDescription& value) {
	auto& alias = *_aliases.emplace_back(std::make_unique<AliasDescription>(AliasDescription {
	    .name = variable.name,
	    .target = value,
	}));
	_variables.erase(std::ranges::find(_variables, &variable, &std::unique_ptr<VariableDescription>::get));
	return alias;
}

AliasDescription& mint::FunctionDescription::create_alias(const VariableDescription& variable,
    const FunctionDefinition& value) {
	auto& alias = *_aliases.emplace_back(std::make_unique<AliasDescription>(AliasDescription {
	    .name = variable.name,
	    .target = value,
	}));
	_variables.erase(std::ranges::find(_variables, &variable, &std::unique_ptr<VariableDescription>::get));
	return alias;
}

SymbolDefinition mint::FunctionDescription::locate(const Symbol& symbol) const {
	if (const auto* description = find_class(symbol)) {
		return *description;
	}
	if (auto description = find_function(symbol); !description.empty()) {
		return description;
	}
	if (const auto* description = find_variable(symbol)) {
		return *description;
	}
	if (const auto* description = find_alias(symbol)) {
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

ClassDescription::ClassDescription(ModuleId module_id, Symbol name, SymbolScope* owner) :
    SymbolScope(owner),
    _name(std::move(name)),
    _module_id(module_id) {}

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

ModuleId mint::ClassDescription::get_module_id() const {
	return _module_id;
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
	        .description = ClassDescription(_module_id, std::move(name), this),
	        .flags = flags,
	    }))
	    ->description;
}

bool mint::ClassDescription::has_member(const Symbol& name) const {
	return std::ranges::find(_members, name, &MemberDescriptionEntry::get_name) != _members.end();
}

bool mint::ClassDescription::has_any_member(const Symbol& name) const {
	return has_member(name) || std::ranges::any_of(base_classes(), [&name](const auto& base) {
		return base.has_any_member(name);
	});
}

const MemberDescriptionEntry* mint::ClassDescription::find_any_member(const Symbol& name) const {
	if (const auto* member = find_member(name)) {
		return member;
	}
	for (const auto& base : base_classes()) {
		if (const auto* member = base.find_any_member(name)) {
			return member;
		}
	}
	return nullptr;
}

const VariableDescription* mint::ClassDescription::find_attribute(const Symbol& name) const {
	if (const auto it = std::ranges::find(_members, name, &MemberDescriptionEntry::get_name); it != _members.end()) {
		if (const auto* attribute = std::get_if<VariableDescription>(&(*it)->description)) {
			return attribute;
		}
	}
	return nullptr;
}

const VariableDescription* mint::ClassDescription::find_any_attribute(const Symbol& name) const {
	if (const auto* attribute = find_attribute(name)) {
		return attribute;
	}
	for (const auto& base : base_classes()) {
		if (const auto* attribute = base.find_any_attribute(name)) {
			return attribute;
		}
	}
	return nullptr;
}

FunctionDefinition mint::ClassDescription::find_method(const Symbol& name) const {
	auto functions = FunctionDefinition();
	for (const auto& member : _members) {
		if (name == member->get_name()) {
			if (const auto* method = std::get_if<FunctionDescription>(&member->description)) {
				functions.emplace_back(std::ref(*method));
			}
		}
	}
	return functions;
}

FunctionDefinition mint::ClassDescription::find_any_method(const Symbol& name) const {
	if (const auto method = find_method(name); !method.empty()) {
		return method;
	}
	for (const auto& base : base_classes()) {
		if (const auto method = base.find_any_method(name); !method.empty()) {
			return method;
		}
	}
	return {};
}

const MemberDescriptionEntry* ClassDescription::find_member(const Symbol& name) const {
	if (const auto it = std::ranges::find(_members, name, &MemberDescriptionEntry::get_name); it != _members.end()) {
		return it->get();
	}
	return nullptr;
}

VariableDescription& ClassDescription::create_attribute(Symbol name, TypeAnnotation type, Reference::Flags flags,
    const Reference* value) {
	return std::get<VariableDescription>(_members
	        .emplace_back(std::make_unique<MemberDescriptionEntry>(MemberDescriptionEntry {
	            .description = VariableDescription(VariableDescription {
	                .name = std::move(name),
	                .type = std::move(type),
	                .flags = flags,
	            }),
	            .value = value,
	        }))
	        ->description);
}

FunctionDescription& ClassDescription::create_method(Symbol name, TypeAnnotation return_type, Reference::Flags flags,
    const Reference* value) {
	return std::get<FunctionDescription>(_members
	        .emplace_back(std::make_unique<MemberDescriptionEntry>(MemberDescriptionEntry {
	            .description = FunctionDescription(_module_id, std::move(name), std::move(return_type), flags, this),
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
	if (const auto* description = find_attribute(symbol)) {
		return *description;
	}
	if (auto description = find_method(symbol); !description.empty()) {
		return description;
	}
	for (const auto& base : base_classes()) {
		if (auto definition = base.locate(symbol); definition.is_valid()) {
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

const PackageDescription* PackageDescription::find_package(const Symbol& name) const {
	if (const auto it = _packages.find(name); it != _packages.end()) {
		return it->second.get();
	}
	return nullptr;
}

const PackageDescription* mint::PackageDescription::find_any_package(const Symbol& name) const {
	if (const auto* description = find_package(name)) {
		return description;
	}
	if (const auto* description = find_alias(name)) {
		if (const auto* package = std::get_if<PackageDefinition>(&description->target)) {
			return &package->get();
		}
	}
	return {};
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

const ClassDescription* mint::PackageDescription::find_any_class(const Symbol& name) const {
	if (const auto* description = find_class(name)) {
		return description;
	}
	if (const auto* description = find_alias(name)) {
		if (const auto* type = std::get_if<ClassDefinition>(&description->target)) {
			return &type->get();
		}
	}
	return {};
}

ClassDescription& PackageDescription::create_class(ModuleId module_id, Symbol name, Reference::Flags flags) {
	return _classes
	    .emplace_back(std::make_unique<ClassDescriptionEntry>(ClassDescriptionEntry {
	        .description = ClassDescription(module_id, std::move(name), this),
	        .flags = flags,
	    }))
	    ->description;
}

FunctionDefinition mint::PackageDescription::find_function(const Symbol& name) const {
	auto functions = FunctionDefinition();
	for (const auto& function : _functions) {
		if (name == function->get_name()) {
			functions.emplace_back(std::ref(*function));
		}
	}
	return functions;
}

FunctionDefinition mint::PackageDescription::find_any_function(const Symbol& name) const {
	if (auto description = find_function(name); !description.empty()) {
		return description;
	}
	if (const auto* description = find_alias(name)) {
		if (const auto* function = std::get_if<FunctionDefinition>(&description->target)) {
			return *function;
		}
	}
	return {};
}

FunctionDescription& mint::PackageDescription::create_function(ModuleId module_id, Symbol name,
    TypeAnnotation return_type, Reference::Flags flags) {
	return *_functions.emplace_back(
	    std::make_unique<FunctionDescription>(module_id, std::move(name), std::move(return_type), flags, this));
}

FunctionDescription& mint::PackageDescription::create_function(ModuleId module_id, TypeAnnotation return_type,
    Reference::Flags flags) {
	return *_functions.emplace_back(
	    std::make_unique<FunctionDescription>(module_id, std::move(return_type), flags, this));
}

const VariableDescription* mint::PackageDescription::find_variable(const Symbol& name) const {
	if (const auto it = std::ranges::find(_variables, name, &VariableDescription::name); it != _variables.end()) {
		return it->get();
	}
	return nullptr;
}

VariableDescription& mint::PackageDescription::create_variable(VariableDescription variable) {
	return *_variables.emplace_back(std::make_unique<VariableDescription>(std::move(variable)));
}

VariableDescription& mint::PackageDescription::create_variable(Symbol name, TypeAnnotation type,
    Reference::Flags flags) {
	return *_variables.emplace_back(std::make_unique<VariableDescription>(VariableDescription {
	    .name = std::move(name),
	    .type = std::move(type),
	    .flags = flags,
	}));
}

VariableDescription& mint::PackageDescription::update_variable(const VariableDescription& variable,
    TypeAnnotation type) {
	const auto it = std::ranges::find(_variables, &variable, &std::unique_ptr<VariableDescription>::get);
	assert(it != _variables.end());
	(*it)->type = std::move(type);
	return **it;
}

const AliasDescription* mint::PackageDescription::find_alias(const Symbol& name) const {
	if (const auto it = std::ranges::find(_aliases, name, &AliasDescription::name); it != _aliases.end()) {
		return it->get();
	}
	return nullptr;
}

AliasDescription& mint::PackageDescription::create_alias(const VariableDescription& variable,
    const PackageDescription& value) {
	auto& alias = *_aliases.emplace_back(std::make_unique<AliasDescription>(AliasDescription {
	    .name = variable.name,
	    .target = value,
	}));
	_variables.erase(std::ranges::find(_variables, &variable, &std::unique_ptr<VariableDescription>::get));
	return alias;
}

AliasDescription& mint::PackageDescription::create_alias(const VariableDescription& variable,
    const ClassDescription& value) {
	auto& alias = *_aliases.emplace_back(std::make_unique<AliasDescription>(AliasDescription {
	    .name = variable.name,
	    .target = value,
	}));
	_variables.erase(std::ranges::find(_variables, &variable, &std::unique_ptr<VariableDescription>::get));
	return alias;
}

AliasDescription& mint::PackageDescription::create_alias(const VariableDescription& variable,
    const FunctionDefinition& value) {
	auto& alias = *_aliases.emplace_back(std::make_unique<AliasDescription>(AliasDescription {
	    .name = variable.name,
	    .target = value,
	}));
	_variables.erase(std::ranges::find(_variables, &variable, &std::unique_ptr<VariableDescription>::get));
	return alias;
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
	if (auto description = find_function(symbol); !description.empty()) {
		return description;
	}
	if (const auto* description = find_variable(symbol)) {
		return *description;
	}
	if (const auto* description = find_alias(symbol)) {
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

ClassDescription& mint::GlobalDataDescription::builtin(BuiltinTypeKind type) {
	switch (type) {
	case BuiltinTypeKind::string:
		return StringClass::description();
	case BuiltinTypeKind::regex:
		return RegexClass::description();
	case BuiltinTypeKind::array:
		return ArrayClass::description();
	case BuiltinTypeKind::hash:
		return HashClass::description();
	case BuiltinTypeKind::iterator:
		return IteratorClass::description();
	case BuiltinTypeKind::async_iterator:
		return AsyncIteratorClass::description();
	case BuiltinTypeKind::libobject:
		return LibObjectClass::description();
	case BuiltinTypeKind::library:
		return LibraryClass::description();
	}
}

mint::ModuleDescription::ModuleDescription(ModuleId module_id, SymbolPath path) :
    _path(std::move(path)),
    _module_id(module_id) {}

mint::ModuleDescription::ModuleDescription(ModuleDescription&&) noexcept = default;

mint::ModuleDescription::~ModuleDescription() = default;

mint::ModuleDescription& ModuleDescription::operator=(ModuleDescription&&) noexcept = default;

SymbolPath mint::ModuleDescription::get_path() const {
	return _path;
}

ModuleId mint::ModuleDescription::get_module_id() const {
	return _module_id;
}

const ClassDescription* mint::ModuleDescription::find_class(const Symbol& name) const {
	if (const auto it = std::ranges::find(_classes, name, &ClassDescription::get_name); it != _classes.end()) {
		return it->get();
	}
	return nullptr;
}

const ClassDescription* mint::ModuleDescription::find_any_class(const Symbol& name) const {
	if (const auto* description = find_class(name)) {
		return description;
	}
	if (const auto* description = find_alias(name)) {
		if (const auto* type = std::get_if<ClassDefinition>(&description->target)) {
			return &type->get();
		}
	}
	return {};
}

ClassDescription& mint::ModuleDescription::create_class(Symbol name) {
	return *_classes.emplace_back(std::make_unique<ClassDescription>(_module_id, std::move(name), this));
}

FunctionDefinition mint::ModuleDescription::find_function(const Symbol& name) const {
	auto functions = FunctionDefinition();
	for (const auto& function : _functions) {
		if (name == function->get_name()) {
			functions.emplace_back(std::ref(*function));
		}
	}
	return functions;
}

FunctionDefinition mint::ModuleDescription::find_any_function(const Symbol& name) const {
	if (auto description = find_function(name); !description.empty()) {
		return description;
	}
	if (const auto* description = find_alias(name)) {
		if (const auto* function = std::get_if<FunctionDefinition>(&description->target)) {
			return *function;
		}
	}
	return {};
}

FunctionDescription& mint::ModuleDescription::create_function(Symbol name, TypeAnnotation return_type,
    Reference::Flags flags) {
	return *_functions.emplace_back(
	    std::make_unique<FunctionDescription>(_module_id, std::move(name), std::move(return_type), flags, this));
}

FunctionDescription& mint::ModuleDescription::create_function(TypeAnnotation return_type, Reference::Flags flags) {
	return *_functions.emplace_back(
	    std::make_unique<FunctionDescription>(_module_id, std::move(return_type), flags, this));
}

const VariableDescription* mint::ModuleDescription::find_variable(const Symbol& name) const {
	if (const auto it = std::ranges::find(_variables, name, &VariableDescription::name); it != _variables.end()) {
		return it->get();
	}
	return nullptr;
}

VariableDescription& mint::ModuleDescription::create_variable(VariableDescription variable) {
	return *_variables.emplace_back(std::make_unique<VariableDescription>(std::move(variable)));
}

VariableDescription& mint::ModuleDescription::create_variable(Symbol name, TypeAnnotation type, Reference::Flags flags) {
	return *_variables.emplace_back(std::make_unique<VariableDescription>(VariableDescription {
	    .name = std::move(name),
	    .type = std::move(type),
	    .flags = flags,
	}));
}

VariableDescription& mint::ModuleDescription::update_variable(const VariableDescription& variable, TypeAnnotation type) {
	const auto it = std::ranges::find(_variables, &variable, &std::unique_ptr<VariableDescription>::get);
	assert(it != _variables.end());
	(*it)->type = std::move(type);
	return **it;
}

const AliasDescription* mint::ModuleDescription::find_alias(const Symbol& name) const {
	if (const auto it = std::ranges::find(_aliases, name, &AliasDescription::name); it != _aliases.end()) {
		return it->get();
	}
	return nullptr;
}

AliasDescription& mint::ModuleDescription::create_alias(const VariableDescription& variable,
    const PackageDescription& value) {
	auto& alias = *_aliases.emplace_back(std::make_unique<AliasDescription>(AliasDescription {
	    .name = variable.name,
	    .target = value,
	}));
	_variables.erase(std::ranges::find(_variables, &variable, &std::unique_ptr<VariableDescription>::get));
	return alias;
}

AliasDescription& mint::ModuleDescription::create_alias(const VariableDescription& variable,
    const ClassDescription& value) {
	auto& alias = *_aliases.emplace_back(std::make_unique<AliasDescription>(AliasDescription {
	    .name = variable.name,
	    .target = value,
	}));
	_variables.erase(std::ranges::find(_variables, &variable, &std::unique_ptr<VariableDescription>::get));
	return alias;
}

AliasDescription& mint::ModuleDescription::create_alias(const VariableDescription& variable,
    const FunctionDefinition& value) {
	auto& alias = *_aliases.emplace_back(std::make_unique<AliasDescription>(AliasDescription {
	    .name = variable.name,
	    .target = value,
	}));
	_variables.erase(std::ranges::find(_variables, &variable, &std::unique_ptr<VariableDescription>::get));
	return alias;
}

SymbolDefinition mint::ModuleDescription::locate(const Symbol& symbol) const {
	if (const auto* description = find_class(symbol)) {
		return *description;
	}
	if (auto description = find_function(symbol); !description.empty()) {
		return description;
	}
	if (const auto* description = find_variable(symbol)) {
		return *description;
	}
	if (const auto* description = find_alias(symbol)) {
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

TypeAnnotation mint::AliasDescription::get_type() const {
	return std::visit(Overloaded {
	                      [](const PackageDescription& description) -> TypeAnnotation {
		                      return description.get_path();
	                      },
	                      [](const ClassDescription& description) -> TypeAnnotation {
		                      return description.get_path();
	                      },
	                      [](const FunctionDefinition&) -> TypeAnnotation {
		                      return PrimitiveTypeKind::function;
	                      },
	                  },
	    target);
}

SymbolDefinition mint::AliasDescription::target_definition() const {
	return std::visit(Overloaded {
	                      [](const PackageDescription& description) -> SymbolDefinition {
		                      return description;
	                      },
	                      [](const ClassDescription& description) -> SymbolDefinition {
		                      return description;
	                      },
	                      [](const FunctionDefinition& definition) -> SymbolDefinition {
		                      return definition;
	                      },
	                  },
	    target);
}

SymbolDefinition mint::AliasDescription::locate(const Symbol& symbol) const {
	return std::visit(Overloaded {
	                      [&](const PackageDescription& description) -> SymbolDefinition {
		                      return description.locate(symbol);
	                      },
	                      [&](const ClassDescription& description) -> SymbolDefinition {
		                      return description.locate(symbol);
	                      },
	                      [&](const FunctionDefinition& description) -> SymbolDefinition {
		                      for (const FunctionDescription& signature : description) {
			                      if (const auto match = signature.locate(symbol); match.is_valid()) {
				                      return match;
			                      }
		                      }
		                      return {};
	                      },
	                  },
	    target);
}
