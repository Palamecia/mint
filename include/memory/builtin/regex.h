#ifndef REGEX_H
#define REGEX_H

#include "memory/class.h"
#include "memory/object.h"

#include <regex>

namespace mint {

class MINT_EXPORT RegexClass : public Class {
public:
	static RegexClass *instance();

private:
	RegexClass();
};

struct MINT_EXPORT Regex : public Object {
	Regex();
	std::string initializer;
	std::regex expr;
};

}

#endif // REGEX_H
