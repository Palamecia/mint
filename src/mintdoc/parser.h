#ifndef PARSER_H
#define PARSER_H

#include <memory/reference.h>
#include <string>
#include <vector>

class Dictionnary;
struct Definition;

class Parser {
public:
	Parser(const std::string &script);
	~Parser();

	void parse(Dictionnary *dictionnary);

private:
	enum State {
		expect_start,
		expect_value,
		expect_value_subexpression,
		expect_parenthesis_operator,
		expect_bracket_operator,
		expect_capture,
		expect_signature,
		expect_signature_begin,
		expect_signature_subexpression,
		expect_package,
		expect_class,
		expect_enum,
		expect_function,
		expect_base
	};

	enum ParserState {
		parsing_start,
		parsing_value,
		parsing_operator
	};

	struct Context {
		std::string name;
		Definition *definition;
		int bloc;
	};

	State getState() const;
	void setState(State state);
	void pushState(State state);
	void popState();

	Context *currentContext() const;
	std::string definitionName(const std::string &name) const;
	void pushContext(const std::string &name, Definition* definition);
	void bindDefinitionToContext(Definition* definition);
	void bindDefinitionToContext(Context* context, Definition* definition);

	void openBlock();
	void closeBlock();

	void startModifiers(mint::Reference::Flags flags);
	void addModifiers(mint::Reference::Flags flags);
	mint::Reference::Flags retrieveModifiers();

	std::string cleanupDoc(const std::string &comment) const;
	std::string cleanupSingleLineDoc(std::stringstream &stream) const;
	std::string cleanupMultiLineDoc(std::stringstream &stream) const;

	std::string m_script;
	std::vector<State> m_states;
	State m_state;
	ParserState m_parserState;

	mint::Reference::Flags m_modifiers;
	std::vector<Context *> m_contexts;
	Context* m_context;
};

#endif // PARSER_H
