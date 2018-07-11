#ifndef CLASS_H
#define CLASS_H

#include "memory/object.h"
#include "memory/globaldata.h"

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

	struct MemberInfo {
		size_t offset;
		Class *owner;
		Reference value;
	};

	typedef std::map<std::string, MemberInfo *> MembersMapping;

	class GlobalMembers : public ClassRegister {
	public:
		GlobalMembers();
		~GlobalMembers();

		MembersMapping &members();

	private:
		MembersMapping m_members;
	};

	Object *makeInstance();

	PackageData *getPackage() const;

	std::string name() const;
	Metatype metatype() const;
	std::set<Class *> &parents();
	MembersMapping &members();
	GlobalMembers &globals();
	size_t size() const;

protected:
	void createBuiltinMember(const std::string &name, int signature, std::pair<int, int> offset);

private:
	PackageData *m_package;
	Metatype m_metatype;
	std::string m_name;
	std::set<Class *> m_parents;
	MembersMapping m_members;
	GlobalMembers m_globals;
};

}

#endif // CLASS_H
