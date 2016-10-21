#ifndef ARRAY_H
#define ARRAY_H

#include "memory/class.h"
#include "memory/object.h"

class ArrayClass : public Class {
public:
	static ArrayClass *instance();

private:
	ArrayClass();
};

struct Array : public Object {
	Array();
	typedef std::vector<SharedReference> values_type;
	values_type values;
};

#endif // ARRAY_H
