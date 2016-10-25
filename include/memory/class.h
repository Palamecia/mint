#ifndef CLASS_H
#define CLASS_H

#include "memory/object.h"
#include "memory/globaldata.h"

#include <string>
#include <map>

typedef unsigned int uint;

class Class {
public:
	enum Metatype {
		object,
		string,
		array,
		hash,
		iterator,
		library,
		libobject
	};

	Class(const std::string &name, Metatype metatype = object);
	~Class();

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

	std::string name() const;
	Metatype metatype() const;
	MembersMapping &members();
	GlobalMembers &globals();
	size_t size() const;

protected:
	void createBuiltinMember(const std::string &name, int signature, std::pair<int, int> offset);

private:
	Metatype m_metatype;
	std::string m_name;
	MembersMapping m_members;
	GlobalMembers m_globals;
};

#endif // CLASS_H
