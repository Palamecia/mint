#ifndef MODULE_H
#define MODULE_H

#include "definition.h"

#include <string>
#include <map>

struct Module {
	enum Type { script, group };

	Type type;
	std::string name;
	std::string doc;
	std::map<std::string, std::string> links;
	std::map<std::string, Definition *> definitions;
	std::map<Definition::Type, std::map<std::string, Definition *>> elements;
};

#endif // MODULE_H
