#ifndef OBJECT_H
#define OBJECT_H

#include "reference.h"
#include <vector>

class Class;

struct Null : public Data {
	Null();
};

struct None : public Data {
	None();
};

struct Number : public Data {
	double data;
	Number();
};

struct Object : public Data {
	Class *metadata;
	Reference *data;
	Object(Class *type);
	virtual ~Object();
};

struct Function : public Data {
	std::map<int, std::pair<int, int>> mapping;
	Function();
};

struct Hash : public Data {
	Hash();

	struct less {
		bool operator ()(const Reference &a, const Reference &b) { return false; /** \todo ??? */ }
	};

	std::map<Reference, Reference, Hash::less> values;
};

struct Array : public Data {
	Array();
	std::vector<Reference> values;
};

struct String : public Object {
	String();
	std::string str;
};

#endif // OBJECT_H
