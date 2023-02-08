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

using namespace std;
using namespace mint;

static const Symbol operator_symbols[] = {
	Symbol::new_method,
	Symbol::delete_method,
	Symbol::copy_operator,
	Symbol::call_operator,
	Symbol::add_operator,
	Symbol::sub_operator,
	Symbol::mul_operator,
	Symbol::div_operator,
	Symbol::pow_operator,
	Symbol::mod_operator,
	Symbol::in_operator,
	Symbol::eq_operator,
	Symbol::ne_operator,
	Symbol::lt_operator,
	Symbol::gt_operator,
	Symbol::le_operator,
	Symbol::ge_operator,
	Symbol::and_operator,
	Symbol::or_operator,
	Symbol::band_operator,
	Symbol::bor_operator,
	Symbol::xor_operator,
	Symbol::inc_operator,
	Symbol::dec_operator,
	Symbol::not_operator,
	Symbol::compl_operator,
	Symbol::shift_left_operator,
	Symbol::shift_right_operator,
	Symbol::inclusive_range_operator,
	Symbol::exclusive_range_operator,
	Symbol::subscript_operator,
	Symbol::subscript_move_operator,
	Symbol::regex_match_operator,
	Symbol::regex_unmatch_operator
};

static_assert(Class::operator_count == extent<decltype (operator_symbols)>::value);

Symbol mint::get_operator_symbol(Class::Operator op) {
	return operator_symbols[op];
}

optional<Class::Operator> mint::get_symbol_operator(const Symbol &symbol) {
	for (size_t op = 0; op < Class::operator_count; ++op) {
		if (symbol == operator_symbols[op]) {
			return static_cast<Class::Operator>(op);
		}
	}
	return nullopt;
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
	for (auto &member : m_members) {
		delete member.second;
	}
	for (auto &member : m_globals) {
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

const vector<Class *> &Class::bases() const {
	if (m_description) {
		return m_description->bases();
	}
	static const vector<Class *> g_empty;
	return g_empty;
}

size_t Class::size() const {
	return m_slots.size();
}

bool Class::is_base_of(const Class *other) const {
	if (other == nullptr) {
		return false;
	}
	for (const Class *base : other->bases()) {
		if (base == this) {
			return true;
		}
		if (is_base_of(base)) {
			return true;
		}
	}
	return false;
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
	const auto &bases = other->bases();
	return std::find(bases.begin(), bases.end(), this) != bases.end();
}

bool Class::is_copyable() const {
	return m_copyable;
}

void Class::disable_copy() {
	m_copyable = false;
}

void Class::cleanup_memory() {

	for (auto &member : m_members) {
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

	for (auto &member : m_operators) {
		member = nullptr;
	}
}

void Class::cleanup_metadata() {

	for (auto &member : m_globals) {
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
