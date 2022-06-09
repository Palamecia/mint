#include "ast/classregister.h"
#include "memory/globaldata.h"
#include "memory/class.h"
#include "system/error.h"

using namespace mint;
using namespace std;

static Class::MemberInfo *get_member_infos(Class *desc, const Symbol &member) {

	auto it = desc->members().find(member);
	if (it == desc->members().end()) {
		Class::MemberInfo *info = new Class::MemberInfo;
		info->offset = desc->members().size();
		it = desc->members().emplace(member, info).first;
	}

	return it->second;
}

ClassRegister::ClassRegister() {}

ClassRegister::~ClassRegister() {
	for_each(m_definedClasses.begin(), m_definedClasses.end(), default_delete<ClassDescription>());
}

ClassRegister::Id ClassRegister::createClass(ClassDescription *desc) {
	size_t id = m_definedClasses.size();
	m_definedClasses.push_back(desc);
	return static_cast<Id>(id);
}

ClassDescription *ClassRegister::findClassDescription(const Symbol &name) const{

	for (ClassDescription *desc : m_definedClasses) {
		if (name == desc->name()) {
			return desc;
		}
	}

	return nullptr;
}

ClassDescription *ClassRegister::getClassDescription(Id id)const {

	size_t index = static_cast<size_t>(id);

	if (index < m_definedClasses.size()) {
		return m_definedClasses[index];
	}

	return nullptr;
}

size_t ClassRegister::count() const {
	return m_definedClasses.size();
}

ClassDescription::ClassDescription(PackageData *package, Reference::Flags flags, const string &name) :
	m_owner(nullptr),
	m_package(package),
	m_flags(flags),
	m_name(name),
	m_metadata(nullptr) {

}

ClassDescription::~ClassDescription() {
	delete m_metadata;
}

ClassDescription *ClassDescription::Path::locate(PackageData *package) const {

	PackageData *pack = nullptr;
	ClassDescription *desc = nullptr;

	for (const Symbol &symbol : m_symbols) {
		if (desc) {
			desc = desc->findClassDescription(symbol);
			if (UNLIKELY(desc == nullptr)) {
				string symbol_str = symbol.str();
				error("expected class name got '%s'", symbol_str.c_str());
			}
		}
		else if (pack) {
			desc = pack->findClassDescription(symbol);
			if (desc == nullptr) {
				pack = pack->findPackage(symbol);
				if (UNLIKELY(pack == nullptr)) {
					string symbol_str = symbol.str();
					error("expected package or class name got '%s'", symbol_str.c_str());
				}
			}
		}
		else {
			desc = package->findClassDescription(symbol);
			if (desc == nullptr) {
				pack = package->findPackage(symbol);
				if (pack == nullptr) {
					desc = GlobalData::instance()->findClassDescription(symbol);
					if (desc == nullptr) {
						pack = GlobalData::instance()->findPackage(symbol);
						if (UNLIKELY(pack == nullptr)) {
							string symbol_str = symbol.str();
							error("expected package or class name got '%s'", symbol_str.c_str());
						}
					}
				}
			}
		}
	}

	if (UNLIKELY(desc == nullptr)) {
		error("invalid use of package as class");
	}

	return desc;
}

string ClassDescription::Path::toString() const {
	string path;
	for (auto i = m_symbols.begin(); i != m_symbols.end(); ++i) {
		if (i != m_symbols.begin()) {
			path += ".";
		}
		path += i->str();
	}
	return path;
}

void ClassDescription::Path::appendSymbol(const Symbol &symbol) {
	m_symbols.push_back(symbol);
}

void ClassDescription::Path::clear() {
	m_symbols.clear();
}

Symbol ClassDescription::name() const {
	return m_name;
}

string ClassDescription::fullName() const {

	if (m_owner) {
		return m_owner->fullName() + "." + name().str();
	}

	if (m_package != GlobalData::instance()) {
		return m_package->fullName() + "." + name().str();
	}

	return name().str();
}

Reference::Flags ClassDescription::flags() const {
	return m_flags;
}

void ClassDescription::addBase(const Path &base) {
	m_bases.push_back(base);
}

ClassRegister::Id ClassDescription::createClass(ClassDescription *desc) {
	desc->m_owner = this;
	return ClassRegister::createClass(desc);
}

bool ClassDescription::createMember(Class::Operator op, Reference &&value) {
	return m_operators.emplace(op, move(value)).second;
}

bool ClassDescription::createMember(const Symbol &name, Reference &&value) {

	auto *context = (value.flags() & Reference::global) ? &m_globals: &m_members;
	return context->emplace(name, move(value)).second;
}

bool ClassDescription::updateMember(Class::Operator op, Reference &&value) {

	auto it = m_operators.find(op);

	if (it != m_operators.end()) {

		Reference &member = it->second;

		if (member.flags() != value.flags()) {
			return false;
		}

		if ((member.data()->format == Data::fmt_function) && (value.data()->format == Data::fmt_function)) {
			for (auto def : value.data<Function>()->mapping) {
				if (!member.data<Function>()->mapping.insert(def).second) {
					return false;
				}
			}
			return true;
		}
	}

	return m_operators.emplace(op, move(value)).second;
}

bool ClassDescription::updateMember(const Symbol &name, Reference &&value) {

	auto *context = (value.flags() & Reference::global) ? &m_globals: &m_members;
	auto it = context->find(name);

	if (it != context->end()) {

		Reference &member = it->second;

		if (member.flags() != value.flags()) {
			return false;
		}

		if ((member.data()->format == Data::fmt_function) && (value.data()->format == Data::fmt_function)) {
			for (auto def : value.data<Function>()->mapping) {
				if (!member.data<Function>()->mapping.insert(def).second) {
					return false;
				}
			}
			return true;
		}
	}

	return context->emplace(name, move(value)).second;
}

const set<Class *> &ClassDescription::bases() const {
	return m_basesMetadata;
}

Class *ClassDescription::generate() {

	if (m_metadata) {
		return m_metadata;
	}

	m_metadata = new Class(m_package, fullName());
	m_metadata->m_description = this;

	for (const Path &path : m_bases) {

		ClassDescription *desc = path.locate(m_metadata->getPackage());
		Class *base = desc->generate();

		if (UNLIKELY(base == nullptr)) {
			string name_str = desc->name().str();
			error("class '%s' was not declared", name_str.c_str());
		}

		m_basesMetadata.insert(base);

		for (auto &member : base->members()) {

			if (m_members.find(member.first) != m_members.end()) {
				continue;
			}

			Class::MemberInfo *info = new Class::MemberInfo {
				m_metadata->members().size(),
				member.second->owner,
				WeakReference::share(member.second->value)
			};

			if (UNLIKELY(!m_metadata->members().emplace(member.first, info).second)) {
				string member_str = member.first.str();
				string name_str = m_metadata->name();
				error("member '%s' is ambiguous for class '%s'", member_str.c_str(), name_str.c_str());
			}
		}

		for (size_t i = 0; i < m_operators.size(); ++i) {

			Class::Operator op = static_cast<Class::Operator>(i);

			if (!base->findOperator(op)) {
				continue;
			}

			if (m_operators.find(op) != m_operators.end()) {
				continue;
			}

			if (m_metadata->findOperator(op)) {
				string operator_str = get_operator_symbol(op).str();
				string name_str = m_metadata->name();
				error("member '%s' is ambiguous for class '%s'", operator_str.c_str(), name_str.c_str());
			}

			m_metadata->m_operators[op] = m_metadata->members()[get_operator_symbol(op)];
		}
	}

	for (auto &member : m_operators) {
		Class::MemberInfo *info = get_member_infos(m_metadata, get_operator_symbol(member.first));
		info->owner = m_metadata;
		info->value = WeakReference::share(member.second);
		m_metadata->m_operators[member.first] = info;
	}

	for (auto &member : m_members) {
		Class::MemberInfo *info = get_member_infos(m_metadata, member.first);
		info->owner = m_metadata;
		info->value = WeakReference::share(member.second);
	}

	for (auto &member : m_globals) {
		Class::MemberInfo *info = new Class::MemberInfo {
			Class::MemberInfo::InvalidOffset,
			m_metadata,
			WeakReference::share(member.second)
		};
		if (UNLIKELY(!m_metadata->globals().emplace(member.first, info).second)) {
			string member_str = member.first.str();
			error("global member '%s' cannot be overridden", member_str.c_str());
		}
	}

	for (ClassRegister::Id id = 0; ClassDescription *desc = getClassDescription(id); ++id) {

		Symbol &&symbol = desc->name();

		if (UNLIKELY(m_metadata->globals().find(symbol) != m_metadata->globals().end())) {
			string symbol_str = symbol.str();
			error("multiple definition of class '%s'", symbol_str.c_str());
		}

		Class::MemberInfo *info = new Class::MemberInfo {
			Class::MemberInfo::InvalidOffset,
			m_metadata,
			WeakReference(Reference::global | Reference::const_address | Reference::const_value | desc->flags(), desc->generate()->makeInstance())
		};
		m_metadata->globals().emplace(symbol, info);
	}

	return m_metadata;
}

void ClassDescription::cleanupMemory() {

}

void ClassDescription::cleanupMetadata() {
	m_operators.clear();
	m_members.clear();
	m_globals.clear();
}
