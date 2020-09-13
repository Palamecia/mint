#ifndef CLASS_H
#define CLASS_H

#include "memory/object.h"
#include "memory/reference.h"
#include "memory/globaldata.h"

#include <limits>
#include <string>
#include <map>

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

	using TypesMapping = std::map<std::string, TypeInfo *>;
	using MembersMapping = std::map<std::string, MemberInfo *>;

	class MINT_EXPORT GlobalMembers : public ClassRegister {
	public:
		GlobalMembers(Class *metadata);
		~GlobalMembers();

		void registerClass(int id) override;
		TypeInfo *getClass(const std::string &name);

		MembersMapping &members();

		void clearGlobalReferences();

	private:
		Class *m_metadata;
		TypesMapping m_classes;
		MembersMapping m_members;
	};

	Object *makeInstance();

	PackageData *getPackage() const;

	std::string name() const;
	Metatype metatype() const;
	const std::set<Class *> &bases() const;
	std::set<Class *> &bases();
	MembersMapping &members();
	GlobalMembers &globals();
	size_t size() const;

	bool isBaseOf(const Class *other) const;
	bool isBaseOrSame(const Class *other) const;

	bool isCopyable() const;
	void disableCopy();

	void clearGlobalReferences();

protected:
	void createBuiltinMember(const std::string &name, int signature, std::pair<int, int> offset);

private:
	PackageData *m_package;
	Metatype m_metatype;
	std::string m_name;
	std::set<Class *> m_bases;
	MembersMapping m_members;
	GlobalMembers m_globals;
	bool m_copyable;
};

}

#endif // CLASS_H
