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

#ifndef MINT_NODE_H
#define MINT_NODE_H

#include "mint/ast/symbol.h"
#include "mint/memory/reference.h"

namespace mint {

union MINT_EXPORT Node {
	enum Command : std::uint8_t {
		LOAD_MODULE,

		LOAD_FAST,
		LOAD_SYMBOL,
		LOAD_MEMBER,
		LOAD_OPERATOR,
		LOAD_CONSTANT,
		LOAD_VAR_SYMBOL,
		LOAD_VAR_MEMBER,
		CLONE_REFERENCE,
		RELOAD_REFERENCE,
		UNLOAD_REFERENCE,
		LOAD_EXTRA_ARGUMENTS,
		RESET_SYMBOL,
		RESET_FAST,

		DECLARE_FAST,
		DECLARE_SYMBOL,
		DECLARE_FUNCTION,
		FUNCTION_OVERLOAD,
		ALLOC_ITERATOR,
		INIT_ITERATOR,
		ALLOC_ARRAY,
		INIT_ARRAY,
		ALLOC_HASH,
		INIT_HASH,
		CREATE_LIB,

		REGEX_MATCH,
		REGEX_UNMATCH,

		STRICT_EQ_OP,
		STRICT_NE_OP,

		OPEN_PACKAGE,
		CLOSE_PACKAGE,
		REGISTER_CLASS,

		MOVE_OP,
		COPY_OP,
		ADD_OP,
		SUB_OP,
		MOD_OP,
		MUL_OP,
		DIV_OP,
		POW_OP,
		IS_OP,
		EQ_OP,
		NE_OP,
		LT_OP,
		GT_OP,
		LE_OP,
		GE_OP,
		INC_OP,
		DEC_OP,
		NOT_OP,
		AND_OP,
		OR_OP,
		BAND_OP,
		BOR_OP,
		XOR_OP,
		COMPL_OP,
		POS_OP,
		NEG_OP,
		SHIFT_LEFT_OP,
		SHIFT_RIGHT_OP,
		INCLUSIVE_RANGE_OP,
		EXCLUSIVE_RANGE_OP,
		SUBSCRIPT_OP,
		SUBSCRIPT_MOVE_OP,
		TYPEOF_OP,
		MEMBERSOF_OP,
		FIND_OP,
		IN_OP,

		FIND_DEFINED_SYMBOL,
		FIND_DEFINED_MEMBER,
		FIND_DEFINED_VAR_SYMBOL,
		FIND_DEFINED_VAR_MEMBER,
		CHECK_DEFINED,

		FIND_INIT,
		FIND_NEXT,
		FIND_CHECK,
		RANGE_INIT,
		RANGE_NEXT,
		RANGE_CHECK,
		RANGE_ITERATOR_CHECK,

		BEGIN_GENERATOR_EXPRESSION,
		END_GENERATOR_EXPRESSION,
		YIELD_EXPRESSION,

		OPEN_PRINTER,
		CLOSE_PRINTER,
		PRINT,

		OR_PRE_CHECK,
		AND_PRE_CHECK,
		CASE_JUMP,
		JUMP_ZERO,
		JUMP,

		SET_RETRIEVE_POINT,
		UNSET_RETRIEVE_POINT,
		RAISE,

		YIELD,
		EXIT_GENERATOR,
		YIELD_EXIT_GENERATOR,

		INIT_CAPTURE,
		CAPTURE_SYMBOL,
		CAPTURE_AS,
		CAPTURE_ALL,
		CALL,
		CALL_MEMBER,
		CALL_BUILTIN,
		INIT_CALL,
		INIT_MEMBER_CALL,
		INIT_OPERATOR_CALL,
		INIT_VAR_MEMBER_CALL,
		INIT_EXCEPTION,
		RESET_EXCEPTION,
		INIT_PARAM,
		EXIT_CALL,
		EXIT_THREAD,
		EXIT_EXEC,
		EXIT_MODULE
	};

	Node(Command command);
	Node(int parameter);
	Node(Symbol *symbol);
	Node(Reference *constant);

	Command command;
	int parameter;
	Symbol *symbol;
	Reference *constant;
};

}

#endif // MINT_NODE_H
