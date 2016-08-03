#ifndef CLASS_H
#define CLASS_H

#include "object.h"

#include <string>
#include <map>


#define STRING_TYPE -1
#define ARRAY_TYPE -2
#define HASH_TYPE -3
#define ITERATOR_TYPE -4

typedef unsigned int uint;

class Class {
public:
	struct MemberInfo {
		size_t offset;
		Class *owner;
		Reference value;
	};

	Class(const std::string &name);
	~Class();

	Object *makeInstance();

	std::string name() const;
	std::map<std::string, MemberInfo *> &members();
	size_t size() const;

protected:
	void createBuiltinMember(const std::string &name, int signature, std::pair<int, int> offset);

private:
	std::string m_name;
	std::map<std::string, MemberInfo *> m_members;
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

#endif // CLASS_H
