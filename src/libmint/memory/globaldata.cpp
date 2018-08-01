#include "memory/globaldata.h"
#include "memory/class.h"
#include "system/error.h"

#include <algorithm>
#include <limits>

using namespace std;
using namespace mint;

Class::MemberInfo *get_member_infos(Class *desc, const string &member) {

	auto it = desc->members().find(member);
	if (it == desc->members().end()) {
		Class::MemberInfo *info = new Class::MemberInfo;
		info->offset = desc->members().size();
		it = desc->members().emplace(member, info).first;
	}

	return it->second;
}

ClassDescription::ClassDescription(Reference::Flags flags, Class *metadata) :
	m_metadata(metadata),
	m_flags(flags),
	m_generated(false) {

}

ClassDescription::~ClassDescription() {
	delete m_metadata;
}

ClassDescription *ClassDescription::Path::locate(PackageData *package) const {

	PackageData *pack = nullptr;
	ClassDescription *desc = nullptr;

	for (const string &symbol : *this) {
		if (desc) {
			desc = desc->findSubClass(symbol);
			if (desc == nullptr) {
				error("expected class name got '%s'", symbol.c_str());
			}
		}
		else if (pack) {
			desc = pack->findClassDescription(symbol);
			if (desc == nullptr) {
				pack = pack->findPackage(symbol);
				if (pack == nullptr) {
					error("expected package or class name got '%s'", symbol.c_str());
				}
			}
		}
		else {
			desc = package->findClassDescription(symbol);
			if (desc == nullptr) {
				pack = package->findPackage(symbol);
				if (pack == nullptr) {
					desc = GlobalData::instance().findClassDescription(symbol);
					if (desc == nullptr) {
						pack = GlobalData::instance().findPackage(symbol);
						if (pack == nullptr) {
							error("expected package or class name got '%s'", symbol.c_str());
						}
					}
				}
			}
		}
	}

	if (desc == nullptr) {
		error("invalid use of package as class");
	}

	return desc;
}

string ClassDescription::Path::toString() const {
	string path;
	for (auto i = begin(); i != end(); ++i) {
		if (i != begin()) {
			path += ".";
		}
		path += *i;
	}
	return path;
}

string ClassDescription::name() const {
	return m_metadata->name();
}

Reference::Flags ClassDescription::flags() const {
	return m_flags;
}

void ClassDescription::addParent(const Path &parent) {
	m_parents.push_back(parent);
}

bool ClassDescription::createMember(const string &name, SharedReference value) {

	auto *context = (value->flags() & Reference::global) ? &m_globals: &m_members;
	return context->emplace(name, value).second;
}

bool ClassDescription::updateMember(const string &name, SharedReference value) {

	auto *context = (value->flags() & Reference::global) ? &m_globals: &m_members;
	auto it = context->find(name);

	if (it != context->end()) {

		SharedReference &member = it->second;

		if (member->flags() != value->flags()) {
			return false;
		}

		if ((member->data()->format == Data::fmt_function) && (value->data()->format == Data::fmt_function)) {
			for (auto def : value->data<Function>()->mapping) {
				if (!member->data<Function>()->mapping.insert(def).second) {
					return false;
				}
			}
			return true;
		}
	}

	return context->emplace(name, value).second;
}

ClassDescription *ClassDescription::findSubClass(const string &name) const {

	for (ClassDescription *desc : m_subClasses) {
		if (desc->name() == name) {
			return desc;
		}
	}

	return nullptr;
}

void ClassDescription::addSubClass(ClassDescription *desc) {
	m_subClasses.push_back(desc);
}

Class *ClassDescription::generate() {

	if (m_generated) {
		return m_metadata;
	}

	for (const Path &path : m_parents) {

		ClassDescription *desc = path.locate(m_metadata->getPackage());
		Class *parent = desc->generate();

		if (parent == nullptr) {
			error("class '%s' was not declared", desc->name().c_str());
		}
		m_metadata->parents().insert(parent);
		for (auto member : parent->members()) {
			Class::MemberInfo *info = new Class::MemberInfo;
			info->offset = m_metadata->members().size();
			info->value.clone(member.second->value);
			info->owner = member.second->owner;
			if (!m_metadata->members().emplace(member.first, info).second) {
				error("member '%s' is ambiguous for class '%s'", member.first.c_str(), m_metadata->name().c_str());
			}
		}
	}

	for (auto member : m_members) {
		Class::MemberInfo *info = get_member_infos(m_metadata, member.first);
		info->value.clone(*member.second);
		info->owner = m_metadata;
	}

	for (auto member : m_globals) {
		Class::MemberInfo *info = new Class::MemberInfo;
		info->offset = Class::MemberInfo::InvalidOffset;
		info->value.clone(*member.second);
		info->owner = m_metadata;
		if (!m_metadata->globals().members().emplace(member.first, info).second) {
			error("global member '%s' cannot be overridden", member.first.c_str());
		}
	}

	for (auto sub : m_subClasses) {
		m_metadata->globals().registerClass(m_metadata->globals().createClass(sub));
	}

	m_generated = true;
	return m_metadata;
}

ClassRegister::ClassRegister() {}

ClassRegister::~ClassRegister() {
	for_each(m_definedClasses.begin(), m_definedClasses.end(), default_delete<ClassDescription>());
}

int ClassRegister::createClass(ClassDescription *desc) {

	size_t id = m_definedClasses.size();
	m_definedClasses.push_back(desc);
	return id;
}

ClassDescription *ClassRegister::findClassDescription(const string &name) const{

	for (ClassDescription *desc : m_definedClasses) {
		if (desc->name() == name) {
			return desc;
		}
	}

	return nullptr;
}

ClassDescription *ClassRegister::getDefinedClass(int id) {

	if (id < static_cast<int>(m_definedClasses.size())) {
		return m_definedClasses[id];
	}

	return nullptr;
}

PackageData::PackageData(const string &name) :
	m_name(name) {

}

PackageData::~PackageData() {
	for (auto package : m_packages) {
		delete package.second;
	}
	m_symbols.clear();
}

PackageData *PackageData::getPackage(const string &name) {
	auto it = m_packages.find(name);
	if (it == m_packages.end()) {
		PackageData *package = new PackageData(name);
		m_symbols.emplace(name, Reference(Reference::const_address | Reference::const_value, Reference::alloc<Package>(package)));
		it = m_packages.emplace(name, package).first;
	}
	return it->second;
}

PackageData *PackageData::findPackage(const std::string &name) const {
	auto it = m_packages.find(name);
	if (it != m_packages.end()) {
		return it->second;
	}
	return nullptr;
}

void PackageData::registerClass(int id) {

	ClassDescription *desc = getDefinedClass(id);

	if (m_classes.find(desc->name()) != m_classes.end()) {
		error("multiple definition of class '%s'", desc->name().c_str());
	}

	m_classes.emplace(desc->name(), desc->generate());
}

Class *PackageData::getClass(const string &name) {

	auto it = m_classes.find(name);
	if (it != m_classes.end()) {
		return it->second;
	}
	return nullptr;
}

string PackageData::name() const {
	return m_name;
}

SymbolTable &PackageData::symbols() {
	return m_symbols;
}

GlobalData::GlobalData() : PackageData("(default)") {

}

GlobalData &GlobalData::instance() {
	static GlobalData g_instance;
	return g_instance;
}
