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

#ifndef MINT_PROCESS_HIGHLIGHTER_H
#define MINT_PROCESS_HIGHLIGHTER_H

#include "mint/ast/class_register.h"
#include "mint/compiler/lexical_handler.h"
#include "mint/compiler/token.h"
#include "mint/memory/reference.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

namespace mint {

class PackageData;
class ClassDescription;

class Highlighter : public LexicalHandler {
public:
	Highlighter(const AbstractSyntaxTree& ast, std::string_view::size_type offset);
	Highlighter(const Highlighter&) = delete;
	Highlighter(Highlighter&&) = delete;
	~Highlighter() override = default;

	Highlighter& operator=(const Highlighter&) = delete;
	Highlighter& operator=(Highlighter&&) = delete;

	[[nodiscard]] std::string output() const;

protected:
	bool on_script_begin() override;
	bool on_script_end() override;

	bool on_symbol_token(const std::vector<std::string>& context, const std::string& token,
	    std::string::size_type offset) override;
	bool on_token(Token type, const std::string& token, std::string::size_type offset) override;
	bool on_white_space(const std::string& token, std::string::size_type offset) override;
	bool on_comment(const std::string& token, std::string::size_type offset) override;

	enum class Style : std::uint8_t {
		text,
		comment,
		keyword,
		operator_keyword,
		controle_keyword,
		constant,
		function,
		user_type,
		number_literal,
		string_literal,
		regex_literal,
		standard_symbol,
		module_path,
		brace,
		brace_match
	};

	void set_style(Style style);

	[[nodiscard]] const Reference* find_defined_symbol(const std::vector<std::string>& context,
	    const std::string& token) const;
	[[nodiscard]] std::optional<std::tuple<const PackageData*, const ClassDescription*>> resolve_path(
	    const std::vector<std::string>& context) const;

private:
	std::reference_wrapper<const AbstractSyntaxTree> _ast;
	std::string_view::size_type _offset;
	std::string _output;

	std::size_t _brace_depth = 0;
	std::optional<std::size_t> _brace_match;
	std::size_t _bracket_depth = 0;
	std::optional<std::size_t> _bracket_match;
	std::size_t _parenthesis_depth = 0;
	std::optional<std::size_t> _parenthesis_match;
};

}

#endif // MINT_PROCESS_HIGHLIGHTER_H
