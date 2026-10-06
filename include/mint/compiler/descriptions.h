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
#include <functional>
#include <memory>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace mint {

class Class;
class ClassBuilder;
class ClassDescription;
class GlobalData;
class PackageData;
class Program;

struct MINT_EXPORT VariableDescription {
	Symbol name;
	TypeAnnotation type;
	Reference* data = nullptr;
};

struct MINT_EXPORT ParameterDescription {
	Symbol name;
	TypeAnnotation type;
	Reference::Flags flags = Reference::default_flags;
};

class MINT_EXPORT FunctionDescription : public SymbolScope {
	std::optional<Symbol> _name;
	Reference* _data = nullptr;
	TypeAnnotation _return_type;
	std::vector<ParameterDescription> _parameters;
	std::vector<std::unique_ptr<ClassDescription>> _classes;
	std::vector<std::unique_ptr<FunctionDescription>> _functions;
public:
	explicit FunctionDescription(SymbolScope* owner);
	FunctionDescription(Symbol name, SymbolScope* owner);
	FunctionDescription(const FunctionDescription&) = delete;
	FunctionDescription(FunctionDescription&&) noexcept;
	~FunctionDescription() override;

	FunctionDescription& operator=(const FunctionDescription&) = delete;
	FunctionDescription& operator=(FunctionDescription&&) noexcept;

	[[nodiscard]] std::optional<Symbol> name() const;
	[[nodiscard]] std::string full_name() const;
	[[nodiscard]] SymbolPath get_path() const;

	[[nodiscard]] Reference* data() const;
	void set_data(Reference* data);

	[[nodiscard]] std::span<const ParameterDescription> parameters() const;
	void add_parameter(ParameterDescription parameters);

	[[nodiscard]] TypeAnnotation get_return_type() const;
	void set_return_type(TypeAnnotation return_type);

	ClassDescription& create_class(Symbol name);

	FunctionDescription& create_function(Symbol name);
	FunctionDescription& create_function();

	[[nodiscard]] SymbolDefinition locate(const Symbol& symbol) const override;

	void cleanup_memory() override;
	void cleanup_metadata() override;
};

struct ClassDescriptionEntry {
	std::unique_ptr<ClassDescription> description;
	Reference::Flags flags = Reference::default_flags;
};

class MINT_EXPORT PackageDescription : public SymbolScope {
	Symbol _name;
	std::unique_ptr<PackageData> _data;
	std::vector<ClassDescriptionEntry> _defined_classes;
	std::vector<std::unique_ptr<VariableDescription>> _globals;
	std::vector<std::unique_ptr<FunctionDescription>> _functions;
	std::unordered_map<Symbol, std::unique_ptr<PackageDescription>> _packages;
public:
	PackageDescription(Program& program, Symbol name, PackageDescription* owner = nullptr);
	PackageDescription(const PackageDescription&) = delete;
	PackageDescription(PackageDescription&&) noexcept;
	~PackageDescription() override;

	PackageDescription& operator=(const PackageDescription&) = delete;
	PackageDescription& operator=(PackageDescription&&) noexcept;

	[[nodiscard]] Symbol name() const;
	[[nodiscard]] std::string full_name() const;
	[[nodiscard]] SymbolPath get_path() const;
	[[nodiscard]] virtual bool is_global_data() const;

	[[nodiscard]] const PackageDescription* get_owner_package() const;
	[[nodiscard]] PackageDescription* get_owner_package();

	[[nodiscard]] PackageDescription* find_package(const Symbol& name) const;
	[[nodiscard]] PackageDescription& get_or_create_package(Program& program, const Symbol& name);

	/*[[nodiscard]] auto packages() {
		return std::views::transform(_packages,
		    [](auto& item) -> std::pair<Symbol, std::reference_wrapper<PackageDescription>> {
			    return {item.first, *item.second};
		    });
	}*/

	[[nodiscard]] ClassDescription* find_class(const Symbol& name) const;
	[[nodiscard]] ClassDescription& create_class(Symbol name, Reference::Flags flags);

	/*[[nodiscard]] auto classes() {
		return std::views::filter(_symbols, [](auto& item) {
			return item.second.data().format() == Data::Format::object
			       && item.second.template data<Object>().data == nullptr;
		}) | std::views::transform([](auto& item) -> std::pair<Symbol, std::reference_wrapper<Class>> {
			return {item.first, item.second.template data<Object>().metadata};
		});
	}*/

	FunctionDescription& create_function(Symbol name);
	FunctionDescription& create_function();

	[[nodiscard]] PackageData& data() const;

	[[nodiscard]] SymbolDefinition locate(const Symbol& symbol) const override;

	void cleanup_memory() override;
	void cleanup_metadata() override;

protected:
	PackageDescription(std::unique_ptr<PackageData>&& data, Symbol name, PackageDescription* owner = nullptr);
};

class GlobalDataDescription final : public PackageDescription {
public:
	explicit GlobalDataDescription(Program& program);

	[[nodiscard]] bool is_global_data() const final;
	[[nodiscard]] GlobalData& global_data() const;
};

struct MemberDescriptionEntry {
	std::variant<std::unique_ptr<VariableDescription>, std::unique_ptr<FunctionDescription>> description;
	Reference::Flags flags = Reference::default_flags;
};

class MINT_EXPORT ClassDescription : public SymbolScope {
	Symbol _name;
	std::vector<SymbolPath> _bases;
	std::vector<std::unique_ptr<MemberDescriptionEntry>> _members;
	std::vector<std::unique_ptr<ClassDescriptionEntry>> _classes;

	mutable std::unique_ptr<Class> _metadata;
	mutable std::vector<std::reference_wrapper<Class>> _bases_metadata;
public:
	explicit ClassDescription(Symbol name, SymbolScope* owner);
	ClassDescription(const ClassDescription&) = delete;
	ClassDescription(ClassDescription&&) noexcept;
	~ClassDescription() override;

	ClassDescription& operator=(const ClassDescription&) = delete;
	ClassDescription& operator=(ClassDescription&&) noexcept;

	[[nodiscard]] Symbol name() const;
	[[nodiscard]] std::string full_name() const;
	[[nodiscard]] SymbolPath get_path() const;

	[[nodiscard]] const PackageDescription* get_owner_package() const;
	[[nodiscard]] PackageDescription* get_owner_package();

	[[nodiscard]] const ClassDescription* get_owner_class() const;
	[[nodiscard]] ClassDescription* get_owner_class();

	[[nodiscard]] std::span<const SymbolPath> bases() const;
	[[nodiscard]] bool is_same(const SymbolPath& other) const;
	[[nodiscard]] bool is_derived_from(const SymbolPath& other) const;
	void add_base(const SymbolPath& base);

	[[nodiscard]] auto classes() const {
		return std::views::transform(_classes, [](const auto& entry) -> const ClassDescriptionEntry& {
			return *entry;
		});
	}

	ClassDescription* find_class(const Symbol& name) const;
	ClassDescription& create_class(Symbol name, Reference::Flags flags);

	[[nodiscard]] bool has_member(const Symbol& name) const;
	[[nodiscard]] const Reference* find_member(const Symbol& name) const;
	[[nodiscard]] std::span<const MemberDescriptionEntry> members() const;
	VariableDescription& create_attribute(Symbol name, TypeAnnotation type, Reference::Flags flags);
	FunctionDescription& create_method(Symbol name, Reference::Flags flags);
	FunctionDescription& update_method(Symbol name, Reference::Flags flags);

	[[nodiscard]] std::span<std::reference_wrapper<Class>> generated_bases() const;
	[[nodiscard]] Class& generate(const ClassBuilder& builder) const;

	[[nodiscard]] SymbolDefinition locate(const Symbol& symbol) const override;

	void cleanup_memory() override;
	void cleanup_metadata() override;

	void mark() {
		for (const auto& member : _members) {
			member.second.data().mark();
		}
	}
};

class MINT_EXPORT ModuleDescription : public SymbolScope {
	SymbolPath _path;
	std::vector<std::unique_ptr<ClassDescription>> _classes;
	std::vector<std::unique_ptr<FunctionDescription>> _functions;
public:
	explicit ModuleDescription(SymbolPath path);
	ModuleDescription(const ModuleDescription&) = delete;
	ModuleDescription(ModuleDescription&&) = default;
	~ModuleDescription() override = default;

	ModuleDescription& operator=(const ModuleDescription&) = delete;
	ModuleDescription& operator=(ModuleDescription&&) = default;

	[[nodiscard]] SymbolPath get_path() const;

	ClassDescription& create_class(Symbol name);

	FunctionDescription& create_function(Symbol name);
	FunctionDescription& create_function();

	[[nodiscard]] SymbolDefinition locate(const Symbol& symbol) const override;

	void cleanup_memory() override;
	void cleanup_metadata() override;

	void mark() {
		for (const auto& desc : _classes) {
			desc->mark();
		}
	}
};

MINT_EXPORT Symbol get_class_name(const ClassDescriptionEntry& entry);
MINT_EXPORT Symbol get_member_name(const MemberDescriptionEntry& entry);
MINT_EXPORT Reference* get_member_data(const MemberDescriptionEntry& entry);

/*[[nodiscard]] auto class_descriptions() const {
	return std::views::transform(_defined_classes,
	    [](const auto& entry) -> std::pair<ClassDescription&, Reference::Flags> {
		    return {entry.desc, entry.flags};
	    });
}*/

}

#endif // MINT_COMPILER_DESCRIPTIONS_H
