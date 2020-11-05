#ifndef DEFINITION_H
#define DEFINITION_H

#include <string>
#include <vector>

struct Definition {
	enum Type {
		package_definition,
		enum_definition,
		class_definition,
		constant_definition,
		function_definition
	};

	Definition(Type type, const std::string &name);
	virtual ~Definition();

	Type type;
	uint64_t flags;
	std::string name;
};

struct Package : public Definition {
	Package(const std::string &name);

	std::string doc;
};

struct Enum : public Definition {
	Enum(const std::string &name);

	std::string doc;
};

struct Class : public Definition {
	Class(const std::string &name);

	std::vector<std::string> bases;
	std::string doc;
};

struct Constant : public Definition {
	Constant(const std::string &name);

	std::string value;
	std::string doc;
};

struct Function : public Definition {
	struct Signature {
		std::string format;
		std::string doc;
	};

	Function(const std::string &name);
	~Function();

	std::vector<Signature *> signatures;
};

#endif // DEFINITION_H
