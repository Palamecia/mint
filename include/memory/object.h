#ifndef OBJECT_H
#define OBJECT_H

#include "memory/reference.h"

#include <memory>
#include <vector>
#include <deque>

class Class;

struct Null : public Data {
protected:
	friend class Reference;
	Null();
};

struct None : public Data {
protected:
	friend class Reference;
	None();
};

struct Number : public Data {
	double value;

protected:
	friend class Reference;
	Number();
};

struct Boolean : public Data {
	bool value;

protected:
	friend class Reference;
	Boolean();
};

struct Object : public Data {
	virtual ~Object();

	void construct();
	void construct(const Object &other);

	Class *metadata;
	Reference *data;

protected:
	friend class Reference;
	Object(Class *type);
};

struct Function : public Data {
	struct Handler {
		Handler(int module, int offset);

		typedef std::map<std::string, Reference> Capture;

		int module;
		int offset;
		std::shared_ptr<Capture> capture;
	};

	typedef std::map<int, Handler> mapping_type;
	mapping_type mapping;

protected:
	friend class Reference;
	Function();
};

#endif // OBJECT_H
