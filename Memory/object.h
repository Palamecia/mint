#ifndef OBJECT_H
#define OBJECT_H

#include "Memory/reference.h"
#include <memory>
#include <vector>
#include <deque>

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
	void construct();
};

struct Function : public Data {
	Function();
	typedef std::map<int, std::pair<int, int>> mapping_type;
	mapping_type mapping;
};

struct String : public Object {
	String();
	std::string str;
};

struct Array : public Object {
	Array();
	typedef std::vector<std::unique_ptr<Reference>> values_type;
	values_type values;
};

struct Hash : public Object {
	Hash();
	struct compare {
		bool operator ()(const Reference &a, const Reference &b) const;
	};
	typedef std::map<Reference, Reference, compare> values_type;
	values_type values;
};

struct Iterator : public Object {
	Iterator();
	typedef std::deque<SharedReference> ctx_type;
	ctx_type ctx;
};

#endif // OBJECT_H
