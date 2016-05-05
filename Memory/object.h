#ifndef OBJECT_H
#define OBJECT_H

#include "Memory/reference.h"
#include <vector>
#include <queue>

class Class;

struct Null : public Data {
	Null();
};

struct None : public Data {
	None();
};

struct Number : public Data {
	double value;
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
	struct compare {
		bool operator ()(const Reference &a, const Reference &b) const;
	};
	std::map<Reference, Reference, compare> values;
};

struct Array : public Data {
	Array();
	std::vector<Reference> values;
};

struct String : public Object {
	String();
	std::string str;
};

struct Iterator : public Object {
	Iterator();
	std::queue<SharedReference> ctx;
};

#endif // OBJECT_H
