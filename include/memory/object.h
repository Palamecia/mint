#ifndef OBJECT_H
#define OBJECT_H

#include "memory/reference.h"

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

struct Boolean : public Data {
	bool value;
	Boolean();
};

struct Object : public Data {
	Object(Class *type);
	virtual ~Object();

	void construct();
	void construct(const Object &other);

	Class *metadata;
	Reference *data;
};

struct Function : public Data {
	Function();
	struct Handler {
		Handler(int module, int offset);

		typedef std::map<std::string, Reference> Capture;

		int module;
		int offset;
		std::shared_ptr<Capture> capture;
	};

	typedef std::map<int, Handler> mapping_type;
	mapping_type mapping;
};

#endif // OBJECT_H
