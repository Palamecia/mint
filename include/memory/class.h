#ifndef CLASS_H
#define CLASS_H

#include "memory/object.h"
#include "memory/globaldata.h"

#include <string>
#include <map>

#define STRING_TYPE -1
#define ARRAY_TYPE -2
#define HASH_TYPE -3
#define ITERATOR_TYPE -4
#define LIBRARY_TYPE -5
#define LIB_OBJECT_TYPE -6

typedef unsigned int uint;

class Class {
public:
	Class(const std::string &name);
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
	MembersMapping &members();
	GlobalMembers &globals();
	size_t size() const;

protected:
	void createBuiltinMember(const std::string &name, int signature, std::pair<int, int> offset);

private:
	std::string m_name;
	MembersMapping m_members;
	GlobalMembers m_globals;
};

#endif // CLASS_H
