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

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "mint/memory/class.h"
#include "mint/ast/abstract_syntax_tree.h"
#include "mint/ast/class_description.h"
#include "mint/ast/class_register.h"
#include "mint/ast/module.h"
#include "mint/ast/symbol.h"
#include "mint/memory/data.h"
#include "mint/memory/reference.h"
#include "mint/memory/object.h"
#include "mint/memory/global_data.h"
#include "mint/memory/memory_tools.h"

using namespace mint;

Symbol mint::get_operator_symbol(Class::Operator op) {
	switch (op) {
	case Class::new_operator:
		return builtin_symbols::new_method;
	case Class::delete_operator:
		return builtin_symbols::delete_method;
	case Class::copy_operator:
		return builtin_symbols::copy_operator;
	case Class::call_operator:
		return builtin_symbols::call_operator;
	case Class::add_operator:
		return builtin_symbols::add_operator;
	case Class::sub_operator:
		return builtin_symbols::sub_operator;
	case Class::mul_operator:
		return builtin_symbols::mul_operator;
	case Class::div_operator:
		return builtin_symbols::div_operator;
	case Class::pow_operator:
		return builtin_symbols::pow_operator;
	case Class::mod_operator:
		return builtin_symbols::mod_operator;
	case Class::in_operator:
		return builtin_symbols::in_operator;
	case Class::eq_operator:
		return builtin_symbols::eq_operator;
	case Class::ne_operator:
		return builtin_symbols::ne_operator;
	case Class::lt_operator:
		return builtin_symbols::lt_operator;
	case Class::gt_operator:
		return builtin_symbols::gt_operator;
	case Class::le_operator:
		return builtin_symbols::le_operator;
	case Class::ge_operator:
		return builtin_symbols::ge_operator;
	case Class::and_operator:
		return builtin_symbols::and_operator;
	case Class::or_operator:
		return builtin_symbols::or_operator;
	case Class::band_operator:
		return builtin_symbols::band_operator;
	case Class::bor_operator:
		return builtin_symbols::bor_operator;
	case Class::xor_operator:
		return builtin_symbols::xor_operator;
	case Class::inc_operator:
		return builtin_symbols::inc_operator;
	case Class::dec_operator:
		return builtin_symbols::dec_operator;
	case Class::not_operator:
		return builtin_symbols::not_operator;
	case Class::compl_operator:
		return builtin_symbols::compl_operator;
	case Class::shift_left_operator:
		return builtin_symbols::shift_left_operator;
	case Class::shift_right_operator:
		return builtin_symbols::shift_right_operator;
	case Class::inclusive_range_operator:
		return builtin_symbols::inclusive_range_operator;
	case Class::exclusive_range_operator:
		return builtin_symbols::exclusive_range_operator;
	case Class::subscript_operator:
		return builtin_symbols::subscript_operator;
	case Class::subscript_move_operator:
		return builtin_symbols::subscript_move_operator;
	case Class::regex_match_operator:
		return builtin_symbols::regex_match_operator;
	case Class::regex_unmatch_operator:
		return builtin_symbols::regex_unmatch_operator;
	}
	return {""};
}

std::optional<Class::Operator> mint::get_symbol_operator(const Symbol& symbol) {
	static const std::unordered_map<Symbol, Class::Operator> operators {
	    {builtin_symbols::new_method, Class::new_operator},
	    {builtin_symbols::delete_method, Class::delete_operator},
	    {builtin_symbols::copy_operator, Class::copy_operator},
	    {builtin_symbols::call_operator, Class::call_operator},
	    {builtin_symbols::add_operator, Class::add_operator},
	    {builtin_symbols::sub_operator, Class::sub_operator},
	    {builtin_symbols::mul_operator, Class::mul_operator},
	    {builtin_symbols::div_operator, Class::div_operator},
	    {builtin_symbols::pow_operator, Class::pow_operator},
	    {builtin_symbols::mod_operator, Class::mod_operator},
	    {builtin_symbols::in_operator, Class::in_operator},
	    {builtin_symbols::eq_operator, Class::eq_operator},
	    {builtin_symbols::ne_operator, Class::ne_operator},
	    {builtin_symbols::lt_operator, Class::lt_operator},
	    {builtin_symbols::gt_operator, Class::gt_operator},
	    {builtin_symbols::le_operator, Class::le_operator},
	    {builtin_symbols::ge_operator, Class::ge_operator},
	    {builtin_symbols::and_operator, Class::and_operator},
	    {builtin_symbols::or_operator, Class::or_operator},
	    {builtin_symbols::band_operator, Class::band_operator},
	    {builtin_symbols::bor_operator, Class::bor_operator},
	    {builtin_symbols::xor_operator, Class::xor_operator},
	    {builtin_symbols::inc_operator, Class::inc_operator},
	    {builtin_symbols::dec_operator, Class::dec_operator},
	    {builtin_symbols::not_operator, Class::not_operator},
	    {builtin_symbols::compl_operator, Class::compl_operator},
	    {builtin_symbols::shift_left_operator, Class::shift_left_operator},
	    {builtin_symbols::shift_right_operator, Class::shift_right_operator},
	    {builtin_symbols::inclusive_range_operator, Class::inclusive_range_operator},
	    {builtin_symbols::exclusive_range_operator, Class::exclusive_range_operator},
	    {builtin_symbols::subscript_operator, Class::subscript_operator},
	    {builtin_symbols::subscript_move_operator, Class::subscript_move_operator},
	    {builtin_symbols::regex_match_operator, Class::regex_match_operator},
	    {builtin_symbols::regex_unmatch_operator, Class::regex_unmatch_operator},
	};
	if (auto it = operators.find(symbol); it != operators.end()) {
		return it->second;
	}
	return std::nullopt;
}

Class::Class(PackageData& package, std::string name, Metatype metatype) :
    _metatype(metatype),
    _name(std::move(name)),
    _package(package),
    _operators({}) {
	_operators.fill(nullptr);
	register_root();
}

Class::~Class() {
	unregister_root();
}

Symbol Class::name() const {
	return _description->name();
}

PackageData& Class::get_package() const {
	return _package.get();
}

ClassDescription& Class::get_description() const {
	return *_description;
}

const Class::MemberInfo* Class::find_class(const Symbol& name) const {
	if (auto it = _members.find(name); it != _members.end() && is_instance_of(it->second->value, Data::Format::object)
	                                   && is_class(it->second->value.data<Object>())) {
		return it->second.get();
	}
	return nullptr;
}

Class::MemberInfo* Class::find_class(const Symbol& name) {
	if (auto it = _members.find(name); it != _members.end() && is_instance_of(it->second->value, Data::Format::object)
	                                   && is_class(it->second->value.data<Object>())) {
		return it->second.get();
	}
	return nullptr;
}

const std::vector<std::reference_wrapper<Class>>& Class::bases() const {
	if (_description) {
		return _description->bases();
	}
	static const std::vector<std::reference_wrapper<Class>> g_empty;
	return g_empty;
}

std::size_t Class::size() const {
	return _slots.size();
}

bool Class::is_same(const Class& other) const {
	return this == &other;
}

bool Class::is_base_of(const Class& other) const {
	return std::ranges::any_of(other.bases(), [this](const auto& base) {
		return is_same(base) || is_base_of(base.get());
	});
}

bool Class::is_base_or_same(const Class& other) const {
	if (this == &other) {
		return true;
	}
	return is_base_of(other);
}

bool Class::is_direct_base_or_same(const Class& other) const {
	if (this == &other) {
		return true;
	}
	const auto& other_bases = other.bases();
	return std::ranges::find(other_bases, this, [](const auto& base) {
		return &base.get();
	}) != other_bases.end();
}

const Class::MemberInfo& Class::make_allocate_method_reference(AbstractSyntaxTree& ast) {
	return *_globals
	            .emplace(builtin_symbols::allocate_method,
	                make_member_info({
	                    .owner = std::ref(*this),
	                    .value = make_reference<Function>(Reference::const_address | Reference::const_value
	                                                          | Reference::global | Reference::protected_visibility,
	                        ast.create_global_builtin_method(*this, 0,
	                            [](Class& metadata, Cursor& cursor) {
		                            auto instance = make_reference<Object>(Reference::temporary, metadata);
		                            instance.data<Object>().construct();
		                            cursor.stack().emplace_back(std::move(instance));
	                            })),
	                }))
	            .first->second;
}

bool Class::is_trivially_copyable() const {
	return _trivially_copyable;
}

void Class::disable_trivial_copy() {
	_trivially_copyable = false;
}

void Class::cleanup_memory() {

	_members.clear();

	for (auto member = _globals.begin(); member != _globals.end();) {
		if (is_class(member->second->value)) {
			member = std::next(member);
		}
		else {
			member = _globals.erase(member);
		}
	}

	std::ranges::fill(_operators, nullptr);
}

void Class::cleanup_metadata() {
	_globals.clear();
}

void Class::create_builtin_member(Operator op, Reference&& value) {
	const auto op_index = static_cast<std::size_t>(op);
	assert(op_index < _operators.size());
	assert(_operators[op_index] == nullptr);
	if (ClassRegister::is_slot(value)) {
		auto info = make_member_info({
		    .offset = _slots.size(),
		    .owner = *this,
		    .value = std::move(value),
		});
		_operators[op_index] = info.get();
		_slots.emplace_back(*info);
		_members.emplace(get_operator_symbol(op), std::move(info));
	}
	else {
		auto info = make_member_info({
		    .owner = *this,
		    .value = std::move(value),
		});
		_operators[op_index] = info.get();
		_members.emplace(get_operator_symbol(op), std::move(info));
	}
}

void Class::create_builtin_member(Operator op, std::pair<int, FunctionHandle&> member) {
	const auto op_index = static_cast<std::size_t>(op);
	assert(op_index < _operators.size());
	if (auto* op_info = _operators[op_index]) {
		op_info->value.data<Function>().mapping.insert(member);
	}
	else {
		auto info = make_member_info({
		    .owner = *this,
		    .value = make_reference<Function>(Reference::const_address | Reference::const_value, member),
		});
		_operators[op_index] = info.get();
		_members.emplace(get_operator_symbol(op), std::move(info));
	}
}

void Class::create_builtin_member(const Symbol& symbol, Reference&& value) {
	assert(!_members.contains(symbol));
	if (ClassRegister::is_slot(value)) {
		auto info = make_member_info({
		    .offset = _slots.size(),
		    .owner = *this,
		    .value = std::move(value),
		});
		_slots.emplace_back(*info);
		_members.emplace(symbol, std::move(info));
	}
	else {
		_members.emplace(symbol, make_member_info({
		                             .owner = *this,
		                             .value = std::move(value),
		                         }));
	}
}

void Class::create_builtin_member(const Symbol& symbol, std::pair<int, FunctionHandle&> member) {
	if (auto it = _members.find(symbol); it != _members.end()) {
		it->second->value.data<Function>().mapping.insert(member);
	}
	else {
		_members.emplace(symbol,
		    make_member_info({
		        .owner = *this,
		        .value = make_reference<Function>(Reference::const_address | Reference::const_value, member),
		    }));
	}
}
