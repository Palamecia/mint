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

#ifndef MINT_PROCESS_BRACEMATCHER_H
#define MINT_PROCESS_BRACEMATCHER_H

#include "mint/compiler/lexicalhandler.h"

#include <optional>

namespace mint {

class BraceMatcher : public LexicalHandler {
public:
	BraceMatcher() = delete;
	BraceMatcher(BraceMatcher &&) = delete;
	BraceMatcher(const BraceMatcher &) = delete;
	BraceMatcher(std::pair<std::string_view::size_type, bool> &match, std::string_view::size_type offset);
	~BraceMatcher() override = default;

	BraceMatcher &operator=(BraceMatcher &&) = delete;
	BraceMatcher &operator=(const BraceMatcher &) = delete;

protected:
	bool on_token(mint::token::Type type, const std::string &token, std::string::size_type offset) override;
	bool on_comment_begin(std::string::size_type offset) override;
	bool on_comment_end(std::string::size_type offset) override;
	bool on_script_end() override;

private:
	std::pair<std::string_view::size_type, bool> &m_match;
	std::string_view::size_type m_offset;

	bool m_comment = false;
	std::optional<size_t> m_brace_open;
	std::vector<size_t> m_brace_depth;
	std::optional<size_t> m_bracket_open;
	std::vector<size_t> m_bracket_depth;
	std::optional<size_t> m_parenthesis_open;
	std::vector<size_t> m_parenthesis_depth;
};

}

#endif // MINT_PROCESS_BRACEMATCHER_H
