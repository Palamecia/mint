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
	Regex(const Regex &other);

	std::string initializer;
	std::regex expr;

private:
	friend class Reference;
	static LocalPool<Regex> g_pool;
};

}

#endif // REGEX_H
