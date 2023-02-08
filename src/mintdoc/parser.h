/**
 * Copyright (c) 2024 Gauvain CHERY.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef PARSER_H
#define PARSER_H

#include <mint/memory/reference.h>
#include <string>
#include <vector>

class Dictionnary;
struct Definition;

class Parser {
public:
	Parser(const std::string &path);
	~Parser();

	void parse(Dictionnary *dictionnary);

protected:
	void parse_error(const char *message, size_t column, size_t start_line = 0);

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

	State get_state() const;
	void set_state(State state);
	void push_state(State state);
	void pop_state();

	Context *current_context() const;
	std::string definition_name(const std::string &name) const;
	void push_context(const std::string &name, Definition* definition);
	void bind_definition_to_context(Definition* definition);
	void bind_definition_to_context(Context* context, Definition* definition);

	void open_block();
	void close_block();

	void start_modifiers(mint::Reference::Flags flags);
	void add_modifiers(mint::Reference::Flags flags);
	mint::Reference::Flags retrieve_modifiers();

	std::string cleanup_doc(const std::string &comment);
	std::string cleanup_single_line_doc(std::stringstream &stream);
	std::string cleanup_multi_line_doc(std::stringstream &stream);

	std::string m_path;
	size_t m_line_number;

	std::vector<State> m_states;
	State m_state;
	ParserState m_parser_state;

	mint::Reference::Flags m_modifiers;
	std::vector<Context *> m_contexts;
	Context* m_context;
};

#endif // PARSER_H
