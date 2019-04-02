#ifndef ARRAY_H
#define ARRAY_H

#include "memory/class.h"
#include "memory/object.h"

namespace mint {

class MINT_EXPORT ArrayClass : public Class {
public:
	static ArrayClass *instance();

private:
	ArrayClass();
};

struct MINT_EXPORT Array : public Object {
	Array();
	~Array();

	typedef std::vector<SharedReference> values_type;
	values_type values;
};

}

#endif // ARRAY_H
