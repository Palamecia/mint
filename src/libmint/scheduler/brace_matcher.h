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

#ifndef LIBMINT_SCHEDULER_BRACE_MATCHER_H
#define LIBMINT_SCHEDULER_BRACE_MATCHER_H

#include "mint/compiler/lexical_handler.h"
#include "mint/compiler/token.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace mint {

class BraceMatcher : public LexicalHandler {
public:
	BraceMatcher(std::string_view::size_type offset);
	BraceMatcher(BraceMatcher&&) = delete;
	BraceMatcher(const BraceMatcher&) = delete;
	~BraceMatcher() override = default;

	BraceMatcher& operator=(BraceMatcher&&) = delete;
	BraceMatcher& operator=(const BraceMatcher&) = delete;

	[[nodiscard]] std::pair<std::string_view::size_type, bool> match() const;

protected:
	bool on_token(mint::Token type, const std::string& token, std::string::size_type offset) override;
	bool on_comment_begin(std::string::size_type offset) override;
	bool on_comment_end(std::string::size_type offset) override;
	bool on_script_end() override;

private:
	std::string_view::size_type _offset;
	std::pair<std::string_view::size_type, bool> _match = {std::string_view::npos, true};

	bool _comment = false;
	std::optional<std::size_t> _brace_open;
	std::vector<std::size_t> _brace_depth;
	std::optional<std::size_t> _bracket_open;
	std::vector<std::size_t> _bracket_depth;
	std::optional<std::size_t> _parenthesis_open;
	std::vector<std::size_t> _parenthesis_depth;
};

}

#endif // LIBMINT_SCHEDULER_BRACE_MATCHER_H
