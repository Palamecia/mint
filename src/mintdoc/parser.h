/**
 * Copyright (c) 2026 Gauvain CHERY.
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

#include "mint/compiler/lexical_handler.h"
#include "mint/compiler/token.h"
#include "mint/memory/reference.h"
#include <cstddef>
#include <filesystem>
#include <cstdint>
#include <functional>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "definition.h"

class Dictionary;

class Parser : protected mint::LexicalHandler {
public:
	Parser(std::filesystem::path path);

	void parse(Dictionary& dictionary);

protected:
	bool on_token(mint::Token type, const std::string& token, std::string::size_type offset) override;

	bool on_new_line(std::size_t line_number, std::string::size_type offset) override;
	bool on_comment_begin(std::string::size_type offset) override;

	void parse_error(const std::string& message, std::size_t column, std::size_t begin_line = 0,
	    std::size_t end_line = 0);

private:
	enum class State : std::uint8_t {
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

	struct ScriptContext {
		std::string name;
		std::shared_ptr<Definition> definition;
		int depth;
	};

	struct Context {
		std::size_t line_number = 1;
		std::string::size_type line_offset = 0;

		std::string comment;
		std::size_t comment_line_number = 0;
		std::size_t comment_column_number = 0;

		std::vector<State> states;
		State state = State::expect_start;

		mint::Reference::Flags modifiers = mint::Reference::default_flags;
		std::vector<std::unique_ptr<ScriptContext>> contexts;
		std::unique_ptr<ScriptContext> context;

		std::reference_wrapper<Dictionary> dictionary;
		std::shared_ptr<Function::Signature> signature;
		std::shared_ptr<Definition> definition;
		std::intmax_t next_enum_constant = 0;
		std::string base;
	};

	[[nodiscard]] State get_state() const;
	void set_state(State state);
	void push_state(State state);
	void pop_state();

	[[nodiscard]] ScriptContext* current_context() const;
	[[nodiscard]] std::string definition_name(const std::string& token) const;
	void push_context(const std::string& name, const std::shared_ptr<Definition>& definition);
	void bind_definition_to_context(Definition& definition);
	static void bind_definition_to_context(ScriptContext& context, Definition& definition);

	void open_block();
	void close_block();

	void start_modifiers(mint::Reference::Flags flags);
	void add_modifiers(mint::Reference::Flags flags);
	[[nodiscard]] mint::Reference::Flags retrieve_modifiers();

	[[nodiscard]] std::string cleanup_doc(const std::string& comment, std::size_t line, std::size_t column);
	[[nodiscard]] std::string cleanup_single_line_doc(std::stringstream& stream, std::size_t line, std::size_t column);
	[[nodiscard]] std::string cleanup_multi_line_doc(std::stringstream& stream, std::size_t line, std::size_t column);
	void cleanup_script(std::stringstream& stream, std::string& documentation, std::size_t line, std::size_t column,
	    std::size_t& current_line);

	std::filesystem::path _path;
	std::unique_ptr<Context> _context;
};

#endif // MINTDOC_PARSER_H
