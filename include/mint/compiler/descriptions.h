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

#ifndef MINT_COMPILER_DESCRIPTIONS_H
#define MINT_COMPILER_DESCRIPTIONS_H

#include "mint/compiler/type_annotation.h"
#include "mint/config.h"
#include "mint/memory/reference.h"
#include "mint/program/symbol.h"
#include "symbol_scope.h"
#include <cstddef>
#include <functional>
#include <generator>
#include <memory>
#include <optional>
#include <ranges>
#include <span>
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>

namespace mint {

class Class;
class ClassBuilder;
class ClassDescription;
class GlobalData;
class PackageData;
class Program;

struct MemberDescriptionEntry;
struct ClassDescriptionEntry;

struct VariableDescription {
	Symbol name;
	TypeAnnotation type;
	Reference::Flags flags = Reference::default_flags;
};

struct MINT_EXPORT ParameterDescription {
	Symbol name;
	TypeAnnotation type;
	Reference::Flags flags = Reference::default_flags;
};

class MINT_EXPORT FunctionDescription : public SymbolScope {
	std::optional<Symbol> _name;
	TypeAnnotation _return_type;
	bool _variadic = false;
	std::size_t _required_parameter_count = 0;
	std::vector<ParameterDescription> _parameters;
	std::vector<std::unique_ptr<ClassDescription>> _classes;
	std::vector<std::unique_ptr<FunctionDescription>> _functions;
	std::vector<std::unique_ptr<VariableDescription>> _variables;
public:
	FunctionDescription(TypeAnnotation return_type, SymbolScope* owner);
	FunctionDescription(Symbol name, TypeAnnotation return_type, SymbolScope* owner);
	FunctionDescription(const FunctionDescription&) = delete;
	FunctionDescription(FunctionDescription&&) noexcept;
	~FunctionDescription() override;

	FunctionDescription& operator=(const FunctionDescription&) = delete;
	FunctionDescription& operator=(FunctionDescription&&) noexcept;

	[[nodiscard]] Symbol get_name() const;
	[[nodiscard]] SymbolPath get_path() const;
	[[nodiscard]] TypeAnnotation get_return_type() const;

	[[nodiscard]] bool is_root_scope() const override;
	[[nodiscard]] auto* get_owner_package(this auto&& self);
	[[nodiscard]] auto* get_owner_class(this auto&& self);

	[[nodiscard]] std::span<const ParameterDescription> parameters() const;
	[[nodiscard]] const ParameterDescription* find_parameter(const Symbol& name) const;
	FunctionDescription& add_parameter(ParameterDescription parameters);

	[[nodiscard]] std::size_t get_required_parameter_count() const;
	FunctionDescription& set_required_parameter_count(std::size_t count);

	[[nodiscard]] bool is_variadic() const;
	FunctionDescription& set_variadic(bool variadic);

	[[nodiscard]] auto classes() const;
	[[nodiscard]] const ClassDescription* find_class(const Symbol& name) const;
	[[nodiscard]] ClassDescription& create_class(Symbol name);

	[[nodiscard]] auto functions() const;
	[[nodiscard]] FunctionDefinition find_function(const Symbol& name) const;
	[[nodiscard]] FunctionDescription& create_function(Symbol name, TypeAnnotation return_type);
	[[nodiscard]] FunctionDescription& create_function(TypeAnnotation return_type);

	[[nodiscard]] auto variables() const;
	[[nodiscard]] const VariableDescription* find_variable(const Symbol& name) const;
	[[nodiscard]] VariableDescription& create_variable(VariableDescription variable);
	[[nodiscard]] VariableDescription& create_variable(Symbol name, TypeAnnotation type, Reference::Flags flags);

	[[nodiscard]] SymbolDefinition locate(const Symbol& symbol) const override;
	using SymbolScope::locate;

	void cleanup_memory() override;
	void cleanup_metadata() override;
};

struct MINT_EXPORT MemberDescriptionEntry {
	using Description = std::variant<VariableDescription, FunctionDescription>;

	Description description;
	const Reference* value = nullptr;

	[[nodiscard]] Symbol get_name() const;
};

class MINT_EXPORT ClassDescription : public SymbolScope {
	Symbol _name;
	std::vector<SymbolPath> _bases;
	std::vector<std::unique_ptr<MemberDescriptionEntry>> _members;
	std::vector<std::unique_ptr<ClassDescriptionEntry>> _classes;

	mutable std::unique_ptr<Class> _metadata;
public:
	explicit ClassDescription(Symbol name, SymbolScope* owner);
	ClassDescription(const ClassDescription&) = delete;
	ClassDescription(ClassDescription&&) noexcept;
	~ClassDescription() override;

	ClassDescription& operator=(const ClassDescription&) = delete;
	ClassDescription& operator=(ClassDescription&&) noexcept;

	[[nodiscard]] Symbol get_name() const;
	[[nodiscard]] SymbolPath get_path() const;

	[[nodiscard]] bool is_root_scope() const override;
	[[nodiscard]] auto* get_owner_package(this auto&& self);
	[[nodiscard]] auto* get_owner_class(this auto&& self);

	[[nodiscard]] auto base_classes() const;
	[[nodiscard]] std::span<const SymbolPath> bases() const;
	[[nodiscard]] bool is_same(const SymbolPath& other) const;
	[[nodiscard]] bool is_derived_from(const SymbolPath& other) const;
	void add_base(const SymbolPath& base);

	[[nodiscard]] auto classes() const;
	[[nodiscard]] const ClassDescription* find_class(const Symbol& name) const;
	ClassDescription& create_class(Symbol name, Reference::Flags flags);

	[[nodiscard]] auto members() const;
	[[nodiscard]] bool has_member(const Symbol& name) const;
	[[nodiscard]] bool has_any_member(const Symbol& name) const;
	[[nodiscard]] std::generator<const MemberDescriptionEntry&> all_members() const;
	[[nodiscard]] const MemberDescriptionEntry* find_member(const Symbol& name) const;
	[[nodiscard]] const VariableDescription* find_attribute(const Symbol& name) const;
	[[nodiscard]] FunctionDefinition find_method(const Symbol& name) const;
	VariableDescription& create_attribute(Symbol name, TypeAnnotation type, const Reference* value);
	FunctionDescription& create_method(Symbol name, TypeAnnotation return_type, const Reference* value);

	Class& generate(const ClassBuilder& builder) const;

	[[nodiscard]] SymbolDefinition locate(const Symbol& symbol) const override;
	using SymbolScope::locate;

	void cleanup_memory() override;
	void cleanup_metadata() override;
};

struct ClassDescriptionEntry {
	ClassDescription description;
	Reference::Flags flags = Reference::default_flags;

	[[nodiscard]] Symbol get_name() const;
};

class MINT_EXPORT PackageDescription : public SymbolScope {
	Symbol _name;
	std::unique_ptr<PackageData> _data;
	std::vector<std::unique_ptr<ClassDescriptionEntry>> _classes;
	std::vector<std::unique_ptr<FunctionDescription>> _functions;
	std::vector<std::unique_ptr<VariableDescription>> _variables;
	std::unordered_map<Symbol, std::unique_ptr<PackageDescription>> _packages;
public:
	PackageDescription(Program& program, Symbol name, PackageDescription* owner = nullptr);
	PackageDescription(const PackageDescription&) = delete;
	PackageDescription(PackageDescription&&) noexcept;
	~PackageDescription() override;

	PackageDescription& operator=(const PackageDescription&) = delete;
	PackageDescription& operator=(PackageDescription&&) noexcept;

	[[nodiscard]] Symbol get_name() const;
	[[nodiscard]] SymbolPath get_path() const;
	[[nodiscard]] virtual bool is_global_data() const;

	[[nodiscard]] bool is_root_scope() const override;
	[[nodiscard]] auto* get_owner_package(this auto&& self);

	[[nodiscard]] auto packages();
	[[nodiscard]] PackageDescription* find_package(const Symbol& name) const;
	[[nodiscard]] PackageDescription& get_or_create_package(Program& program, const Symbol& name);

	[[nodiscard]] auto classes() const;
	[[nodiscard]] const ClassDescription* find_class(const Symbol& name) const;
	[[nodiscard]] ClassDescription& create_class(Symbol name, Reference::Flags flags);

	[[nodiscard]] auto functions() const;
	[[nodiscard]] FunctionDefinition find_function(const Symbol& name) const;
	[[nodiscard]] FunctionDescription& create_function(Symbol name, TypeAnnotation return_type);
	[[nodiscard]] FunctionDescription& create_function(TypeAnnotation return_type);

	[[nodiscard]] auto variables() const;
	[[nodiscard]] const VariableDescription* find_variable(const Symbol& name) const;
	[[nodiscard]] VariableDescription& create_variable(VariableDescription variable);
	[[nodiscard]] VariableDescription& create_variable(Symbol name, TypeAnnotation type, Reference::Flags flags);

	[[nodiscard]] PackageData& data() const;

	[[nodiscard]] SymbolDefinition locate(const Symbol& symbol) const override;
	using SymbolScope::locate;

	void cleanup_memory() override;
	void cleanup_metadata() override;

protected:
	PackageDescription(std::unique_ptr<PackageData>&& data, Symbol name, PackageDescription* owner = nullptr);
};

class GlobalDataDescription final : public PackageDescription {
public:
	explicit GlobalDataDescription(Program& program);

	[[nodiscard]] bool is_root_scope() const final;
	[[nodiscard]] bool is_global_data() const final;
	[[nodiscard]] GlobalData& global_data() const;

	[[nodiscard]] ClassDescription& builtin(BuiltinTypeKind type);
};

class MINT_EXPORT ModuleDescription : public SymbolScope {
	SymbolPath _path;
	std::vector<std::unique_ptr<ClassDescription>> _classes;
	std::vector<std::unique_ptr<FunctionDescription>> _functions;
	std::vector<std::unique_ptr<VariableDescription>> _variables;
public:
	explicit ModuleDescription(SymbolPath path);
	ModuleDescription(const ModuleDescription&) = delete;
	ModuleDescription(ModuleDescription&&) = default;
	~ModuleDescription() override = default;

	ModuleDescription& operator=(const ModuleDescription&) = delete;
	ModuleDescription& operator=(ModuleDescription&&) = default;

	[[nodiscard]] Symbol get_name() const;
	[[nodiscard]] SymbolPath get_path() const;

	[[nodiscard]] bool is_root_scope() const override;

	[[nodiscard]] auto classes() const;
	[[nodiscard]] const ClassDescription* find_class(const Symbol& name) const;
	[[nodiscard]] ClassDescription& create_class(Symbol name);

	[[nodiscard]] auto functions() const;
	[[nodiscard]] FunctionDefinition find_function(const Symbol& name) const;
	[[nodiscard]] FunctionDescription& create_function(Symbol name, TypeAnnotation return_type);
	[[nodiscard]] FunctionDescription& create_function(TypeAnnotation return_type);

	[[nodiscard]] auto variables() const;
	[[nodiscard]] const VariableDescription* find_variable(const Symbol& name) const;
	[[nodiscard]] VariableDescription& create_variable(VariableDescription variable);
	[[nodiscard]] VariableDescription& create_variable(Symbol name, TypeAnnotation type, Reference::Flags flags);

	[[nodiscard]] SymbolDefinition locate(const Symbol& symbol) const override;
	using SymbolScope::locate;

	void cleanup_memory() override;
	void cleanup_metadata() override;
};

inline bool mint::FunctionDescription::is_root_scope() const {
	return true;
}

inline auto* FunctionDescription::get_owner_package(this auto&& self) {

	constexpr bool is_const = std::is_const_v<std::remove_reference_t<decltype(self)>>;
	using package_t = std::conditional_t<is_const, const PackageDescription, PackageDescription>;
	using class_t = std::conditional_t<is_const, const ClassDescription, ClassDescription>;

	auto* scope = self.get_owner_scope();

	if (auto* package = dynamic_cast<package_t*>(scope)) {
		return package;
	}

	if (auto* owner = dynamic_cast<class_t*>(scope)) {
		return owner->get_owner_package();
	}

	return static_cast<package_t*>(nullptr);
}

inline auto* FunctionDescription::get_owner_class(this auto&& self) {
	constexpr bool is_const = std::is_const_v<std::remove_reference_t<decltype(self)>>;
	using class_t = std::conditional_t<is_const, const ClassDescription, ClassDescription>;
	return dynamic_cast<class_t*>(self.get_owner_scope());
}

inline auto FunctionDescription::classes() const {
	return std::views::transform(_classes, [](const auto& entry) -> const ClassDescription& {
		return *entry;
	});
}

inline auto FunctionDescription::functions() const {
	return std::views::transform(_functions, [](const auto& function) -> const FunctionDescription& {
		return *function;
	});
}

inline auto FunctionDescription::variables() const {
	return std::views::transform(_variables, [](const auto& variable) -> const VariableDescription& {
		return *variable;
	});
}

inline bool ClassDescription::is_root_scope() const {
	return false;
}

inline auto* ClassDescription::get_owner_package(this auto&& self) {

	constexpr bool is_const = std::is_const_v<std::remove_reference_t<decltype(self)>>;
	using package_t = std::conditional_t<is_const, const PackageDescription, PackageDescription>;
	using class_t = std::conditional_t<is_const, const ClassDescription, ClassDescription>;

	auto* scope = self.get_owner_scope();

	if (auto* package = dynamic_cast<package_t*>(scope)) {
		return package;
	}

	if (auto* owner = dynamic_cast<class_t*>(scope)) {
		return owner->get_owner_package();
	}

	return static_cast<package_t*>(nullptr);
}

inline auto* ClassDescription::get_owner_class(this auto&& self) {
	constexpr bool is_const = std::is_const_v<std::remove_reference_t<decltype(self)>>;
	using class_t = std::conditional_t<is_const, const ClassDescription, ClassDescription>;
	return dynamic_cast<class_t*>(self.get_owner_scope());
}

inline auto ClassDescription::base_classes() const {
	return _bases | std::views::transform([&](const auto& base) {
		const auto base_definition = get_root_scope().locate(base);
		return std::get_if<ClassDefinition>(&base_definition);
	}) | std::views::filter([](const auto* base) {
		return base != nullptr;
	}) | std::views::transform([&](const auto* base) -> const ClassDescription& {
		return base->get();
	});
}

inline auto ClassDescription::classes() const {
	return std::views::transform(_classes, [](const auto& entry) -> const ClassDescriptionEntry& {
		return *entry;
	});
}

inline auto ClassDescription::members() const {
	return std::views::transform(_members, [](const auto& entry) -> const MemberDescriptionEntry& {
		return *entry;
	});
}

inline std::generator<const MemberDescriptionEntry&> ClassDescription::all_members() const {
	for (const auto& member : members()) {
		co_yield member;
	}
	// TODO: filter overriden members
	for (const auto& base : base_classes()) {
		co_yield std::ranges::elements_of(base.all_members());
	}
}

inline bool PackageDescription::is_root_scope() const {
	return false;
}

inline auto* PackageDescription::get_owner_package(this auto&& self) {
	constexpr bool is_const = std::is_const_v<std::remove_reference_t<decltype(self)>>;
	using package_t = std::conditional_t<is_const, const PackageDescription, PackageDescription>;
	return dynamic_cast<package_t*>(self.get_owner_scope());
}

inline auto PackageDescription::packages() {
	return std::views::transform(_packages, [](const auto& package) -> const PackageDescription& {
		return *package.second;
	});
}

inline auto PackageDescription::classes() const {
	return std::views::transform(_classes, [](const auto& entry) -> const ClassDescriptionEntry& {
		return *entry;
	});
}

inline auto PackageDescription::functions() const {
	return std::views::transform(_functions, [](const auto& function) -> const FunctionDescription& {
		return *function;
	});
}

inline auto PackageDescription::variables() const {
	return std::views::transform(_variables, [](const auto& variable) -> const VariableDescription& {
		return *variable;
	});
}

inline bool GlobalDataDescription::is_root_scope() const {
	return true;
}

inline bool ModuleDescription::is_root_scope() const {
	return true;
}

inline auto ModuleDescription::classes() const {
	return std::views::transform(_classes, [](const auto& entry) -> const ClassDescriptionEntry& {
		return *entry;
	});
}

inline auto ModuleDescription::functions() const {
	return std::views::transform(_functions, [](const auto& function) -> const FunctionDescription& {
		return *function;
	});
}

inline auto ModuleDescription::variables() const {
	return std::views::transform(_variables, [](const auto& variable) -> const VariableDescription& {
		return *variable;
	});
}

}

#endif // MINT_COMPILER_DESCRIPTIONS_H
