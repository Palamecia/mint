#ifndef OBJECT_H
#define OBJECT_H

#include "memory/reference.h"

#include <memory>
#include <vector>
#include <deque>

namespace mint {

class Class;

struct MINT_EXPORT Null : public Data {
protected:
	friend class Reference;
	Null();
};

struct MINT_EXPORT None : public Data {
protected:
	friend class Reference;
	None();
};

struct MINT_EXPORT Number : public Data {
	double value;

protected:
	friend class Reference;
	Number();
};

struct MINT_EXPORT Boolean : public Data {
	bool value;

protected:
	friend class Reference;
	Boolean();
};

struct MINT_EXPORT Object : public Data {
	Class *const metadata;
	Reference *data;

	void construct();
	void construct(const Object &other);

protected:
	friend class Reference;
	Object(Class *type);

	friend class Destructor;
	friend class GarbadgeCollector;
	virtual ~Object();
};

struct MINT_EXPORT Function : public Data {
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

}

#endif // OBJECT_H
