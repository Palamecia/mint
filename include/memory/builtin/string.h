#ifndef STRING_H
#define STRING_H

#include "memory/class.h"
#include "memory/object.h"

namespace mint {

class MINT_EXPORT StringClass : public Class {
public:
	static StringClass *instance();

private:
	StringClass();
};

struct MINT_EXPORT String : public Object {
	String();
	std::string str;
};

}

#endif // STRING_H
