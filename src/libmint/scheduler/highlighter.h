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

#ifndef MINT_PROCESS_HIGHLIGHTER_H
#define MINT_PROCESS_HIGHLIGHTER_H

#include "mint/compiler/lexicalhandler.h"

#include <optional>

namespace mint {

class PackageData;
class ClassDescription;

class Highlighter : public LexicalHandler {
public:
	Highlighter(std::string &output, std::string_view::size_type offset);

protected:
	bool on_script_begin() override;
	bool on_script_end() override;

	bool on_symbol_token(const std::vector<std::string> &context, const std::string &token,
						 std::string::size_type offset) override;
	bool on_token(token::Type type, const std::string &token, std::string::size_type offset) override;
	bool on_white_space(const std::string &token, std::string::size_type offset) override;
	bool on_comment(const std::string &token, std::string::size_type offset) override;

	enum Style {
		TEXT,
		COMMENT,
		KEYWORD,
		CONSTANT,
		USER_TYPE,
		MODULE_PATH,
		NUMBER_LITERAL,
		STRING_LITERAL,
		REGEX_LITERAL,
		STANDARD_SYMBOL,
		BRACE,
		BRACE_MATCH
	};

	void set_style(Style style);

	static bool is_defined_class(const std::vector<std::string> &context, const std::string &token);
	static bool is_defined_symbol(const std::vector<std::string> &context, const std::string &token);
	static bool resolve_path(const std::vector<std::string> &context, PackageData *&pack, ClassDescription *&desc);

private:
	std::string &m_output;
	std::string_view::size_type m_offset;

	size_t m_brace_depth = 0;
	std::optional<size_t> m_brace_match;
	size_t m_bracket_depth = 0;
	std::optional<size_t> m_bracket_match;
	size_t m_parenthesis_depth = 0;
	std::optional<size_t> m_parenthesis_match;
};

}

#endif // MINT_PROCESS_HIGHLIGHTER_H
