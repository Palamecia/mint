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

#ifndef MINT_AST_FUNCTION_LITERAL_H
#define MINT_AST_FUNCTION_LITERAL_H

#include <cstddef>
#include <string_view>

namespace mint {

consteval int variadic(int signature) {
	return ~signature;
}

struct FunctionLiteral {
	template<std::size_t N>
	consteval FunctionLiteral(const char (&str)[N]) :
	    script(str),
	    signature(parse_signature(str)) {}

	FunctionLiteral(const FunctionLiteral&) = delete;
	FunctionLiteral(FunctionLiteral&&) = delete;
	~FunctionLiteral() = default;

	FunctionLiteral& operator=(const FunctionLiteral&) = delete;
	FunctionLiteral& operator=(FunctionLiteral&&) = delete;

	const std::string_view script;
	const int signature;

private:
	static consteval bool is_space(char c) {
		return c == ' ' || c == '\t' || c == '\n' || c == '\r';
	}

	static consteval void skip_spaces(const char*& p) {
		while (is_space(*p)) {
			++p;
		}
	}

	static consteval bool starts_with(const char* p, const char* text) {
		while (*text) {
			if (*p++ != *text++) {
				return false;
			}
		}

		return true;
	}

	static consteval int parse_signature(const char* script) {

		const char* cptr = script;
		bool param_started = false;

		while (!param_started && *cptr) {

			// skip leading whitespace
			while (*cptr && is_space(*cptr)) {
				++cptr;
			}

			if (starts_with(cptr, "def")) {
				param_started = true;
				cptr += 3;
			}
			else if (starts_with(cptr, "async")) {
				cptr += 5;
			}
			else {
				throw "Function must start with 'def'";
			}
		}

		// Find '('
		while (*cptr && *cptr != '(') {
			++cptr;
		}

		if (*cptr != '(') {
			throw "Missing '(' in function declaration";
		}

		++cptr;

		skip_spaces(cptr);

		// def ()
		if (*cptr == ')') {
			return 0;
		}

		int count = 0;

		while (*cptr) {
			skip_spaces(cptr);

			// ...
			if (starts_with(cptr, "...")) {
				cptr += 3;

				skip_spaces(cptr);

				if (*cptr != ')') {
					throw "'...' must be last parameter";
				}

				return variadic(count);
			}

			// Consume parameter
			bool found = false;

			while (*cptr && *cptr != ',' && *cptr != ')') {
				if (!is_space(*cptr)) {
					found = true;
				}

				++cptr;
			}

			if (!found) {
				throw "Invalid parameter";
			}

			++count;

			skip_spaces(cptr);

			if (*cptr == ',') {
				++cptr;
				continue;
			}

			if (*cptr == ')') {
				return count;
			}

			throw "Unexpected token";
		}

		throw "Missing ')'";
	}
};

}

#endif // MINT_AST_FUNCTION_LITERAL_H
