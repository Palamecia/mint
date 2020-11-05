#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>

class Dictionnary;
class Definition;

class Parser {
public:
	Parser(const std::string &script);
	~Parser();

	void parse(Dictionnary *dictionnary);

private:
	enum State {
		expect_start,
		expect_value,
		expect_parenthesis_operator,
		expect_bracket_operator,
		expect_signature,
		expect_package,
		expect_class,
		expect_enum,
		expect_function,
		expect_base
	};

	struct Context {
		std::string name;
		Definition *definition;
		int bloc;
	};

	State getState() const;
	void setState(State state);

	Context *currentContext() const;
	std::string definitionName(const std::string &name) const;
	void pushContext(const std::string &name, Definition* definition);

	void openBlock();
	void closeBlock();

	std::string cleanupDoc(const std::string &comment) const;
	std::string cleanupSingleLineDoc(std::stringstream &stream) const;
	std::string cleanupMultiLineDoc(std::stringstream &stream) const;

	std::string m_script;
	State m_state;

	std::vector<Context *> m_contexts;
	Context* m_context;
};

#endif // PARSER_H
