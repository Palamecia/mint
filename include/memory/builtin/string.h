#ifndef MINT_STRING_H
#define MINT_STRING_H

#include "memory/class.h"
#include "memory/object.h"

namespace mint {

class MINT_EXPORT StringClass : public Class {
public:
	static StringClass *instance();

private:
	friend class GlobalData;
	StringClass();
};

struct MINT_EXPORT String : public Object {
	String();
	String(const String &other);
	String(const std::string& value);

	std::string str;

private:
	friend class Reference;
	static LocalPool<String> g_pool;
};

MINT_EXPORT std::string to_string(intmax_t value);
MINT_EXPORT std::string to_string(double value);
MINT_EXPORT std::string to_string(void *value);

}

#endif // MINT_STRING_H
