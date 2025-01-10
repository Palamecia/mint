%{
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

#ifndef PARSER_HPP
#define PARSER_HPP

#include "mint/compiler/buildtool.h"
#include "mint/compiler/compiler.h"
#include <memory>

#define YYSTYPE std::string
#define yylex context->next_token

using namespace mint;

%}

%define api.namespace {mint}
%parse-param {mint::BuildContext *context}

%token ASSERT_TOKEN
%token BREAK_TOKEN
%token CASE_TOKEN
%token CATCH_TOKEN
%token CLASS_TOKEN
%token CONST_TOKEN
%token CONTINUE_TOKEN
%token DEF_TOKEN
%token DEFAULT_TOKEN
%token ELIF_TOKEN
%token ELSE_TOKEN
%token ENUM_TOKEN
%token EXIT_TOKEN
%token FINAL_TOKEN
%token FOR_TOKEN
%token IF_TOKEN
%token IN_TOKEN
%token LET_TOKEN
%token LIB_TOKEN
%token LOAD_TOKEN
%token OVERRIDE_TOKEN
%token PACKAGE_TOKEN
%token PRINT_TOKEN
%token RAISE_TOKEN
%token RETURN_TOKEN
%token SWITCH_TOKEN
%token TRY_TOKEN
%token WHILE_TOKEN
%token YIELD_TOKEN
%token VAR_TOKEN
%token CONSTANT_TOKEN
%token STRING_TOKEN
%token NUMBER_TOKEN
%token SYMBOL_TOKEN

%token NO_LINE_END_TOKEN
%token LINE_END_TOKEN
%token FILE_END_TOKEN
%token COMMENT_TOKEN
%token DOLLAR_TOKEN
%token AT_TOKEN
%token SHARP_TOKEN
%token BACK_SLASH_TOKEN

%left COMMA_TOKEN
%left DBL_PIPE_TOKEN
%left DBL_AMP_TOKEN
%left PIPE_TOKEN
%left CARET_TOKEN
%left AMP_TOKEN
%right EQUAL_TOKEN QUESTION_TOKEN COLON_TOKEN COLON_EQUAL_TOKEN CLOSE_BRACKET_EQUAL_TOKEN PLUS_EQUAL_TOKEN MINUS_EQUAL_TOKEN ASTERISK_EQUAL_TOKEN SLASH_EQUAL_TOKEN PERCENT_EQUAL_TOKEN DBL_LEFT_ANGLED_EQUAL_TOKEN DBL_RIGHT_ANGLED_EQUAL_TOKEN AMP_EQUAL_TOKEN PIPE_EQUAL_TOKEN CARET_EQUAL_TOKEN EQUAL_RIGHT_ANGLED_TOKEN
%left DBL_DOT_TOKEN TPL_DOT_TOKEN
%left DBL_EQUAL_TOKEN EXCLAMATION_EQUAL_TOKEN IS_TOKEN EQUAL_TILDE_TOKEN EXCLAMATION_TILDE_TOKEN TPL_EQUAL_TOKEN EXCLAMATION_DBL_EQUAL_TOKEN
%left LEFT_ANGLED_TOKEN RIGHT_ANGLED_TOKEN LEFT_ANGLED_EQUAL_TOKEN RIGHT_ANGLED_EQUAL_TOKEN
%left DBL_LEFT_ANGLED_TOKEN DBL_RIGHT_ANGLED_TOKEN
%left PLUS_TOKEN MINUS_TOKEN
%left ASTERISK_TOKEN SLASH_TOKEN PERCENT_TOKEN
%right PREFIX_DBL_PLUS_TOKEN PREFIX_DBL_MINUS_TOKEN PREFIX_PLUS_TOKEN PREFIX_MINUS_TOKEN EXCLAMATION_TOKEN TILDE_TOKEN TYPEOF_TOKEN MEMBERSOF_TOKEN DEFINED_TOKEN
%left DBL_PLUS_TOKEN DBL_MINUS_TOKEN DBL_ASTERISK_TOKEN
%left DOT_TOKEN OPEN_PARENTHESIS_TOKEN CLOSE_PARENTHESIS_TOKEN OPEN_BRACKET_TOKEN CLOSE_BRACKET_TOKEN OPEN_BRACE_TOKEN CLOSE_BRACE_TOKEN

%%

module_rule:
    stmt_list_rule FILE_END_TOKEN {
	    context->push_node(Node::EXIT_MODULE);
		fflush(stdout);
		YYACCEPT;
	}
	| FILE_END_TOKEN {
	    context->push_node(Node::EXIT_MODULE);
		fflush(stdout);
		YYACCEPT;
	};

stmt_list_rule:
	stmt_list_rule stmt_rule
	| stmt_rule;

stmt_rule:
    LOAD_TOKEN module_path_rule LINE_END_TOKEN {
	    context->push_node(Node::LOAD_MODULE);
		context->push_node($2.c_str());
		context->commit_line();
	}
	| try_rule stmt_bloc_rule {
		context->reset_scoped_symbols();
		context->unregister_retrieve_point();
		context->push_node(Node::UNSET_RETRIEVE_POINT);
		context->push_node(Node::JUMP);
		context->start_jump_forward();
		context->shift_jump_forward();
		context->resolve_jump_forward();
		context->push_node(Node::UNLOAD_REFERENCE);
		context->resolve_jump_forward();
		context->close_block();
	}
	| try_bloc_rule catch_rule stmt_bloc_rule {
		context->reset_scoped_symbols();
		context->reset_exception();
		context->resolve_jump_forward();
		context->close_block();
	}
	| if_cond_rule stmt_bloc_rule {
		context->reset_scoped_symbols();
		context->resolve_jump_forward();
		context->close_block();
	}
	| if_bloc_rule else_rule stmt_bloc_rule {
		context->reset_scoped_symbols();
		context->resolve_jump_forward();
		context->close_block();
	}
	| if_bloc_rule elif_bloc_rule {
		context->resolve_jump_forward();
		context->close_block();
	}
	| if_bloc_rule elif_bloc_rule else_rule stmt_bloc_rule {
		context->reset_scoped_symbols();
		context->resolve_jump_forward();
		context->close_block();
	}
	| switch_cond_rule OPEN_BRACE_TOKEN case_list_rule CLOSE_BRACE_TOKEN {
		context->reset_scoped_symbols();
		context->push_node(Node::JUMP);
		context->start_jump_forward();
		context->build_case_table();
		context->resolve_jump_forward();
		context->resolve_jump_forward();
		context->close_block();
	}
	| while_cond_rule stmt_bloc_rule {
		context->reset_scoped_symbols();
		context->push_node(Node::JUMP);
		context->resolve_jump_backward();
		context->resolve_jump_forward();
		context->close_block();
	}
	| for_cond_rule stmt_bloc_rule {
		context->reset_scoped_symbols();
		context->push_node(Node::JUMP);
		context->resolve_jump_backward();
		context->resolve_jump_forward();
		context->close_block();
	}
	| BREAK_TOKEN LINE_END_TOKEN {
		if (!context->is_in_loop() && !context->is_in_switch()) {
			context->parse_error("break statement not within loop or switch");
			YYERROR;
		}
		context->prepare_break();
		context->push_node(Node::JUMP);
		context->bloc_jump_forward();
		context->commit_line();
	}
	| CONTINUE_TOKEN LINE_END_TOKEN {
		if (!context->is_in_loop()) {
			context->parse_error("continue statement not within loop");
			YYERROR;
		}
		context->prepare_continue();
		context->push_node(Node::JUMP);
		context->bloc_jump_backward();
		context->commit_line();
	}
	| print_rule stmt_bloc_rule {
		context->reset_scoped_symbols();
		context->close_block();
		context->close_printer();
	}
	| YIELD_TOKEN expr_rule LINE_END_TOKEN {
		if (!context->is_in_function()) {
			context->parse_error("unexpected 'yield' statement outside of function");
			YYERROR;
		}
		context->set_generator();
		context->push_node(Node::YIELD);
		context->commit_line();
	}
	| return_rule expr_rule LINE_END_TOKEN {
		context->set_exit_point();
		if (context->is_in_generator()) {
		    context->push_node(Node::YIELD_EXIT_GENERATOR);
		}
		else {
			context->push_node(Node::EXIT_CALL);
		}
		context->commit_line();
	}
	| RAISE_TOKEN expr_rule LINE_END_TOKEN {
	    context->reset_scoped_symbols_until(BuildContext::TRY_TYPE);
		context->push_node(Node::RAISE);
		context->commit_line();
	}
	| EXIT_TOKEN expr_rule LINE_END_TOKEN {
		context->push_node(Node::EXIT_EXEC);
		context->commit_line();
	}
	| EXIT_TOKEN LINE_END_TOKEN {
		context->push_node(Node::LOAD_CONSTANT);
		context->push_node(Compiler::make_data("0", Compiler::DATA_NUMBER_HINT));
		context->push_node(Node::EXIT_EXEC);
		context->commit_line();
	}
	| ident_iterator_item_rule ident_iterator_end_rule EQUAL_TOKEN expr_rule LINE_END_TOKEN {
		context->push_node(Node::COPY_OP);
		context->commit_expr_result();
		context->commit_line();
	}
	| ident_iterator_item_rule ident_iterator_end_rule EQUAL_TOKEN generator_expr_rule LINE_END_TOKEN {
		context->push_node(Node::COPY_OP);
		context->commit_expr_result();
		context->commit_line();
	}
	| create_ident_iterator_rule EQUAL_TOKEN expr_rule LINE_END_TOKEN {
		context->push_node(Node::COPY_OP);
		context->commit_expr_result();
		context->commit_line();
	}
	| create_ident_iterator_rule EQUAL_TOKEN generator_expr_rule LINE_END_TOKEN {
		context->push_node(Node::COPY_OP);
		context->commit_expr_result();
		context->commit_line();
	}
	| expr_rule LINE_END_TOKEN {
		context->commit_expr_result();
		context->commit_line();
	}
	| modifier_rule def_start_rule def_capture_rule SYMBOL_TOKEN def_args_rule stmt_bloc_rule {
		if (context->is_in_generator()) {
			context->push_node(Node::EXIT_GENERATOR);
		}
		else if (!context->has_returned()) {
			context->push_node(Node::LOAD_CONSTANT);
			context->push_node(Compiler::make_none());
			context->push_node(Node::EXIT_CALL);
		}
		context->resolve_jump_forward();
		context->push_node(Node::CREATE_FUNCTION);
		context->push_node($4.c_str());
		context->push_node(Reference::GLOBAL | Reference::CONST_ADDRESS | context->retrieve_modifiers());
		context->save_definition();
		context->push_node(Node::FUNCTION_OVERLOAD);
		context->push_node(Node::UNLOAD_REFERENCE);
	}
	| def_start_rule def_capture_rule SYMBOL_TOKEN def_args_rule stmt_bloc_rule {
		if (context->is_in_generator()) {
			context->push_node(Node::EXIT_GENERATOR);
		}
		else if (!context->has_returned()) {
			context->push_node(Node::LOAD_CONSTANT);
			context->push_node(Compiler::make_none());
			context->push_node(Node::EXIT_CALL);
		}
		context->resolve_jump_forward();
		context->push_node(Node::CREATE_FUNCTION);
		context->push_node($3.c_str());
		context->push_node(Reference::GLOBAL | Reference::CONST_ADDRESS);
		context->save_definition();
		context->push_node(Node::FUNCTION_OVERLOAD);
		context->push_node(Node::UNLOAD_REFERENCE);
	}
	| package_block_rule
	| class_desc_rule
	| enum_desc_rule
	| LINE_END_TOKEN {
		context->commit_line();
	};

module_name_rule:
    ASSERT_TOKEN { $$ = $1; }
	| BREAK_TOKEN { $$ = $1; }
	| CASE_TOKEN { $$ = $1; }
	| CATCH_TOKEN { $$ = $1; }
	| CLASS_TOKEN { $$ = $1; }
	| CONST_TOKEN { $$ = $1; }
	| CONTINUE_TOKEN { $$ = $1; }
	| DEF_TOKEN { $$ = $1; }
	| DEFAULT_TOKEN { $$ = $1; }
	| ELIF_TOKEN { $$ = $1; }
	| ELSE_TOKEN { $$ = $1; }
	| ENUM_TOKEN { $$ = $1; }
	| EXIT_TOKEN { $$ = $1; }
	| FOR_TOKEN { $$ = $1; }
	| IF_TOKEN { $$ = $1; }
	| IN_TOKEN { $$ = $1; }
	| LET_TOKEN { $$ = $1; }
	| LIB_TOKEN { $$ = $1; }
	| LOAD_TOKEN { $$ = $1; }
	| PACKAGE_TOKEN { $$ = $1; }
	| PRINT_TOKEN { $$ = $1; }
	| RAISE_TOKEN { $$ = $1; }
	| RETURN_TOKEN { $$ = $1; }
	| SWITCH_TOKEN { $$ = $1; }
	| TRY_TOKEN { $$ = $1; }
	| WHILE_TOKEN { $$ = $1; }
	| YIELD_TOKEN { $$ = $1; }
	| VAR_TOKEN { $$ = $1; }
	| SYMBOL_TOKEN { $$ = $1; }
	| module_name_rule MINUS_TOKEN module_name_rule {
		$$ = $1 + $2 + $3;
	};

module_path_rule:
	module_name_rule {
		$$ = $1;
	}
	| module_path_rule DOT_TOKEN module_name_rule {
		$$ = $1 + $2 + $3;
	};

package_rule:
    PACKAGE_TOKEN SYMBOL_TOKEN {
		context->open_package($2);
	};

package_block_rule:
    package_rule OPEN_BRACE_TOKEN stmt_list_rule CLOSE_BRACE_TOKEN {
		context->close_package();
	};

class_rule:
    CLASS_TOKEN SYMBOL_TOKEN {
		context->start_class_description($2);
	};

parent_rule:
    COLON_TOKEN parent_list_rule
	| ;

parent_list_rule:
	parent_ident_rule {
		context->save_base_class_path();
	}
	| parent_list_rule COMMA_TOKEN parent_ident_rule {
		context->save_base_class_path();
	};

parent_ident_rule:
    SYMBOL_TOKEN {
		context->append_symbol_to_base_class_path($1);
	}
	| parent_ident_rule DOT_TOKEN SYMBOL_TOKEN {
		context->append_symbol_to_base_class_path($3);
	};

class_desc_rule:
	class_rule parent_rule desc_bloc_rule {
		context->resolve_class_description();
	};

member_class_rule:
	class_rule
	| member_type_modifier_rule CLASS_TOKEN SYMBOL_TOKEN {
		context->start_class_description($3, context->retrieve_modifiers());
	};

member_class_desc_rule:
	member_class_rule parent_rule desc_bloc_rule {
		context->resolve_class_description();
	};

member_enum_rule:
	enum_rule
	| member_type_modifier_rule ENUM_TOKEN SYMBOL_TOKEN {
		context->start_enum_description($3, context->retrieve_modifiers());
	};

member_enum_desc_rule:
	member_enum_rule enum_block_rule {
		context->resolve_enum_description();
	};

member_type_modifier_rule:
    PLUS_TOKEN {
		context->start_modifiers(Reference::DEFAULT);
	}
	| SHARP_TOKEN {
		context->start_modifiers(Reference::PROTECTED_VISIBILITY);
	}
	| MINUS_TOKEN {
		context->start_modifiers(Reference::PRIVATE_VISIBILITY);
	}
	| TILDE_TOKEN {
		context->start_modifiers(Reference::PACKAGE_VISIBILITY);
	};

desc_bloc_rule:
    OPEN_BRACE_TOKEN desc_list_rule CLOSE_BRACE_TOKEN
	| OPEN_BRACE_TOKEN CLOSE_BRACE_TOKEN;

desc_list_rule:
	desc_list_rule desc_rule
	| desc_rule;

desc_rule:
    member_desc_rule LINE_END_TOKEN {
		if (!context->create_member(context->retrieve_modifiers(), Symbol($1))) {
			YYERROR;
		}
		context->commit_line();
	}
	| member_desc_rule EQUAL_TOKEN CONSTANT_TOKEN LINE_END_TOKEN {
		if (!context->create_member(context->retrieve_modifiers(), Symbol($1), Compiler::make_data($3, Compiler::DATA_UNKNOWN_HINT))) {
			YYERROR;
		}
		context->commit_line();
	}
	| member_desc_rule EQUAL_TOKEN STRING_TOKEN LINE_END_TOKEN {
		if (!context->create_member(context->retrieve_modifiers(), Symbol($1), Compiler::make_data($3, Compiler::DATA_STRING_HINT))) {
			YYERROR;
		}
		context->commit_line();
	}
	| member_desc_rule EQUAL_TOKEN regex_rule LINE_END_TOKEN {
		if (!context->create_member(context->retrieve_modifiers(), Symbol($1), Compiler::make_data($3, Compiler::DATA_REGEX_HINT))) {
			YYERROR;
		}
		context->commit_line();
	}
	| member_desc_rule EQUAL_TOKEN regex_rule regex_rule SYMBOL_TOKEN LINE_END_TOKEN {
		if (!context->create_member(context->retrieve_modifiers(), Symbol($1), Compiler::make_data($3 + $4, Compiler::DATA_REGEX_HINT))) {
			YYERROR;
		}
		context->commit_line();
	}
	| member_desc_rule EQUAL_TOKEN NUMBER_TOKEN LINE_END_TOKEN {
		if (!context->create_member(context->retrieve_modifiers(), Symbol($1), Compiler::make_data($3, Compiler::DATA_NUMBER_HINT))) {
			YYERROR;
		}
		context->commit_line();
	}
	| member_desc_rule EQUAL_TOKEN OPEN_BRACKET_TOKEN CLOSE_BRACKET_TOKEN LINE_END_TOKEN {
		if (!context->create_member(context->retrieve_modifiers(), Symbol($1), Compiler::make_array())) {
			YYERROR;
		}
		context->commit_line();
	}
	| member_desc_rule EQUAL_TOKEN OPEN_BRACE_TOKEN CLOSE_BRACE_TOKEN LINE_END_TOKEN {
		if (!context->create_member(context->retrieve_modifiers(), Symbol($1), Compiler::make_hash())) {
			YYERROR;
		}
		context->commit_line();
	}
	| member_desc_rule EQUAL_TOKEN LIB_TOKEN OPEN_PARENTHESIS_TOKEN STRING_TOKEN CLOSE_PARENTHESIS_TOKEN LINE_END_TOKEN {
		if (!context->create_member(context->retrieve_modifiers(), Symbol($1), Compiler::make_library($5))) {
			YYERROR;
		}
		context->commit_line();
	}
	| member_desc_rule EQUAL_TOKEN def_start_rule def_args_rule stmt_bloc_rule {
		if (context->is_in_generator()) {
			context->push_node(Node::EXIT_GENERATOR);
		}
		else if (!context->has_returned()) {
			context->push_node(Node::LOAD_CONSTANT);
			context->push_node(Compiler::make_none());
			context->push_node(Node::EXIT_CALL);
		}
		context->resolve_jump_forward();

		if (!context->create_member(context->retrieve_modifiers(), Symbol($1), context->retrieve_definition())) {
			YYERROR;
		}
	}
	| member_desc_rule PLUS_EQUAL_TOKEN def_start_rule def_args_rule stmt_bloc_rule {
		if (context->is_in_generator()) {
			context->push_node(Node::EXIT_GENERATOR);
		}
		else if (!context->has_returned()) {
			context->push_node(Node::LOAD_CONSTANT);
			context->push_node(Compiler::make_none());
			context->push_node(Node::EXIT_CALL);
		}
		context->resolve_jump_forward();

		if (!context->update_member(context->retrieve_modifiers(), Symbol($1), context->retrieve_definition())) {
			YYERROR;
		}
	}
	| def_start_rule SYMBOL_TOKEN def_args_rule stmt_bloc_rule {
		if (context->is_in_generator()) {
			context->push_node(Node::EXIT_GENERATOR);
		}
		else if (!context->has_returned()) {
			context->push_node(Node::LOAD_CONSTANT);
			context->push_node(Compiler::make_none());
			context->push_node(Node::EXIT_CALL);
		}
		context->resolve_jump_forward();

		if (!context->update_member(Reference::DEFAULT, Symbol($2), context->retrieve_definition())) {
			YYERROR;
		}
	}
	| def_start_rule operator_desc_rule def_args_rule stmt_bloc_rule {
		if (context->is_in_generator()) {
			context->push_node(Node::EXIT_GENERATOR);
		}
		else if (!context->has_returned()) {
			context->push_node(Node::LOAD_CONSTANT);
			context->push_node(Compiler::make_none());
			context->push_node(Node::EXIT_CALL);
		}
		context->resolve_jump_forward();

		if (!context->update_member(Reference::DEFAULT, context->retrieve_operator(), context->retrieve_definition())) {
			YYERROR;
		}
	}
	| desc_modifier_rule def_start_rule SYMBOL_TOKEN def_args_rule stmt_bloc_rule {
		if (context->is_in_generator()) {
			context->push_node(Node::EXIT_GENERATOR);
		}
		else if (!context->has_returned()) {
			context->push_node(Node::LOAD_CONSTANT);
			context->push_node(Compiler::make_none());
			context->push_node(Node::EXIT_CALL);
		}
		context->resolve_jump_forward();

		if (!context->update_member(context->retrieve_modifiers(), Symbol($3), context->retrieve_definition())) {
			YYERROR;
		}
	}
	| desc_modifier_rule def_start_rule operator_desc_rule def_args_rule stmt_bloc_rule {
		if (context->is_in_generator()) {
			context->push_node(Node::EXIT_GENERATOR);
		}
		else if (!context->has_returned()) {
			context->push_node(Node::LOAD_CONSTANT);
			context->push_node(Compiler::make_none());
			context->push_node(Node::EXIT_CALL);
		}
		context->resolve_jump_forward();

		if (!context->update_member(context->retrieve_modifiers(), context->retrieve_operator(), context->retrieve_definition())) {
			YYERROR;
		}
	}
	| member_class_desc_rule
	| member_enum_desc_rule
	| LINE_END_TOKEN {
		context->commit_line();
	};

member_desc_rule:
    SYMBOL_TOKEN {
		context->start_modifiers(Reference::DEFAULT);
		$$ = $1;
	}
	| desc_modifier_rule SYMBOL_TOKEN {
		$$ = $2;
	};

desc_base_modifier_rule:
	modifier_rule
	| FINAL_TOKEN {
		context->start_modifiers(Reference::FINAL_MEMBER);
	}
	| OVERRIDE_TOKEN {
		context->start_modifiers(Reference::OVERRIDE_MEMBER);
	}
	| FINAL_TOKEN modifier_rule {
		context->add_modifiers(Reference::FINAL_MEMBER);
	}
	| OVERRIDE_TOKEN modifier_rule {
		context->add_modifiers(Reference::OVERRIDE_MEMBER);
	};

desc_modifier_rule:
	desc_base_modifier_rule
	| PLUS_TOKEN {
		context->start_modifiers(Reference::DEFAULT);
	}
	| SHARP_TOKEN {
		context->start_modifiers(Reference::PROTECTED_VISIBILITY);
	}
	| MINUS_TOKEN {
		context->start_modifiers(Reference::PRIVATE_VISIBILITY);
	}
	| TILDE_TOKEN {
		context->start_modifiers(Reference::PACKAGE_VISIBILITY);
	}
	| PLUS_TOKEN desc_base_modifier_rule {
		context->add_modifiers(Reference::DEFAULT);
	}
	| SHARP_TOKEN desc_base_modifier_rule {
		context->add_modifiers(Reference::PROTECTED_VISIBILITY);
	}
	| MINUS_TOKEN desc_base_modifier_rule {
		context->add_modifiers(Reference::PRIVATE_VISIBILITY);
	}
	| TILDE_TOKEN desc_base_modifier_rule {
		context->add_modifiers(Reference::PACKAGE_VISIBILITY);
	};

operator_desc_rule:
    IN_TOKEN {
		context->start_operator(Class::IN_OPERATOR);
		$$ = $1;
	}
	| COLON_EQUAL_TOKEN {
		context->start_operator(Class::COPY_OPERATOR);
		$$ = $1;
	}
	| DBL_PIPE_TOKEN {
		context->start_operator(Class::OR_OPERATOR);
		$$ = $1;
	}
	| DBL_AMP_TOKEN {
		context->start_operator(Class::AND_OPERATOR);
		$$ = $1;
	}
	| PIPE_TOKEN {
		context->start_operator(Class::BOR_OPERATOR);
		$$ = $1;
	}
	| CARET_TOKEN {
		context->start_operator(Class::XOR_OPERATOR);
		$$ = $1;
	}
	| AMP_TOKEN {
		context->start_operator(Class::BAND_OPERATOR);
		$$ = $1;
	}
	| DBL_EQUAL_TOKEN {
		context->start_operator(Class::EQ_OPERATOR);
		$$ = $1;
	}
	| EXCLAMATION_EQUAL_TOKEN {
		context->start_operator(Class::NE_OPERATOR);
		$$ = $1;
	}
	| LEFT_ANGLED_TOKEN {
		context->start_operator(Class::LT_OPERATOR);
		$$ = $1;
	}
	| RIGHT_ANGLED_TOKEN {
		context->start_operator(Class::GT_OPERATOR);
		$$ = $1;
	}
	| LEFT_ANGLED_EQUAL_TOKEN {
		context->start_operator(Class::LE_OPERATOR);
		$$ = $1;
	}
	| RIGHT_ANGLED_EQUAL_TOKEN {
		context->start_operator(Class::GE_OPERATOR);
		$$ = $1;
	}
	| DBL_LEFT_ANGLED_TOKEN {
		context->start_operator(Class::SHIFT_LEFT_OPERATOR);
		$$ = $1;
	}
	| DBL_RIGHT_ANGLED_TOKEN {
		context->start_operator(Class::SHIFT_RIGHT_OPERATOR);
		$$ = $1;
	}
	| PLUS_TOKEN {
		context->start_operator(Class::ADD_OPERATOR);
		$$ = $1;
	}
	| MINUS_TOKEN {
		context->start_operator(Class::SUB_OPERATOR);
		$$ = $1;
	}
	| ASTERISK_TOKEN {
		context->start_operator(Class::MUL_OPERATOR);
		$$ = $1;
	}
	| SLASH_TOKEN {
		context->start_operator(Class::DIV_OPERATOR);
		$$ = $1;
	}
	| PERCENT_TOKEN {
		context->start_operator(Class::MOD_OPERATOR);
		$$ = $1;
	}
	| EXCLAMATION_TOKEN {
		context->start_operator(Class::NOT_OPERATOR);
		$$ = $1;
	}
	| TILDE_TOKEN {
		context->start_operator(Class::COMPL_OPERATOR);
		$$ = $1;
	}
	| DBL_PLUS_TOKEN {
		context->start_operator(Class::INC_OPERATOR);
		$$ = $1;
	}
	| DBL_MINUS_TOKEN {
		context->start_operator(Class::DEC_OPERATOR);
		$$ = $1;
	}
	| DBL_ASTERISK_TOKEN {
		context->start_operator(Class::POW_OPERATOR);
		$$ = $1;
	}
	| DBL_DOT_TOKEN {
		context->start_operator(Class::INCLUSIVE_RANGE_OPERATOR);
		$$ = $1;
	}
	| TPL_DOT_TOKEN {
		context->start_operator(Class::EXCLUSIVE_RANGE_OPERATOR);
		$$ = $1;
	}
	| OPEN_PARENTHESIS_TOKEN CLOSE_PARENTHESIS_TOKEN {
		context->start_operator(Class::CALL_OPERATOR);
		$$ = $1 + $2;
	}
	| OPEN_BRACKET_TOKEN CLOSE_BRACKET_TOKEN {
		context->start_operator(Class::SUBSCRIPT_OPERATOR);
		$$ = $1 + $2;
	}
	| OPEN_BRACKET_TOKEN CLOSE_BRACKET_EQUAL_TOKEN {
		context->start_operator(Class::SUBSCRIPT_MOVE_OPERATOR);
		$$ = $1 + $2;
	};

enum_rule:
    ENUM_TOKEN SYMBOL_TOKEN {
		context->start_enum_description($2);
	};

enum_desc_rule:
	enum_rule enum_block_rule {
		context->resolve_enum_description();
	};

enum_block_rule:
    OPEN_BRACE_TOKEN enum_list_rule CLOSE_BRACE_TOKEN;

enum_list_rule:
	enum_list_rule enum_item_rule
	| enum_item_rule;

enum_item_rule:
    SYMBOL_TOKEN EQUAL_TOKEN NUMBER_TOKEN {
		Reference::Flags flags = Reference::CONST_VALUE | Reference::CONST_ADDRESS | Reference::GLOBAL;
		if (!context->create_member(flags, Symbol($1), Compiler::make_data($3, Compiler::DATA_NUMBER_HINT))) {
			YYERROR;
		}
		context->set_current_enum_value(atoi($3.c_str()));
	}
	| SYMBOL_TOKEN {
		Reference::Flags flags = Reference::CONST_VALUE | Reference::CONST_ADDRESS | Reference::GLOBAL;
		if (!context->create_member(flags, Symbol($1), Compiler::make_data(std::to_string(context->next_enum_value()), Compiler::DATA_NUMBER_HINT))) {
			YYERROR;
		}
	}
	| LINE_END_TOKEN {
		context->commit_line();
	};

generator_expr_rule:
	if_cond_expr_rule stmt_bloc_rule {
		context->reset_scoped_symbols();
		context->resolve_jump_forward();
		context->close_generator_expression();
		context->close_block();
	}
	| if_bloc_expr_rule else_rule stmt_bloc_rule {
		context->reset_scoped_symbols();
		context->resolve_jump_forward();
		context->close_generator_expression();
		context->close_block();
	}
	| if_bloc_expr_rule elif_bloc_rule {
		context->resolve_jump_forward();
		context->close_generator_expression();
		context->close_block();
	}
	| if_bloc_expr_rule elif_bloc_rule else_rule stmt_bloc_rule {
		context->reset_scoped_symbols();
		context->resolve_jump_forward();
		context->close_generator_expression();
		context->close_block();
	}
	| switch_cond_expr_rule OPEN_BRACE_TOKEN case_list_rule CLOSE_BRACE_TOKEN {
		context->reset_scoped_symbols();
		context->push_node(Node::JUMP);
		context->start_jump_forward();
		context->build_case_table();
		context->resolve_jump_forward();
		context->resolve_jump_forward();
		context->close_generator_expression();
		context->close_block();
	}
	| while_cond_expr_rule stmt_bloc_rule {
		context->reset_scoped_symbols();
		context->push_node(Node::JUMP);
		context->resolve_jump_backward();
		context->resolve_jump_forward();
		context->close_generator_expression();
		context->close_block();
	}
	| for_cond_expr_rule stmt_bloc_rule {
		context->reset_scoped_symbols();
		context->push_node(Node::JUMP);
		context->resolve_jump_backward();
		context->resolve_jump_forward();
		context->close_generator_expression();
		context->close_block();
	};

try_rule:
    TRY_TOKEN {
		context->register_retrieve_point();
		context->push_node(Node::SET_RETRIEVE_POINT);
		context->start_jump_forward();
		context->open_block(BuildContext::TRY_TYPE);
	};

catch_rule:
    CATCH_TOKEN SYMBOL_TOKEN {
		context->close_block();
		context->unregister_retrieve_point();
		context->push_node(Node::UNSET_RETRIEVE_POINT);
		context->push_node(Node::JUMP);
		context->start_jump_forward();
		context->shift_jump_forward();
		context->resolve_jump_forward();
		context->open_block(BuildContext::CATCH_TYPE);
		context->push_node(Node::INIT_EXCEPTION);
		context->push_node($2.c_str());
		context->set_exception_symbol($2);
	};

try_bloc_rule:
	try_rule stmt_bloc_rule {
		context->reset_scoped_symbols();
	};

if_bloc_expr_rule:
	if_cond_expr_rule stmt_bloc_rule {
		context->reset_scoped_symbols();
	};

if_bloc_rule:
	if_cond_rule stmt_bloc_rule {
		context->reset_scoped_symbols();
	};

elif_bloc_rule:
	elif_cond_rule stmt_bloc_rule {
		context->reset_scoped_symbols();
		context->shift_jump_forward();
		context->resolve_jump_forward();
	}
	| elif_bloc_rule elif_cond_rule stmt_bloc_rule {
		context->reset_scoped_symbols();
		context->shift_jump_forward();
		context->resolve_jump_forward();
	};

stmt_bloc_rule:
    OPEN_BRACE_TOKEN stmt_list_rule CLOSE_BRACE_TOKEN
	| OPEN_BRACE_TOKEN YIELD_TOKEN expr_rule CLOSE_BRACE_TOKEN {
		if (!context->is_in_function()) {
			context->parse_error("unexpected 'yield' statement outside of function");
			YYERROR;
		}
		context->set_generator();
		context->push_node(Node::YIELD);
	}
	| OPEN_BRACE_TOKEN return_rule expr_rule CLOSE_BRACE_TOKEN {
		context->set_exit_point();
		if (context->is_in_generator()) {
			context->push_node(Node::YIELD_EXIT_GENERATOR);
		}
		else {
			context->push_node(Node::EXIT_CALL);
		}
	}
	| OPEN_BRACE_TOKEN expr_rule CLOSE_BRACE_TOKEN {
		context->commit_expr_result();
	}
	| OPEN_BRACE_TOKEN CLOSE_BRACE_TOKEN;

if_cond_expr_rule:
	if_expr_rule expr_rule {
		context->resolve_condition();
		context->push_node(Node::JUMP_ZERO);
		context->start_jump_forward();
		context->open_block(BuildContext::IF_TYPE);
	}
	| if_expr_rule find_rule {
		context->resolve_condition();
		context->push_node(Node::JUMP_ZERO);
		context->start_jump_forward();
		context->open_block(BuildContext::IF_TYPE);
	};

if_cond_rule:
	if_rule expr_rule {
		context->resolve_condition();
		context->push_node(Node::JUMP_ZERO);
		context->start_jump_forward();
		context->open_block(BuildContext::IF_TYPE);
	}
	| if_rule find_rule {
		context->resolve_condition();
		context->push_node(Node::JUMP_ZERO);
		context->start_jump_forward();
		context->open_block(BuildContext::IF_TYPE);
	};

elif_cond_rule:
	elif_rule expr_rule {
		context->resolve_condition();
		context->close_block();
		context->push_node(Node::JUMP_ZERO);
		context->start_jump_forward();
		context->open_block(BuildContext::ELIF_TYPE);
	}
	| elif_rule find_rule {
		context->resolve_condition();
		context->close_block();
		context->push_node(Node::JUMP_ZERO);
		context->start_jump_forward();
		context->open_block(BuildContext::ELIF_TYPE);
	};

if_expr_rule:
    IF_TOKEN {
		context->open_generator_expression();
		context->start_condition();
	};

if_rule:
    IF_TOKEN {
		context->start_condition();
	};

elif_rule:
    ELIF_TOKEN {
		context->push_node(Node::JUMP);
		context->start_jump_forward();
		context->shift_jump_forward();
		context->resolve_jump_forward();
		context->start_condition();
	};

else_rule:
    ELSE_TOKEN {
		context->push_node(Node::JUMP);
		context->start_jump_forward();

		context->close_block();
		context->shift_jump_forward();
		context->resolve_jump_forward();
		context->open_block(BuildContext::ELSE_TYPE);
	};

switch_cond_expr_rule:
	switch_expr_rule expr_rule {
		context->resolve_condition();
		context->open_block(BuildContext::SWITCH_TYPE);
	};

switch_cond_rule:
	switch_rule expr_rule {
		context->resolve_condition();
		context->open_block(BuildContext::SWITCH_TYPE);
	};

switch_expr_rule:
    SWITCH_TOKEN {
		context->open_generator_expression();
		context->start_condition();
	};

switch_rule:
    SWITCH_TOKEN {
		context->start_condition();
	};

case_rule:
    CASE_TOKEN {
		context->start_case_label();
	};

case_symbol_rule:
    SYMBOL_TOKEN {
		context->push_node(Node::LOAD_SYMBOL);
		context->push_node($1.c_str());
		$$ = $1;
	}
	| case_symbol_rule DOT_TOKEN SYMBOL_TOKEN {
		context->push_node(Node::LOAD_MEMBER);
		context->push_node($3.c_str());
		$$ = $1 + $2 + $3;
	};

case_constant_rule:
	constant_rule {
		if (Data *data = Compiler::make_data($1, Compiler::DATA_UNKNOWN_HINT)) {
			context->push_node(Node::LOAD_CONSTANT);
			context->push_node(data);
			$$ = $1;
		}
		else {
			error("token '" + $1 + "' is not a valid constant");
			YYERROR;
		}
	}
	| PLUS_TOKEN NUMBER_TOKEN {
		if (Data *data = Compiler::make_data($2, Compiler::DATA_NUMBER_HINT)) {
			context->push_node(Node::LOAD_CONSTANT);
			context->push_node(data);
			context->push_node(Node::POS_OP);
			$$ = $2;
		}
		else {
			error("token '" + $2 + "' is not a valid constant");
			YYERROR;
		}
	}
	| MINUS_TOKEN NUMBER_TOKEN {
		if (Data *data = Compiler::make_data($2, Compiler::DATA_NUMBER_HINT)) {
			context->push_node(Node::LOAD_CONSTANT);
			context->push_node(data);
			context->push_node(Node::NEG_OP);
			$$ = $1 + $2;
		}
		else {
			error("token '" + $2 + "' is not a valid constant");
			YYERROR;
		}
	};

case_constant_list_rule:
    case_constant_list_rule case_constant_rule COMMA_TOKEN {
		context->add_to_call();
		$$ = $1 + $2 + $3;
	}
	| case_constant_rule COMMA_TOKEN {
		context->push_node(Node::ALLOC_ITERATOR);
		context->start_call();
		context->add_to_call();
		$$ = $1 + $2;
	};

case_constant_list_end_rule:
	case_constant_rule {
		context->push_node(Node::CREATE_ITERATOR);
		context->add_to_call();
		context->resolve_call();
		$$ = $1;
	}
	| {
		context->push_node(Node::CREATE_ITERATOR);
		context->resolve_call();
	};

case_label_rule:
    case_rule IN_TOKEN case_constant_rule DBL_DOT_TOKEN case_constant_rule {
		context->push_node(Node::INCLUSIVE_RANGE_OP);
		context->start_jump_backward();
		context->push_node(Node::FIND_NEXT);
		context->push_node(Node::FIND_CHECK);
		context->start_jump_forward();
		context->push_node(Node::JUMP);
		context->resolve_jump_backward();
		context->resolve_jump_forward();
		context->resolve_case_label($3 + $4 + $5);
	}
	| case_rule IN_TOKEN case_constant_rule TPL_DOT_TOKEN case_constant_rule {
		context->push_node(Node::EXCLUSIVE_RANGE_OP);
		context->start_jump_backward();
		context->push_node(Node::FIND_NEXT);
		context->push_node(Node::FIND_CHECK);
		context->start_jump_forward();
		context->push_node(Node::JUMP);
		context->resolve_jump_backward();
		context->resolve_jump_forward();
		context->resolve_case_label($3 + $4 + $5);
	}
	| case_rule IN_TOKEN case_constant_list_rule case_constant_list_end_rule {
		context->start_jump_backward();
		context->push_node(Node::FIND_NEXT);
		context->push_node(Node::FIND_CHECK);
		context->start_jump_forward();
		context->push_node(Node::JUMP);
		context->resolve_jump_backward();
		context->resolve_jump_forward();
		context->resolve_case_label($3 + $4);
	}
	| case_rule IN_TOKEN case_constant_rule {
		context->push_node(Node::FIND_OP);
		context->push_node(Node::FIND_INIT);
		context->start_jump_backward();
		context->push_node(Node::FIND_NEXT);
		context->push_node(Node::FIND_CHECK);
		context->start_jump_forward();
		context->push_node(Node::JUMP);
		context->resolve_jump_backward();
		context->resolve_jump_forward();
		context->resolve_case_label($3);
	}
	| case_rule IN_TOKEN case_symbol_rule {
		context->push_node(Node::FIND_OP);
		context->push_node(Node::FIND_INIT);
		context->start_jump_backward();
		context->push_node(Node::FIND_NEXT);
		context->push_node(Node::FIND_CHECK);
		context->start_jump_forward();
		context->push_node(Node::JUMP);
		context->resolve_jump_backward();
		context->resolve_jump_forward();
		context->resolve_case_label($3);
	}
	| case_rule IS_TOKEN case_constant_rule {
		context->push_node(Node::IS_OP);
		context->resolve_case_label($3);
	}
	| case_rule IS_TOKEN case_symbol_rule {
		context->push_node(Node::IS_OP);
		context->resolve_case_label($3);
	}
	| case_rule case_constant_rule {
		context->push_node(Node::EQ_OP);
		context->resolve_case_label($2);
	}
	| case_rule case_symbol_rule {
		context->push_node(Node::EQ_OP);
		context->resolve_case_label($2);
	};

default_rule:
    DEFAULT_TOKEN {
		context->set_default_label();
	};

case_list_rule:
    LINE_END_TOKEN {
		context->commit_line();
	}
	| case_label_rule COLON_TOKEN stmt_list_rule
	| case_list_rule case_label_rule COLON_TOKEN stmt_list_rule
	| default_rule COLON_TOKEN stmt_list_rule
	| case_list_rule default_rule COLON_TOKEN stmt_list_rule
	| case_label_rule EQUAL_RIGHT_ANGLED_TOKEN expr_rule LINE_END_TOKEN {
	    context->commit_expr_result();
		context->prepare_break();
		context->push_node(Node::JUMP);
		context->bloc_jump_forward();
		context->commit_line();
	}
	| case_list_rule case_label_rule EQUAL_RIGHT_ANGLED_TOKEN expr_rule LINE_END_TOKEN {
	    context->commit_expr_result();
		context->prepare_break();
		context->push_node(Node::JUMP);
		context->bloc_jump_forward();
		context->commit_line();
	}
	| default_rule EQUAL_RIGHT_ANGLED_TOKEN expr_rule LINE_END_TOKEN {
	    context->commit_expr_result();
		context->prepare_break();
		context->push_node(Node::JUMP);
		context->bloc_jump_forward();
		context->commit_line();
	}
	| case_list_rule default_rule EQUAL_RIGHT_ANGLED_TOKEN expr_rule LINE_END_TOKEN {
	    context->commit_expr_result();
		context->prepare_break();
		context->push_node(Node::JUMP);
		context->bloc_jump_forward();
		context->commit_line();
	};

while_cond_expr_rule:
	while_expr_rule expr_rule {
		context->resolve_condition();
		context->push_node(Node::JUMP_ZERO);
		context->start_jump_forward();
		context->open_block(BuildContext::CONDITIONAL_LOOP_TYPE);
	}
	| while_expr_rule find_rule {
		context->resolve_condition();
		context->push_node(Node::JUMP_ZERO);
		context->start_jump_forward();
		context->open_block(BuildContext::CONDITIONAL_LOOP_TYPE);
	};

while_cond_rule:
    while_rule expr_rule {
		context->resolve_condition();
		context->push_node(Node::JUMP_ZERO);
		context->start_jump_forward();
		context->open_block(BuildContext::CONDITIONAL_LOOP_TYPE);
	}
	| while_rule find_rule {
		context->resolve_condition();
		context->push_node(Node::JUMP_ZERO);
		context->start_jump_forward();
		context->open_block(BuildContext::CONDITIONAL_LOOP_TYPE);
	};

while_expr_rule:
    WHILE_TOKEN {
		context->open_generator_expression();
		context->start_jump_backward();
		context->start_condition();
	};

while_rule:
    WHILE_TOKEN {
		context->start_jump_backward();
		context->start_condition();
	};

find_rule:
    expr_rule IN_TOKEN find_init_rule {
		context->start_jump_backward();
		context->push_node(Node::FIND_NEXT);
		context->push_node(Node::FIND_CHECK);
		context->start_jump_forward();
		context->push_node(Node::JUMP);
		context->resolve_jump_backward();
		context->resolve_jump_forward();
	}
	| expr_rule EXCLAMATION_TOKEN IN_TOKEN find_init_rule {
		context->start_jump_backward();
		context->push_node(Node::FIND_NEXT);
		context->push_node(Node::FIND_CHECK);
		context->start_jump_forward();
		context->push_node(Node::JUMP);
		context->resolve_jump_backward();
		context->resolve_jump_forward();
		context->push_node(Node::NOT_OP);
	};

find_init_rule:
	expr_rule {
		context->push_node(Node::FIND_OP);
		context->push_node(Node::FIND_INIT);
	};

for_cond_expr_rule:
    for_expr_rule OPEN_PARENTHESIS_TOKEN range_init_rule range_next_rule range_cond_rule CLOSE_PARENTHESIS_TOKEN {
		context->resolve_condition();
		context->open_block(BuildContext::CUSTOM_RANGE_LOOP_TYPE);
	}
	| for_iterator_in_expr_rule expr_rule {
		context->push_node(Node::IN_OP);
		context->push_node(Node::RANGE_INIT);
		context->resolve_condition();
		context->push_node(Node::JUMP);
		context->start_jump_forward();
		context->start_jump_backward();
		context->push_node(Node::RANGE_NEXT);
		context->resolve_jump_forward();
		context->push_node(Node::RANGE_ITERATOR_CHECK);
		context->start_jump_forward();
		context->open_block(BuildContext::RANGE_LOOP_TYPE);
	}
	| for_in_expr_rule expr_rule {
		context->push_node(Node::IN_OP);
		context->push_node(Node::RANGE_INIT);
		context->resolve_condition();
		context->push_node(Node::JUMP);
		context->start_jump_forward();
		context->start_jump_backward();
		context->push_node(Node::RANGE_NEXT);
		context->resolve_jump_forward();
		context->push_node(Node::RANGE_CHECK);
		context->start_jump_forward();
		context->open_block(BuildContext::RANGE_LOOP_TYPE);
	};

for_cond_rule:
    for_rule OPEN_PARENTHESIS_TOKEN range_init_rule range_next_rule range_cond_rule CLOSE_PARENTHESIS_TOKEN {
		context->resolve_condition();
		context->open_block(BuildContext::CUSTOM_RANGE_LOOP_TYPE);
	}
	| for_iterator_in_rule expr_rule {
		context->push_node(Node::IN_OP);
		context->push_node(Node::RANGE_INIT);
		context->resolve_condition();
		context->push_node(Node::JUMP);
		context->start_jump_forward();
		context->start_jump_backward();
		context->push_node(Node::RANGE_NEXT);
		context->resolve_jump_forward();
		context->push_node(Node::RANGE_ITERATOR_CHECK);
		context->start_jump_forward();
		context->open_block(BuildContext::RANGE_LOOP_TYPE);
	}
	| for_in_rule expr_rule {
		context->push_node(Node::IN_OP);
		context->push_node(Node::RANGE_INIT);
		context->resolve_condition();
		context->push_node(Node::JUMP);
		context->start_jump_forward();
		context->start_jump_backward();
		context->push_node(Node::RANGE_NEXT);
		context->resolve_jump_forward();
		context->push_node(Node::RANGE_CHECK);
		context->start_jump_forward();
		context->open_block(BuildContext::RANGE_LOOP_TYPE);
	};

for_expr_rule:
    FOR_TOKEN {
		context->open_generator_expression();
		context->start_range_loop();
	};

for_rule:
    FOR_TOKEN {
		context->start_range_loop();
	};

for_in_expr_rule:
    for_expr_rule ident_rule IN_TOKEN {
		context->resolve_range_loop();
		context->start_condition();
	};

for_in_rule:
    for_rule ident_rule IN_TOKEN {
		context->resolve_range_loop();
		context->start_condition();
	};

for_iterator_in_expr_rule:
    for_expr_rule ident_iterator_item_rule ident_iterator_end_rule IN_TOKEN {
		context->resolve_range_loop();
		context->start_condition();
	}
	| for_expr_rule create_ident_iterator_rule IN_TOKEN {
		context->resolve_range_loop();
		context->start_condition();
	};

for_iterator_in_rule:
    for_rule ident_iterator_item_rule ident_iterator_end_rule IN_TOKEN {
		context->resolve_range_loop();
		context->start_condition();
	}
	| for_rule create_ident_iterator_rule IN_TOKEN {
		context->resolve_range_loop();
		context->start_condition();
	};

range_init_rule:
    expr_rule COMMA_TOKEN {
		context->push_node(Node::UNLOAD_REFERENCE);
		context->push_node(Node::JUMP);
		context->start_jump_forward();
		context->start_jump_backward();
		context->resolve_range_loop();
		context->start_condition();
	};

range_next_rule:
    expr_rule COMMA_TOKEN {
		context->push_node(Node::UNLOAD_REFERENCE);
		context->resolve_jump_forward();
	};

range_cond_rule:
	expr_rule {
		context->push_node(Node::JUMP_ZERO);
		context->start_jump_forward();
	};

return_rule:
    RETURN_TOKEN {
		if (!context->is_in_function()) {
			context->parse_error("unexpected 'return' statement outside of function");
			YYERROR;
		}
		context->prepare_return();
	};

start_hash_rule:
    OPEN_BRACE_TOKEN {
		context->push_node(Node::ALLOC_HASH);
		context->start_call();
	};

stop_hash_rule:
    CLOSE_BRACE_TOKEN {
		context->push_node(Node::CREATE_HASH);
		context->resolve_call();
	};

hash_item_rule:
    hash_item_rule separator_rule expr_rule COLON_TOKEN expr_rule {
		context->add_to_call();
	}
	| expr_rule COLON_TOKEN expr_rule {
		context->add_to_call();
	};

start_array_rule:
    OPEN_BRACKET_TOKEN {
		context->push_node(Node::ALLOC_ARRAY);
		context->start_call();
	};

stop_array_rule:
    CLOSE_BRACKET_TOKEN {
		context->push_node(Node::CREATE_ARRAY);
		context->resolve_call();
	};

array_item_list_rule:
	array_item_list_rule separator_rule array_item_rule
	| array_item_rule;

array_item_rule:
	expr_rule {
		context->add_to_call();
	}
	| ASTERISK_TOKEN expr_rule {
		context->push_node(Node::IN_OP);
		context->push_node(Node::LOAD_EXTRA_ARGUMENTS);
	}
	| generator_expr_rule {
		context->push_node(Node::LOAD_EXTRA_ARGUMENTS);
	};

iterator_item_rule:
	iterator_item_rule expr_rule separator_rule {
		context->add_to_call();
	}
	| expr_rule separator_rule {
		context->push_node(Node::ALLOC_ITERATOR);
		context->start_call();
		context->add_to_call();
	}
	| iterator_item_rule ASTERISK_TOKEN expr_rule separator_rule {
		context->push_node(Node::IN_OP);
		context->push_node(Node::LOAD_EXTRA_ARGUMENTS);
	}
	| ASTERISK_TOKEN expr_rule separator_rule {
		context->push_node(Node::ALLOC_ITERATOR);
		context->start_call();
		context->push_node(Node::IN_OP);
		context->push_node(Node::LOAD_EXTRA_ARGUMENTS);
	};

iterator_end_rule:
	expr_rule {
		context->push_node(Node::CREATE_ITERATOR);
		context->add_to_call();
		context->resolve_call();
	}
	| {
		context->push_node(Node::CREATE_ITERATOR);
		context->resolve_call();
	};

ident_iterator_item_rule:
	ident_iterator_item_rule ident_rule separator_rule {
		context->add_to_call();
	}
	| ident_rule separator_rule {
		context->push_node(Node::ALLOC_ITERATOR);
		context->start_call();
		context->add_to_call();
	};

ident_iterator_end_rule:
	ident_rule {
		context->push_node(Node::CREATE_ITERATOR);
		context->add_to_call();
		context->resolve_call();
	}
	| {
		context->push_node(Node::CREATE_ITERATOR);
		context->resolve_call();
	};

let_modifier_rule:
    LET_TOKEN {
	    context->start_modifiers(Reference::DEFAULT);
	};

create_ident_iterator_rule:
    LET_TOKEN modifier_rule create_ident_iterator_scoped_item_rule create_ident_iterator_scoped_end_rule
	| let_modifier_rule create_ident_iterator_scoped_item_rule create_ident_iterator_scoped_end_rule
	| modifier_rule create_ident_iterator_item_rule create_ident_iterator_end_rule;

create_ident_iterator_scoped_item_rule:
    create_ident_iterator_scoped_item_rule SYMBOL_TOKEN COMMA_TOKEN {
		const int index = context->create_fast_scoped_symbol_index($2);
		if (index != -1) {
			context->push_node(Node::CREATE_FAST);
			context->push_node($2.c_str());
			context->push_node(index);
			context->push_node(context->get_modifiers());
		}
		else {
			context->push_node(Node::CREATE_SYMBOL);
			context->push_node($2.c_str());
			context->push_node(context->get_modifiers());
		}
		context->add_to_call();
	}
	| OPEN_PARENTHESIS_TOKEN SYMBOL_TOKEN COMMA_TOKEN {
		context->push_node(Node::ALLOC_ITERATOR);
		context->start_call();
		const int index = context->create_fast_scoped_symbol_index($2);
		if (index != -1) {
			context->push_node(Node::CREATE_FAST);
			context->push_node($2.c_str());
			context->push_node(index);
			context->push_node(context->get_modifiers());
		}
		else {
			context->push_node(Node::CREATE_SYMBOL);
			context->push_node($2.c_str());
			context->push_node(context->get_modifiers());
		}
		context->add_to_call();
	};

create_ident_iterator_scoped_end_rule:
    SYMBOL_TOKEN CLOSE_PARENTHESIS_TOKEN {
		const int index = context->create_fast_scoped_symbol_index($1);
		if (index != -1) {
			context->push_node(Node::CREATE_FAST);
			context->push_node($1.c_str());
			context->push_node(index);
			context->push_node(context->retrieve_modifiers());
		}
		else {
			context->push_node(Node::CREATE_SYMBOL);
			context->push_node($1.c_str());
			context->push_node(context->retrieve_modifiers());
		}
		context->push_node(Node::CREATE_ITERATOR);
		context->add_to_call();
		context->resolve_call();
	};

create_ident_iterator_item_rule:
    create_ident_iterator_item_rule SYMBOL_TOKEN COMMA_TOKEN {
		const int index = context->create_fast_symbol_index($2);
		if (index != -1) {
			context->push_node(Node::CREATE_FAST);
			context->push_node($2.c_str());
			context->push_node(index);
			context->push_node(context->get_modifiers());
		}
		else {
			context->push_node(Node::CREATE_SYMBOL);
			context->push_node($2.c_str());
			context->push_node(context->get_modifiers());
		}
		context->add_to_call();
	}
	| OPEN_PARENTHESIS_TOKEN SYMBOL_TOKEN COMMA_TOKEN {
		context->push_node(Node::ALLOC_ITERATOR);
		context->start_call();
		const int index = context->create_fast_symbol_index($2);
		if (index != -1) {
			context->push_node(Node::CREATE_FAST);
			context->push_node($2.c_str());
			context->push_node(index);
			context->push_node(context->get_modifiers());
		}
		else {
			context->push_node(Node::CREATE_SYMBOL);
			context->push_node($2.c_str());
			context->push_node(context->get_modifiers());
		}
		context->add_to_call();
	};

create_ident_iterator_end_rule:
    SYMBOL_TOKEN CLOSE_PARENTHESIS_TOKEN {
		const int index = context->create_fast_symbol_index($1);
		if (index != -1) {
			context->push_node(Node::CREATE_FAST);
			context->push_node($1.c_str());
			context->push_node(index);
			context->push_node(context->retrieve_modifiers());
		}
		else {
			context->push_node(Node::CREATE_SYMBOL);
			context->push_node($1.c_str());
			context->push_node(context->retrieve_modifiers());
		}
		context->push_node(Node::CREATE_ITERATOR);
		context->add_to_call();
		context->resolve_call();
	};

print_rule:
    PRINT_TOKEN {
		context->push_node(Node::LOAD_CONSTANT);
		context->push_node(Compiler::make_data("1", Compiler::DATA_NUMBER_HINT));
		context->open_printer();
		context->open_block(BuildContext::PRINT_TYPE);
	}
	| PRINT_TOKEN OPEN_PARENTHESIS_TOKEN expr_rule CLOSE_PARENTHESIS_TOKEN {
		context->open_printer();
		context->open_block(BuildContext::PRINT_TYPE);
	};

expr_rule:
    expr_rule EQUAL_TOKEN generator_expr_rule {
		context->push_node(Node::MOVE_OP);
	}
	| expr_rule EQUAL_TOKEN expr_rule {
		context->push_node(Node::MOVE_OP);
	}
	| expr_rule COLON_EQUAL_TOKEN generator_expr_rule {
		context->push_node(Node::COPY_OP);
	}
	| expr_rule COLON_EQUAL_TOKEN expr_rule {
		context->push_node(Node::COPY_OP);
	}
	| expr_rule EQUAL_RIGHT_ANGLED_TOKEN {
	    context->push_node(Node::ALLOC_ITERATOR);
		context->start_call();
		context->add_to_call();
		context->push_node(Node::CREATE_ITERATOR);
		context->resolve_call();
	} generator_expr_rule {
	    context->push_node(Node::COPY_OP);
	}
	| expr_rule PLUS_TOKEN expr_rule {
		context->push_node(Node::ADD_OP);
	}
	| expr_rule MINUS_TOKEN expr_rule {
		context->push_node(Node::SUB_OP);
	}
	| expr_rule ASTERISK_TOKEN expr_rule {
		context->push_node(Node::MUL_OP);
	}
	| expr_rule SLASH_TOKEN expr_rule {
		context->push_node(Node::DIV_OP);
	}
	| expr_rule PERCENT_TOKEN expr_rule {
		context->push_node(Node::MOD_OP);
	}
	| expr_rule DBL_ASTERISK_TOKEN expr_rule {
		context->push_node(Node::POW_OP);
	}
	| expr_rule IS_TOKEN expr_rule {
		context->push_node(Node::IS_OP);
	}
	| expr_rule DBL_EQUAL_TOKEN expr_rule {
		context->push_node(Node::EQ_OP);
	}
	| expr_rule EXCLAMATION_EQUAL_TOKEN expr_rule {
		context->push_node(Node::NE_OP);
	}
	| expr_rule LEFT_ANGLED_TOKEN expr_rule {
		context->push_node(Node::LT_OP);
	}
	| expr_rule RIGHT_ANGLED_TOKEN expr_rule {
		context->push_node(Node::GT_OP);
	}
	| expr_rule LEFT_ANGLED_EQUAL_TOKEN expr_rule {
		context->push_node(Node::LE_OP);
	}
	| expr_rule RIGHT_ANGLED_EQUAL_TOKEN expr_rule {
		context->push_node(Node::GE_OP);
	}
	| expr_rule DBL_LEFT_ANGLED_TOKEN expr_rule {
		context->push_node(Node::SHIFT_LEFT_OP);
	}
	| expr_rule DBL_RIGHT_ANGLED_TOKEN expr_rule {
		context->push_node(Node::SHIFT_RIGHT_OP);
	}
	| expr_rule DBL_DOT_TOKEN expr_rule {
		context->push_node(Node::INCLUSIVE_RANGE_OP);
	}
	| expr_rule TPL_DOT_TOKEN expr_rule {
		context->push_node(Node::EXCLUSIVE_RANGE_OP);
	}
	| DBL_PLUS_TOKEN expr_rule %prec PREFIX_DBL_PLUS_TOKEN {
		context->push_node(Node::INC_OP);
	}
	| DBL_MINUS_TOKEN expr_rule %prec PREFIX_DBL_MINUS_TOKEN {
		context->push_node(Node::DEC_OP);
	}
	| expr_rule DBL_PLUS_TOKEN {
	    context->push_node(Node::CLONE_REFERENCE);
		context->push_node(Node::INC_OP);
		context->push_node(Node::UNLOAD_REFERENCE);
	}
	| expr_rule DBL_MINUS_TOKEN {
	    context->push_node(Node::CLONE_REFERENCE);
		context->push_node(Node::DEC_OP);
		context->push_node(Node::UNLOAD_REFERENCE);
	}
	| EXCLAMATION_TOKEN expr_rule {
		context->push_node(Node::NOT_OP);
	}
	| expr_rule DBL_PIPE_TOKEN {
		context->push_node(Node::OR_PRE_CHECK);
		context->start_jump_forward();
	} expr_rule {
		context->push_node(Node::OR_OP);
		context->resolve_jump_forward();
	}
	| expr_rule DBL_AMP_TOKEN {
		context->push_node(Node::AND_PRE_CHECK);
		context->start_jump_forward();
	} expr_rule {
		context->push_node(Node::AND_OP);
		context->resolve_jump_forward();
	}
	| expr_rule PIPE_TOKEN expr_rule {
		context->push_node(Node::BOR_OP);
	}
	| expr_rule AMP_TOKEN expr_rule {
		context->push_node(Node::BAND_OP);
	}
	| expr_rule CARET_TOKEN expr_rule {
		context->push_node(Node::XOR_OP);
	}
	| TILDE_TOKEN expr_rule {
		context->push_node(Node::COMPL_OP);
	}
	| PLUS_TOKEN expr_rule %prec PREFIX_PLUS_TOKEN {
		context->push_node(Node::POS_OP);
	}
	| MINUS_TOKEN expr_rule %prec PREFIX_MINUS_TOKEN {
		context->push_node(Node::NEG_OP);
	}
	| TYPEOF_TOKEN expr_rule {
		context->push_node(Node::TYPEOF_OP);
	}
	| MEMBERSOF_TOKEN expr_rule {
		context->push_node(Node::MEMBERSOF_OP);
	}
	| DEFINED_TOKEN defined_symbol_rule {
		context->push_node(Node::CHECK_DEFINED);
	}
	| expr_rule OPEN_BRACKET_TOKEN expr_rule CLOSE_BRACKET_EQUAL_TOKEN expr_rule {
		context->push_node(Node::SUBSCRIPT_MOVE_OP);
	}
	| expr_rule subscript_rule
	| member_ident_rule
	| ident_rule call_args_rule
	| def_rule call_args_rule
	| expr_rule subscript_rule call_args_rule
	| expr_rule DOT_TOKEN call_member_args_rule
	| OPEN_PARENTHESIS_TOKEN expr_rule CLOSE_PARENTHESIS_TOKEN call_args_rule
	| expr_rule PLUS_EQUAL_TOKEN {
		context->push_node(Node::RELOAD_REFERENCE);
	} expr_rule {
		context->push_node(Node::ADD_OP);
		context->push_node(Node::MOVE_OP);
	}
	| expr_rule MINUS_EQUAL_TOKEN {
		context->push_node(Node::RELOAD_REFERENCE);
	} expr_rule {
		context->push_node(Node::SUB_OP);
		context->push_node(Node::MOVE_OP);
	}
	| expr_rule ASTERISK_EQUAL_TOKEN {
		context->push_node(Node::RELOAD_REFERENCE);
	} expr_rule {
		context->push_node(Node::MUL_OP);
		context->push_node(Node::MOVE_OP);
	}
	| expr_rule SLASH_EQUAL_TOKEN {
		context->push_node(Node::RELOAD_REFERENCE);
	} expr_rule {
		context->push_node(Node::DIV_OP);
		context->push_node(Node::MOVE_OP);
	}
	| expr_rule PERCENT_EQUAL_TOKEN {
		context->push_node(Node::RELOAD_REFERENCE);
	} expr_rule {
		context->push_node(Node::MOD_OP);
		context->push_node(Node::MOVE_OP);
	}
	| expr_rule DBL_LEFT_ANGLED_EQUAL_TOKEN {
		context->push_node(Node::RELOAD_REFERENCE);
	} expr_rule {
		context->push_node(Node::SHIFT_LEFT_OP);
		context->push_node(Node::MOVE_OP);
	}
	| expr_rule DBL_RIGHT_ANGLED_EQUAL_TOKEN {
		context->push_node(Node::RELOAD_REFERENCE);
	} expr_rule {
		context->push_node(Node::SHIFT_RIGHT_OP);
		context->push_node(Node::MOVE_OP);
	}
	| expr_rule AMP_EQUAL_TOKEN {
		context->push_node(Node::RELOAD_REFERENCE);
	} expr_rule {
		context->push_node(Node::BAND_OP);
		context->push_node(Node::MOVE_OP);
	}
	| expr_rule PIPE_EQUAL_TOKEN {
		context->push_node(Node::RELOAD_REFERENCE);
	} expr_rule {
		context->push_node(Node::BOR_OP);
		context->push_node(Node::MOVE_OP);
	}
	| expr_rule CARET_EQUAL_TOKEN {
		context->push_node(Node::RELOAD_REFERENCE);
	} expr_rule {
		context->push_node(Node::XOR_OP);
		context->push_node(Node::MOVE_OP);
	}
	| expr_rule EQUAL_TILDE_TOKEN expr_rule {
		context->push_node(Node::REGEX_MATCH);
	}
	| expr_rule EXCLAMATION_TILDE_TOKEN expr_rule {
		context->push_node(Node::REGEX_UNMATCH);
	}
	| expr_rule TPL_EQUAL_TOKEN expr_rule {
		context->push_node(Node::STRICT_EQ_OP);
	}
	| expr_rule EXCLAMATION_DBL_EQUAL_TOKEN expr_rule {
		context->push_node(Node::STRICT_NE_OP);
	}
	| expr_rule QUESTION_TOKEN {
		context->push_node(Node::JUMP_ZERO);
		context->start_jump_forward();
	} expr_rule COLON_TOKEN {
		context->push_node(Node::JUMP);
		context->start_jump_forward();
		context->shift_jump_forward();
		context->resolve_jump_forward();
	} expr_rule {
		context->resolve_jump_forward();
	}
	| OPEN_PARENTHESIS_TOKEN CLOSE_PARENTHESIS_TOKEN {
		context->push_node(Node::ALLOC_ITERATOR);
		context->start_call();
		context->push_node(Node::CREATE_ITERATOR);
		context->resolve_call();
	}
	| OPEN_PARENTHESIS_TOKEN expr_rule CLOSE_PARENTHESIS_TOKEN
	| OPEN_PARENTHESIS_TOKEN iterator_item_rule iterator_end_rule CLOSE_PARENTHESIS_TOKEN
	| start_array_rule empty_lines_rule array_item_list_rule empty_lines_rule stop_array_rule
	| start_array_rule empty_lines_rule array_item_list_rule stop_array_rule
	| start_array_rule array_item_list_rule empty_lines_rule stop_array_rule
	| start_array_rule array_item_list_rule stop_array_rule
	| start_array_rule stop_array_rule
	| start_hash_rule empty_lines_rule hash_item_rule empty_lines_rule stop_hash_rule
	| start_hash_rule empty_lines_rule hash_item_rule stop_hash_rule
	| start_hash_rule hash_item_rule empty_lines_rule stop_hash_rule
	| start_hash_rule hash_item_rule stop_hash_rule
	| start_hash_rule stop_hash_rule
	| def_rule
	| ident_rule;

subscript_rule:
    OPEN_BRACKET_TOKEN expr_rule CLOSE_BRACKET_TOKEN {
		context->push_node(Node::SUBSCRIPT_OP);
	};

call_args_rule:
	call_arg_start_rule call_arg_list_rule call_arg_stop_rule
	| call_args_rule call_arg_start_rule call_arg_list_rule call_arg_stop_rule;

call_member_args_rule:
	call_member_arg_start_rule call_arg_list_rule call_member_arg_stop_rule
	| call_member_args_rule call_arg_start_rule call_arg_list_rule call_arg_stop_rule;

call_arg_start_rule:
    OPEN_PARENTHESIS_TOKEN {
		context->push_node(Node::INIT_CALL);
		context->start_call();
	};

call_arg_stop_rule:
    CLOSE_PARENTHESIS_TOKEN {
		context->push_node(Node::CALL);
		context->resolve_call();
	};

call_member_arg_start_rule:
    SYMBOL_TOKEN OPEN_PARENTHESIS_TOKEN {
		context->push_node(Node::INIT_MEMBER_CALL);
		context->push_node($1.c_str());
		context->start_call();
	}
	| operator_desc_rule OPEN_PARENTHESIS_TOKEN {
		context->push_node(Node::INIT_OPERATOR_CALL);
		context->push_node(context->retrieve_operator());
		context->start_call();
	}
	| var_symbol_rule OPEN_PARENTHESIS_TOKEN {
		context->push_node(Node::INIT_VAR_MEMBER_CALL);
		context->start_call();
	};

call_member_arg_stop_rule:
    CLOSE_PARENTHESIS_TOKEN {
		context->push_node(Node::CALL_MEMBER);
		context->resolve_call();
	};

call_arg_list_rule:
	call_arg_list_rule separator_rule call_arg_rule
	| call_arg_rule
	| ;

call_arg_rule:
	expr_rule {
		context->add_to_call();
	}
	| def_arrow_rule {
	    context->add_to_call();
	}
	| ASTERISK_TOKEN expr_rule {
		context->push_node(Node::IN_OP);
		context->push_node(Node::LOAD_EXTRA_ARGUMENTS);
	};

def_rule:
	def_start_rule def_capture_rule def_args_rule stmt_bloc_rule {
		if (context->is_in_generator()) {
			context->push_node(Node::EXIT_GENERATOR);
		}
		else if (!context->has_returned()) {
			context->push_node(Node::LOAD_CONSTANT);
			context->push_node(Compiler::make_none());
			context->push_node(Node::EXIT_CALL);
		}
		context->resolve_jump_forward();
		context->save_definition();
	}
	| def_start_rule def_capture_rule def_no_args_rule stmt_bloc_rule {
		if (context->is_in_generator()) {
			context->push_node(Node::EXIT_GENERATOR);
		}
		else if (!context->has_returned()) {
			context->push_node(Node::LOAD_CONSTANT);
			context->push_node(Compiler::make_none());
			context->push_node(Node::EXIT_CALL);
		}
		context->resolve_jump_forward();
		context->save_definition();
	};

def_arrow_rule:
    def_start_rule def_capture_rule def_args_rule def_arrow_stmt_rule {
	    context->set_exit_point();
		context->push_node(Node::EXIT_CALL);
		context->resolve_jump_forward();
		context->save_definition();
	};

def_start_rule:
    DEF_TOKEN {
		context->push_node(Node::JUMP);
		context->start_jump_forward();
		context->start_definition();
	};

def_capture_rule:
	def_capture_start_rule def_capture_list_rule def_capture_stop_rule
	| ;

def_capture_start_rule:
    OPEN_BRACKET_TOKEN {
		context->start_capture();
	};

def_capture_stop_rule:
    CLOSE_BRACKET_TOKEN {
		context->resolve_capture();
	};

def_capture_list_rule:
    SYMBOL_TOKEN EQUAL_TOKEN expr_rule separator_rule def_capture_list_rule {
		if (!context->capture_as($1)) {
			YYERROR;
		}
	}
	| SYMBOL_TOKEN EQUAL_TOKEN expr_rule {
		if (!context->capture_as($1)) {
			YYERROR;
		}
	}
	| SYMBOL_TOKEN separator_rule def_capture_list_rule {
		if (!context->capture($1)) {
			YYERROR;
		}
	}
	| SYMBOL_TOKEN {
		if (!context->capture($1)) {
			YYERROR;
		}
	}
	| TPL_DOT_TOKEN {
		if (!context->capture_all()) {
			YYERROR;
		}
	};

def_no_args_rule:
	{
		if (!context->save_parameters()) {
			YYERROR;
		}
	};

def_args_rule:
	def_arg_start_rule def_arg_list_rule def_arg_stop_rule;

def_arg_start_rule:
    OPEN_PARENTHESIS_TOKEN;

def_arg_stop_rule:
    CLOSE_PARENTHESIS_TOKEN {
		if (!context->save_parameters()) {
			YYERROR;
		}
	};

def_arg_list_rule:
	def_arg_rule separator_rule def_arg_list_rule
	| def_arg_rule
	| ;

def_arg_rule:
    SYMBOL_TOKEN {
		if (!context->add_parameter($1)) {
			YYERROR;
		}
	}
	| SYMBOL_TOKEN EQUAL_TOKEN expr_rule {
		if (!context->add_definition_signature()) {
			YYERROR;
		}
		if (!context->add_parameter($1)) {
			YYERROR;
		}
	}
	| modifier_rule SYMBOL_TOKEN {
		if (!context->add_parameter($2, context->retrieve_modifiers())) {
			YYERROR;
		}
	}
	| modifier_rule SYMBOL_TOKEN EQUAL_TOKEN expr_rule {
		if (!context->add_definition_signature()) {
			YYERROR;
		}
		if (!context->add_parameter($2, context->retrieve_modifiers())) {
			YYERROR;
		}
	}
	| TPL_DOT_TOKEN {
		if (!context->set_variadic()) {
			YYERROR;
		}
	};

def_arrow_stmt_rule:
    def_arrow_stmt_start_rule expr_rule;

def_arrow_stmt_start_rule:
    EQUAL_RIGHT_ANGLED_TOKEN {
	    context->prepare_return();
	};

member_ident_rule:
    expr_rule DOT_TOKEN SYMBOL_TOKEN {
		context->push_node(Node::LOAD_MEMBER);
		context->push_node($3.c_str());
	}
	| expr_rule DOT_TOKEN operator_desc_rule {
		context->push_node(Node::LOAD_OPERATOR);
		context->push_node(context->retrieve_operator());
	}
	| expr_rule DOT_TOKEN var_symbol_rule {
		context->push_node(Node::LOAD_VAR_MEMBER);
	};

defined_symbol_rule:
    SYMBOL_TOKEN {
		context->push_node(Node::FIND_DEFINED_SYMBOL);
		context->push_node($1.c_str());
	}
	| defined_symbol_rule DOT_TOKEN SYMBOL_TOKEN {
		context->push_node(Node::FIND_DEFINED_MEMBER);
		context->push_node($3.c_str());
	}
	| var_symbol_rule {
		context->push_node(Node::FIND_DEFINED_VAR_SYMBOL);
		context->push_node($1.c_str());
	}
	| defined_symbol_rule DOT_TOKEN var_symbol_rule {
		context->push_node(Node::FIND_DEFINED_VAR_MEMBER);
		context->push_node($3.c_str());
	}
	| constant_rule {
		context->push_node(Node::LOAD_CONSTANT);
		if (Data *data = Compiler::make_data($1, Compiler::DATA_UNKNOWN_HINT)) {
			context->push_node(data);
		}
		else {
			error("token '" + $1 + "' is not a valid constant");
			YYERROR;
		}
	};

ident_rule:
	constant_rule {
		context->push_node(Node::LOAD_CONSTANT);
		if (Data *data = Compiler::make_data($1, Compiler::DATA_UNKNOWN_HINT)) {
			context->push_node(data);
		}
		else {
			error("token '" + $1 + "' is not a valid constant");
			YYERROR;
		}
	}
	| LIB_TOKEN {
		context->push_node(Node::CREATE_LIB);
	}
	| var_symbol_rule {
		context->push_node(Node::LOAD_VAR_SYMBOL);
	}
	| SYMBOL_TOKEN {
		const int index = context->fast_symbol_index($1);
		if (index != -1) {
			context->push_node(Node::LOAD_FAST);
			context->push_node($1.c_str());
			context->push_node(index);
		}
		else {
			context->push_node(Node::LOAD_SYMBOL);
			context->push_node($1.c_str());
		}
	}
	| LET_TOKEN SYMBOL_TOKEN {
		const int index = context->create_fast_scoped_symbol_index($2);
		if (index != -1) {
			context->push_node(Node::CREATE_FAST);
			context->push_node($2.c_str());
			context->push_node(index);
			context->push_node(Reference::DEFAULT);
		}
		else {
			context->push_node(Node::CREATE_SYMBOL);
			context->push_node($2.c_str());
			context->push_node(Reference::DEFAULT);
		}
	}
	| modifier_rule SYMBOL_TOKEN {
		const int index = context->create_fast_symbol_index($2);
		if (index != -1) {
			context->push_node(Node::CREATE_FAST);
			context->push_node($2.c_str());
			context->push_node(index);
			context->push_node(context->retrieve_modifiers());
		}
		else {
			context->push_node(Node::CREATE_SYMBOL);
			context->push_node($2.c_str());
			context->push_node(context->retrieve_modifiers());
		}
	}
	| LET_TOKEN modifier_rule SYMBOL_TOKEN {
		const int index = context->create_fast_scoped_symbol_index($3);
		if (index != -1) {
			context->push_node(Node::CREATE_FAST);
			context->push_node($3.c_str());
			context->push_node(index);
			context->push_node(context->retrieve_modifiers());
		}
		else {
			context->push_node(Node::CREATE_SYMBOL);
			context->push_node($3.c_str());
			context->push_node(context->retrieve_modifiers());
		}
	};

constant_rule:
    CONSTANT_TOKEN {
		$$ = $1;
	}
	| regex_rule {
		$$ = $1;
	}
	| regex_rule SYMBOL_TOKEN {
		$$ = $1 + $2;
	}
	| STRING_TOKEN {
		$$ = $1;
	}
	| NUMBER_TOKEN {
		$$ = $1;
	};

regex_rule:
    SLASH_TOKEN {
		$$ = $1 + context->lexer.read_regex();
	} SLASH_TOKEN {
		$$ = $2 + $1;
	};

var_symbol_rule:
    DOLLAR_TOKEN OPEN_BRACE_TOKEN expr_rule CLOSE_BRACE_TOKEN;

modifier_rule:
    VAR_TOKEN {
		context->start_modifiers(Reference::DEFAULT);
	}
	| DOLLAR_TOKEN {
		context->start_modifiers(Reference::CONST_ADDRESS);
	}
	| PERCENT_TOKEN {
		context->start_modifiers(Reference::CONST_VALUE);
	}
	| CONST_TOKEN {
		context->start_modifiers(Reference::CONST_ADDRESS | Reference::CONST_VALUE);
	}
	| AT_TOKEN {
		context->start_modifiers(Reference::GLOBAL);
	}
	| modifier_rule VAR_TOKEN {
		context->add_modifiers(Reference::DEFAULT);
	}
	| modifier_rule DOLLAR_TOKEN {
		context->add_modifiers(Reference::CONST_ADDRESS);
	}
	| modifier_rule PERCENT_TOKEN {
		context->add_modifiers(Reference::CONST_VALUE);
	}
	| modifier_rule CONST_TOKEN {
		context->add_modifiers(Reference::CONST_ADDRESS | Reference::CONST_VALUE);
	}
	| modifier_rule AT_TOKEN {
		context->add_modifiers(Reference::GLOBAL);
	};

separator_rule:
    COMMA_TOKEN | separator_rule LINE_END_TOKEN {
		context->commit_line();
	};

empty_lines_rule:
    LINE_END_TOKEN {
		context->commit_line();
	}
	| empty_lines_rule LINE_END_TOKEN {
		context->commit_line();
	};

%%

void parser::error(const std::string &msg) {
	context->parse_error(msg.c_str());
}

int BuildContext::next_token(std::string *token) {

	if (lexer.at_end()) {
	    return parser::token::FILE_END_TOKEN;
	}

	*token = lexer.next_token();
	return lexer.token_type(*token);
}

bool Compiler::build(DataStream *stream, const Module::Info &node) {

	std::unique_ptr<BuildContext> context(new BuildContext(stream, node));
	parser parser(context.get());

	if (is_printing()) {
		context->force_printer();
	}

	return !parser.parse();
}

#endif // PARSER_HPP
