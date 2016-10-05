#ifndef OBJECT_H
#define OBJECT_H

#include "Memory/reference.h"

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
	typedef std::vector<SharedReference> values_type;
	static values_type::value_type move_item(const values_type::value_type &item);
	values_type values;
};

struct Hash : public Object {
	Hash();
	struct compare {
		bool operator ()(const SharedReference &a, const SharedReference &b) const;
	};
	typedef std::map<SharedReference, SharedReference, compare> values_type;
	static values_type::value_type move_item(const values_type::value_type &item);
	values_type values;
};

struct Iterator : public Object {
	Iterator();
	typedef std::deque<SharedReference> ctx_type;
	ctx_type ctx;
};

#endif // OBJECT_H
