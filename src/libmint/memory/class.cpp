#include "memory/class.h"
#include "memory/object.h"
#include "memory/globaldata.h"
#include "memory/memorytool.h"
#include "system/error.h"

using namespace std;
using namespace mint;

static const Symbol OperatorSymbols[] = {
	Symbol::New,
	Symbol::Delete,
	Symbol::CopyOperator,
	Symbol::CallOperator,
	Symbol::AddOperator,
	Symbol::SubOperator,
	Symbol::MulOperator,
	Symbol::DivOperator,
	Symbol::PowOperator,
	Symbol::ModOperator,
	Symbol::InOperator,
	Symbol::EqOperator,
	Symbol::NeOperator,
	Symbol::LtOperator,
	Symbol::GtOperator,
	Symbol::LeOperator,
	Symbol::GeOperator,
	Symbol::AndOperator,
	Symbol::OrOperator,
	Symbol::BandOperator,
	Symbol::BorOperator,
	Symbol::XorOperator,
	Symbol::IncOperator,
	Symbol::DecOperator,
	Symbol::NotOperator,
	Symbol::ComplOperator,
	Symbol::ShiftLeftOperator,
	Symbol::ShiftRightOperator,
	Symbol::InclusiveRangeOperator,
	Symbol::ExclusiveRangeOperator,
	Symbol::SubscriptOperator,
	Symbol::SubscriptMoveOperator,
	Symbol::RegexMatchOperator,
	Symbol::RegexUnmatchOperator
};

static constexpr const size_t OperatorCount = extent<decltype (OperatorSymbols)>::value;

Symbol mint::get_operator_symbol(Class::Operator op) {
	return OperatorSymbols[op];
}

Class::Class(const std::string &name, Metatype metatype) :
	Class(&GlobalData::instance(), name, metatype) {

}

Class::Class(PackageData *package, const std::string &name, Metatype metatype) :
	m_metatype(metatype),
	m_copyable(true),
	m_name(name),
	m_package(package),
	m_description(nullptr),
	m_operators(OperatorCount) {

}

Class::~Class() {
	for (auto &member : m_members) {
		delete member.second;
	}
	for (auto &member : m_globals) {
		delete member.second;
	}
}

Class::MemberInfo *Class::getClass(const Symbol &name) {

	auto it = m_members.find(name);
	if (it != m_members.end() && it->second->value.data()->format == Data::fmt_object && is_class(it->second->value.data<Object>())) {
		return it->second;
	}
	return nullptr;
}

Object *Class::makeInstance() {
	return Reference::alloc<Object>(this);
}

PackageData *Class::getPackage() const {
	return m_package;
}

ClassDescription *Class::getDescription() const {
	return m_description;
}

const set<Class *> &Class::bases() const {
	if (m_description) {
		return m_description->bases();
	}
	static const set<Class *> g_empty;
	return g_empty;
}

size_t Class::size() const {
	return m_members.size();
}

bool Class::isBaseOf(const Class *other) const {
	if (other == nullptr) {
		return false;
	}
	for (const Class *base : other->bases()) {
		if (base == this) {
			return true;
		}
		if (isBaseOf(base)) {
			return true;
		}
	}
	return false;
}

bool Class::isBaseOrSame(const Class *other) const {
	if (other == this) {
		return true;
	}
	return isBaseOf(other);
}

bool Class::isCopyable() const {
	return m_copyable;
}

void Class::disableCopy() {
	m_copyable = false;
}

void Class::cleanupMemory() {

	m_description->cleanupMemory();

	for (auto member = m_globals.begin(); member != m_globals.end();) {
		if (member->second->value.data()->format == Data::fmt_object && is_class(member->second->value.data<Object>())) {
			member->second->value.data<Object>()->metadata->cleanupMemory();
			++member;
		}
		else {
			delete member->second;
			member = m_globals.erase(member);
		}
	}
}

void Class::cleanupMetadata() {

	m_description->cleanupMetadata();

	for (auto &member : m_operators) {
		member = nullptr;
	}

	for (auto &member : m_members) {
		delete member.second;
	}

	m_members.clear();

	for (auto &member : m_globals) {
		if (member.second->value.data()->format == Data::fmt_object && is_class(member.second->value.data<Object>())) {
			member.second->value.data<Object>()->metadata->cleanupMetadata();
		}
		delete member.second;
	}

	m_globals.clear();
}

void Class::createBuiltinMember(Operator op, WeakReference &&value) {
	assert(m_operators[op] == nullptr);
	m_members.emplace(OperatorSymbols[op], m_operators[op] = new MemberInfo { m_members.size(), this, move(value) });
}

void Class::createBuiltinMember(Operator op, std::pair<int, Module::Handle *> member) {

	if (MemberInfo *info = m_operators[op]) {
		Function *data = info->value.data<Function>();
		data->mapping.emplace(member.first, member.second);
	}
	else {
		Function *data = Reference::alloc<Function>();
		data->mapping.emplace(member.first, member.second);
		m_members.emplace(OperatorSymbols[op], m_operators[op] = new MemberInfo{ m_members.size(), this, StrongReference(Reference::const_address | Reference::const_value, data) });
	}
}

void Class::createBuiltinMember(const Symbol &symbol, WeakReference &&value) {
	assert(m_members.find(symbol) == m_members.end());
	m_members.emplace(symbol, new MemberInfo { m_members.size(), this, move(value) });
}

void Class::createBuiltinMember(const Symbol &symbol, std::pair<int, Module::Handle *> member) {

	auto it = m_members.find(symbol);

	if (it != m_members.end()) {
		Function *data = it->second->value.data<Function>();
		data->mapping.emplace(member.first, member.second);
	}
	else {
		Function *data = Reference::alloc<Function>();
		data->mapping.emplace(member.first, member.second);
		m_members.emplace(symbol, new MemberInfo{ m_members.size(), this, StrongReference(Reference::const_address | Reference::const_value, data) });
	}
}
