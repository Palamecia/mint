#ifndef CLASS_H
#define CLASS_H

#include "Memory/object.h"
#include "Memory/globaldata.h"

#include <string>
#include <map>


#define STRING_TYPE -1
#define ARRAY_TYPE -2
#define HASH_TYPE -3
#define ITERATOR_TYPE -4
#define LIBRARY_TYPE -5

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

class StringClass : public Class {
public:
	static StringClass *instance();

private:
	StringClass();
};

class ArrayClass : public Class {
public:
	static ArrayClass *instance();

private:
	ArrayClass();
};

class HashClass : public Class {
public:
	static HashClass *instance();

private:
	HashClass();
};

class IteratorClass : public Class {
public:
	static IteratorClass *instance();

private:
	IteratorClass();
};

class LibraryClass : public Class {
public:
	static LibraryClass *instance();

private:
	LibraryClass();
};

#endif // CLASS_H
