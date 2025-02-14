/**
 * Copyright (c) 2025 Gauvain CHERY.
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

#ifndef MINTDOC_PARSER_H
#define MINTDOC_PARSER_H

#include <mint/compiler/lexicalhandler.h>
#include <mint/memory/reference.h>
#include <filesystem>
#include <cstdint>
#include <string>
#include <vector>

#include "definition.h"

class Dictionary;

class Parser : protected mint::LexicalHandler {
public:
	Parser(std::filesystem::path path);
	Parser(const Parser &) = delete;
	Parser(Parser &&) = delete;
	~Parser();

	Parser &operator=(const Parser &) = delete;
	Parser &operator=(Parser &&) = delete;

	void parse(Dictionary *dictionary);

protected:
	bool on_token(mint::token::Type type, const std::string &token, std::string::size_type offset) override;

	bool on_new_line(size_t line_number, std::string::size_type offset) override;
	bool on_comment_begin(std::string::size_type offset) override;

	void parse_error(const char *message, size_t column, size_t begin_line = 0, size_t end_line = 0);

private:
	enum State : std::uint8_t {
		EXPECT_START,
		EXPECT_VALUE,
		EXPECT_VALUE_SUBEXPRESSION,
		EXPECT_PARENTHESIS_OPERATOR,
		EXPECT_BRACKET_OPERATOR,
		EXPECT_CAPTURE,
		EXPECT_SIGNATURE,
		EXPECT_SIGNATURE_BEGIN,
		EXPECT_SIGNATURE_SUBEXPRESSION,
		EXPECT_PACKAGE,
		EXPECT_CLASS,
		EXPECT_ENUM,
		EXPECT_FUNCTION,
		EXPECT_BASE
	};

	struct Context {
		std::string name;
		Definition *definition;
		int block;
	};

	[[nodiscard]] State get_state() const;
	void set_state(State state);
	void push_state(State state);
	void pop_state();

	[[nodiscard]] Context *current_context() const;
	[[nodiscard]] std::string definition_name(const std::string &token) const;
	void push_context(const std::string &name, Definition *definition);
	void bind_definition_to_context(Definition *definition);
	void bind_definition_to_context(Context *context, Definition *definition);

	void open_block();
	void close_block();

	void start_modifiers(mint::Reference::Flags flags);
	void add_modifiers(mint::Reference::Flags flags);
	[[nodiscard]] mint::Reference::Flags retrieve_modifiers();

	[[nodiscard]] std::string cleanup_doc(const std::string &comment, size_t line, size_t column);
	[[nodiscard]] std::string cleanup_single_line_doc(std::stringstream &stream, size_t line, size_t column);
	[[nodiscard]] std::string cleanup_multi_line_doc(std::stringstream &stream, size_t line, size_t column);
	void cleanup_script(std::stringstream &stream, std::string &documentation, size_t line, size_t column,
						size_t &current_line);

	std::filesystem::path m_path;
	size_t m_line_number = 1;
	std::string::size_type m_line_offset = 0;

	std::string m_comment;
	size_t m_comment_line_number = 0;
	size_t m_comment_column_number = 0;

	std::vector<State> m_states;
	State m_state = EXPECT_START;

	mint::Reference::Flags m_modifiers = mint::Reference::DEFAULT;
	std::vector<Context *> m_contexts;
	Context *m_context = nullptr;

	Dictionary *m_dictionary = nullptr;
	Function::Signature *m_signature = nullptr;
	Definition *m_definition = nullptr;
	intmax_t m_next_enum_constant = 0;
	std::string m_base;
};

#endif // MINTDOC_PARSER_H
