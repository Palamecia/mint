#ifndef CLASS_H
#define CLASS_H

#include "object.h"

#include <string>
#include <map>

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

class IteratorClass : public Class {
public:
	static IteratorClass *instance();

private:
	IteratorClass();
};

#endif // CLASS_H
