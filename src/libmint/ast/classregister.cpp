/**
 * Copyright (c) 2025 Gauvain CHERY.
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
#include "mint/memory/memorytool.h"
#include "mint/memory/globaldata.h"
#include "mint/memory/class.h"
#include "mint/system/error.h"

using namespace mint;

namespace {

std::tuple<bool, int> function_signature_mismatch(const Function *expected, const Reference &value) {
	if (is_instance_of(value, Data::FMT_FUNCTION)) {
		const Function::Mapping &mapping = value.data<Function>()->mapping;
		for (const auto &[signature, _] : expected->mapping) {
			if (UNLIKELY(mapping.find(signature) == mapping.end())) {
				return {true, signature};
			}
		}
	}
	else if (is_instance_of(value, Data::FMT_OBJECT)) {
		if (const Class::MemberInfo *member = value.data<Object>()->metadata->find_operator(Class::CALL_OPERATOR)) {
			return function_signature_mismatch(expected, member->value);
		}
		for (const auto &[signature, _] : expected->mapping) {
			if (UNLIKELY(signature != 1)) {
				return {true, signature};
			}
		}
	}
	else {
		for (const auto &[signature, _] : expected->mapping) {
			if (UNLIKELY(signature != 1)) {
				return {true, signature};
			}
		}
	}
	return {false, 0};
}

}

ClassRegister::Path::Path(const Symbol &symbol) :
	m_symbols({symbol}) {}

ClassRegister::Path::Path(std::initializer_list<Symbol> symbols) :
	m_symbols(symbols) {}

ClassRegister::Path::Path(const Path &other, const Symbol &symbol) :
	m_symbols(other.m_symbols) {
	m_symbols.push_back(symbol);
}

ClassDescription *ClassRegister::Path::locate() const {

	const PackageData *pack = nullptr;
	ClassDescription *desc = nullptr;

	for (const Symbol &symbol : m_symbols) {
		if (desc) {
			desc = desc->find_class_description(symbol);
			if (UNLIKELY(desc == nullptr)) {
				error("expected class name got '%s'", symbol.str().c_str());
			}
		}
		else if (pack) {
			desc = pack->find_class_description(symbol);
			if (desc == nullptr) {
				pack = pack->find_package(symbol);
				if (UNLIKELY(pack == nullptr)) {
					error("expected package or class name got '%s'", symbol.str().c_str());
				}
			}
		}
		else {
			pack = GlobalData::instance()->find_package(symbol);
			if (pack == nullptr) {
				desc = GlobalData::instance()->find_class_description(symbol);
				if (UNLIKELY(desc == nullptr)) {
					error("expected package or class name got '%s'", symbol.str().c_str());
				}
			}
		}
	}

	if (UNLIKELY(desc == nullptr)) {
		error("invalid use of package as class");
	}

	return desc;
}

std::string ClassRegister::Path::to_string() const {
	std::string path;
	for (auto i = m_symbols.begin(); i != m_symbols.end(); ++i) {
		if (i != m_symbols.begin()) {
			path += ".";
		}
		path += i->str();
	}
	return path;
}

void ClassRegister::Path::append_symbol(const Symbol &symbol) {
	m_symbols.push_back(symbol);
}

void ClassRegister::Path::clear() {
	m_symbols.clear();
}

ClassRegister::~ClassRegister() {
	std::for_each(m_defined_classes.begin(), m_defined_classes.end(), std::default_delete<ClassDescription>());
}

ClassRegister::Id ClassRegister::create_class(ClassDescription *desc) {
	size_t id = m_defined_classes.size();
	m_defined_classes.push_back(desc);
	return static_cast<Id>(id);
}

ClassDescription *ClassRegister::find_class_description(const Symbol &name) const {
	auto it = std::find_if(m_defined_classes.begin(), m_defined_classes.end(), [name](const ClassDescription *desc) {
		return name == desc->name();
	});
	if (it != m_defined_classes.end()) {
		return *it;
	}
	return nullptr;
}

ClassDescription *ClassRegister::get_class_description(Id id) const {

	auto index = static_cast<size_t>(id);

	if (index < m_defined_classes.size()) {
		return m_defined_classes[index];
	}

	return nullptr;
}

size_t ClassRegister::count() const {
	return m_defined_classes.size();
}

void ClassRegister::cleanup_memory() {
	for_each(m_defined_classes.rbegin(), m_defined_classes.rend(), [](ClassDescription *description) {
		description->cleanup_memory();
	});
}

void ClassRegister::cleanup_metadata() {
	for_each(m_defined_classes.rbegin(), m_defined_classes.rend(), [](ClassDescription *description) {
		description->cleanup_metadata();
	});
}

ClassDescription::ClassDescription(PackageData *package, Reference::Flags flags, const std::string &name) :
	m_owner(nullptr),
	m_package(package),
	m_flags(flags),
	m_name(name),
	m_metadata(nullptr) {}

ClassDescription::~ClassDescription() {
	delete m_metadata;
}

Symbol ClassDescription::name() const {
	return m_name;
}

std::string ClassDescription::full_name() const {
	if (m_owner) {
		return m_owner->full_name() + "." + name().str();
	}
	if (m_package != GlobalData::instance()) {
		return m_package->full_name() + "." + name().str();
	}
	return name().str();
}

Reference::Flags ClassDescription::flags() const {
	return m_flags;
}

ClassDescription::Path ClassDescription::get_path() const {
	if (m_owner) {
		return {m_owner->get_path(), name()};
	}
	if (m_package != GlobalData::instance()) {
		return {m_package->get_path(), name()};
	}
	return {name()};
}

void ClassDescription::add_base(const Path &base) {
	m_bases.push_back(base);
}

ClassRegister::Id ClassDescription::create_class(ClassDescription *desc) {
	desc->m_owner = this;
	return ClassRegister::create_class(desc);
}

bool ClassDescription::create_member(Class::Operator op, Reference &&value) {
	return m_operators.emplace(op, std::move(value)).second;
}

bool ClassDescription::create_member(const Symbol &name, Reference &&value) {
	auto *context = (value.flags() & Reference::GLOBAL) ? &m_globals : &m_members;
	return context->emplace(name, std::move(value)).second;
}

bool ClassDescription::update_member(Class::Operator op, Reference &&value) {

	auto it = m_operators.find(op);

	if (it != m_operators.end()) {

		Reference &member = it->second;

		if (member.flags() != value.flags()) {
			return false;
		}

		if ((member.data()->format == Data::FMT_FUNCTION) && (value.data()->format == Data::FMT_FUNCTION)) {
			return std::all_of(value.data<Function>()->mapping.begin(), value.data<Function>()->mapping.end(),
							   [&member](auto def) {
								   return member.data<Function>()->mapping.insert(def).second;
							   });
		}
	}

	return m_operators.emplace(op, std::move(value)).second;
}

bool ClassDescription::update_member(const Symbol &name, Reference &&value) {

	auto *context = (value.flags() & Reference::GLOBAL) ? &m_globals : &m_members;
	auto it = context->find(name);

	if (it != context->end()) {

		Reference &member = it->second;

		if (member.flags() != value.flags()) {
			return false;
		}

		if ((member.data()->format == Data::FMT_FUNCTION) && (value.data()->format == Data::FMT_FUNCTION)) {
			return std::all_of(value.data<Function>()->mapping.begin(), value.data<Function>()->mapping.end(),
							   [&member](auto def) {
								   return member.data<Function>()->mapping.insert(def).second;
							   });
		}
	}

	return context->emplace(name, std::move(value)).second;
}

const std::vector<Class *> &ClassDescription::bases() const {
	return m_bases_metadata;
}

Class *ClassDescription::generate() {

	if (m_metadata) {
		return m_metadata;
	}

	m_metadata = new Class(m_package, full_name());
	m_metadata->m_description = this;
	m_bases_metadata.reserve(m_bases.size());

	SymbolMapping<std::vector<std::reference_wrapper<Reference>>> member_overrides;

	auto create_member_info = [this](Class::MemberInfo *member) -> Class::MemberInfo * {
		if (member->offset != Class::MemberInfo::INVALID_OFFSET) {
			auto *info = new Class::MemberInfo {
				/*.offset = */ m_metadata->m_slots.size(),
				/*.owner = */ member->owner,
				/*.value = */ WeakReference::share(member->value),
			};
			m_metadata->m_slots.push_back(info);
			return info;
		}
		return new Class::MemberInfo {
			/*.offset = */ Class::MemberInfo::INVALID_OFFSET,
			/*.owner = */ member->owner,
			/*.value = */ WeakReference::share(member->value),
		};
	};

	for (const Path &path : m_bases) {

		ClassDescription *desc = path.locate();
		Class *base = desc->generate();

		if (UNLIKELY(base == nullptr)) {
			error("class '%s' was not declared", desc->name().str().c_str());
		}

		m_bases_metadata.emplace_back(base);

		for (auto &[symbol, member] : base->members()) {

			if (m_members.contains(symbol)) {
				if (UNLIKELY(member->value.flags() & Reference::FINAL_MEMBER)) {
					error("member '%s' overrides a final member of '%s' for class '%s'", symbol.str().c_str(),
						  base->full_name().c_str(), m_metadata->full_name().c_str());
				}
				member_overrides[symbol].emplace_back(member->value);
				continue;
			}

			Class::MemberInfo *info = create_member_info(member);
			if (UNLIKELY(!m_metadata->m_members.emplace(symbol, info).second)) {
				error("member '%s' is ambiguous for class '%s'", symbol.str().c_str(), m_metadata->full_name().c_str());
			}
		}

		for (auto &[op, member] : desc->m_operators) {

			if (m_operators.find(op) != m_operators.end()) {
				Symbol symbol = get_operator_symbol(op);
				if (UNLIKELY(member.flags() & Reference::FINAL_MEMBER)) {
					error("member '%s' overrides a final member of '%s' for class '%s'", symbol.str().c_str(),
						  base->full_name().c_str(), m_metadata->full_name().c_str());
				}
				member_overrides[symbol].emplace_back(member);
				continue;
			}

			if (UNLIKELY(m_metadata->find_operator(op))) {
				error("member '%s' is ambiguous for class '%s'", get_operator_symbol(op).str().c_str(),
					  m_metadata->full_name().c_str());
			}

			m_metadata->m_operators[op] = m_metadata->members()[get_operator_symbol(op)];
		}

		if (!base->is_copyable()) {
			m_metadata->disable_copy();
		}
	}

	auto update_member_info = [this, &member_overrides](const Symbol &symbol,
														WeakReference &value) -> Class::MemberInfo * {
		auto &members = m_metadata->m_members;
		auto it = members.find(symbol);
		if (it == members.end()) {
			if (is_slot(value)) {
				auto *info = new Class::MemberInfo {m_metadata->m_slots.size()};
				it = members.emplace(symbol, info).first;
				m_metadata->m_slots.push_back(info);
			}
			else {
				auto *info = new Class::MemberInfo {Class::MemberInfo::INVALID_OFFSET};
				it = members.emplace(symbol, info).first;
			}
		}
		if (value.flags() & Reference::OVERRIDE_MEMBER) {
			auto member_override = member_overrides.find(symbol);
			if (UNLIKELY(member_override == member_overrides.end())) {
				error("member '%s' is marked override but does not override a member for class '%s'",
					  symbol.str().c_str(), m_metadata->full_name().c_str());
			}
			for (const Reference &base_member : member_override->second) {
				if (is_instance_of(base_member, Data::FMT_FUNCTION)) {
					if (auto [mismatch, signature] = function_signature_mismatch(base_member.data<Function>(), value);
						UNLIKELY(mismatch)) {
						error("member '%s' is marked override but is missing signature '()'(%d) for class '%s'",
							  symbol.str().c_str(), signature, m_metadata->full_name().c_str());
					}
				}
			}
		}
		Class::MemberInfo *info = it->second;
		info->value = WeakReference::share(value);
		info->owner = m_metadata;
		return info;
	};

	for (auto &[op, value] : m_operators) {
		m_metadata->m_operators[op] = update_member_info(get_operator_symbol(op), value);
	}

	for (auto &[symbol, value] : m_members) {
		update_member_info(symbol, value);
		if (symbol == builtin_symbols::CLONE_METHOD) {
			m_metadata->disable_copy();
		}
	}

	for (auto &[symbol, value] : m_globals) {
		auto *info = new Class::MemberInfo {
			/*.offset = */ Class::MemberInfo::INVALID_OFFSET,
			/*.owner = */ m_metadata,
			/*.value = */ WeakReference::share(value),
		};
		if (UNLIKELY(!m_metadata->m_globals.emplace(symbol, info).second)) {
			error("global member '%s' cannot be overridden", symbol.str().c_str());
		}
	}

	for (ClassRegister::Id id = 0; ClassDescription *desc = get_class_description(id); ++id) {

		Symbol &&symbol = desc->name();

		if (UNLIKELY(m_metadata->globals().contains(symbol))) {
			error("multiple definition of class '%s'", symbol.str().c_str());
		}

		auto *info = new Class::MemberInfo {
			/*.offset = */ Class::MemberInfo::INVALID_OFFSET,
			/*.owner = */ m_metadata,
			/*.value = */
			WeakReference(Reference::GLOBAL | Reference::CONST_ADDRESS | Reference::CONST_VALUE | desc->flags(),
						  desc->generate()->make_instance()),
		};
		m_metadata->globals().emplace(symbol, info);
	}

	return m_metadata;
}

void ClassDescription::cleanup_memory() {

	ClassRegister::cleanup_memory();

	if (m_metadata) {
		m_metadata->cleanup_memory();
	}

	m_members.clear();
	m_globals.clear();
	m_operators.clear();
}

void ClassDescription::cleanup_metadata() {

	ClassRegister::cleanup_metadata();

	if (m_metadata) {
		m_metadata->cleanup_metadata();
	}
}
