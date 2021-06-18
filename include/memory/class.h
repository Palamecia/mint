#ifndef CLASS_H
#define CLASS_H

#include "memory/object.h"
#include "memory/reference.h"
#include "memory/globaldata.h"
#include "ast/symbolmapping.hpp"

#include <limits>
#include <string>
#include <set>

namespace mint {

class MINT_EXPORT Class {
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

	Class(const std::string &name, Metatype metatype = object);
	Class(PackageData *package, const std::string &name, Metatype metatype = object);
	virtual ~Class();

	struct TypeInfo {
		Class *owner;
		Class *description;
		Reference::Flags flags;
	};

	struct MemberInfo {
		static constexpr size_t InvalidOffset = std::numeric_limits<size_t>::max();
		size_t offset;
		Class *owner;
		StrongReference value;
	};

	using TypesMapping = SymbolMapping<TypeInfo *>;
	using MembersMapping = SymbolMapping<MemberInfo *>;

	class MINT_EXPORT GlobalMembers : public ClassRegister {
	public:
		GlobalMembers(Class *metadata);
		~GlobalMembers();

		void registerClass(int id) override;
		TypeInfo *getClass(const Symbol &name);

		inline MembersMapping &members();

		void cleanup();

	private:
		Class *m_metadata;
		TypesMapping m_classes;
		MembersMapping m_members;
	};

	Object *makeInstance();

	PackageData *getPackage() const;

	inline std::string name() const;
	inline Metatype metatype() const;
	inline MembersMapping &members();
	inline GlobalMembers &globals();
	const std::set<Class *> &bases() const;
	std::set<Class *> &bases();
	size_t size() const;

	bool isBaseOf(const Class *other) const;
	bool isBaseOrSame(const Class *other) const;

	bool isCopyable() const;
	void disableCopy();

	void cleanup();

protected:
	void createBuiltinMember(const Symbol &symbol, std::pair<int, Module::Handle *> member);

private:
	PackageData *m_package;
	Metatype m_metatype;
	std::string m_name;
	std::set<Class *> m_bases;
	MembersMapping m_members;
	GlobalMembers m_globals;
	bool m_copyable;
};

std::string Class::name() const { return m_name; }
Class::Metatype Class::metatype() const { return m_metatype; }
Class::MembersMapping &Class::members() { return m_members; }
Class::GlobalMembers &Class::globals() { return m_globals; }

Class::MembersMapping &Class::GlobalMembers::members() { return m_members; }

}

#endif // CLASS_H
