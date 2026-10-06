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

#include "mint/compiler/class_builder.h"
#include "mint/compiler/descriptions.h"
#include "mint/compiler/symbol_scope.h"
#include "mint/memory/class.h"
#include "mint/memory/data.h"
#include "mint/memory/memory_tools.h"
#include "mint/memory/object.h"
#include "mint/memory/reference.h"
#include "mint/program/symbol.h"
#include "mint/system/error.h"
#include <cassert>
#include <functional>
#include <memory>
#include <span>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

using namespace mint;

namespace {

std::tuple<bool, int> function_signature_mismatch(const Function& expected, const Reference& value) {
	if (is_instance_of(value, Data::Format::function)) {
		const Function::Mapping& mapping = value.data<Function>().mapping;
		for (const auto& [signature, _] : expected.mapping) {
			if (mapping.find(signature) == mapping.end()) [[unlikely]] {
				return {true, signature};
			}
		}
	}
	else if (is_instance_of(value, Data::Format::object)) {
		if (const Class::MemberInfo* member = value.data<Object>().metadata.find_operator(Class::call_operator)) {
			return function_signature_mismatch(expected, member->value);
		}
		for (const auto& [signature, _] : expected.mapping) {
			if (signature != 1) [[unlikely]] {
				return {true, signature};
			}
		}
	}
	else {
		for (const auto& [signature, _] : expected.mapping) {
			if (signature != 1) [[unlikely]] {
				return {true, signature};
			}
		}
	}
	return {false, 0};
}

}

mint::ClassBuilder::ClassBuilder(Program& program) :
    _program(program) {}

std::unique_ptr<Class> mint::ClassBuilder::make_class(const ClassDescription& description) const {

	const auto* owner_package = description.get_owner_package();
	auto metadata = std::make_unique<Class>(owner_package ? owner_package->data() : program().global_data(),
	    description);
	metadata->_description = &description;

	const auto member_definitions = import_member_definitions(description);

	const auto bases = description.bases();
	metadata->_bases.reserve(bases.size());

	auto member_overrides = import_member_overrides(*metadata, description.get_root_scope(), bases, member_definitions);

	for (const auto& [symbol, values] : member_definitions) {
		auto value = make_value(symbol, values);
		if (value.flags() & Reference::global) {
			auto info = make_member_info({
			    .owner = std::ref(*metadata),
			    .value = std::move(value),
			});
			if (!metadata->_globals.emplace(symbol, std::move(info)).second) [[unlikely]] {
				error("global member '{}' cannot be overridden", symbol.str());
			}
		}
		else if (auto op = get_symbol_operator(symbol)) {
			metadata->_operators[*op] = update_member_info(*metadata, symbol, std::move(value), member_overrides);
		}
		else {
			update_member_info(*metadata, symbol, std::move(value), member_overrides);
			if (symbol == builtin_symbols::clone_method) {
				metadata->disable_trivial_copy();
			}
		}
	}

	for (const auto& [desc, flags] : description.classes()) {

		auto symbol = desc.get_name();

		if (metadata->_globals.contains(symbol)) [[unlikely]] {
			error("multiple definition of class '{}'", symbol.str());
		}

		metadata->_globals.emplace(std::move(symbol), make_member_info({
		                                                  .owner = std::ref(*metadata),
		                                                  .value = make_reference<Object>(flags, desc.generate(*this)),
		                                              }));
	}

	metadata->_globals.emplace(builtin_symbols::allocate_method,
	    make_member_info({
	        .owner = std::ref(*metadata),
	        .value = make_reference<Function>(Reference::const_address | Reference::const_value | Reference::global
	                                              | Reference::protected_visibility,
	            _program.get().create_global_builtin_method(*metadata, 0,
	                [](const Class& metadata, Cursor& cursor) {
		                auto instance = make_reference<Object>(Reference::temporary, metadata);
		                instance.data<Object>().construct();
		                cursor.stack().emplace_back(std::move(instance));
	                })),
	    }));

	return metadata;
}

Program& mint::ClassBuilder::program() const {
	return _program;
}

const ClassDescription* mint::ClassBuilder::locate_base_class(const SymbolScope& root_scope,
    const SymbolPath& path) const {
	if (const auto base_definition = root_scope.locate(path);
	    const auto* base_description = std::get_if<ClassDefinition>(&base_definition)) {
		return &base_description->get();
	}
	if (const auto base_definition = _program.get().global_data_description().locate(path);
	    const auto* base_description = std::get_if<ClassDefinition>(&base_definition)) {
		return &base_description->get();
	}
	return nullptr;
}

std::unordered_map<Symbol, std::vector<const Reference*>> mint::ClassBuilder::import_member_definitions(
    const ClassDescription& description) {

	auto member_definitions = std::unordered_map<Symbol, std::vector<const Reference*>>();

	for (const auto& member : description.members()) {
		if (member.value) {
			member_definitions[member.get_name()].push_back(member.value);
		}
	}

	return member_definitions;
}

std::unordered_map<Symbol, std::vector<const Reference*>> mint::ClassBuilder::import_member_overrides(Class& metadata,
    const SymbolScope& root_scope, std::span<const SymbolPath> bases,
    const std::unordered_map<Symbol, std::vector<const Reference*>>& member_definitions) const {

	auto member_overrides = std::unordered_map<Symbol, std::vector<const Reference*>>();

	for (const SymbolPath& path : bases) {

		if (path.empty()) [[unlikely]] {
			error("expected class name in '{}' bases list", metadata.get_full_name());
		}

		const auto* base_description = locate_base_class(root_scope, path);
		if (base_description == nullptr) [[unlikely]] {
			error("expected class name in '{}' bases list, got '{}", metadata.get_full_name(), path.to_string());
		}

		auto& base = base_description->generate(*this);
		metadata._bases.emplace_back(base);

		for (auto [symbol, member] : base.members()) {
			if (member_definitions.contains(symbol)) {
				if (member.value.flags() & Reference::final_member) [[unlikely]] {
					error("member '{}' overrides a final member of '{}' for class '{}'", symbol.str(),
					    base.get_full_name(), metadata.get_full_name());
				}
				member_overrides[symbol].emplace_back(&member.value);
			}
			else {
				const auto [it, uniquely_declared] = metadata._members.emplace(symbol,
				    create_member_info(metadata, member));
				if (!uniquely_declared) [[unlikely]] {
					error("member '{}' is ambiguous for class '{}'", symbol.str(), metadata.get_full_name());
				}
				if (auto op = get_symbol_operator(symbol)) {
					metadata._operators[*op] = it->second.get();
				}
			}
		}

		if (!base.is_trivially_copyable()) {
			metadata.disable_trivial_copy();
		}
	}

	return member_overrides;
}

Reference mint::ClassBuilder::make_value(const Symbol& symbol, const std::vector<const Reference*>& values) {

	assert(!values.empty());

	if (values.size() == 1) {
		return {copy_from, *values.front()};
	}

	auto mapping = Function::Mapping();
	const auto flags = values.front()->flags();

	for (const auto* value : values) {
		if (value->data().format() != Data::Format::function) {
			error("member '{}' was already defined", symbol.str());
		}
		if (value->flags() != flags) {
			error("member '{}' was already defined", symbol.str());
		}
		for (const auto& signature : value->data<Function>().mapping) {
			if (!mapping.insert(signature).second) {
				error("member '{}' was already defined", symbol.str());
			}
		}
	}

	return make_reference<Function>(flags, std::move(mapping));
}

std::unique_ptr<Class::MemberInfo> mint::ClassBuilder::create_member_info(Class& metadata,
    const Class::MemberInfo& member) {
	if (member.offset != Class::MemberInfo::invalid_offset) {
		auto info = make_member_info({
		    .offset = metadata._slots.size(),
		    .owner = member.owner,
		    .value = member.value,
		});
		metadata._slots.emplace_back(*info);
		return info;
	}
	return make_member_info({
	    .owner = member.owner,
	    .value = member.value,
	});
}

Class::MemberInfo* mint::ClassBuilder::update_member_info(Class& metadata, Symbol symbol, Reference value,
    std::unordered_map<Symbol, std::vector<const Reference*>>& member_overrides) {
	auto it = metadata._members.find(symbol);
	if (it == metadata._members.end()) {
		if (is_slot(value)) {
			auto info = make_member_info({
			    .offset = metadata._slots.size(),
			    .owner = std::ref(metadata),
			});
			metadata._slots.emplace_back(*info);
			it = metadata._members.emplace(std::move(symbol), std::move(info)).first;
		}
		else {
			auto info = make_member_info({
			    .owner = std::ref(metadata),
			});
			it = metadata._members.emplace(std::move(symbol), std::move(info)).first;
		}
	}
	else {
		it->second->owner = std::ref(metadata);
	}
	if (value.flags() & Reference::override_member) {
		const auto member_override = member_overrides.find(it->first);
		if (member_override == member_overrides.end()) [[unlikely]] {
			error("member '{}' is marked override but does not override a member for class '{}'", it->first.str(),
			    metadata.get_full_name());
		}
		for (const Reference* base_member : member_override->second) {
			if (is_instance_of(*base_member, Data::Format::function)) {
				if (auto [mismatch, signature] = function_signature_mismatch(base_member->data<Function>(), value);
				    mismatch) [[unlikely]] {
					error("member '{}' is marked override but is missing signature '()'({}) for class '{}'",
					    it->first.str(), signature, metadata.get_full_name());
				}
			}
		}
	}
	Class::MemberInfo& info = *it->second;
	info.value = std::move(value);
	return &info;
}
