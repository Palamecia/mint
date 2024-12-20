/**
 * Copyright (c) 2024 Gauvain CHERY.
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

#include "mint/memory/class.h"
#include "mint/memory/object.h"
#include "mint/memory/globaldata.h"
#include "mint/memory/memorytool.h"

using namespace mint;

static const Symbol operator_symbols[] = {
	builtin_symbols::new_method,
	builtin_symbols::delete_method,
	builtin_symbols::copy_operator,
	builtin_symbols::call_operator,
	builtin_symbols::add_operator,
	builtin_symbols::sub_operator,
	builtin_symbols::mul_operator,
	builtin_symbols::div_operator,
	builtin_symbols::pow_operator,
	builtin_symbols::mod_operator,
	builtin_symbols::in_operator,
	builtin_symbols::eq_operator,
	builtin_symbols::ne_operator,
	builtin_symbols::lt_operator,
	builtin_symbols::gt_operator,
	builtin_symbols::le_operator,
	builtin_symbols::ge_operator,
	builtin_symbols::and_operator,
	builtin_symbols::or_operator,
	builtin_symbols::band_operator,
	builtin_symbols::bor_operator,
	builtin_symbols::xor_operator,
	builtin_symbols::inc_operator,
	builtin_symbols::dec_operator,
	builtin_symbols::not_operator,
	builtin_symbols::compl_operator,
	builtin_symbols::shift_left_operator,
	builtin_symbols::shift_right_operator,
	builtin_symbols::inclusive_range_operator,
	builtin_symbols::exclusive_range_operator,
	builtin_symbols::subscript_operator,
	builtin_symbols::subscript_move_operator,
	builtin_symbols::regex_match_operator,
	builtin_symbols::regex_unmatch_operator
};

static_assert(Class::operator_count == std::size(operator_symbols));

Symbol mint::get_operator_symbol(Class::Operator op) {
	return operator_symbols[op];
}

std::optional<Class::Operator> mint::get_symbol_operator(const Symbol &symbol) {
	for (size_t op = 0; op < Class::operator_count; ++op) {
		if (symbol == operator_symbols[op]) {
			return static_cast<Class::Operator>(op);
		}
	}
	return std::nullopt;
}

Class::Class(const std::string &name, Metatype metatype) :
	Class(GlobalData::instance(), name, metatype) {

}

Class::Class(PackageData *package, const std::string &name, Metatype metatype) :
	m_metatype(metatype),
	m_name(name),
	m_package(package) {
	m_operators.fill(nullptr);
}

Class::~Class() {
	for (const auto &member : m_members) {
		delete member.second;
	}
	for (const auto &member : m_globals) {
		delete member.second;
	}
}

Class::MemberInfo *Class::get_class(const Symbol &name) {

	auto it = m_members.find(name);
	if (it != m_members.end() && it->second->value.data()->format == Data::fmt_object && is_class(it->second->value.data<Object>())) {
		return it->second;
	}
	return nullptr;
}

Object *Class::make_instance() {
	return GarbageCollector::instance().alloc<Object>(this);
}

Symbol Class::name() const {
	return m_description->name();
}

PackageData *Class::get_package() const {
	return m_package;
}

ClassDescription *Class::get_description() const {
	return m_description;
}

const std::vector<Class *> &Class::bases() const {
	if (m_description) {
		return m_description->bases();
	}
	static const std::vector<Class *> g_empty;
	return g_empty;
}

size_t Class::size() const {
	return m_slots.size();
}

bool Class::is_base_of(const Class *other) const {
	if (other == nullptr) {
		return false;
	}
	const std::vector<Class *> &other_bases = other->bases();
	return std::any_of(other_bases.begin(), other_bases.end(), [this](const Class *base) {
		return base == this || is_base_of(base);
	});
}

bool Class::is_base_or_same(const Class *other) const {
	if (other == this) {
		return true;
	}
	return is_base_of(other);
}

bool Class::is_direct_base_or_same(const Class *other) const {
	if (other == this) {
		return true;
	}
	const auto &other_bases = other->bases();
	return std::find(other_bases.begin(), other_bases.end(), this) != other_bases.end();
}

bool Class::is_copyable() const {
	return m_copyable;
}

void Class::disable_copy() {
	m_copyable = false;
}

void Class::cleanup_memory() {

	for (const auto &member : m_members) {
		delete member.second;
	}

	m_members.clear();

	for (auto member = m_globals.begin(); member != m_globals.end();) {
		if (is_class(member->second->value)) {
			member = std::next(member);
		}
		else {
			delete member->second;
			member = m_globals.erase(member);
		}
	}

	std::fill(m_operators.begin(), m_operators.end(), nullptr);
}

void Class::cleanup_metadata() {

	for (const auto &member : m_globals) {
		delete member.second;
	}

	m_globals.clear();
}

void Class::create_builtin_member(Operator op, WeakReference &&value) {
	assert(m_operators[op] == nullptr);
	if (ClassRegister::is_slot(value)) {
		MemberInfo *info = new MemberInfo { m_slots.size(), this, std::move(value) };
		m_members.emplace(operator_symbols[op], m_operators[op] = info);
		m_slots.push_back(info);
	}
	else {
		m_members.emplace(operator_symbols[op], m_operators[op] = new MemberInfo { MemberInfo::invalid_offset, this, std::move(value) });
	}
}

void Class::create_builtin_member(Operator op, std::pair<int, Module::Handle *> member) {

	if (MemberInfo *info = m_operators[op]) {
		Function *data = info->value.data<Function>();
		data->mapping.emplace(member.first, member.second);
	}
	else {
		Function *data = GarbageCollector::instance().alloc<Function>();
		data->mapping.emplace(member.first, member.second);
		m_members.emplace(operator_symbols[op], m_operators[op] = new MemberInfo { MemberInfo::invalid_offset, this, WeakReference(Reference::const_address | Reference::const_value, data) });
	}
}

void Class::create_builtin_member(const Symbol &symbol, WeakReference &&value) {
	assert(!m_members.contains(symbol));
	if (ClassRegister::is_slot(value)) {
		MemberInfo *info = new MemberInfo { m_slots.size(), this, std::move(value) };
		m_members.emplace(symbol,info);
		m_slots.push_back(info);
	}
	else {
		m_members.emplace(symbol, new MemberInfo { MemberInfo::invalid_offset, this, std::move(value) });
	}
}

void Class::create_builtin_member(const Symbol &symbol, std::pair<int, Module::Handle *> member) {

	auto it = m_members.find(symbol);

	if (it != m_members.end()) {
		Function *data = it->second->value.data<Function>();
		data->mapping.emplace(member.first, member.second);
	}
	else {
		Function *data = GarbageCollector::instance().alloc<Function>();
		data->mapping.emplace(member.first, member.second);
		m_members.emplace(symbol, new MemberInfo { MemberInfo::invalid_offset, this, WeakReference(Reference::const_address | Reference::const_value, data) });
	}
}
