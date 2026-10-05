/**
 * Copyright (c) 2026 Gauvain CHÉRY.
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

#include "mint/compiler/build_context.h"
#include "mint/system/error.h"
#include "parser.h"

#include <cstdio>
#include <string>
#include <tuple>

namespace mint {

BuildContext::BuildContext(DataStream& stream) :
	_lexer(stream) {}

void BuildContext::commit_line() {
	commit_line_impl();
}

std::string BuildContext::read_regex() {
	return _lexer.read_regex();
}

int BuildContext::next_token(std::string* token) {

	int type = parser::token::file_end_token;
	if (_lexer.at_end()) {
		return type;
	}

	std::tie(*token, type) = _lexer.next_token();
	while (type == parser::token::comment_token || type == parser::token::no_line_end_token) {
		std::tie(*token, type) = _lexer.next_token();
	}
	return type;
}

void BuildContext::parse_error(const std::string& error_msg) const {
	fflush(stdout);
	error("{}", _lexer.format_error(error_msg));
}

}
