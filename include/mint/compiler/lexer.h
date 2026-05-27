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

#ifndef MINT_COMPILER_LEXER_H
#define MINT_COMPILER_LEXER_H

#include "mint/config.h"
#include "mint/system/data_stream.h"

#include <functional>
#include <map>
#include <string>

namespace mint {

class MINT_EXPORT Lexer {
	std::reference_wrapper<DataStream> _stream;
	int _cptr = 0;
	int _remaining = 0; // hack
public:
	explicit Lexer(DataStream& stream);

	std::string next_token();
	static int token_type(const std::string& token);

	std::string read_regex();

	[[nodiscard]] std::string format_error(const std::string& error) const;
	[[nodiscard]] bool at_end() const;

	static bool is_digit(int c);
	static bool is_white_space(int c);
	static bool is_operator(const std::string& token);
	static bool is_operator(const std::string& token, int* type);

protected:
	std::string tokenize_string(char delim);

private:
	static const std::map<std::string, int> keywords;
	static const std::map<std::string, int> operators;
};

}

#endif // MINT_COMPILER_LEXER_H
