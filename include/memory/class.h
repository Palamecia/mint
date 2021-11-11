#ifndef CLASS_H
#define CLASS_H

#include "memory/object.h"
#include "memory/reference.h"
#include "ast/symbolmapping.hpp"

#include <limits>
#include <string>
#include <array>
#include <set>

namespace mint {

class ClassDescription;

class MINT_EXPORT Class {
	friend class ClassDescription;
public:
	enum Metatype {
		object,
		string,
		regex,
		array,
		hash,
		iterator,
		library,
		libobject
	};

	enum Operator {
		new_operator,
		delete_operator,
		copy_operator,
		call_operator,
		add_operator,
		sub_operator,
		mul_operator,
		div_operator,
		pow_operator,
		mod_operator,
		in_operator,
		eq_operator,
		ne_operator,
		lt_operator,
		gt_operator,
		le_operator,
		ge_operator,
		and_operator,
		or_operator,
		band_operator,
		bor_operator,
		xor_operator,
		inc_operator,
		dec_operator,
		not_operator,
		compl_operator,
		shift_left_operator,
		shift_right_operator,
		inclusive_range_operator,
		exclusive_range_operator,
		subscript_operator,
		subscript_move_operator,
		regex_match_operator,
		regex_unmatch_operator
	};

	struct MemberInfo {
		static constexpr size_t InvalidOffset = std::numeric_limits<size_t>::max();
		size_t offset;
		Class *owner;
		StrongReference value;
	};

	using MembersMapping = SymbolMapping<MemberInfo *>;

	Class(const std::string &name, Metatype metatype = object);
	Class(PackageData *package, const std::string &name, Metatype metatype = object);
	virtual ~Class();

	MemberInfo *getClass(const Symbol &name);
	Object *makeInstance();

	inline Metatype metatype() const;
	inline std::string name() const;

	PackageData *getPackage() const;
	ClassDescription *getDescription() const;
	inline MemberInfo *findOperator(Operator op) const;

	inline MembersMapping &members();
	inline MembersMapping &globals();
	size_t size() const;

	const std::set<Class *> &bases() const;
	bool isBaseOf(const Class *other) const;
	bool isBaseOrSame(const Class *other) const;

	bool isCopyable() const;
	void disableCopy();

	void cleanupMemory();
	void cleanupMetadata();

protected:
	void createBuiltinMember(Operator op, WeakReference &&value = WeakReference());
	void createBuiltinMember(Operator op, std::pair<int, Module::Handle *> member);
	void createBuiltinMember(const Symbol &symbol, WeakReference &&value = WeakReference());
	void createBuiltinMember(const Symbol &symbol, std::pair<int, Module::Handle *> member);

private:
	Metatype m_metatype;
	bool m_copyable;

	std::string m_name;
	PackageData *m_package;
	ClassDescription *m_description;

	std::vector<MemberInfo *> m_operators;
	MembersMapping m_members;
	MembersMapping m_globals;
};

Class::Metatype Class::metatype() const { return m_metatype; }
std::string Class::name() const { return m_name; }
Class::MemberInfo *Class::findOperator(Operator op) const { return m_operators[op]; }
Class::MembersMapping &Class::members() { return m_members; }
Class::MembersMapping &Class::globals() { return m_globals; }

MINT_EXPORT Symbol get_operator_symbol(Class::Operator op);

}

#endif // CLASS_H
