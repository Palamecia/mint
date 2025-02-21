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

#ifndef MINT_TOKEN_H
#define MINT_TOKEN_H

#include "mint/config.h"

#include <cstdint>

namespace mint::token {

enum Type : std::uint8_t {
	ASSERT_TOKEN,
	BREAK_TOKEN,
	CASE_TOKEN,
	CATCH_TOKEN,
	CLASS_TOKEN,
	CONST_TOKEN,
	CONTINUE_TOKEN,
	DEF_TOKEN,
	DEFAULT_TOKEN,
	ELIF_TOKEN,
	ELSE_TOKEN,
	ENUM_TOKEN,
	EXIT_TOKEN,
	FINAL_TOKEN,
	FOR_TOKEN,
	IF_TOKEN,
	IN_TOKEN,
	LET_TOKEN,
	LIB_TOKEN,
	LOAD_TOKEN,
	OVERRIDE_TOKEN,
	PACKAGE_TOKEN,
	PRINT_TOKEN,
	RAISE_TOKEN,
	RETURN_TOKEN,
	SWITCH_TOKEN,
	TRY_TOKEN,
	WHILE_TOKEN,
	YIELD_TOKEN,
	VAR_TOKEN,
	CONSTANT_TOKEN,
	STRING_TOKEN,
	NUMBER_TOKEN,
	SYMBOL_TOKEN,
	NO_LINE_END_TOKEN,
	LINE_END_TOKEN,
	FILE_END_TOKEN,
	COMMENT_TOKEN,
	DOLLAR_TOKEN,
	AT_TOKEN,
	SHARP_TOKEN,
	BACK_SLASH_TOKEN,
	COMMA_TOKEN,
	DBL_PIPE_TOKEN,
	DBL_AMP_TOKEN,
	PIPE_TOKEN,
	CARET_TOKEN,
	AMP_TOKEN,
	EQUAL_TOKEN,
	QUESTION_TOKEN,
	COLON_TOKEN,
	COLON_EQUAL_TOKEN,
	CLOSE_BRACKET_EQUAL_TOKEN,
	PLUS_EQUAL_TOKEN,
	MINUS_EQUAL_TOKEN,
	ASTERISK_EQUAL_TOKEN,
	SLASH_EQUAL_TOKEN,
	PERCENT_EQUAL_TOKEN,
	DBL_LEFT_ANGLED_EQUAL_TOKEN,
	DBL_RIGHT_ANGLED_EQUAL_TOKEN,
	AMP_EQUAL_TOKEN,
	PIPE_EQUAL_TOKEN,
	CARET_EQUAL_TOKEN,
	EQUAL_RIGHT_ANGLED_TOKEN,
	DBL_DOT_TOKEN,
	TPL_DOT_TOKEN,
	DBL_EQUAL_TOKEN,
	TPL_EQUAL_TOKEN,
	EXCLAMATION_EQUAL_TOKEN,
	EXCLAMATION_DBL_EQUAL_TOKEN,
	IS_TOKEN,
	EQUAL_TILDE_TOKEN,
	EXCLAMATION_TILDE_TOKEN,
	LEFT_ANGLED_TOKEN,
	RIGHT_ANGLED_TOKEN,
	LEFT_ANGLED_EQUAL_TOKEN,
	RIGHT_ANGLED_EQUAL_TOKEN,
	DBL_LEFT_ANGLED_TOKEN,
	DBL_RIGHT_ANGLED_TOKEN,
	PLUS_TOKEN,
	MINUS_TOKEN,
	ASTERISK_TOKEN,
	SLASH_TOKEN,
	PERCENT_TOKEN,
	EXCLAMATION_TOKEN,
	TILDE_TOKEN,
	TYPEOF_TOKEN,
	MEMBERSOF_TOKEN,
	DEFINED_TOKEN,
	DBL_PLUS_TOKEN,
	DBL_MINUS_TOKEN,
	DBL_ASTERISK_TOKEN,
	DOT_TOKEN,
	OPEN_PARENTHESIS_TOKEN,
	CLOSE_PARENTHESIS_TOKEN,
	OPEN_BRACKET_TOKEN,
	CLOSE_BRACKET_TOKEN,
	OPEN_BRACE_TOKEN,
	CLOSE_BRACE_TOKEN,

	// special tokens
	MODULE_PATH_TOKEN,
	REGEX_TOKEN
};

MINT_EXPORT Type from_local_id(int id);

}

#endif // MINT_TOKEN_H
