%{
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

#ifndef MINT_COMPILER_PARSER_HPP
#define MINT_COMPILER_PARSER_HPP

#include "mint/compiler/buildtool.h"
#include "mint/compiler/compiler.h"
#include <memory>

#define YYSTYPE std::string
#define yylex context.next_token

using namespace mint;

%}

%define api.namespace {mint}
%parse-param {mint::BuildContext& context}

%token assert_token
%token async_token
%token break_token
%token case_token
%token catch_token
%token class_token
%token const_token
%token continue_token
%token def_token
%token default_token
%token elif_token
%token else_token
%token enum_token
%token exit_token
%token final_token
%token for_token
%token if_token
%token in_token
%token let_token
%token lib_token
%token load_token
%token override_token
%token package_token
%token print_token
%token raise_token
%token return_token
%token switch_token
%token try_token
%token while_token
%token yield_token
%token var_token
%token constant_token
%token string_token
%token number_token
%token symbol_token

%token no_line_end_token
%token line_end_token
%token file_end_token
%token comment_token
%token dollar_token
%token at_token
%token sharp_token
%token back_slash_token

%left comma_token
%left dbl_pipe_token
%left dbl_amp_token
%left pipe_token
%left caret_token
%left amp_token
%right equal_token question_token colon_token colon_equal_token equal_colon_token close_bracket_equal_token plus_equal_token minus_equal_token asterisk_equal_token slash_equal_token percent_equal_token dbl_left_angled_equal_token dbl_right_angled_equal_token amp_equal_token pipe_equal_token caret_equal_token equal_right_angled_token
%left dbl_dot_token tpl_dot_token
%left dbl_equal_token exclamation_equal_token is_token equal_tilde_token exclamation_tilde_token tpl_equal_token exclamation_dbl_equal_token
%left left_angled_token right_angled_token left_angled_equal_token right_angled_equal_token
%left dbl_left_angled_token dbl_right_angled_token
%left plus_token minus_token
%left asterisk_token slash_token percent_token
%right prefix_dbl_plus_token prefix_dbl_minus_token prefix_plus_token prefix_minus_token exclamation_token tilde_token await_token typeof_token membersof_token defined_token
%left dbl_plus_token dbl_minus_token dbl_asterisk_token
%left dot_token question_dot_token open_parenthesis_token close_parenthesis_token open_bracket_token close_bracket_token open_brace_token close_brace_token

%%

module_rule:
    stmt_list_rule file_end_token {
	    context.push_node(Node::Command::exit_module);
		fflush(stdout);
		YYACCEPT;
	}
	| file_end_token {
	    context.push_node(Node::Command::exit_module);
		fflush(stdout);
		YYACCEPT;
	};

stmt_list_rule:
	stmt_list_rule stmt_rule
	| stmt_rule;

stmt_rule:
    load_token module_path_rule line_end_token {
	    context.push_node(Node::Command::load_module);
		context.push_node($2.c_str());
		context.commit_line();
	}
	| try_rule stmt_bloc_rule {
		context.reset_scoped_symbols();
		context.unregister_retrieve_point();
		context.push_node(Node::Command::unset_retrieve_point);
		context.push_node(Node::Command::jump);
		context.start_jump_forward();
		context.shift_jump_forward();
		context.resolve_jump_forward();
		context.push_node(Node::Command::unload_reference);
		context.resolve_jump_forward();
		context.close_block();
	}
	| try_bloc_rule catch_rule stmt_bloc_rule {
		context.reset_scoped_symbols();
		context.reset_exception();
		context.resolve_jump_forward();
		context.close_block();
	}
	| if_cond_rule stmt_bloc_rule {
		context.reset_scoped_symbols();
		context.resolve_jump_forward();
		context.close_block();
	}
	| if_bloc_rule else_rule stmt_bloc_rule {
		context.reset_scoped_symbols();
		context.resolve_jump_forward();
		context.close_block();
	}
	| if_bloc_rule elif_bloc_rule {
		context.resolve_jump_forward();
		context.close_block();
	}
	| if_bloc_rule elif_bloc_rule else_rule stmt_bloc_rule {
		context.reset_scoped_symbols();
		context.resolve_jump_forward();
		context.close_block();
	}
	| switch_cond_rule open_brace_token case_list_rule close_brace_token {
		context.reset_scoped_symbols();
		context.push_node(Node::Command::jump);
		context.start_jump_forward();
		context.build_case_table();
		context.resolve_jump_forward();
		context.resolve_jump_forward();
		context.close_block();
	}
	| while_cond_rule stmt_bloc_rule {
		context.reset_scoped_symbols();
		context.push_node(Node::Command::jump);
		context.resolve_jump_backward();
		context.resolve_jump_forward();
		context.close_block();
	}
	| for_cond_rule stmt_bloc_rule {
		context.reset_scoped_symbols();
		context.push_node(Node::Command::jump);
		context.resolve_jump_backward();
		context.resolve_jump_forward();
		context.close_block();
	}
	| break_token line_end_token {
		if (!context.is_in_loop() && !context.is_in_switch()) {
			context.parse_error("break statement not within loop or switch");
			YYERROR;
		}
		context.prepare_break();
		context.push_node(Node::Command::jump);
		context.bloc_jump_forward();
		context.commit_line();
	}
	| continue_token line_end_token {
		if (!context.is_in_loop()) {
			context.parse_error("continue statement not within loop");
			YYERROR;
		}
		context.prepare_continue();
		context.push_node(Node::Command::jump);
		context.bloc_jump_backward();
		context.commit_line();
	}
	| print_token open_parenthesis_token expr_rule print_stmt_sep_rule expr_rule close_parenthesis_token line_end_token {
		context.commit_expr_result();
		context.close_printer();
		context.commit_line();
	}
	| print_token open_parenthesis_token expr_rule close_parenthesis_token line_end_token {
		context.push_node(Node::Command::load_constant);
		context.push_node(Compiler::make_number(1.));
		context.open_printer();
		context.commit_expr_result();
		context.close_printer();
		context.commit_line();
	}
	| print_token print_bloc_target_rule stmt_bloc_rule {
		context.reset_scoped_symbols();
		context.close_block();
		context.close_printer();
	}
	| yield_token expr_rule line_end_token {
		if (context.is_in_generator_expression()) {
			context.push_node(Node::Command::yield);
			context.commit_line();
		}
		else if (context.is_in_function()) {
			context.set_generator();
			context.push_node(Node::Command::yield);
			context.commit_line();
		}
		else {
			context.parse_error("unexpected 'yield' statement outside of function");
			YYERROR;
		}
	}
	| return_rule expr_rule line_end_token {
		context.set_exit_point();
		if (context.is_in_generator()) {
			if (context.is_in_async_function()) {
				context.push_node(Node::Command::yield);
				context.push_node(Node::Command::exit_coroutine);
			}
			else {
				context.push_node(Node::Command::yield_exit_generator);
			}
		}
		else {
			if (context.is_in_async_function()) {
				context.push_node(Node::Command::resume_coroutine);
			}
			else {
				context.push_node(Node::Command::exit_call);
			}
		}
		context.commit_line();
	}
	| raise_token expr_rule line_end_token {
	    context.reset_scoped_symbols_until(BuildContext::BlockType::try_type);
		context.push_node(Node::Command::raise);
		context.commit_line();
	}
	| exit_token expr_rule line_end_token {
		context.push_node(Node::Command::exit_exec);
		context.commit_line();
	}
	| exit_token line_end_token {
		context.push_node(Node::Command::load_constant);
		context.push_node(Compiler::make_number(0.));
		context.push_node(Node::Command::exit_exec);
		context.commit_line();
	}
	| ident_iterator_item_rule ident_iterator_end_rule equal_token expr_rule line_end_token {
		context.push_node(Node::Command::copy_operator);
		context.commit_expr_result();
		context.commit_line();
	}
	| ident_iterator_item_rule ident_iterator_end_rule equal_token generator_expr_rule line_end_token {
		context.push_node(Node::Command::copy_operator);
		context.commit_expr_result();
		context.commit_line();
	}
	| create_ident_iterator_rule equal_token expr_rule line_end_token {
		context.push_node(Node::Command::copy_operator);
		context.commit_expr_result();
		context.commit_line();
	}
	| create_ident_iterator_rule equal_token generator_expr_rule line_end_token {
		context.push_node(Node::Command::copy_operator);
		context.commit_expr_result();
		context.commit_line();
	}
	| expr_rule line_end_token {
		context.commit_expr_result();
		context.commit_line();
	}
	| modifier_rule def_start_rule def_capture_rule symbol_token def_args_rule stmt_bloc_rule {
		if (context.is_in_generator()) {
			if (!context.is_in_async_function()) {
				context.push_node(Node::Command::exit_generator);
			}
			else if (!context.has_returned()) {
				context.push_node(Node::Command::exit_coroutine);
			}
		}
		else if (!context.has_returned()) {
			if (context.is_in_async_function()) {
				context.push_node(Node::Command::load_constant);
				context.push_node(Compiler::make_none());
				context.push_node(Node::Command::resume_coroutine);
			}
			else {
				context.push_node(Node::Command::load_constant);
				context.push_node(Compiler::make_none());
				context.push_node(Node::Command::exit_call);
			}
		}
		const auto flags = context.is_in_nested_function()
							? Reference::const_address | context.retrieve_modifiers()
							: Reference::global | Reference::const_address | context.retrieve_modifiers();
		context.resolve_jump_forward();
		context.push_node(Node::Command::declare_function);
		context.push_node($4.c_str());
		context.push_node(flags);
		context.save_definition();
		context.push_node(Node::Command::function_overload);
		context.push_node(Node::Command::unload_reference);
	}
	| def_start_rule def_capture_rule symbol_token def_args_rule stmt_bloc_rule {
		if (context.is_in_generator()) {
			if (!context.is_in_async_function()) {
				context.push_node(Node::Command::exit_generator);
			}
			else if (!context.has_returned()) {
				context.push_node(Node::Command::exit_coroutine);
			}
		}
		else if (!context.has_returned()) {
			if (context.is_in_async_function()) {
				context.push_node(Node::Command::load_constant);
				context.push_node(Compiler::make_none());
				context.push_node(Node::Command::resume_coroutine);
			}
			else {
				context.push_node(Node::Command::load_constant);
				context.push_node(Compiler::make_none());
				context.push_node(Node::Command::exit_call);
			}
		}
		const auto flags = context.is_in_nested_function()
							? Reference::const_address
							: Reference::global | Reference::const_address;
		context.resolve_jump_forward();
		context.push_node(Node::Command::declare_function);
		context.push_node($3.c_str());
		context.push_node(flags);
		context.save_definition();
		context.push_node(Node::Command::function_overload);
		context.push_node(Node::Command::unload_reference);
	}
	| package_block_rule
	| class_desc_rule
	| enum_desc_rule
	| line_end_token {
		context.commit_line();
	};

module_name_rule:
    assert_token { $$ = $1; }
    | async_token { $$ = $1; }
    | await_token { $$ = $1; }
	| break_token { $$ = $1; }
	| case_token { $$ = $1; }
	| catch_token { $$ = $1; }
	| class_token { $$ = $1; }
	| const_token { $$ = $1; }
	| continue_token { $$ = $1; }
	| def_token { $$ = $1; }
	| default_token { $$ = $1; }
	| elif_token { $$ = $1; }
	| else_token { $$ = $1; }
	| enum_token { $$ = $1; }
	| exit_token { $$ = $1; }
	| for_token { $$ = $1; }
	| if_token { $$ = $1; }
	| in_token { $$ = $1; }
	| let_token { $$ = $1; }
	| lib_token { $$ = $1; }
	| load_token { $$ = $1; }
	| package_token { $$ = $1; }
	| print_token { $$ = $1; }
	| raise_token { $$ = $1; }
	| return_token { $$ = $1; }
	| switch_token { $$ = $1; }
	| try_token { $$ = $1; }
	| while_token { $$ = $1; }
	| yield_token { $$ = $1; }
	| var_token { $$ = $1; }
	| symbol_token { $$ = $1; }
	| module_name_rule minus_token module_name_rule {
		$$ = $1 + $2 + $3;
	};

module_path_rule:
	module_name_rule {
		$$ = $1;
	}
	| module_path_rule dot_token module_name_rule {
		$$ = $1 + $2 + $3;
	};

package_rule:
    package_token symbol_token {
		context.open_package($2);
	};

package_block_rule:
    package_rule open_brace_token stmt_list_rule close_brace_token {
		context.close_package();
	};

class_rule:
    class_token symbol_token {
		context.start_class_description($2);
	};

parent_rule:
    colon_token parent_list_rule
	| ;

parent_list_rule:
	parent_ident_rule {
		context.save_base_class_path();
	}
	| parent_list_rule comma_token parent_ident_rule {
		context.save_base_class_path();
	};

parent_ident_rule:
    symbol_token {
		context.append_symbol_to_base_class_path($1);
	}
	| parent_ident_rule dot_token symbol_token {
		context.append_symbol_to_base_class_path($3);
	};

class_desc_rule:
	class_rule parent_rule desc_bloc_rule {
		context.resolve_class_description();
	};

member_class_rule:
	class_rule
	| member_type_modifier_rule class_token symbol_token {
		context.start_class_description($3, context.retrieve_modifiers());
	};

member_class_desc_rule:
	member_class_rule parent_rule desc_bloc_rule {
		context.resolve_class_description();
	};

member_enum_rule:
	enum_rule
	| member_type_modifier_rule enum_token symbol_token {
		context.start_enum_description($3, context.retrieve_modifiers());
	};

member_enum_desc_rule:
	member_enum_rule enum_block_rule {
		context.resolve_enum_description();
	};

member_type_modifier_rule:
    plus_token {
		context.start_modifiers(Reference::default_flags);
	}
	| sharp_token {
		context.start_modifiers(Reference::protected_visibility);
	}
	| minus_token {
		context.start_modifiers(Reference::private_visibility);
	}
	| tilde_token {
		context.start_modifiers(Reference::package_visibility);
	};

desc_bloc_rule:
    open_brace_token desc_list_rule close_brace_token
	| open_brace_token close_brace_token;

desc_list_rule:
	desc_list_rule desc_rule
	| desc_rule;

desc_rule:
    member_desc_rule line_end_token {
		if (!context.create_member(context.retrieve_modifiers(), Symbol($1), Compiler::make_none())) {
			YYERROR;
		}
		context.commit_line();
	}
	| member_desc_rule equal_token constant_token line_end_token {
		if (!context.create_member(context.retrieve_modifiers(), Symbol($1), context.compiler().make_data($3, Compiler::DataHint::data_unknown_hint))) {
			YYERROR;
		}
		context.commit_line();
	}
	| member_desc_rule equal_token string_token line_end_token {
		if (!context.create_member(context.retrieve_modifiers(), Symbol($1), context.compiler().make_data($3, Compiler::DataHint::data_string_hint))) {
			YYERROR;
		}
		context.commit_line();
	}
	| member_desc_rule equal_token regex_rule line_end_token {
		if (!context.create_member(context.retrieve_modifiers(), Symbol($1), context.compiler().make_data($3, Compiler::DataHint::data_regex_hint))) {
			YYERROR;
		}
		context.commit_line();
	}
	| member_desc_rule equal_token regex_rule regex_rule symbol_token line_end_token {
		if (!context.create_member(context.retrieve_modifiers(), Symbol($1), context.compiler().make_data($3 + $4, Compiler::DataHint::data_regex_hint))) {
			YYERROR;
		}
		context.commit_line();
	}
	| member_desc_rule equal_token number_token line_end_token {
		if (!context.create_member(context.retrieve_modifiers(), Symbol($1), context.compiler().make_data($3, Compiler::DataHint::data_number_hint))) {
			YYERROR;
		}
		context.commit_line();
	}
	| member_desc_rule equal_token open_bracket_token close_bracket_token line_end_token {
		if (!context.create_member(context.retrieve_modifiers(), Symbol($1), context.compiler().make_array())) {
			YYERROR;
		}
		context.commit_line();
	}
	| member_desc_rule equal_token open_brace_token close_brace_token line_end_token {
		if (!context.create_member(context.retrieve_modifiers(), Symbol($1), context.compiler().make_hash())) {
			YYERROR;
		}
		context.commit_line();
	}
	| member_desc_rule equal_token lib_token open_parenthesis_token string_token close_parenthesis_token line_end_token {
		if (!context.create_member(context.retrieve_modifiers(), Symbol($1), context.compiler().make_library($5))) {
			YYERROR;
		}
		context.commit_line();
	}
	| member_desc_rule equal_token def_start_rule def_args_rule stmt_bloc_rule {
		if (context.is_in_generator()) {
			if (!context.is_in_async_function()) {
				context.push_node(Node::Command::exit_generator);
			}
			else if (!context.has_returned()) {
				context.push_node(Node::Command::exit_coroutine);
			}
		}
		else if (!context.has_returned()) {
			if (context.is_in_async_function()) {
				context.push_node(Node::Command::load_constant);
				context.push_node(Compiler::make_none());
				context.push_node(Node::Command::resume_coroutine);
			}
			else {
				context.push_node(Node::Command::load_constant);
				context.push_node(Compiler::make_none());
				context.push_node(Node::Command::exit_call);
			}
		}
		context.resolve_jump_forward();

		if (!context.create_member(context.retrieve_modifiers(), Symbol($1), context.retrieve_definition())) {
			YYERROR;
		}
	}
	| member_desc_rule plus_equal_token def_start_rule def_args_rule stmt_bloc_rule {
		if (context.is_in_generator()) {
			if (!context.is_in_async_function()) {
				context.push_node(Node::Command::exit_generator);
			}
			else if (!context.has_returned()) {
				context.push_node(Node::Command::exit_coroutine);
			}
		}
		else if (!context.has_returned()) {
			if (context.is_in_async_function()) {
				context.push_node(Node::Command::load_constant);
				context.push_node(Compiler::make_none());
				context.push_node(Node::Command::resume_coroutine);
			}
			else {
				context.push_node(Node::Command::load_constant);
				context.push_node(Compiler::make_none());
				context.push_node(Node::Command::exit_call);
			}
		}
		context.resolve_jump_forward();

		if (!context.update_member(context.retrieve_modifiers(), Symbol($1), context.retrieve_definition())) {
			YYERROR;
		}
	}
	| def_start_rule symbol_token def_args_rule stmt_bloc_rule {
		if (context.is_in_generator()) {
			if (!context.is_in_async_function()) {
				context.push_node(Node::Command::exit_generator);
			}
			else if (!context.has_returned()) {
				context.push_node(Node::Command::exit_coroutine);
			}
		}
		else if (!context.has_returned()) {
			if (context.is_in_async_function()) {
				context.push_node(Node::Command::load_constant);
				context.push_node(Compiler::make_none());
				context.push_node(Node::Command::resume_coroutine);
			}
			else {
				context.push_node(Node::Command::load_constant);
				context.push_node(Compiler::make_none());
				context.push_node(Node::Command::exit_call);
			}
		}
		context.resolve_jump_forward();

		if (!context.update_member(Reference::default_flags, Symbol($2), context.retrieve_definition())) {
			YYERROR;
		}
	}
	| def_start_rule await_token def_args_rule stmt_bloc_rule {
		if (context.is_in_generator()) {
			if (!context.is_in_async_function()) {
				context.push_node(Node::Command::exit_generator);
			}
			else if (!context.has_returned()) {
				context.push_node(Node::Command::exit_coroutine);
			}
		}
		else if (!context.has_returned()) {
			if (context.is_in_async_function()) {
				context.push_node(Node::Command::load_constant);
				context.push_node(Compiler::make_none());
				context.push_node(Node::Command::resume_coroutine);
			}
			else {
				context.push_node(Node::Command::load_constant);
				context.push_node(Compiler::make_none());
				context.push_node(Node::Command::exit_call);
			}
		}
		context.resolve_jump_forward();

		if (!context.update_member(Reference::default_flags, Symbol($2), context.retrieve_definition())) {
			YYERROR;
		}
	}
	| def_start_rule operator_desc_rule def_args_rule stmt_bloc_rule {
		if (context.is_in_generator()) {
			if (!context.is_in_async_function()) {
				context.push_node(Node::Command::exit_generator);
			}
			else if (!context.has_returned()) {
				context.push_node(Node::Command::exit_coroutine);
			}
		}
		else if (!context.has_returned()) {
			if (context.is_in_async_function()) {
				context.push_node(Node::Command::load_constant);
				context.push_node(Compiler::make_none());
				context.push_node(Node::Command::resume_coroutine);
			}
			else {
				context.push_node(Node::Command::load_constant);
				context.push_node(Compiler::make_none());
				context.push_node(Node::Command::exit_call);
			}
		}
		context.resolve_jump_forward();

		if (!context.update_member(Reference::default_flags, context.retrieve_operator_symbol(), context.retrieve_definition())) {
			YYERROR;
		}
	}
	| desc_modifier_rule def_start_rule symbol_token def_args_rule stmt_bloc_rule {
		if (context.is_in_generator()) {
			if (!context.is_in_async_function()) {
				context.push_node(Node::Command::exit_generator);
			}
			else if (!context.has_returned()) {
				context.push_node(Node::Command::exit_coroutine);
			}
		}
		else if (!context.has_returned()) {
			if (context.is_in_async_function()) {
				context.push_node(Node::Command::load_constant);
				context.push_node(Compiler::make_none());
				context.push_node(Node::Command::resume_coroutine);
			}
			else {
				context.push_node(Node::Command::load_constant);
				context.push_node(Compiler::make_none());
				context.push_node(Node::Command::exit_call);
			}
		}
		context.resolve_jump_forward();

		if (!context.update_member(context.retrieve_modifiers(), Symbol($3), context.retrieve_definition())) {
			YYERROR;
		}
	}
	| desc_modifier_rule def_start_rule await_token def_args_rule stmt_bloc_rule {
		if (context.is_in_generator()) {
			if (!context.is_in_async_function()) {
				context.push_node(Node::Command::exit_generator);
			}
			else if (!context.has_returned()) {
				context.push_node(Node::Command::exit_coroutine);
			}
		}
		else if (!context.has_returned()) {
			if (context.is_in_async_function()) {
				context.push_node(Node::Command::load_constant);
				context.push_node(Compiler::make_none());
				context.push_node(Node::Command::resume_coroutine);
			}
			else {
				context.push_node(Node::Command::load_constant);
				context.push_node(Compiler::make_none());
				context.push_node(Node::Command::exit_call);
			}
		}
		context.resolve_jump_forward();

		if (!context.update_member(context.retrieve_modifiers(), Symbol($3), context.retrieve_definition())) {
			YYERROR;
		}
	}
	| desc_modifier_rule def_start_rule operator_desc_rule def_args_rule stmt_bloc_rule {
		if (context.is_in_generator()) {
			if (!context.is_in_async_function()) {
				context.push_node(Node::Command::exit_generator);
			}
			else if (!context.has_returned()) {
				context.push_node(Node::Command::exit_coroutine);
			}
		}
		else if (!context.has_returned()) {
			if (context.is_in_async_function()) {
				context.push_node(Node::Command::load_constant);
				context.push_node(Compiler::make_none());
				context.push_node(Node::Command::resume_coroutine);
			}
			else {
				context.push_node(Node::Command::load_constant);
				context.push_node(Compiler::make_none());
				context.push_node(Node::Command::exit_call);
			}
		}
		context.resolve_jump_forward();

		if (!context.update_member(context.retrieve_modifiers(), context.retrieve_operator_symbol(), context.retrieve_definition())) {
			YYERROR;
		}
	}
	| member_class_desc_rule
	| member_enum_desc_rule
	| line_end_token {
		context.commit_line();
	};

member_desc_rule:
    symbol_token {
		context.start_modifiers(Reference::default_flags);
		$$ = $1;
	}
	| desc_modifier_rule symbol_token {
		$$ = $2;
	};

desc_base_modifier_rule:
	modifier_rule
	| final_token {
		context.start_modifiers(Reference::final_member);
	}
	| override_token {
		context.start_modifiers(Reference::override_member);
	}
	| final_token modifier_rule {
		context.add_modifiers(Reference::final_member);
	}
	| override_token modifier_rule {
		context.add_modifiers(Reference::override_member);
	};

desc_modifier_rule:
	desc_base_modifier_rule
	| plus_token {
		context.start_modifiers(Reference::default_flags);
	}
	| sharp_token {
		context.start_modifiers(Reference::protected_visibility);
	}
	| minus_token {
		context.start_modifiers(Reference::private_visibility);
	}
	| tilde_token {
		context.start_modifiers(Reference::package_visibility);
	}
	| plus_token desc_base_modifier_rule {
		context.add_modifiers(Reference::default_flags);
	}
	| sharp_token desc_base_modifier_rule {
		context.add_modifiers(Reference::protected_visibility);
	}
	| minus_token desc_base_modifier_rule {
		context.add_modifiers(Reference::private_visibility);
	}
	| tilde_token desc_base_modifier_rule {
		context.add_modifiers(Reference::package_visibility);
	};

operator_desc_rule:
    in_token {
		context.start_operator(Class::in_operator);
	}
	| colon_equal_token {
		context.start_operator(Class::copy_operator);
	}
	| dbl_pipe_token {
		context.start_operator(Class::or_operator);
	}
	| dbl_amp_token {
		context.start_operator(Class::and_operator);
	}
	| pipe_token {
		context.start_operator(Class::bor_operator);
	}
	| caret_token {
		context.start_operator(Class::xor_operator);
	}
	| amp_token {
		context.start_operator(Class::band_operator);
	}
	| dbl_equal_token {
		context.start_operator(Class::eq_operator);
	}
	| exclamation_equal_token {
		context.start_operator(Class::ne_operator);
	}
	| left_angled_token {
		context.start_operator(Class::lt_operator);
	}
	| right_angled_token {
		context.start_operator(Class::gt_operator);
	}
	| left_angled_equal_token {
		context.start_operator(Class::le_operator);
	}
	| right_angled_equal_token {
		context.start_operator(Class::ge_operator);
	}
	| dbl_left_angled_token {
		context.start_operator(Class::shift_left_operator);
	}
	| dbl_right_angled_token {
		context.start_operator(Class::shift_right_operator);
	}
	| plus_token {
		context.start_operator(Class::add_operator);
	}
	| minus_token {
		context.start_operator(Class::sub_operator);
	}
	| asterisk_token {
		context.start_operator(Class::mul_operator);
	}
	| slash_token {
		context.start_operator(Class::div_operator);
	}
	| percent_token {
		context.start_operator(Class::mod_operator);
	}
	| exclamation_token {
		context.start_operator(Class::not_operator);
	}
	| tilde_token {
		context.start_operator(Class::compl_operator);
	}
	| dbl_plus_token {
		context.start_operator(Class::inc_operator);
	}
	| dbl_minus_token {
		context.start_operator(Class::dec_operator);
	}
	| dbl_asterisk_token {
		context.start_operator(Class::pow_operator);
	}
	| dbl_dot_token {
		context.start_operator(Class::inclusive_range_operator);
	}
	| tpl_dot_token {
		context.start_operator(Class::exclusive_range_operator);
	}
	| open_parenthesis_token close_parenthesis_token {
		context.start_operator(Class::call_operator);
	}
	| open_bracket_token close_bracket_token {
		context.start_operator(Class::subscript_operator);
	}
	| open_bracket_token close_bracket_equal_token {
		context.start_operator(Class::subscript_move_operator);
	};

enum_rule:
    enum_token symbol_token {
		context.start_enum_description($2);
	};

enum_desc_rule:
	enum_rule enum_block_rule {
		context.resolve_enum_description();
	};

enum_block_rule:
    open_brace_token enum_list_rule close_brace_token;

enum_list_rule:
	enum_list_rule enum_item_rule
	| enum_item_rule;

enum_item_rule:
    symbol_token equal_token number_token {
		constexpr auto flags = Reference::const_value | Reference::const_address | Reference::global;
		if (!context.create_member(flags, Symbol($1), context.compiler().make_data($3, Compiler::DataHint::data_number_hint))) {
			YYERROR;
		}
		context.set_current_enum_value(atoi($3.c_str()));
	}
	| symbol_token {
		constexpr auto flags = Reference::const_value | Reference::const_address | Reference::global;
		if (!context.create_member(flags, Symbol($1), context.compiler().make_data(std::to_string(context.next_enum_value()), Compiler::DataHint::data_number_hint))) {
			YYERROR;
		}
	}
	| line_end_token {
		context.commit_line();
	};

generator_expr_rule:
	if_cond_generator_rule generator_stmt_bloc_rule {
		context.reset_scoped_symbols();
		context.resolve_jump_forward();
		context.close_block();
		context.close_generator_expression();
	}
	| open_parenthesis_token if_generator_bloc_rule else_rule generator_stmt_bloc_rule close_parenthesis_token {
		context.reset_scoped_symbols();
		context.resolve_jump_forward();
		context.close_block();
		context.close_generator_expression();
	}
	| open_parenthesis_token if_generator_bloc_rule elif_generator_bloc_rule close_parenthesis_token {
		context.resolve_jump_forward();
		context.close_block();
		context.close_generator_expression();
	}
	| open_parenthesis_token if_generator_bloc_rule elif_generator_bloc_rule else_rule generator_stmt_bloc_rule close_parenthesis_token {
		context.reset_scoped_symbols();
		context.resolve_jump_forward();
		context.close_block();
		context.close_generator_expression();
	}
	| switch_cond_generator_rule open_brace_token case_list_rule close_brace_token {
		context.reset_scoped_symbols();
		context.push_node(Node::Command::jump);
		context.start_jump_forward();
		context.build_case_table();
		context.resolve_jump_forward();
		context.resolve_jump_forward();
		context.close_block();
		context.close_generator_expression();
	}
	| while_cond_generator_rule generator_stmt_bloc_rule {
		context.reset_scoped_symbols();
		context.push_node(Node::Command::jump);
		context.resolve_jump_backward();
		context.resolve_jump_forward();
		context.close_block();
		context.close_generator_expression();
	}
	| for_cond_generator_rule generator_stmt_bloc_rule {
		context.reset_scoped_symbols();
		context.push_node(Node::Command::jump);
		context.resolve_jump_backward();
		context.resolve_jump_forward();
		context.close_block();
		context.close_generator_expression();
	};

try_rule:
    try_token {
		context.register_retrieve_point();
		context.push_node(Node::Command::set_retrieve_point);
		context.start_jump_forward();
		context.open_block(BuildContext::BlockType::try_type);
	};

catch_rule:
    catch_token symbol_token {
		context.close_block();
		context.unregister_retrieve_point();
		context.push_node(Node::Command::unset_retrieve_point);
		context.push_node(Node::Command::jump);
		context.start_jump_forward();
		context.shift_jump_forward();
		context.resolve_jump_forward();
		context.open_block(BuildContext::BlockType::catch_type);
		context.push_node(Node::Command::init_exception);
		context.push_node($2.c_str());
		context.set_exception_symbol($2);
	};

try_bloc_rule:
	try_rule stmt_bloc_rule {
		context.reset_scoped_symbols();
	};

if_bloc_rule:
	if_cond_rule stmt_bloc_rule {
		context.reset_scoped_symbols();
	};

if_generator_bloc_rule:
	if_cond_generator_rule generator_stmt_bloc_rule {
		context.reset_scoped_symbols();
	};

elif_bloc_rule:
	elif_cond_rule stmt_bloc_rule {
		context.reset_scoped_symbols();
		context.shift_jump_forward();
		context.resolve_jump_forward();
	}
	| elif_bloc_rule elif_cond_rule stmt_bloc_rule {
		context.reset_scoped_symbols();
		context.shift_jump_forward();
		context.resolve_jump_forward();
	};

elif_generator_bloc_rule:
	elif_cond_rule generator_stmt_bloc_rule {
		context.reset_scoped_symbols();
		context.shift_jump_forward();
		context.resolve_jump_forward();
	}
	| elif_generator_bloc_rule elif_cond_rule generator_stmt_bloc_rule {
		context.reset_scoped_symbols();
		context.shift_jump_forward();
		context.resolve_jump_forward();
	};

stmt_bloc_rule:
    open_brace_token stmt_list_rule close_brace_token
	| open_brace_token yield_token expr_rule close_brace_token {
		if (context.is_in_generator_expression()) {
			context.push_node(Node::Command::yield);
		}
		else if (context.is_in_function()) {
			context.set_generator();
			context.push_node(Node::Command::yield);
		}
		else {	
			context.parse_error("unexpected 'yield' statement outside of function");
			YYERROR;
		}
	}
	| open_brace_token return_rule expr_rule close_brace_token {
		context.set_exit_point();
		if (context.is_in_generator()) {
			if (context.is_in_async_function()) {
				context.push_node(Node::Command::yield);
				context.push_node(Node::Command::exit_coroutine);
			}
			else {
				context.push_node(Node::Command::yield_exit_generator);
			}
		}
		else {
			if (context.is_in_async_function()) {
				context.push_node(Node::Command::resume_coroutine);
			}
			else {
				context.push_node(Node::Command::exit_call);
			}
		}
	}
	| open_brace_token raise_token expr_rule close_brace_token {
		context.reset_scoped_symbols_until(BuildContext::BlockType::try_type);
		context.push_node(Node::Command::raise);
	}
	| open_brace_token expr_rule close_brace_token {
		context.commit_expr_result();
	}
	| open_brace_token close_brace_token;

generator_stmt_bloc_rule:
	equal_right_angled_token expr_rule {
		context.push_node(Node::Command::yield);
	}
	| equal_right_angled_token generator_expr_rule
	| stmt_bloc_rule;

if_cond_rule:
	if_rule expr_rule {
		context.resolve_condition();
		context.push_node(Node::Command::zero_jump);
		context.start_jump_forward();
		context.open_block(BuildContext::BlockType::if_type);
	}
	| if_rule find_rule {
		context.resolve_condition();
		context.push_node(Node::Command::zero_jump);
		context.start_jump_forward();
		context.open_block(BuildContext::BlockType::if_type);
	};

if_cond_generator_rule:
	if_generator_rule expr_rule {
		context.resolve_condition();
		context.push_node(Node::Command::zero_jump);
		context.start_jump_forward();
		context.open_block(BuildContext::BlockType::if_type);
	}
	| if_generator_rule find_rule {
		context.resolve_condition();
		context.push_node(Node::Command::zero_jump);
		context.start_jump_forward();
		context.open_block(BuildContext::BlockType::if_type);
	};

elif_cond_rule:
	elif_rule expr_rule {
		context.resolve_condition();
		context.close_block();
		context.push_node(Node::Command::zero_jump);
		context.start_jump_forward();
		context.open_block(BuildContext::BlockType::elif_type);
	}
	| elif_rule find_rule {
		context.resolve_condition();
		context.close_block();
		context.push_node(Node::Command::zero_jump);
		context.start_jump_forward();
		context.open_block(BuildContext::BlockType::elif_type);
	};

if_rule:
    if_token {
		context.start_condition();
	};

if_generator_rule:
    if_token {
		context.open_generator_expression();
		context.start_condition();
	};

elif_rule:
    elif_token {
		context.push_node(Node::Command::jump);
		context.start_jump_forward();
		context.shift_jump_forward();
		context.resolve_jump_forward();
		context.start_condition();
	};

else_rule:
    else_token {
		context.push_node(Node::Command::jump);
		context.start_jump_forward();

		context.close_block();
		context.shift_jump_forward();
		context.resolve_jump_forward();
		context.open_block(BuildContext::BlockType::else_type);
	};

switch_cond_rule:
	switch_rule expr_rule {
		context.resolve_condition();
		context.open_block(BuildContext::BlockType::switch_type);
	};

switch_cond_generator_rule:
	switch_expr_rule expr_rule {
		context.resolve_condition();
		context.open_block(BuildContext::BlockType::switch_type);
	};

switch_expr_rule:
    switch_token {
		context.open_generator_expression();
		context.start_condition();
	};

switch_rule:
    switch_token {
		context.start_condition();
	};

case_rule:
    case_token {
		context.start_case_label();
	};

case_symbol_rule:
    symbol_token {
		context.push_node(Node::Command::load_symbol);
		context.push_node($1.c_str());
		$$ = $1;
	}
	| case_symbol_rule dot_token symbol_token {
		context.push_node(Node::Command::load_member);
		context.push_node($3.c_str());
		$$ = $1 + $2 + $3;
	};

case_constant_rule:
	constant_rule {
		if (Data *data = context.compiler().make_data($1, Compiler::DataHint::data_unknown_hint)) {
			context.push_node(Node::Command::load_constant);
			context.push_node(*data);
			$$ = $1;
		}
		else {
			error("token '" + $1 + "' is not a valid constant");
			YYERROR;
		}
	}
	| plus_token number_token {
		if (Data *data = context.compiler().make_data($2, Compiler::DataHint::data_number_hint)) {
			context.push_node(Node::Command::load_constant);
			context.push_node(*data);
			context.push_node(Node::Command::pos_operator);
			$$ = $2;
		}
		else {
			error("token '" + $2 + "' is not a valid constant");
			YYERROR;
		}
	}
	| minus_token number_token {
		if (Data *data = context.compiler().make_data($2, Compiler::DataHint::data_number_hint)) {
			context.push_node(Node::Command::load_constant);
			context.push_node(*data);
			context.push_node(Node::Command::neg_operator);
			$$ = $1 + $2;
		}
		else {
			error("token '" + $2 + "' is not a valid constant");
			YYERROR;
		}
	};

case_constant_list_rule:
    case_constant_list_rule case_constant_rule comma_token {
		context.add_to_call();
		$$ = $1 + $2 + $3;
	}
	| case_constant_rule comma_token {
		context.push_node(Node::Command::alloc_iterator);
		context.start_call();
		context.add_to_call();
		$$ = $1 + $2;
	};

case_constant_list_end_rule:
	case_constant_rule {
		context.push_node(Node::Command::init_iterator);
		context.add_to_call();
		context.resolve_call();
		$$ = $1;
	}
	| {
		context.push_node(Node::Command::init_iterator);
		context.resolve_call();
	};

case_label_rule:
    case_rule in_token case_constant_rule dbl_dot_token case_constant_rule {
		context.push_node(Node::Command::inclusive_range_operator);
		context.start_jump_backward();
		context.push_node(Node::Command::find_next);
		context.push_node(Node::Command::find_check);
		context.start_jump_forward();
		context.push_node(Node::Command::jump);
		context.resolve_jump_backward();
		context.resolve_jump_forward();
		context.resolve_case_label($3 + $4 + $5);
	}
	| case_rule in_token case_constant_rule tpl_dot_token case_constant_rule {
		context.push_node(Node::Command::exclusive_range_operator);
		context.start_jump_backward();
		context.push_node(Node::Command::find_next);
		context.push_node(Node::Command::find_check);
		context.start_jump_forward();
		context.push_node(Node::Command::jump);
		context.resolve_jump_backward();
		context.resolve_jump_forward();
		context.resolve_case_label($3 + $4 + $5);
	}
	| case_rule in_token case_constant_list_rule case_constant_list_end_rule {
		context.start_jump_backward();
		context.push_node(Node::Command::find_next);
		context.push_node(Node::Command::find_check);
		context.start_jump_forward();
		context.push_node(Node::Command::jump);
		context.resolve_jump_backward();
		context.resolve_jump_forward();
		context.resolve_case_label($3 + $4);
	}
	| case_rule in_token case_constant_rule {
		context.push_node(Node::Command::find_operator);
		context.push_node(Node::Command::find_init);
		context.start_jump_backward();
		context.push_node(Node::Command::find_next);
		context.push_node(Node::Command::find_check);
		context.start_jump_forward();
		context.push_node(Node::Command::jump);
		context.resolve_jump_backward();
		context.resolve_jump_forward();
		context.resolve_case_label($3);
	}
	| case_rule in_token case_symbol_rule {
		context.push_node(Node::Command::find_operator);
		context.push_node(Node::Command::find_init);
		context.start_jump_backward();
		context.push_node(Node::Command::find_next);
		context.push_node(Node::Command::find_check);
		context.start_jump_forward();
		context.push_node(Node::Command::jump);
		context.resolve_jump_backward();
		context.resolve_jump_forward();
		context.resolve_case_label($3);
	}
	| case_rule is_token case_constant_rule {
		context.push_node(Node::Command::is_operator);
		context.resolve_case_label($3);
	}
	| case_rule is_token case_symbol_rule {
		context.push_node(Node::Command::is_operator);
		context.resolve_case_label($3);
	}
	| case_rule case_constant_rule {
		context.push_node(Node::Command::eq_operator);
		context.resolve_case_label($2);
	}
	| case_rule case_symbol_rule {
		context.push_node(Node::Command::eq_operator);
		context.resolve_case_label($2);
	};

default_rule:
    default_token {
		context.set_default_label();
	};

case_list_rule:
    line_end_token {
		context.commit_line();
	}
	| case_label_rule colon_token stmt_list_rule
	| case_list_rule case_label_rule colon_token stmt_list_rule
	| default_rule colon_token stmt_list_rule
	| case_list_rule default_rule colon_token stmt_list_rule
	| case_label_rule equal_right_angled_token expr_rule line_end_token {
		if (context.is_in_generator_expression()) {
			context.push_node(Node::Command::yield);
		}
		else {
			context.commit_expr_result();
		}
		context.prepare_break();
		context.push_node(Node::Command::jump);
		context.bloc_jump_forward();
		context.commit_line();
	}
	| case_list_rule case_label_rule equal_right_angled_token expr_rule line_end_token {
	    if (context.is_in_generator_expression()) {
			context.push_node(Node::Command::yield);
		}
		else {
			context.commit_expr_result();
		}
		context.prepare_break();
		context.push_node(Node::Command::jump);
		context.bloc_jump_forward();
		context.commit_line();
	}
	| default_rule equal_right_angled_token expr_rule line_end_token {
	    if (context.is_in_generator_expression()) {
			context.push_node(Node::Command::yield);
		}
		else {
			context.commit_expr_result();
		}
		context.prepare_break();
		context.push_node(Node::Command::jump);
		context.bloc_jump_forward();
		context.commit_line();
	}
	| case_list_rule default_rule equal_right_angled_token expr_rule line_end_token {
	    if (context.is_in_generator_expression()) {
			context.push_node(Node::Command::yield);
		}
		else {
			context.commit_expr_result();
		}
		context.prepare_break();
		context.push_node(Node::Command::jump);
		context.bloc_jump_forward();
		context.commit_line();
	};

while_cond_rule:
    while_rule expr_rule {
		context.resolve_condition();
		context.push_node(Node::Command::zero_jump);
		context.start_jump_forward();
		context.open_block(BuildContext::BlockType::conditional_loop_type);
	}
	| while_rule find_rule {
		context.resolve_condition();
		context.push_node(Node::Command::zero_jump);
		context.start_jump_forward();
		context.open_block(BuildContext::BlockType::conditional_loop_type);
	};

while_cond_generator_rule:
	while_expr_rule expr_rule {
		context.resolve_condition();
		context.push_node(Node::Command::zero_jump);
		context.start_jump_forward();
		context.open_block(BuildContext::BlockType::conditional_loop_type);
	}
	| while_expr_rule find_rule {
		context.resolve_condition();
		context.push_node(Node::Command::zero_jump);
		context.start_jump_forward();
		context.open_block(BuildContext::BlockType::conditional_loop_type);
	};

while_expr_rule:
    while_token {
		context.open_generator_expression();
		context.start_jump_backward();
		context.start_condition();
	};

while_rule:
    while_token {
		context.start_jump_backward();
		context.start_condition();
	};

find_rule:
    expr_rule in_token find_init_rule {
		context.start_jump_backward();
		context.push_node(Node::Command::find_next);
		context.push_node(Node::Command::find_check);
		context.start_jump_forward();
		context.push_node(Node::Command::jump);
		context.resolve_jump_backward();
		context.resolve_jump_forward();
	}
	| expr_rule exclamation_token in_token find_init_rule {
		context.start_jump_backward();
		context.push_node(Node::Command::find_next);
		context.push_node(Node::Command::find_check);
		context.start_jump_forward();
		context.push_node(Node::Command::jump);
		context.resolve_jump_backward();
		context.resolve_jump_forward();
		context.push_node(Node::Command::not_operator);
	};

find_init_rule:
	expr_rule {
		context.push_node(Node::Command::find_operator);
		context.push_node(Node::Command::find_init);
	};

for_cond_rule:
    for_rule open_parenthesis_token range_init_rule range_next_rule range_cond_rule close_parenthesis_token {
		context.resolve_condition();
		context.open_block(BuildContext::BlockType::custom_range_loop_type);
	}
	| for_iterator_in_rule expr_rule {
		context.push_node(Node::Command::in_operator);
		context.push_node(Node::Command::range_init);
		context.resolve_condition();
		context.push_node(Node::Command::jump);
		context.start_jump_forward();
		context.start_jump_backward();
		context.push_node(Node::Command::range_next);
		context.resolve_jump_forward();
		context.push_node(Node::Command::range_iterator_check);
		context.start_jump_forward();
		context.open_block(BuildContext::BlockType::range_loop_type);
	}
	| for_in_rule expr_rule {
		context.push_node(Node::Command::in_operator);
		context.push_node(Node::Command::range_init);
		context.resolve_condition();
		context.push_node(Node::Command::jump);
		context.start_jump_forward();
		context.start_jump_backward();
		context.push_node(Node::Command::range_next);
		context.resolve_jump_forward();
		context.push_node(Node::Command::range_check);
		context.start_jump_forward();
		context.open_block(BuildContext::BlockType::range_loop_type);
	};

for_cond_generator_rule:
    for_expr_rule open_parenthesis_token range_init_rule range_next_rule range_cond_rule close_parenthesis_token {
		context.resolve_condition();
		context.open_block(BuildContext::BlockType::custom_range_loop_type);
	}
	| for_iterator_in_expr_rule expr_rule {
		context.push_node(Node::Command::in_operator);
		context.push_node(Node::Command::range_init);
		context.resolve_condition();
		context.push_node(Node::Command::jump);
		context.start_jump_forward();
		context.start_jump_backward();
		context.push_node(Node::Command::range_next);
		context.resolve_jump_forward();
		context.push_node(Node::Command::range_iterator_check);
		context.start_jump_forward();
		context.open_block(BuildContext::BlockType::range_loop_type);
	}
	| for_in_expr_rule expr_rule {
		context.push_node(Node::Command::in_operator);
		context.push_node(Node::Command::range_init);
		context.resolve_condition();
		context.push_node(Node::Command::jump);
		context.start_jump_forward();
		context.start_jump_backward();
		context.push_node(Node::Command::range_next);
		context.resolve_jump_forward();
		context.push_node(Node::Command::range_check);
		context.start_jump_forward();
		context.open_block(BuildContext::BlockType::range_loop_type);
	};

for_expr_rule:
    for_token {
		context.open_generator_expression();
		context.start_range_loop();
	};

for_rule:
    for_token {
		context.start_range_loop();
	};

for_in_expr_rule:
    for_expr_rule ident_rule in_token {
		context.resolve_range_loop();
		context.start_condition();
	};

for_in_rule:
    for_rule ident_rule in_token {
		context.resolve_range_loop();
		context.start_condition();
	};

for_iterator_in_expr_rule:
    for_expr_rule ident_iterator_item_rule ident_iterator_end_rule in_token {
		context.resolve_range_loop();
		context.start_condition();
	}
	| for_expr_rule create_ident_iterator_rule in_token {
		context.resolve_range_loop();
		context.start_condition();
	};

for_iterator_in_rule:
    for_rule ident_iterator_item_rule ident_iterator_end_rule in_token {
		context.resolve_range_loop();
		context.start_condition();
	}
	| for_rule create_ident_iterator_rule in_token {
		context.resolve_range_loop();
		context.start_condition();
	};

range_init_rule:
    expr_rule comma_token {
		context.push_node(Node::Command::unload_reference);
		context.push_node(Node::Command::jump);
		context.start_jump_forward();
		context.start_jump_backward();
		context.resolve_range_loop();
		context.start_condition();
	};

range_next_rule:
    expr_rule comma_token {
		context.push_node(Node::Command::unload_reference);
		context.resolve_jump_forward();
	};

range_cond_rule:
	expr_rule {
		context.push_node(Node::Command::zero_jump);
		context.start_jump_forward();
	};

return_rule:
    return_token {
		if (!context.is_in_function()) {
			context.parse_error("unexpected 'return' statement outside of function");
			YYERROR;
		}
		context.prepare_return();
	};

start_hash_rule:
    open_brace_token {
		context.push_node(Node::Command::alloc_hash);
		context.start_call();
	};

stop_hash_rule:
    close_brace_token {
		context.push_node(Node::Command::init_hash);
		context.resolve_call();
	};

hash_item_rule:
    hash_item_rule separator_rule expr_rule colon_token expr_rule {
		context.add_to_call();
	}
	| expr_rule colon_token expr_rule {
		context.add_to_call();
	};

start_array_rule:
    open_bracket_token {
		context.push_node(Node::Command::alloc_array);
		context.start_call();
	};

stop_array_rule:
    close_bracket_token {
		context.push_node(Node::Command::init_array);
		context.resolve_call();
	};

array_item_list_rule:
	array_item_list_rule separator_rule array_item_rule
	| array_item_rule;

array_item_rule:
	expr_rule {
		context.add_to_call();
	}
	| asterisk_token expr_rule {
		context.push_node(Node::Command::in_operator);
		context.push_node(Node::Command::load_extra_arguments);
	}
	| tpl_dot_token expr_rule {
		context.push_node(Node::Command::in_operator);
		context.push_node(Node::Command::load_extra_arguments);
	}
	| generator_expr_rule {
		context.push_node(Node::Command::load_extra_arguments);
	};

iterator_item_rule:
	iterator_item_rule expr_rule separator_rule {
		context.add_to_call();
	}
	| expr_rule separator_rule {
		context.push_node(Node::Command::alloc_iterator);
		context.start_call();
		context.add_to_call();
	}
	| iterator_item_rule asterisk_token expr_rule separator_rule {
		context.push_node(Node::Command::in_operator);
		context.push_node(Node::Command::load_extra_arguments);
	}
	| iterator_item_rule tpl_dot_token expr_rule separator_rule {
		context.push_node(Node::Command::in_operator);
		context.push_node(Node::Command::load_extra_arguments);
	}
	| asterisk_token expr_rule separator_rule {
		context.push_node(Node::Command::alloc_iterator);
		context.start_call();
		context.push_node(Node::Command::in_operator);
		context.push_node(Node::Command::load_extra_arguments);
	}
	| tpl_dot_token expr_rule separator_rule {
		context.push_node(Node::Command::alloc_iterator);
		context.start_call();
		context.push_node(Node::Command::in_operator);
		context.push_node(Node::Command::load_extra_arguments);
	};

iterator_end_rule:
	expr_rule {
		context.push_node(Node::Command::init_iterator);
		context.add_to_call();
		context.resolve_call();
	}
	| {
		context.push_node(Node::Command::init_iterator);
		context.resolve_call();
	};

ident_iterator_item_rule:
	ident_iterator_item_rule ident_rule separator_rule {
		context.add_to_call();
	}
	| ident_rule separator_rule {
		context.push_node(Node::Command::alloc_iterator);
		context.start_call();
		context.add_to_call();
	};

ident_iterator_end_rule:
	ident_rule {
		context.push_node(Node::Command::init_iterator);
		context.add_to_call();
		context.resolve_call();
	}
	| {
		context.push_node(Node::Command::init_iterator);
		context.resolve_call();
	};

let_modifier_rule:
    let_token {
	    context.start_modifiers(Reference::default_flags);
	};

create_ident_iterator_rule:
    let_token modifier_rule create_ident_iterator_scoped_item_rule create_ident_iterator_scoped_end_rule
	| let_modifier_rule create_ident_iterator_scoped_item_rule create_ident_iterator_scoped_end_rule
	| modifier_rule create_ident_iterator_item_rule create_ident_iterator_end_rule;

create_ident_iterator_scoped_item_rule:
    create_ident_iterator_scoped_item_rule symbol_token comma_token {
		const auto index = context.create_fast_scoped_symbol_index($2);
		if (index != invalid_index) {
			context.push_node(Node::Command::declare_fast);
			context.push_node($2.c_str());
			context.push_node(index);
			context.push_node(context.get_modifiers());
		}
		else {
			context.push_node(Node::Command::declare_symbol);
			context.push_node($2.c_str());
			context.push_node(context.get_modifiers());
		}
		context.add_to_call();
	}
	| open_parenthesis_token symbol_token comma_token {
		context.push_node(Node::Command::alloc_iterator);
		context.start_call();
		const auto index = context.create_fast_scoped_symbol_index($2);
		if (index != invalid_index) {
			context.push_node(Node::Command::declare_fast);
			context.push_node($2.c_str());
			context.push_node(index);
			context.push_node(context.get_modifiers());
		}
		else {
			context.push_node(Node::Command::declare_symbol);
			context.push_node($2.c_str());
			context.push_node(context.get_modifiers());
		}
		context.add_to_call();
	};

create_ident_iterator_scoped_end_rule:
    symbol_token close_parenthesis_token {
		const auto index = context.create_fast_scoped_symbol_index($1);
		if (index != invalid_index) {
			context.push_node(Node::Command::declare_fast);
			context.push_node($1.c_str());
			context.push_node(index);
			context.push_node(context.retrieve_modifiers());
		}
		else {
			context.push_node(Node::Command::declare_symbol);
			context.push_node($1.c_str());
			context.push_node(context.retrieve_modifiers());
		}
		context.push_node(Node::Command::init_iterator);
		context.add_to_call();
		context.resolve_call();
	};

create_ident_iterator_item_rule:
    create_ident_iterator_item_rule symbol_token comma_token {
		const auto index = context.create_fast_symbol_index($2);
		if (index != invalid_index) {
			context.push_node(Node::Command::declare_fast);
			context.push_node($2.c_str());
			context.push_node(index);
			context.push_node(context.get_modifiers());
		}
		else {
			context.push_node(Node::Command::declare_symbol);
			context.push_node($2.c_str());
			context.push_node(context.get_modifiers());
		}
		context.add_to_call();
	}
	| open_parenthesis_token symbol_token comma_token {
		context.push_node(Node::Command::alloc_iterator);
		context.start_call();
		const auto index = context.create_fast_symbol_index($2);
		if (index != invalid_index) {
			context.push_node(Node::Command::declare_fast);
			context.push_node($2.c_str());
			context.push_node(index);
			context.push_node(context.get_modifiers());
		}
		else {
			context.push_node(Node::Command::declare_symbol);
			context.push_node($2.c_str());
			context.push_node(context.get_modifiers());
		}
		context.add_to_call();
	};

create_ident_iterator_end_rule:
    symbol_token close_parenthesis_token {
		const auto index = context.create_fast_symbol_index($1);
		if (index != invalid_index) {
			context.push_node(Node::Command::declare_fast);
			context.push_node($1.c_str());
			context.push_node(index);
			context.push_node(context.retrieve_modifiers());
		}
		else {
			context.push_node(Node::Command::declare_symbol);
			context.push_node($1.c_str());
			context.push_node(context.retrieve_modifiers());
		}
		context.push_node(Node::Command::init_iterator);
		context.add_to_call();
		context.resolve_call();
	};

print_stmt_sep_rule:
	comma_token {
		context.open_printer();
	};

print_bloc_target_rule:
	open_parenthesis_token expr_rule close_parenthesis_token {
		context.open_printer();
		context.open_block(BuildContext::BlockType::print_type);
	}
	| {
		context.push_node(Node::Command::load_constant);
		context.push_node(Compiler::make_number(1.));
		context.open_printer();
		context.open_block(BuildContext::BlockType::print_type);
	};

expr_rule:
    expr_rule equal_token generator_expr_rule {
		context.push_node(Node::Command::move_operator);
	}
	| expr_rule equal_token expr_rule {
		context.push_node(Node::Command::move_operator);
	}
	| expr_rule colon_equal_token generator_expr_rule {
		context.push_node(Node::Command::copy_operator);
	}
	| expr_rule colon_equal_token expr_rule {
		context.push_node(Node::Command::copy_operator);
	}
	| expr_rule equal_colon_token {
	    context.push_node(Node::Command::alloc_iterator);
		context.start_call();
		context.add_to_call();
		context.push_node(Node::Command::init_iterator);
		context.resolve_call();
	} generator_expr_rule {
	    context.push_node(Node::Command::copy_operator);
	}
	| expr_rule plus_token expr_rule {
		context.push_node(Node::Command::add_operator);
	}
	| expr_rule minus_token expr_rule {
		context.push_node(Node::Command::sub_operator);
	}
	| expr_rule asterisk_token expr_rule {
		context.push_node(Node::Command::mul_operator);
	}
	| expr_rule slash_token expr_rule {
		context.push_node(Node::Command::div_operator);
	}
	| expr_rule percent_token expr_rule {
		context.push_node(Node::Command::mod_operator);
	}
	| expr_rule dbl_asterisk_token expr_rule {
		context.push_node(Node::Command::pow_operator);
	}
	| expr_rule is_token expr_rule {
		context.push_node(Node::Command::is_operator);
	}
	| expr_rule dbl_equal_token expr_rule {
		context.push_node(Node::Command::eq_operator);
	}
	| expr_rule exclamation_equal_token expr_rule {
		context.push_node(Node::Command::ne_operator);
	}
	| expr_rule left_angled_token expr_rule {
		context.push_node(Node::Command::lt_operator);
	}
	| expr_rule right_angled_token expr_rule {
		context.push_node(Node::Command::gt_operator);
	}
	| expr_rule left_angled_equal_token expr_rule {
		context.push_node(Node::Command::le_operator);
	}
	| expr_rule right_angled_equal_token expr_rule {
		context.push_node(Node::Command::ge_operator);
	}
	| expr_rule dbl_left_angled_token expr_rule {
		context.push_node(Node::Command::shift_left_operator);
	}
	| expr_rule dbl_right_angled_token expr_rule {
		context.push_node(Node::Command::shift_right_operator);
	}
	| expr_rule dbl_dot_token expr_rule {
		context.push_node(Node::Command::inclusive_range_operator);
	}
	| expr_rule tpl_dot_token expr_rule {
		context.push_node(Node::Command::exclusive_range_operator);
	}
	| dbl_plus_token expr_rule %prec prefix_dbl_plus_token {
		context.push_node(Node::Command::inc_operator);
	}
	| dbl_minus_token expr_rule %prec prefix_dbl_minus_token {
		context.push_node(Node::Command::dec_operator);
	}
	| expr_rule dbl_plus_token {
	    context.push_node(Node::Command::clone_reference);
		context.push_node(Node::Command::inc_operator);
		context.push_node(Node::Command::unload_reference);
	}
	| expr_rule dbl_minus_token {
	    context.push_node(Node::Command::clone_reference);
		context.push_node(Node::Command::dec_operator);
		context.push_node(Node::Command::unload_reference);
	}
	| exclamation_token expr_rule {
		context.push_node(Node::Command::not_operator);
	}
	| expr_rule dbl_pipe_token {
		context.push_node(Node::Command::or_pre_check);
		context.start_jump_forward();
	} expr_rule {
		context.push_node(Node::Command::or_operator);
		context.resolve_jump_forward();
	}
	| expr_rule dbl_amp_token {
		context.push_node(Node::Command::and_pre_check);
		context.start_jump_forward();
	} expr_rule {
		context.push_node(Node::Command::and_operator);
		context.resolve_jump_forward();
	}
	| expr_rule pipe_token expr_rule {
		context.push_node(Node::Command::bor_operator);
	}
	| expr_rule amp_token expr_rule {
		context.push_node(Node::Command::band_operator);
	}
	| expr_rule caret_token expr_rule {
		context.push_node(Node::Command::xor_operator);
	}
	| tilde_token expr_rule {
		context.push_node(Node::Command::compl_operator);
	}
	| plus_token expr_rule %prec prefix_plus_token {
		context.push_node(Node::Command::pos_operator);
	}
	| minus_token expr_rule %prec prefix_minus_token {
		context.push_node(Node::Command::neg_operator);
	}
	| await_token expr_rule {
		if (context.is_in_async_function()) {
			context.push_node(Node::Command::await);
		}
		else {
			context.parse_error("unexpected 'await' statement outside of async function");
			YYERROR;
		}
	}
	| typeof_token expr_rule {
		context.push_node(Node::Command::typeof_operator);
	}
	| membersof_token expr_rule {
		context.push_node(Node::Command::membersof_operator);
	}
	| defined_token defined_symbol_rule {
		context.push_node(Node::Command::check_defined);
	}
	| expr_rule open_bracket_token expr_rule close_bracket_equal_token expr_rule {
		context.push_node(Node::Command::subscript_move_operator);
	}
	| expr_rule subscript_rule
	| member_ident_rule
	| ident_rule call_args_rule
	| def_rule call_args_rule
	| expr_rule subscript_rule call_args_rule
	| expr_rule dot_token call_member_args_rule
	| expr_rule question_dot_token call_defined_member_args_rule
	| open_parenthesis_token expr_rule close_parenthesis_token call_args_rule
	| expr_rule plus_equal_token {
		context.push_node(Node::Command::reload_reference);
	} expr_rule {
		context.push_node(Node::Command::add_operator);
		context.push_node(Node::Command::move_operator);
	}
	| expr_rule minus_equal_token {
		context.push_node(Node::Command::reload_reference);
	} expr_rule {
		context.push_node(Node::Command::sub_operator);
		context.push_node(Node::Command::move_operator);
	}
	| expr_rule asterisk_equal_token {
		context.push_node(Node::Command::reload_reference);
	} expr_rule {
		context.push_node(Node::Command::mul_operator);
		context.push_node(Node::Command::move_operator);
	}
	| expr_rule slash_equal_token {
		context.push_node(Node::Command::reload_reference);
	} expr_rule {
		context.push_node(Node::Command::div_operator);
		context.push_node(Node::Command::move_operator);
	}
	| expr_rule percent_equal_token {
		context.push_node(Node::Command::reload_reference);
	} expr_rule {
		context.push_node(Node::Command::mod_operator);
		context.push_node(Node::Command::move_operator);
	}
	| expr_rule dbl_left_angled_equal_token {
		context.push_node(Node::Command::reload_reference);
	} expr_rule {
		context.push_node(Node::Command::shift_left_operator);
		context.push_node(Node::Command::move_operator);
	}
	| expr_rule dbl_right_angled_equal_token {
		context.push_node(Node::Command::reload_reference);
	} expr_rule {
		context.push_node(Node::Command::shift_right_operator);
		context.push_node(Node::Command::move_operator);
	}
	| expr_rule amp_equal_token {
		context.push_node(Node::Command::reload_reference);
	} expr_rule {
		context.push_node(Node::Command::band_operator);
		context.push_node(Node::Command::move_operator);
	}
	| expr_rule pipe_equal_token {
		context.push_node(Node::Command::reload_reference);
	} expr_rule {
		context.push_node(Node::Command::bor_operator);
		context.push_node(Node::Command::move_operator);
	}
	| expr_rule caret_equal_token {
		context.push_node(Node::Command::reload_reference);
	} expr_rule {
		context.push_node(Node::Command::xor_operator);
		context.push_node(Node::Command::move_operator);
	}
	| expr_rule equal_tilde_token expr_rule {
		context.push_node(Node::Command::regex_match);
	}
	| expr_rule exclamation_tilde_token expr_rule {
		context.push_node(Node::Command::regex_unmatch);
	}
	| expr_rule tpl_equal_token expr_rule {
		context.push_node(Node::Command::strict_eq_operator);
	}
	| expr_rule exclamation_dbl_equal_token expr_rule {
		context.push_node(Node::Command::strict_ne_operator);
	}
	| expr_rule question_token {
		context.push_node(Node::Command::zero_jump);
		context.start_jump_forward();
	} expr_rule colon_token {
		context.push_node(Node::Command::jump);
		context.start_jump_forward();
		context.shift_jump_forward();
		context.resolve_jump_forward();
	} expr_rule {
		context.resolve_jump_forward();
	}
	| open_parenthesis_token close_parenthesis_token {
		context.push_node(Node::Command::alloc_iterator);
		context.start_call();
		context.push_node(Node::Command::init_iterator);
		context.resolve_call();
	}
	| open_parenthesis_token expr_rule close_parenthesis_token
	| open_parenthesis_token generator_expr_rule close_parenthesis_token
	| open_parenthesis_token iterator_item_rule iterator_end_rule close_parenthesis_token
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
    open_bracket_token expr_rule close_bracket_token {
		context.push_node(Node::Command::subscript_operator);
	};

call_args_rule:
	call_arg_start_rule call_arg_list_rule call_arg_stop_rule
	| call_args_rule call_arg_start_rule call_arg_list_rule call_arg_stop_rule;

call_member_args_rule:
	call_member_arg_start_rule call_arg_list_rule call_member_arg_stop_rule
	| call_member_args_rule call_arg_start_rule call_arg_list_rule call_arg_stop_rule;

call_defined_member_args_rule:
	call_defined_member_arg_start_rule call_arg_list_rule call_member_arg_stop_rule {
		context.resolve_jump_forward();
	}
	| call_defined_member_args_rule call_arg_start_rule call_arg_list_rule call_arg_stop_rule;

call_arg_start_rule:
    open_parenthesis_token {
		context.push_node(Node::Command::init_call);
		context.start_call();
	};

call_arg_stop_rule:
    close_parenthesis_token {
		context.push_node(Node::Command::call);
		context.resolve_call();
	};

call_member_arg_start_rule:
    symbol_token open_parenthesis_token {
		context.push_node(Node::Command::init_member_call);
		context.push_node($1.c_str());
		context.start_call();
	}
	| operator_desc_rule open_parenthesis_token {
		context.push_node(Node::Command::init_operator_call);
		context.push_node(context.retrieve_operator());
		context.start_call();
	}
	| var_symbol_rule open_parenthesis_token {
		context.push_node(Node::Command::init_var_member_call);
		context.start_call();
	};

call_defined_member_arg_start_rule:
    symbol_token open_parenthesis_token {
		context.push_node(Node::Command::init_defined_member_call);
		context.push_node($1.c_str());
		context.start_jump_forward();
		context.start_call();
	}
	| operator_desc_rule open_parenthesis_token {
		context.push_node(Node::Command::init_defined_operator_call);
		context.push_node(context.retrieve_operator());
		context.start_jump_forward();
		context.start_call();
	}
	| var_symbol_rule open_parenthesis_token {
		context.push_node(Node::Command::init_defined_var_member_call);
		context.start_jump_forward();
		context.start_call();
	};

call_member_arg_stop_rule:
    close_parenthesis_token {
		context.push_node(Node::Command::call_member);
		context.resolve_call();
	};

call_arg_list_rule:
	call_arg_list_rule separator_rule call_arg_rule
	| call_arg_rule
	| ;

call_arg_rule:
	expr_rule {
		context.add_to_call();
	}
	| def_arrow_rule {
	    context.add_to_call();
	}
	| generator_expr_rule {
	    context.add_to_call();
	}
	| asterisk_token expr_rule {
		context.push_node(Node::Command::in_operator);
		context.push_node(Node::Command::load_extra_arguments);
	}
	| tpl_dot_token expr_rule {
		context.push_node(Node::Command::in_operator);
		context.push_node(Node::Command::load_extra_arguments);
	};

def_rule:
	def_start_rule def_capture_rule def_args_rule stmt_bloc_rule {
		if (context.is_in_generator()) {
			if (!context.is_in_async_function()) {
				context.push_node(Node::Command::exit_generator);
			}
			else if (!context.has_returned()) {
				context.push_node(Node::Command::exit_coroutine);
			}
		}
		else if (!context.has_returned()) {
			if (context.is_in_async_function()) {
				context.push_node(Node::Command::load_constant);
				context.push_node(Compiler::make_none());
				context.push_node(Node::Command::resume_coroutine);
			}
			else {
				context.push_node(Node::Command::load_constant);
				context.push_node(Compiler::make_none());
				context.push_node(Node::Command::exit_call);
			}
		}
		context.resolve_jump_forward();
		context.save_definition();
	}
	| def_start_rule def_capture_rule def_no_args_rule stmt_bloc_rule {
		if (context.is_in_generator()) {
			if (!context.is_in_async_function()) {
				context.push_node(Node::Command::exit_generator);
			}
			else if (!context.has_returned()) {
				context.push_node(Node::Command::exit_coroutine);
			}
		}
		else if (!context.has_returned()) {
			if (context.is_in_async_function()) {
				context.push_node(Node::Command::load_constant);
				context.push_node(Compiler::make_none());
				context.push_node(Node::Command::resume_coroutine);
			}
			else {
				context.push_node(Node::Command::load_constant);
				context.push_node(Compiler::make_none());
				context.push_node(Node::Command::exit_call);
			}
		}
		context.resolve_jump_forward();
		context.save_definition();
	};

def_arrow_rule:
    def_start_rule def_capture_rule def_args_rule def_arrow_stmt_rule {
	    context.set_exit_point();
		if (context.is_in_async_function()) {
			context.push_node(Node::Command::resume_coroutine);
		}
		else {
			context.push_node(Node::Command::exit_call);
		}
		context.resolve_jump_forward();
		context.save_definition();
	};

def_start_rule:
    def_token {
		context.push_node(Node::Command::jump);
		context.start_jump_forward();
		context.start_definition();
	}
	| async_token def_token {
		context.push_node(Node::Command::jump);
		context.start_jump_forward();
		context.start_async_definition();
	};

def_capture_rule:
	def_capture_start_rule def_capture_list_rule def_capture_stop_rule
	| ;

def_capture_start_rule:
    open_bracket_token {
		context.start_capture();
	};

def_capture_stop_rule:
    close_bracket_token {
		context.resolve_capture();
	};

def_capture_list_rule:
    symbol_token equal_token expr_rule separator_rule def_capture_list_rule {
		if (!context.capture_as($1)) {
			YYERROR;
		}
	}
	| symbol_token equal_token expr_rule {
		if (!context.capture_as($1)) {
			YYERROR;
		}
	}
	| symbol_token separator_rule def_capture_list_rule {
		if (!context.capture($1)) {
			YYERROR;
		}
	}
	| symbol_token {
		if (!context.capture($1)) {
			YYERROR;
		}
	}
	| tpl_dot_token {
		if (!context.capture_all()) {
			YYERROR;
		}
	};

def_no_args_rule:
	{
		if (!context.save_parameters()) {
			YYERROR;
		}
	};

def_args_rule:
	def_arg_start_rule def_arg_list_rule def_arg_stop_rule;

def_arg_start_rule:
    open_parenthesis_token;

def_arg_stop_rule:
    close_parenthesis_token {
		if (!context.save_parameters()) {
			YYERROR;
		}
	};

def_arg_list_rule:
	def_arg_rule separator_rule def_arg_list_rule
	| def_arg_rule
	| ;

def_arg_rule:
    symbol_token {
		if (!context.add_parameter($1)) {
			YYERROR;
		}
	}
	| symbol_token equal_token expr_rule {
		if (!context.add_definition_signature()) {
			YYERROR;
		}
		if (!context.add_parameter($1)) {
			YYERROR;
		}
	}
	| modifier_rule symbol_token {
		if (!context.add_parameter($2, context.retrieve_modifiers())) {
			YYERROR;
		}
	}
	| modifier_rule symbol_token equal_token expr_rule {
		if (!context.add_definition_signature()) {
			YYERROR;
		}
		if (!context.add_parameter($2, context.retrieve_modifiers())) {
			YYERROR;
		}
	}
	| tpl_dot_token {
		if (!context.set_variadic()) {
			YYERROR;
		}
	};

def_arrow_stmt_rule:
    def_arrow_stmt_start_rule expr_rule;

def_arrow_stmt_start_rule:
    equal_right_angled_token {
	    context.prepare_return();
	};

member_ident_rule:
    expr_rule dot_token symbol_token {
		context.push_node(Node::Command::load_member);
		context.push_node($3.c_str());
	}
	| expr_rule dot_token operator_desc_rule {
		context.push_node(Node::Command::load_operator);
		context.push_node(context.retrieve_operator());
	}
	| expr_rule dot_token var_symbol_rule {
		context.push_node(Node::Command::load_var_member);
	}
	| expr_rule question_dot_token symbol_token {
		context.push_node(Node::Command::load_defined_member);
		context.push_node($3.c_str());
	}
	| expr_rule question_dot_token operator_desc_rule {
		context.push_node(Node::Command::load_defined_operator);
		context.push_node(context.retrieve_operator());
	}
	| expr_rule question_dot_token var_symbol_rule {
		context.push_node(Node::Command::load_defined_var_member);
	};

defined_symbol_rule:
    symbol_token {
		context.push_node(Node::Command::find_defined_symbol);
		context.push_node($1.c_str());
	}
	| defined_symbol_rule dot_token symbol_token {
		context.push_node(Node::Command::find_defined_member);
		context.push_node($3.c_str());
	}
	| var_symbol_rule {
		context.push_node(Node::Command::find_defined_var_symbol);
	}
	| defined_symbol_rule dot_token var_symbol_rule {
		context.push_node(Node::Command::find_defined_var_member);
	}
	| constant_rule {
		context.push_node(Node::Command::load_constant);
		if (Data *data = context.compiler().make_data($1, Compiler::DataHint::data_unknown_hint)) {
			context.push_node(*data);
		}
		else {
			error("token '" + $1 + "' is not a valid constant");
			YYERROR;
		}
	};

ident_rule:
	constant_rule {
		context.push_node(Node::Command::load_constant);
		if (Data *data = context.compiler().make_data($1, Compiler::DataHint::data_unknown_hint)) {
			context.push_node(*data);
		}
		else {
			error("token '" + $1 + "' is not a valid constant");
			YYERROR;
		}
	}
	| lib_token {
		context.push_node(Node::Command::create_lib);
	}
	| var_symbol_rule {
		context.push_node(Node::Command::load_var_symbol);
	}
	| symbol_token {
		const auto index = context.fast_symbol_index($1);
		if (index != invalid_index) {
			context.push_node(Node::Command::load_fast);
			context.push_node($1.c_str());
			context.push_node(index);
		}
		else {
			context.push_node(Node::Command::load_symbol);
			context.push_node($1.c_str());
		}
	}
	| let_token symbol_token {
		const auto index = context.create_fast_scoped_symbol_index($2);
		if (index != invalid_index) {
			context.push_node(Node::Command::declare_fast);
			context.push_node($2.c_str());
			context.push_node(index);
			context.push_node(Reference::default_flags);
		}
		else {
			context.push_node(Node::Command::declare_symbol);
			context.push_node($2.c_str());
			context.push_node(Reference::default_flags);
		}
	}
	| modifier_rule symbol_token {
		const auto index = context.create_fast_symbol_index($2);
		if (index != invalid_index) {
			context.push_node(Node::Command::declare_fast);
			context.push_node($2.c_str());
			context.push_node(index);
			context.push_node(context.retrieve_modifiers());
		}
		else {
			context.push_node(Node::Command::declare_symbol);
			context.push_node($2.c_str());
			context.push_node(context.retrieve_modifiers());
		}
	}
	| let_token modifier_rule symbol_token {
		const auto index = context.create_fast_scoped_symbol_index($3);
		if (index != invalid_index) {
			context.push_node(Node::Command::declare_fast);
			context.push_node($3.c_str());
			context.push_node(index);
			context.push_node(context.retrieve_modifiers());
		}
		else {
			context.push_node(Node::Command::declare_symbol);
			context.push_node($3.c_str());
			context.push_node(context.retrieve_modifiers());
		}
	};

constant_rule:
    constant_token {
		$$ = $1;
	}
	| regex_rule {
		$$ = $1;
	}
	| regex_rule symbol_token {
		$$ = $1 + $2;
	}
	| string_token {
		$$ = $1;
	}
	| number_token {
		$$ = $1;
	};

regex_rule:
    slash_token {
		$$ = $1 + context.read_regex();
	} slash_token {
		$$ = $2 + $1;
	};

var_symbol_rule:
    dollar_token open_brace_token expr_rule close_brace_token;

modifier_rule:
    var_token {
		context.start_modifiers(Reference::default_flags);
	}
	| dollar_token {
		context.start_modifiers(Reference::const_address);
	}
	| percent_token {
		context.start_modifiers(Reference::const_value);
	}
	| const_token {
		context.start_modifiers(Reference::const_address | Reference::const_value);
	}
	| at_token {
		context.start_modifiers(Reference::global);
	}
	| modifier_rule var_token {
		context.add_modifiers(Reference::default_flags);
	}
	| modifier_rule dollar_token {
		context.add_modifiers(Reference::const_address);
	}
	| modifier_rule percent_token {
		context.add_modifiers(Reference::const_value);
	}
	| modifier_rule const_token {
		context.add_modifiers(Reference::const_address | Reference::const_value);
	}
	| modifier_rule at_token {
		context.add_modifiers(Reference::global);
	};

separator_rule:
    comma_token | separator_rule line_end_token {
		context.commit_line();
	};

empty_lines_rule:
    line_end_token {
		context.commit_line();
	}
	| empty_lines_rule line_end_token {
		context.commit_line();
	};

%%

void parser::error(const std::string &msg) {
	context.parse_error(msg);
}

int BuildContext::next_token(std::string *token) {

	if (_lexer.at_end()) {
	    return parser::token::file_end_token;
	}

	*token = _lexer.next_token();
	return Lexer::token_type(*token);
}

bool Compiler::build(DataStream& stream, const Module::Info& node) {

	auto context = BuildContext(stream, *this, node);
	auto parser = mint::parser(context);

	if (is_printing()) {
		context.force_printer();
	}

	return !parser.parse();
}

#endif // MINT_COMPILER_PARSER_HPP
