#ifndef STRING_H
#define STRING_H

#include "memory/class.h"
#include "memory/object.h"

class StringClass : public Class {
public:
	static StringClass *instance();

private:
	StringClass();
};

struct String : public Object {
	String();
	std::string str;
};

#endif // STRING_H
