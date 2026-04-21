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

#include "mint/ast/classregister.h"
#include "mint/ast/abstractsyntaxtree.h"
#include "mint/ast/symbol.h"
#include "mint/memory/data.h"
#include "mint/memory/memorytool.h"
#include "mint/memory/globaldata.h"
#include "mint/memory/class.h"
#include "mint/memory/object.h"
#include "mint/memory/reference.h"
#include "mint/system/error.h"
#include <algorithm>
#include <cassert>
#include <functional>
#include <initializer_list>
#include <memory>
#include <ranges>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <utility>
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

ClassRegister::Path::Path(const Symbol& symbol) :
    _symbols({symbol}) {}

ClassRegister::Path::Path(std::initializer_list<Symbol> symbols) :
    _symbols(symbols) {}

ClassRegister::Path::Path(const Path& other, const Symbol& symbol) :
    _symbols(other._symbols) {
	_symbols.push_back(symbol);
}

ClassRegister::Path::Path(const std::string& path) {
	for (const auto symbol : std::views::split(path, std::string("."))) {
		_symbols.emplace_back(std::string_view(symbol));
	}
}

const ClassDescription& ClassRegister::Path::locate(const ClassRegister& root_register) const {

	auto symbol = _symbols.begin();

	if (symbol == _symbols.end()) [[unlikely]] {
		error("expected package or class name got empty path");
	}

	const auto* class_register = root_register.locate(*symbol);
	if (class_register == nullptr) {
		class_register = root_register.ast().global_data().locate(*symbol);
		if (class_register == nullptr) [[unlikely]] {
			error("expected package or class name got '{}'", symbol->str());
		}
	}

	while (++symbol != _symbols.end()) {
		class_register = class_register->locate(*symbol);
		if (class_register == nullptr) [[unlikely]] {
			error("expected package or class name got '{}'", symbol->str());
		}
	}

	const auto* class_decription = dynamic_cast<const ClassDescription*>(class_register);
	if (class_decription == nullptr) [[unlikely]] {
		error("class '{}' was not declared", to_string());
	}
	return *class_decription;
}

ClassDescription& ClassRegister::Path::locate(ClassRegister& root_register) const {

	auto symbol = _symbols.begin();

	if (symbol == _symbols.end()) [[unlikely]] {
		error("expected package or class name got empty path");
	}

	auto* class_register = root_register.locate(*symbol);
	if (class_register == nullptr) {
		class_register = root_register.ast().global_data().locate(*symbol);
		if (class_register == nullptr) [[unlikely]] {
			error("expected package or class name got '{}'", symbol->str());
		}
	}

	while (++symbol != _symbols.end()) {
		class_register = class_register->locate(*symbol);
		if (class_register == nullptr) [[unlikely]] {
			error("expected package or class name got '{}'", symbol->str());
		}
	}

	auto* class_decription = dynamic_cast<ClassDescription*>(class_register);
	if (class_decription == nullptr) [[unlikely]] {
		error("class '{}' was not declared", to_string());
	}
	return *class_decription;
}

std::string ClassRegister::Path::to_string() const {
	std::string path;
	for (auto i = _symbols.begin(); i != _symbols.end(); ++i) {
		if (i != _symbols.begin()) {
			path += ".";
		}
		path += i->str();
	}
	return path;
}

void ClassRegister::Path::append_symbol(const Symbol& symbol) {
	_symbols.push_back(symbol);
}

void ClassRegister::Path::clear() {
	_symbols.clear();
}

ClassRegister::ClassRegister(AbstractSyntaxTree& ast) :
    _ast(ast) {}

const ClassRegister& ClassRegister::get_root_register() const {
	if (_owner) {
		return _owner->get_root_register();
	}
	return *this;
}

ClassRegister& ClassRegister::get_root_register() {
	if (_owner) {
		return _owner->get_root_register();
	}
	return *this;
}

const ClassRegister* ClassRegister::get_owner_register() const {
	return _owner;
}

ClassRegister* ClassRegister::get_owner_register() {
	return _owner;
}

void ClassRegister::set_owner_register(ClassRegister* owner) {
	_owner = owner;
}

const PackageData* ClassRegister::get_owner_package() const {
	if (_owner == nullptr) {
		return nullptr;
	}
	if (auto* package = _owner->get_package_data()) {
		return package;
	}
	return _owner->get_owner_package();
}

PackageData* ClassRegister::get_owner_package() {
	if (_owner == nullptr) {
		return nullptr;
	}
	if (auto* package = _owner->get_package_data()) {
		return package;
	}
	return _owner->get_owner_package();
}

ClassDescription* ClassRegister::find_class_description(const Symbol& name) const {
	if (auto it = std::ranges::find(_defined_classes, name,
	        [](const auto& entry) {
		        return entry.desc.get().name();
	        });
	    it != _defined_classes.end()) {
		return &it->desc.get();
	}
	return nullptr;
}

void ClassRegister::register_class_description(ClassDescription& desc, Reference::Flags flags) {
	assert(desc._owner == nullptr);
	desc._owner = this;
	_defined_classes.push_back({
	    .desc = std::ref(desc),
	    .flags = flags,
	});
}

ClassRegister* ClassRegister::locate(const Symbol& symbol) const {
	if (auto* class_decription = find_class_description(symbol)) {
		return class_decription;
	}
	return nullptr;
}

const FunctionData* ClassRegister::get_function_data() const {
	return nullptr;
}

FunctionData* ClassRegister::get_function_data() {
	return nullptr;
}

const PackageData* ClassRegister::get_package_data() const {
	return nullptr;
}

PackageData* ClassRegister::get_package_data() {
	return nullptr;
}

void ClassRegister::cleanup_memory() {
	std::ranges::for_each(std::views::reverse(_defined_classes), [](const auto& entry) {
		entry.desc.get().cleanup_memory();
	});
}

void ClassRegister::cleanup_metadata() {
	std::ranges::for_each(std::views::reverse(_defined_classes), [](const auto& entry) {
		entry.desc.get().cleanup_metadata();
	});
	_defined_classes.clear();
}

ClassDescription::ClassDescription(AbstractSyntaxTree& ast, const std::string& name) :
    ClassRegister(ast),
    _name(name) {}

Symbol ClassDescription::name() const {
	return _name;
}

std::string ClassDescription::full_name() const {
	if (const auto* owner = get_owner_class()) {
		return owner->full_name() + "." + name().str();
	}
	if (const auto* package = get_owner_package()) {
		if (package != &ast().global_data()) {
			return package->full_name() + "." + name().str();
		}
	}
	return name().str();
}

ClassDescription::Path ClassDescription::get_path() const {
	if (const auto* owner = get_owner_class()) {
		return {owner->get_path(), name()};
	}
	if (const auto* package = get_owner_package()) {
		return {package->get_path(), name()};
	}
	return {name()};
}

void ClassDescription::add_base(const Path& base) {
	_bases.push_back(base);
}

const ClassDescription* ClassDescription::get_owner_class() const {
	return dynamic_cast<const ClassDescription*>(get_owner_register());
}

ClassDescription* ClassDescription::get_owner_class() {
	return dynamic_cast<ClassDescription*>(get_owner_register());
}

const Reference* ClassDescription::find_member(const Symbol& name) const {
	if (auto it = _members.find(name); it != _members.end()) {
		return &it->second;
	}
	for (const auto& base_path : _bases) {
		if (const auto* reference = base_path.locate(get_root_register()).find_member(name)) {
			if (reference->flags() & Reference::global) {
				return nullptr;
			}
			return reference;
		}
	}
	return nullptr;
}

bool ClassDescription::create_member(const Symbol& name, const Reference& value) {
	return _members.emplace(name, value).second;
}

bool ClassDescription::update_member(const Symbol& name, const Reference& value) {

	if (auto it = _members.find(name); it != _members.end()) {

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

const std::vector<std::reference_wrapper<Class>>& ClassDescription::bases() const {
	return _bases_metadata;
}

Class& ClassDescription::generate() {

	if (_metadata) {
		return *_metadata;
	}

	auto* owner_package = get_owner_package();
	auto& root_register = get_root_register();

	_metadata = std::make_unique<Class>(owner_package ? *owner_package : ast().global_data(), full_name());
	_metadata->_description = this;
	_bases_metadata.reserve(_bases.size());

	auto member_overrides = std::unordered_map<Symbol, std::vector<std::reference_wrapper<const Reference>>>();

	for (const Path& path : _bases) {

		auto& desc = path.locate(root_register);
		auto& base = desc.generate();
		_bases_metadata.emplace_back(base);

		for (auto [symbol, member] : base.members()) {
			if (_members.contains(symbol)) {
				if (member.get().value.flags() & Reference::final_member) [[unlikely]] {
					error("member '{}' overrides a final member of '{}' for class '{}'", symbol.str(), base.full_name(),
					    _metadata->full_name());
				}
				member_overrides[symbol].emplace_back(member.get().value);
			}
			else {
				const auto [it, uniquely_declared] = _metadata->_members.emplace(symbol, create_member_info(member));
				if (!uniquely_declared) [[unlikely]] {
					error("member '{}' is ambiguous for class '{}'", symbol.str(), _metadata->full_name());
				}
				if (auto op = get_symbol_operator(symbol)) {
					_metadata->_operators[*op] = it->second.get();
				}
			}
		}

		if (!base.is_trivially_copyable()) {
			_metadata->disable_trivial_copy();
		}
	}

	for (auto& [symbol, value] : _members) {
		if (value.flags() & Reference::global) {
			auto info = make_member_info({
			    .owner = std::ref(*_metadata),
			    .value = value,
			});
			if (!_metadata->_globals.emplace(symbol, std::move(info)).second) [[unlikely]] {
				error("global member '{}' cannot be overridden", symbol.str());
			}
		}
		else if (auto op = get_symbol_operator(symbol)) {
			_metadata->_operators[*op] = update_member_info(symbol, value, member_overrides);
		}
		else {
			update_member_info(symbol, value, member_overrides);
			if (symbol == builtin_symbols::clone_method) {
				_metadata->disable_trivial_copy();
			}
		}
	}

	for (auto [desc, flags] : class_descriptions()) {

		Symbol&& symbol = desc.name();

		if (_metadata->_globals.contains(symbol)) [[unlikely]] {
			error("multiple definition of class '{}'", symbol.str());
		}

		_metadata->_globals.emplace(symbol, make_member_info({
		                                        .owner = std::ref(*_metadata),
		                                        .value = make_weak_reference<Object>(flags, desc.generate()),
		                                    }));
	}

	return *_metadata;
}

void ClassDescription::cleanup_memory() {

	ClassRegister::cleanup_memory();

	if (_metadata) {
		_metadata->cleanup_memory();
	}

	_members.clear();
}

void ClassDescription::cleanup_metadata() {

	ClassRegister::cleanup_metadata();

	if (_metadata) {
		_metadata->cleanup_metadata();
	}
}

std::unique_ptr<Class::MemberInfo> mint::ClassDescription::create_member_info(const Class::MemberInfo& member) {
	if (member.offset != Class::MemberInfo::invalid_offset) {
		auto info = make_member_info({
		    .offset = _metadata->_slots.size(),
		    .owner = member.owner,
		    .value = member.value,
		});
		_metadata->_slots.emplace_back(*info);
		return info;
	}
	return make_member_info({
	    .owner = member.owner,
	    .value = member.value,
	});
}

Class::MemberInfo* mint::ClassDescription::update_member_info(const Symbol& symbol, WeakReference& value,
    std::unordered_map<Symbol, std::vector<std::reference_wrapper<const Reference>>>& member_overrides) {
	auto& members = _metadata->_members;
	auto it = members.find(symbol);
	if (it == members.end()) {
		if (is_slot(value)) {
			auto info = make_member_info({
			    .offset = _metadata->_slots.size(),
			    .owner = std::ref(*_metadata),
			});
			_metadata->_slots.emplace_back(*info);
			it = members.emplace(symbol, std::move(info)).first;
		}
		else {
			auto info = make_member_info({
			    .owner = std::ref(*_metadata),
			});
			it = members.emplace(symbol, std::move(info)).first;
		}
	}
	else {
		it->second->owner = std::ref(*_metadata);
	}
	if (value.flags() & Reference::override_member) {
		auto member_override = member_overrides.find(symbol);
		if (member_override == member_overrides.end()) [[unlikely]] {
			error("member '{}' is marked override but does not override a member for class '{}'", symbol.str(),
			    _metadata->full_name());
		}
		for (const Reference& base_member : member_override->second) {
			if (is_instance_of(base_member, Data::Format::function)) {
				if (auto [mismatch, signature] = function_signature_mismatch(base_member.data<Function>(), value);
				    mismatch) [[unlikely]] {
					error("member '{}' is marked override but is missing signature '()'({}) for class '{}'",
					    symbol.str(), signature, _metadata->full_name());
				}
			}
		}
	}
	Class::MemberInfo& info = *it->second;
	info.value = value;
	return &info;
}
