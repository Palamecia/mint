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

%token assert_token
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
%right equal_token question_token dbldot_token dbldot_equal_token close_bracket_equal_token plus_equal_token minus_equal_token asterisk_equal_token slash_equal_token percent_equal_token dbl_left_angled_equal_token dbl_right_angled_equal_token amp_equal_token pipe_equal_token caret_equal_token
%left dot_dot_token tpl_dot_token
%left dbl_equal_token exclamation_equal_token is_token equal_tilde_token exclamation_tilde_token tpl_equal_token exclamation_dbl_equal_token
%left left_angled_token right_angled_token left_angled_equal_token right_angled_equal_token
%left dbl_left_angled_token dbl_right_angled_token
%left plus_token minus_token
%left asterisk_token slash_token percent_token
%right prefix_dbl_plus_token prefix_dbl_minus_token prefix_plus_token prefix_minus_token exclamation_token tilde_token typeof_token membersof_token defined_token
%left dbl_plus_token dbl_minus_token dbl_asterisk_token
%left dot_token open_parenthesis_token close_parenthesis_token open_bracket_token close_bracket_token open_brace_token close_brace_token

%%

module_rule:
	stmt_list_rule file_end_token {
	    context->push_node(Node::exit_module);
		fflush(stdout);
		YYACCEPT;
	}
	| file_end_token {
	    context->push_node(Node::exit_module);
		fflush(stdout);
		YYACCEPT;
	};

stmt_list_rule:
	stmt_list_rule stmt_rule
	| stmt_rule;

stmt_rule:
	load_token module_path_rule line_end_token {
		context->push_node(Node::load_module);
		context->push_node($2.c_str());
		context->commit_line();
	}
	| try_rule stmt_bloc_rule {
		context->reset_scoped_symbols();
		context->unregister_retrieve_point();
		context->push_node(Node::unset_retrieve_point);
		context->push_node(Node::jump);
		context->start_jump_forward();
		context->shift_jump_forward();
		context->resolve_jump_forward();
		context->push_node(Node::unload_reference);
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
	| switch_cond_rule open_brace_token case_list_rule close_brace_token {
		context->reset_scoped_symbols();
		context->push_node(Node::jump);
		context->start_jump_forward();
		context->build_case_table();
		context->resolve_jump_forward();
		context->close_block();
	}
	| while_cond_rule stmt_bloc_rule {
		context->reset_scoped_symbols();
		context->push_node(Node::jump);
		context->resolve_jump_backward();
		context->resolve_jump_forward();
		context->close_block();
	}
	| for_cond_rule stmt_bloc_rule {
		context->reset_scoped_symbols();
		context->push_node(Node::jump);
		context->resolve_jump_backward();
		context->resolve_jump_forward();
		context->close_block();
	}
	| break_token line_end_token {
		if (!context->is_in_loop() && !context->is_in_switch()) {
			context->parse_error("break statement not within loop or switch");
			YYERROR;
		}
		context->prepare_break();
		context->push_node(Node::jump);
		context->bloc_jump_forward();
		context->commit_line();
	}
	| continue_token line_end_token {
		if (!context->is_in_loop()) {
			context->parse_error("continue statement not within loop");
			YYERROR;
		}
		context->prepare_continue();
		context->push_node(Node::jump);
		context->bloc_jump_backward();
		context->commit_line();
	}
	| print_rule stmt_bloc_rule {
		context->reset_scoped_symbols();
		context->close_block();
		context->close_printer();
	}
	| yield_token expr_rule line_end_token {
		if (!context->is_in_function()) {
			context->parse_error("unexpected 'yield' statement outside of function");
			YYERROR;
		}
		context->set_generator();
		context->push_node(Node::yield);
		context->commit_line();
	}
	| return_rule expr_rule line_end_token {
		context->set_exit_point();
		if (context->is_in_generator()) {
			context->push_node(Node::yield_exit_generator);
		}
		else {
			context->push_node(Node::exit_call);
		}
		context->commit_line();
	}
	| raise_token expr_rule line_end_token {
		context->reset_scoped_symbols_until(BuildContext::try_type);
		context->push_node(Node::raise);
		context->commit_line();
	}
	| exit_token expr_rule line_end_token {
		context->push_node(Node::exit_exec);
		context->commit_line();
	}
	| exit_token line_end_token {
		context->push_node(Node::load_constant);
		context->push_node(Compiler::make_data("0", Compiler::data_number_hint));
		context->push_node(Node::exit_exec);
		context->commit_line();
	}
	| ident_iterator_item_rule ident_iterator_end_rule equal_token expr_rule line_end_token {
		context->push_node(Node::copy_op);
		context->commit_expr_result();
		context->commit_line();
	}
	| ident_iterator_item_rule ident_iterator_end_rule equal_token generator_expr_rule line_end_token {
		context->push_node(Node::copy_op);
		context->commit_expr_result();
		context->commit_line();
	}
	| create_ident_iterator_rule equal_token expr_rule line_end_token {
		context->push_node(Node::copy_op);
		context->commit_expr_result();
		context->commit_line();
	}
	| create_ident_iterator_rule equal_token generator_expr_rule line_end_token {
		context->push_node(Node::copy_op);
		context->commit_expr_result();
		context->commit_line();
	}
	| expr_rule line_end_token {
		context->commit_expr_result();
		context->commit_line();
	}
	| modifier_rule def_start_rule def_capture_rule symbol_token def_args_rule stmt_bloc_rule {
		if (context->is_in_generator()) {
			context->push_node(Node::exit_generator);
		}
		else if (!context->has_returned()) {
			context->push_node(Node::load_constant);
			context->push_node(Compiler::make_none());
			context->push_node(Node::exit_call);
		}
		context->resolve_jump_forward();
		context->push_node(Node::create_function);
		context->push_node($4.c_str());
		context->push_node(Reference::global | Reference::const_address | context->retrieve_modifiers());
		context->save_definition();
		context->push_node(Node::function_overload);
		context->push_node(Node::unload_reference);
	}
	| def_start_rule def_capture_rule symbol_token def_args_rule stmt_bloc_rule {
		if (context->is_in_generator()) {
			context->push_node(Node::exit_generator);
		}
		else if (!context->has_returned()) {
			context->push_node(Node::load_constant);
			context->push_node(Compiler::make_none());
			context->push_node(Node::exit_call);
		}
		context->resolve_jump_forward();
		context->push_node(Node::create_function);
		context->push_node($3.c_str());
		context->push_node(Reference::global | Reference::const_address);
		context->save_definition();
		context->push_node(Node::function_overload);
		context->push_node(Node::unload_reference);
	}
	| package_block_rule
	| class_desc_rule
	| enum_desc_rule
	| line_end_token {
		context->commit_line();
	};

module_name_rule:
	assert_token { $$ = $1; }
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
		context->open_package($2);
	};

package_block_rule:
	package_rule open_brace_token stmt_list_rule close_brace_token {
		context->close_package();
	};

class_rule:
	class_token symbol_token {
		context->start_class_description($2);
	};

parent_rule:
	dbldot_token parent_list_rule
	| ;

parent_list_rule:
	parent_ident_rule {
		context->save_base_class_path();
	}
	| parent_list_rule comma_token parent_ident_rule {
		context->save_base_class_path();
	};

parent_ident_rule:
	symbol_token {
		context->append_symbol_to_base_class_path($1);
	}
	| parent_ident_rule dot_token symbol_token {
		context->append_symbol_to_base_class_path($3);
	};

class_desc_rule:
	class_rule parent_rule desc_bloc_rule {
		context->resolve_class_description();
	};

member_class_rule:
	class_rule
	| member_type_modifier_rule class_token symbol_token {
		context->start_class_description($3, context->retrieve_modifiers());
	};

member_class_desc_rule:
	member_class_rule parent_rule desc_bloc_rule {
		context->resolve_class_description();
	};

member_enum_rule:
	enum_rule
	| member_type_modifier_rule enum_token symbol_token {
		context->start_enum_description($3, context->retrieve_modifiers());
	};

member_enum_desc_rule:
	member_enum_rule enum_block_rule {
		context->resolve_enum_description();
	};

member_type_modifier_rule:
	plus_token {
		context->start_modifiers(Reference::standard);
	}
	| sharp_token {
		context->start_modifiers(Reference::protected_visibility);
	}
	| minus_token {
		context->start_modifiers(Reference::private_visibility);
	}
	| tilde_token {
		context->start_modifiers(Reference::package_visibility);
	};

desc_bloc_rule:
	open_brace_token desc_list_rule close_brace_token
	| open_brace_token close_brace_token;

desc_list_rule:
	desc_list_rule desc_rule
	| desc_rule;

desc_rule:
	member_desc_rule line_end_token {
		if (!context->create_member(context->retrieve_modifiers(), Symbol($1))) {
			YYERROR;
		}
		context->commit_line();
	}
	| member_desc_rule equal_token constant_token line_end_token {
		if (!context->create_member(context->retrieve_modifiers(), Symbol($1), Compiler::make_data($3, Compiler::data_unknown_hint))) {
			YYERROR;
		}
		context->commit_line();
	}
	| member_desc_rule equal_token string_token line_end_token {
		if (!context->create_member(context->retrieve_modifiers(), Symbol($1), Compiler::make_data($3, Compiler::data_string_hint))) {
			YYERROR;
		}
		context->commit_line();
	}
	| member_desc_rule equal_token regex_rule line_end_token {
		if (!context->create_member(context->retrieve_modifiers(), Symbol($1), Compiler::make_data($3, Compiler::data_regex_hint))) {
			YYERROR;
		}
		context->commit_line();
	}
	| member_desc_rule equal_token regex_rule regex_rule symbol_token line_end_token {
		if (!context->create_member(context->retrieve_modifiers(), Symbol($1), Compiler::make_data($3 + $4, Compiler::data_regex_hint))) {
			YYERROR;
		}
		context->commit_line();
	}
	| member_desc_rule equal_token number_token line_end_token {
		if (!context->create_member(context->retrieve_modifiers(), Symbol($1), Compiler::make_data($3, Compiler::data_number_hint))) {
			YYERROR;
		}
		context->commit_line();
	}
	| member_desc_rule equal_token open_bracket_token close_bracket_token line_end_token {
		if (!context->create_member(context->retrieve_modifiers(), Symbol($1), Compiler::make_array())) {
			YYERROR;
		}
		context->commit_line();
	}
	| member_desc_rule equal_token open_brace_token close_brace_token line_end_token {
		if (!context->create_member(context->retrieve_modifiers(), Symbol($1), Compiler::make_hash())) {
			YYERROR;
		}
		context->commit_line();
	}
	| member_desc_rule equal_token lib_token open_parenthesis_token string_token close_parenthesis_token line_end_token {
		if (!context->create_member(context->retrieve_modifiers(), Symbol($1), Compiler::make_library($5))) {
			YYERROR;
		}
		context->commit_line();
	}
	| member_desc_rule equal_token def_start_rule def_args_rule stmt_bloc_rule {
		if (context->is_in_generator()) {
			context->push_node(Node::exit_generator);
		}
		else if (!context->has_returned()) {
			context->push_node(Node::load_constant);
			context->push_node(Compiler::make_none());
			context->push_node(Node::exit_call);
		}
		context->resolve_jump_forward();

		if (!context->create_member(context->retrieve_modifiers(), Symbol($1), context->retrieve_definition())) {
			YYERROR;
		}
	}
	| member_desc_rule plus_equal_token def_start_rule def_args_rule stmt_bloc_rule {
		if (context->is_in_generator()) {
			context->push_node(Node::exit_generator);
		}
		else if (!context->has_returned()) {
			context->push_node(Node::load_constant);
			context->push_node(Compiler::make_none());
			context->push_node(Node::exit_call);
		}
		context->resolve_jump_forward();

		if (!context->update_member(context->retrieve_modifiers(), Symbol($1), context->retrieve_definition())) {
			YYERROR;
		}
	}
	| def_start_rule symbol_token def_args_rule stmt_bloc_rule {
		if (context->is_in_generator()) {
			context->push_node(Node::exit_generator);
		}
		else if (!context->has_returned()) {
			context->push_node(Node::load_constant);
			context->push_node(Compiler::make_none());
			context->push_node(Node::exit_call);
		}
		context->resolve_jump_forward();

		if (!context->update_member(Reference::standard, Symbol($2), context->retrieve_definition())) {
			YYERROR;
		}
	}
	| def_start_rule operator_desc_rule def_args_rule stmt_bloc_rule {
		if (context->is_in_generator()) {
			context->push_node(Node::exit_generator);
		}
		else if (!context->has_returned()) {
			context->push_node(Node::load_constant);
			context->push_node(Compiler::make_none());
			context->push_node(Node::exit_call);
		}
		context->resolve_jump_forward();

		if (!context->update_member(Reference::standard, context->retrieve_operator(), context->retrieve_definition())) {
			YYERROR;
		}
	}
	| desc_modifier_rule def_start_rule symbol_token def_args_rule stmt_bloc_rule {
		if (context->is_in_generator()) {
			context->push_node(Node::exit_generator);
		}
		else if (!context->has_returned()) {
			context->push_node(Node::load_constant);
			context->push_node(Compiler::make_none());
			context->push_node(Node::exit_call);
		}
		context->resolve_jump_forward();

		if (!context->update_member(context->retrieve_modifiers(), Symbol($3), context->retrieve_definition())) {
			YYERROR;
		}
	}
	| desc_modifier_rule def_start_rule operator_desc_rule def_args_rule stmt_bloc_rule {
		if (context->is_in_generator()) {
			context->push_node(Node::exit_generator);
		}
		else if (!context->has_returned()) {
			context->push_node(Node::load_constant);
			context->push_node(Compiler::make_none());
			context->push_node(Node::exit_call);
		}
		context->resolve_jump_forward();

		if (!context->update_member(context->retrieve_modifiers(), context->retrieve_operator(), context->retrieve_definition())) {
			YYERROR;
		}
	}
	| member_class_desc_rule
	| member_enum_desc_rule
	| line_end_token {
		context->commit_line();
	};

member_desc_rule:
	symbol_token {
		context->start_modifiers(Reference::standard);
		$$ = $1;
	}
	| desc_modifier_rule symbol_token {
		$$ = $2;
	};

desc_base_modifier_rule:
	modifier_rule
	| final_token {
		context->start_modifiers(Reference::final_member);
	}
	| override_token {
		context->start_modifiers(Reference::override_member);
	}
	| final_token modifier_rule {
		context->add_modifiers(Reference::final_member);
	}
	| override_token modifier_rule {
		context->add_modifiers(Reference::override_member);
	};

desc_modifier_rule:
	desc_base_modifier_rule
	| plus_token {
		context->start_modifiers(Reference::standard);
	}
	| sharp_token {
		context->start_modifiers(Reference::protected_visibility);
	}
	| minus_token {
		context->start_modifiers(Reference::private_visibility);
	}
	| tilde_token {
		context->start_modifiers(Reference::package_visibility);
	}
	| plus_token desc_base_modifier_rule {
		context->add_modifiers(Reference::standard);
	}
	| sharp_token desc_base_modifier_rule {
		context->add_modifiers(Reference::protected_visibility);
	}
	| minus_token desc_base_modifier_rule {
		context->add_modifiers(Reference::private_visibility);
	}
	| tilde_token desc_base_modifier_rule {
		context->add_modifiers(Reference::package_visibility);
	};

operator_desc_rule:
	in_token {
		context->start_operator(Class::in_operator);
		$$ = $1;
	}
	| dbldot_equal_token {
		context->start_operator(Class::copy_operator);
		$$ = $1;
	}
	| dbl_pipe_token {
		context->start_operator(Class::or_operator);
		$$ = $1;
	}
	| dbl_amp_token {
		context->start_operator(Class::and_operator);
		$$ = $1;
	}
	| pipe_token {
		context->start_operator(Class::bor_operator);
		$$ = $1;
	}
	| caret_token {
		context->start_operator(Class::xor_operator);
		$$ = $1;
	}
	| amp_token {
		context->start_operator(Class::band_operator);
		$$ = $1;
	}
	| dbl_equal_token {
		context->start_operator(Class::eq_operator);
		$$ = $1;
	}
	| exclamation_equal_token {
		context->start_operator(Class::ne_operator);
		$$ = $1;
	}
	| left_angled_token {
		context->start_operator(Class::lt_operator);
		$$ = $1;
	}
	| right_angled_token {
		context->start_operator(Class::gt_operator);
		$$ = $1;
	}
	| left_angled_equal_token {
		context->start_operator(Class::le_operator);
		$$ = $1;
	}
	| right_angled_equal_token {
		context->start_operator(Class::ge_operator);
		$$ = $1;
	}
	| dbl_left_angled_token {
		context->start_operator(Class::shift_left_operator);
		$$ = $1;
	}
	| dbl_right_angled_token {
		context->start_operator(Class::shift_right_operator);
		$$ = $1;
	}
	| plus_token {
		context->start_operator(Class::add_operator);
		$$ = $1;
	}
	| minus_token {
		context->start_operator(Class::sub_operator);
		$$ = $1;
	}
	| asterisk_token {
		context->start_operator(Class::mul_operator);
		$$ = $1;
	}
	| slash_token {
		context->start_operator(Class::div_operator);
		$$ = $1;
	}
	| percent_token {
		context->start_operator(Class::mod_operator);
		$$ = $1;
	}
	| exclamation_token {
		context->start_operator(Class::not_operator);
		$$ = $1;
	}
	| tilde_token {
		context->start_operator(Class::compl_operator);
		$$ = $1;
	}
	| dbl_plus_token {
		context->start_operator(Class::inc_operator);
		$$ = $1;
	}
	| dbl_minus_token {
		context->start_operator(Class::dec_operator);
		$$ = $1;
	}
	| dbl_asterisk_token {
		context->start_operator(Class::pow_operator);
		$$ = $1;
	}
	| dot_dot_token {
		context->start_operator(Class::inclusive_range_operator);
		$$ = $1;
	}
	| tpl_dot_token {
		context->start_operator(Class::exclusive_range_operator);
		$$ = $1;
	}
	| open_parenthesis_token close_parenthesis_token {
		context->start_operator(Class::call_operator);
		$$ = $1 + $2;
	}
	| open_bracket_token close_bracket_token {
		context->start_operator(Class::subscript_operator);
		$$ = $1 + $2;
	}
	| open_bracket_token close_bracket_equal_token {
		context->start_operator(Class::subscript_move_operator);
		$$ = $1 + $2;
	};

enum_rule:
	enum_token symbol_token {
		context->start_enum_description($2);
	};

enum_desc_rule:
	enum_rule enum_block_rule {
		context->resolve_enum_description();
	};

enum_block_rule:
	open_brace_token enum_list_rule close_brace_token;

enum_list_rule:
	enum_list_rule enum_item_rule
	| enum_item_rule;

enum_item_rule:
	symbol_token equal_token number_token {
		Reference::Flags flags = Reference::const_value | Reference::const_address | Reference::global;
		if (!context->create_member(flags, Symbol($1), Compiler::make_data($3, Compiler::data_number_hint))) {
			YYERROR;
		}
		context->set_current_enum_value(atoi($3.c_str()));
	}
	| symbol_token {
		Reference::Flags flags = Reference::const_value | Reference::const_address | Reference::global;
		if (!context->create_member(flags, Symbol($1), Compiler::make_data(std::to_string(context->next_enum_value()), Compiler::data_number_hint))) {
			YYERROR;
		}
	}
	| line_end_token {
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
	| switch_cond_expr_rule open_brace_token case_list_rule close_brace_token {
		context->reset_scoped_symbols();
		context->push_node(Node::jump);
		context->start_jump_forward();
		context->build_case_table();
		context->resolve_jump_forward();
		context->close_generator_expression();
		context->close_block();
	}
	| while_cond_expr_rule stmt_bloc_rule {
		context->reset_scoped_symbols();
		context->push_node(Node::jump);
		context->resolve_jump_backward();
		context->resolve_jump_forward();
		context->close_generator_expression();
		context->close_block();
	}
	| for_cond_expr_rule stmt_bloc_rule {
		context->reset_scoped_symbols();
		context->push_node(Node::jump);
		context->resolve_jump_backward();
		context->resolve_jump_forward();
		context->close_generator_expression();
		context->close_block();
	};

try_rule:
	try_token {
		context->register_retrieve_point();
		context->push_node(Node::set_retrieve_point);
		context->start_jump_forward();
		context->open_block(BuildContext::try_type);
	};

catch_rule:
	catch_token symbol_token {
		context->close_block();
		context->unregister_retrieve_point();
		context->push_node(Node::unset_retrieve_point);
		context->push_node(Node::jump);
		context->start_jump_forward();
		context->shift_jump_forward();
		context->resolve_jump_forward();
		context->open_block(BuildContext::catch_type);
		context->push_node(Node::init_exception);
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
	open_brace_token stmt_list_rule close_brace_token
	| open_brace_token yield_token expr_rule close_brace_token {
		if (!context->is_in_function()) {
			context->parse_error("unexpected 'yield' statement outside of function");
			YYERROR;
		}
		context->set_generator();
		context->push_node(Node::yield);
	}
	| open_brace_token return_rule expr_rule close_brace_token {
		context->set_exit_point();
		if (context->is_in_generator()) {
			context->push_node(Node::yield_exit_generator);
		}
		else {
			context->push_node(Node::exit_call);
		}
	}
	| open_brace_token expr_rule close_brace_token {
		context->commit_expr_result();
	}
	| open_brace_token close_brace_token;

if_cond_expr_rule:
	if_expr_rule expr_rule {
		context->resolve_condition();
		context->push_node(Node::jump_zero);
		context->start_jump_forward();
		context->open_block(BuildContext::if_type);
	}
	| if_expr_rule find_rule {
		context->resolve_condition();
		context->push_node(Node::jump_zero);
		context->start_jump_forward();
		context->open_block(BuildContext::if_type);
	};

if_cond_rule:
	if_rule expr_rule {
		context->resolve_condition();
		context->push_node(Node::jump_zero);
		context->start_jump_forward();
		context->open_block(BuildContext::if_type);
	}
	| if_rule find_rule {
		context->resolve_condition();
		context->push_node(Node::jump_zero);
		context->start_jump_forward();
		context->open_block(BuildContext::if_type);
	};

elif_cond_rule:
	elif_rule expr_rule {
		context->resolve_condition();
		context->close_block();
		context->push_node(Node::jump_zero);
		context->start_jump_forward();
		context->open_block(BuildContext::elif_type);
	}
	| elif_rule find_rule {
		context->resolve_condition();
		context->close_block();
		context->push_node(Node::jump_zero);
		context->start_jump_forward();
		context->open_block(BuildContext::elif_type);
	};

if_expr_rule:
	if_token {
		context->open_generator_expression();
		context->start_condition();
	};

if_rule:
	if_token {
		context->start_condition();
	};

elif_rule:
	elif_token {
		context->push_node(Node::jump);
		context->start_jump_forward();
		context->shift_jump_forward();
		context->resolve_jump_forward();
		context->start_condition();
	};

else_rule:
	else_token {
		context->push_node(Node::jump);
		context->start_jump_forward();

		context->close_block();
		context->shift_jump_forward();
		context->resolve_jump_forward();
		context->open_block(BuildContext::else_type);
	};

switch_cond_expr_rule:
	switch_expr_rule expr_rule {
		context->resolve_condition();
		context->open_block(BuildContext::switch_type);
	};

switch_cond_rule:
	switch_rule expr_rule {
		context->resolve_condition();
		context->open_block(BuildContext::switch_type);
	};

switch_expr_rule:
	switch_token {
		context->open_generator_expression();
		context->start_condition();
	};

switch_rule:
	switch_token {
		context->start_condition();
	};

case_rule:
	case_token {
		context->start_case_label();
	};

case_symbol_rule:
	symbol_token {
		context->push_node(Node::load_symbol);
		context->push_node($1.c_str());
		$$ = $1;
	}
	| case_symbol_rule dot_token symbol_token {
		context->push_node(Node::load_member);
		context->push_node($3.c_str());
		$$ = $1 + $2 + $3;
	};

case_constant_rule:
	constant_rule {
		if (Data *data = Compiler::make_data($1, Compiler::data_unknown_hint)) {
			context->push_node(Node::load_constant);
			context->push_node(data);
			$$ = $1;
		}
		else {
			error("token '" + $1 + "' is not a valid constant");
			YYERROR;
		}
	}
	| plus_token number_token {
		if (Data *data = Compiler::make_data($2, Compiler::data_number_hint)) {
			context->push_node(Node::load_constant);
			context->push_node(data);
			context->push_node(Node::pos_op);
			$$ = $2;
		}
		else {
			error("token '" + $2 + "' is not a valid constant");
			YYERROR;
		}
	}
	| minus_token number_token {
		if (Data *data = Compiler::make_data($2, Compiler::data_number_hint)) {
			context->push_node(Node::load_constant);
			context->push_node(data);
			context->push_node(Node::neg_op);
			$$ = $1 + $2;
		}
		else {
			error("token '" + $2 + "' is not a valid constant");
			YYERROR;
		}
	};

case_constant_list_rule:
	case_constant_list_rule case_constant_rule comma_token {
		context->add_to_call();
		$$ = $1 + $2 + $3;
	}
	| case_constant_rule comma_token {
		context->push_node(Node::alloc_iterator);
		context->start_call();
		context->add_to_call();
		$$ = $1 + $2;
	};

case_constant_list_end_rule:
	case_constant_rule {
		context->push_node(Node::create_iterator);
		context->add_to_call();
		context->resolve_call();
		$$ = $1;
	}
	| {
		context->push_node(Node::create_iterator);
		context->resolve_call();
	};

case_label_rule:
	case_rule in_token case_constant_rule dot_dot_token case_constant_rule dbldot_token {
		context->push_node(Node::inclusive_range_op);
		context->start_jump_backward();
		context->push_node(Node::find_next);
		context->push_node(Node::find_check);
		context->start_jump_forward();
		context->push_node(Node::jump);
		context->resolve_jump_backward();
		context->resolve_jump_forward();
		context->resolve_case_label($3 + $4 + $5);
	}
	| case_rule in_token case_constant_rule tpl_dot_token case_constant_rule dbldot_token {
		context->push_node(Node::exclusive_range_op);
		context->start_jump_backward();
		context->push_node(Node::find_next);
		context->push_node(Node::find_check);
		context->start_jump_forward();
		context->push_node(Node::jump);
		context->resolve_jump_backward();
		context->resolve_jump_forward();
		context->resolve_case_label($3 + $4 + $5);
	}
	| case_rule in_token case_constant_list_rule case_constant_list_end_rule dbldot_token {
		context->start_jump_backward();
		context->push_node(Node::find_next);
		context->push_node(Node::find_check);
		context->start_jump_forward();
		context->push_node(Node::jump);
		context->resolve_jump_backward();
		context->resolve_jump_forward();
		context->resolve_case_label($3 + $4);
	}
	| case_rule in_token case_constant_rule dbldot_token {
		context->push_node(Node::find_op);
		context->push_node(Node::find_init);
		context->start_jump_backward();
		context->push_node(Node::find_next);
		context->push_node(Node::find_check);
		context->start_jump_forward();
		context->push_node(Node::jump);
		context->resolve_jump_backward();
		context->resolve_jump_forward();
		context->resolve_case_label($3);
	}
	| case_rule in_token case_symbol_rule dbldot_token {
		context->push_node(Node::find_op);
		context->push_node(Node::find_init);
		context->start_jump_backward();
		context->push_node(Node::find_next);
		context->push_node(Node::find_check);
		context->start_jump_forward();
		context->push_node(Node::jump);
		context->resolve_jump_backward();
		context->resolve_jump_forward();
		context->resolve_case_label($3);
	}
	| case_rule is_token case_constant_rule dbldot_token {
		context->push_node(Node::is_op);
		context->resolve_case_label($3);
	}
	| case_rule is_token case_symbol_rule dbldot_token {
		context->push_node(Node::is_op);
		context->resolve_case_label($3);
	}
	| case_rule case_constant_rule dbldot_token {
		context->push_node(Node::eq_op);
		context->resolve_case_label($2);
	}
	| case_rule case_symbol_rule dbldot_token {
		context->push_node(Node::eq_op);
		context->resolve_case_label($2);
	};

default_rule:
	default_token dbldot_token {
		context->set_default_label();
	};

case_list_rule:
	line_end_token {
		context->commit_line();
	}
	| case_label_rule stmt_list_rule
	| case_list_rule case_label_rule stmt_list_rule
	| default_rule stmt_list_rule
	| case_list_rule default_rule stmt_list_rule;

while_cond_expr_rule:
	while_expr_rule expr_rule {
		context->resolve_condition();
		context->push_node(Node::jump_zero);
		context->start_jump_forward();
		context->open_block(BuildContext::conditional_loop_type);
	}
	| while_expr_rule find_rule {
		context->resolve_condition();
		context->push_node(Node::jump_zero);
		context->start_jump_forward();
		context->open_block(BuildContext::conditional_loop_type);
	};

while_cond_rule:
    while_rule expr_rule {
		context->resolve_condition();
		context->push_node(Node::jump_zero);
		context->start_jump_forward();
		context->open_block(BuildContext::conditional_loop_type);
	}
	| while_rule find_rule {
		context->resolve_condition();
		context->push_node(Node::jump_zero);
		context->start_jump_forward();
		context->open_block(BuildContext::conditional_loop_type);
	};

while_expr_rule:
	while_token {
		context->open_generator_expression();
		context->start_jump_backward();
		context->start_condition();
	};

while_rule:
	while_token {
		context->start_jump_backward();
		context->start_condition();
	};

find_rule:
	expr_rule in_token find_init_rule {
		context->start_jump_backward();
		context->push_node(Node::find_next);
		context->push_node(Node::find_check);
		context->start_jump_forward();
		context->push_node(Node::jump);
		context->resolve_jump_backward();
		context->resolve_jump_forward();
	}
	| expr_rule exclamation_token in_token find_init_rule {
		context->start_jump_backward();
		context->push_node(Node::find_next);
		context->push_node(Node::find_check);
		context->start_jump_forward();
		context->push_node(Node::jump);
		context->resolve_jump_backward();
		context->resolve_jump_forward();
		context->push_node(Node::not_op);
	};

find_init_rule:
	expr_rule {
		context->push_node(Node::find_op);
		context->push_node(Node::find_init);
	};

for_cond_expr_rule:
	for_expr_rule open_parenthesis_token range_init_rule range_next_rule range_cond_rule close_parenthesis_token {
		context->resolve_condition();
		context->open_block(BuildContext::custom_range_loop_type);
	}
	| for_iterator_in_expr_rule expr_rule {
		context->push_node(Node::in_op);
		context->push_node(Node::range_init);
		context->resolve_condition();
		context->push_node(Node::jump);
		context->start_jump_forward();
		context->start_jump_backward();
		context->push_node(Node::range_next);
		context->resolve_jump_forward();
		context->push_node(Node::range_iterator_check);
		context->start_jump_forward();
		context->open_block(BuildContext::range_loop_type);
	}
	| for_in_expr_rule expr_rule {
		context->push_node(Node::in_op);
		context->push_node(Node::range_init);
		context->resolve_condition();
		context->push_node(Node::jump);
		context->start_jump_forward();
		context->start_jump_backward();
		context->push_node(Node::range_next);
		context->resolve_jump_forward();
		context->push_node(Node::range_check);
		context->start_jump_forward();
		context->open_block(BuildContext::range_loop_type);
	};

for_cond_rule:
	for_rule open_parenthesis_token range_init_rule range_next_rule range_cond_rule close_parenthesis_token {
		context->resolve_condition();
		context->open_block(BuildContext::custom_range_loop_type);
	}
	| for_iterator_in_rule expr_rule {
		context->push_node(Node::in_op);
		context->push_node(Node::range_init);
		context->resolve_condition();
		context->push_node(Node::jump);
		context->start_jump_forward();
		context->start_jump_backward();
		context->push_node(Node::range_next);
		context->resolve_jump_forward();
		context->push_node(Node::range_iterator_check);
		context->start_jump_forward();
		context->open_block(BuildContext::range_loop_type);
	}
	| for_in_rule expr_rule {
		context->push_node(Node::in_op);
		context->push_node(Node::range_init);
		context->resolve_condition();
		context->push_node(Node::jump);
		context->start_jump_forward();
		context->start_jump_backward();
		context->push_node(Node::range_next);
		context->resolve_jump_forward();
		context->push_node(Node::range_check);
		context->start_jump_forward();
		context->open_block(BuildContext::range_loop_type);
	};

for_expr_rule:
	for_token {
		context->open_generator_expression();
		context->start_range_loop();
	};

for_rule:
	for_token {
		context->start_range_loop();
	};

for_in_expr_rule:
	for_expr_rule ident_rule in_token {
		context->resolve_range_loop();
		context->start_condition();
	};

for_in_rule:
	for_rule ident_rule in_token {
		context->resolve_range_loop();
		context->start_condition();
	};

for_iterator_in_expr_rule:
	for_expr_rule ident_iterator_item_rule ident_iterator_end_rule in_token {
		context->resolve_range_loop();
		context->start_condition();
	}
	| for_expr_rule create_ident_iterator_rule in_token {
		context->resolve_range_loop();
		context->start_condition();
	};

for_iterator_in_rule:
	for_rule ident_iterator_item_rule ident_iterator_end_rule in_token {
		context->resolve_range_loop();
		context->start_condition();
	}
	| for_rule create_ident_iterator_rule in_token {
		context->resolve_range_loop();
		context->start_condition();
	};

range_init_rule:
	expr_rule comma_token {
		context->push_node(Node::unload_reference);
		context->push_node(Node::jump);
		context->start_jump_forward();
		context->start_jump_backward();
		context->resolve_range_loop();
		context->start_condition();
	};

range_next_rule:
	expr_rule comma_token {
		context->push_node(Node::unload_reference);
		context->resolve_jump_forward();
	};

range_cond_rule:
	expr_rule {
		context->push_node(Node::jump_zero);
		context->start_jump_forward();
	};

return_rule:
	return_token {
		if (!context->is_in_function()) {
			context->parse_error("unexpected 'return' statement outside of function");
			YYERROR;
		}
		context->prepare_return();
	};

start_hash_rule:
	open_brace_token {
		context->push_node(Node::alloc_hash);
		context->start_call();
	};

stop_hash_rule:
	close_brace_token {
		context->push_node(Node::create_hash);
		context->resolve_call();
	};

hash_item_rule:
	hash_item_rule separator_rule expr_rule dbldot_token expr_rule {
		context->add_to_call();
	}
	| expr_rule dbldot_token expr_rule {
		context->add_to_call();
	};

start_array_rule:
	open_bracket_token {
		context->push_node(Node::alloc_array);
		context->start_call();
	};

stop_array_rule:
	close_bracket_token {
		context->push_node(Node::create_array);
		context->resolve_call();
	};

array_item_list_rule:
	array_item_list_rule separator_rule array_item_rule
	| array_item_rule;

array_item_rule:
	expr_rule {
		context->add_to_call();
	}
	| asterisk_token expr_rule {
		context->push_node(Node::in_op);
		context->push_node(Node::load_extra_arguments);
	}
	| generator_expr_rule {
		context->push_node(Node::load_extra_arguments);
	};

iterator_item_rule:
	iterator_item_rule expr_rule separator_rule {
		context->add_to_call();
	}
	| expr_rule separator_rule {
		context->push_node(Node::alloc_iterator);
		context->start_call();
		context->add_to_call();
	}
	| iterator_item_rule asterisk_token expr_rule separator_rule {
		context->push_node(Node::in_op);
		context->push_node(Node::load_extra_arguments);
	}
	| asterisk_token expr_rule separator_rule {
		context->push_node(Node::alloc_iterator);
		context->start_call();
		context->push_node(Node::in_op);
		context->push_node(Node::load_extra_arguments);
	};

iterator_end_rule:
	expr_rule {
		context->push_node(Node::create_iterator);
		context->add_to_call();
		context->resolve_call();
	}
	| {
		context->push_node(Node::create_iterator);
		context->resolve_call();
	};

ident_iterator_item_rule:
	ident_iterator_item_rule ident_rule separator_rule {
		context->add_to_call();
	}
	| ident_rule separator_rule {
		context->push_node(Node::alloc_iterator);
		context->start_call();
		context->add_to_call();
	};

ident_iterator_end_rule:
	ident_rule {
		context->push_node(Node::create_iterator);
		context->add_to_call();
		context->resolve_call();
	}
	| {
		context->push_node(Node::create_iterator);
		context->resolve_call();
	};

let_modifier_rule:
    let_token {
	    context->start_modifiers(Reference::standard);
	};

create_ident_iterator_rule:
	let_token modifier_rule create_ident_iterator_scoped_item_rule create_ident_iterator_scoped_end_rule
	| let_modifier_rule create_ident_iterator_scoped_item_rule create_ident_iterator_scoped_end_rule
	| modifier_rule create_ident_iterator_item_rule create_ident_iterator_end_rule;

create_ident_iterator_scoped_item_rule:
	create_ident_iterator_scoped_item_rule symbol_token comma_token {
		const int index = context->create_fast_scoped_symbol_index($2);
		if (index != -1) {
			context->push_node(Node::create_fast);
			context->push_node($2.c_str());
			context->push_node(index);
			context->push_node(context->get_modifiers());
		}
		else {
			context->push_node(Node::create_symbol);
			context->push_node($2.c_str());
			context->push_node(context->get_modifiers());
		}
		context->add_to_call();
	}
	| open_parenthesis_token symbol_token comma_token {
		context->push_node(Node::alloc_iterator);
		context->start_call();
		const int index = context->create_fast_scoped_symbol_index($2);
		if (index != -1) {
			context->push_node(Node::create_fast);
			context->push_node($2.c_str());
			context->push_node(index);
			context->push_node(context->get_modifiers());
		}
		else {
			context->push_node(Node::create_symbol);
			context->push_node($2.c_str());
			context->push_node(context->get_modifiers());
		}
		context->add_to_call();
	};

create_ident_iterator_scoped_end_rule:
	symbol_token close_parenthesis_token {
		const int index = context->create_fast_scoped_symbol_index($1);
		if (index != -1) {
			context->push_node(Node::create_fast);
			context->push_node($1.c_str());
			context->push_node(index);
			context->push_node(context->retrieve_modifiers());
		}
		else {
			context->push_node(Node::create_symbol);
			context->push_node($1.c_str());
			context->push_node(context->retrieve_modifiers());
		}
		context->push_node(Node::create_iterator);
		context->add_to_call();
		context->resolve_call();
	};

create_ident_iterator_item_rule:
	create_ident_iterator_item_rule symbol_token comma_token {
		const int index = context->create_fast_symbol_index($2);
		if (index != -1) {
			context->push_node(Node::create_fast);
			context->push_node($2.c_str());
			context->push_node(index);
			context->push_node(context->get_modifiers());
		}
		else {
			context->push_node(Node::create_symbol);
			context->push_node($2.c_str());
			context->push_node(context->get_modifiers());
		}
		context->add_to_call();
	}
	| open_parenthesis_token symbol_token comma_token {
		context->push_node(Node::alloc_iterator);
		context->start_call();
		const int index = context->create_fast_symbol_index($2);
		if (index != -1) {
			context->push_node(Node::create_fast);
			context->push_node($2.c_str());
			context->push_node(index);
			context->push_node(context->get_modifiers());
		}
		else {
			context->push_node(Node::create_symbol);
			context->push_node($2.c_str());
			context->push_node(context->get_modifiers());
		}
		context->add_to_call();
	};

create_ident_iterator_end_rule:
	symbol_token close_parenthesis_token {
		const int index = context->create_fast_symbol_index($1);
		if (index != -1) {
			context->push_node(Node::create_fast);
			context->push_node($1.c_str());
			context->push_node(index);
			context->push_node(context->retrieve_modifiers());
		}
		else {
			context->push_node(Node::create_symbol);
			context->push_node($1.c_str());
			context->push_node(context->retrieve_modifiers());
		}
		context->push_node(Node::create_iterator);
		context->add_to_call();
		context->resolve_call();
	};

print_rule:
	print_token {
		context->push_node(Node::load_constant);
		context->push_node(Compiler::make_data("1", Compiler::data_number_hint));
		context->open_printer();
		context->open_block(BuildContext::print_type);
	}
	| print_token open_parenthesis_token expr_rule close_parenthesis_token {
		context->open_printer();
		context->open_block(BuildContext::print_type);
	};

expr_rule:
	expr_rule equal_token generator_expr_rule {
		context->push_node(Node::move_op);
	}
	| expr_rule equal_token expr_rule {
		context->push_node(Node::move_op);
	}
	| expr_rule dbldot_equal_token generator_expr_rule {
		context->push_node(Node::copy_op);
	}
	| expr_rule dbldot_equal_token expr_rule {
		context->push_node(Node::copy_op);
	}
	| expr_rule plus_token expr_rule {
		context->push_node(Node::add_op);
	}
	| expr_rule minus_token expr_rule {
		context->push_node(Node::sub_op);
	}
	| expr_rule asterisk_token expr_rule {
		context->push_node(Node::mul_op);
	}
	| expr_rule slash_token expr_rule {
		context->push_node(Node::div_op);
	}
	| expr_rule percent_token expr_rule {
		context->push_node(Node::mod_op);
	}
	| expr_rule dbl_asterisk_token expr_rule {
		context->push_node(Node::pow_op);
	}
	| expr_rule is_token expr_rule {
		context->push_node(Node::is_op);
	}
	| expr_rule dbl_equal_token expr_rule {
		context->push_node(Node::eq_op);
	}
	| expr_rule exclamation_equal_token expr_rule {
		context->push_node(Node::ne_op);
	}
	| expr_rule left_angled_token expr_rule {
		context->push_node(Node::lt_op);
	}
	| expr_rule right_angled_token expr_rule {
		context->push_node(Node::gt_op);
	}
	| expr_rule left_angled_equal_token expr_rule {
		context->push_node(Node::le_op);
	}
	| expr_rule right_angled_equal_token expr_rule {
		context->push_node(Node::ge_op);
	}
	| expr_rule dbl_left_angled_token expr_rule {
		context->push_node(Node::shift_left_op);
	}
	| expr_rule dbl_right_angled_token expr_rule {
		context->push_node(Node::shift_right_op);
	}
	| expr_rule dot_dot_token expr_rule {
		context->push_node(Node::inclusive_range_op);
	}
	| expr_rule tpl_dot_token expr_rule {
		context->push_node(Node::exclusive_range_op);
	}
	| dbl_plus_token expr_rule %prec prefix_dbl_plus_token {
		context->push_node(Node::inc_op);
	}
	| dbl_minus_token expr_rule %prec prefix_dbl_minus_token {
		context->push_node(Node::dec_op);
	}
	| expr_rule dbl_plus_token {
	    context->push_node(Node::clone_reference);
		context->push_node(Node::inc_op);
		context->push_node(Node::unload_reference);
	}
	| expr_rule dbl_minus_token {
	    context->push_node(Node::clone_reference);
		context->push_node(Node::dec_op);
		context->push_node(Node::unload_reference);
	}
	| exclamation_token expr_rule {
		context->push_node(Node::not_op);
	}
	| expr_rule dbl_pipe_token {
		context->push_node(Node::or_pre_check);
		context->start_jump_forward();
	} expr_rule {
		context->push_node(Node::or_op);
		context->resolve_jump_forward();
	}
	| expr_rule dbl_amp_token {
		context->push_node(Node::and_pre_check);
		context->start_jump_forward();
	} expr_rule {
		context->push_node(Node::and_op);
		context->resolve_jump_forward();
	}
	| expr_rule pipe_token expr_rule {
		context->push_node(Node::bor_op);
	}
	| expr_rule amp_token expr_rule {
		context->push_node(Node::band_op);
	}
	| expr_rule caret_token expr_rule {
		context->push_node(Node::xor_op);
	}
	| tilde_token expr_rule {
		context->push_node(Node::compl_op);
	}
	| plus_token expr_rule %prec prefix_plus_token {
		context->push_node(Node::pos_op);
	}
	| minus_token expr_rule %prec prefix_minus_token {
		context->push_node(Node::neg_op);
	}
	| typeof_token expr_rule {
		context->push_node(Node::typeof_op);
	}
	| membersof_token expr_rule {
		context->push_node(Node::membersof_op);
	}
	| defined_token defined_symbol_rule {
		context->push_node(Node::check_defined);
	}
	| expr_rule open_bracket_token expr_rule close_bracket_equal_token expr_rule {
		context->push_node(Node::subscript_move_op);
	}
	| expr_rule subscript_rule
	| member_ident_rule
	| ident_rule call_args_rule
	| def_rule call_args_rule
	| expr_rule subscript_rule call_args_rule
	| expr_rule dot_token call_member_args_rule
	| open_parenthesis_token expr_rule close_parenthesis_token call_args_rule
	| expr_rule plus_equal_token {
		context->push_node(Node::reload_reference);
	} expr_rule {
		context->push_node(Node::add_op);
		context->push_node(Node::move_op);
	}
	| expr_rule minus_equal_token {
		context->push_node(Node::reload_reference);
	} expr_rule {
		context->push_node(Node::sub_op);
		context->push_node(Node::move_op);
	}
	| expr_rule asterisk_equal_token {
		context->push_node(Node::reload_reference);
	} expr_rule {
		context->push_node(Node::mul_op);
		context->push_node(Node::move_op);
	}
	| expr_rule slash_equal_token {
		context->push_node(Node::reload_reference);
	} expr_rule {
		context->push_node(Node::div_op);
		context->push_node(Node::move_op);
	}
	| expr_rule percent_equal_token {
		context->push_node(Node::reload_reference);
	} expr_rule {
		context->push_node(Node::mod_op);
		context->push_node(Node::move_op);
	}
	| expr_rule dbl_left_angled_equal_token {
		context->push_node(Node::reload_reference);
	} expr_rule {
		context->push_node(Node::shift_left_op);
		context->push_node(Node::move_op);
	}
	| expr_rule dbl_right_angled_equal_token {
		context->push_node(Node::reload_reference);
	} expr_rule {
		context->push_node(Node::shift_right_op);
		context->push_node(Node::move_op);
	}
	| expr_rule amp_equal_token {
		context->push_node(Node::reload_reference);
	} expr_rule {
		context->push_node(Node::band_op);
		context->push_node(Node::move_op);
	}
	| expr_rule pipe_equal_token {
		context->push_node(Node::reload_reference);
	} expr_rule {
		context->push_node(Node::bor_op);
		context->push_node(Node::move_op);
	}
	| expr_rule caret_equal_token {
		context->push_node(Node::reload_reference);
	} expr_rule {
		context->push_node(Node::xor_op);
		context->push_node(Node::move_op);
	}
	| expr_rule equal_tilde_token expr_rule {
		context->push_node(Node::regex_match);
	}
	| expr_rule exclamation_tilde_token expr_rule {
		context->push_node(Node::regex_unmatch);
	}
	| expr_rule tpl_equal_token expr_rule {
		context->push_node(Node::strict_eq_op);
	}
	| expr_rule exclamation_dbl_equal_token expr_rule {
		context->push_node(Node::strict_ne_op);
	}
	| expr_rule question_token {
		context->push_node(Node::jump_zero);
		context->start_jump_forward();
	} expr_rule dbldot_token {
		context->push_node(Node::jump);
		context->start_jump_forward();
		context->shift_jump_forward();
		context->resolve_jump_forward();
	} expr_rule {
		context->resolve_jump_forward();
	}
	| open_parenthesis_token close_parenthesis_token {
		context->push_node(Node::alloc_iterator);
		context->start_call();
		context->push_node(Node::create_iterator);
		context->resolve_call();
	}
	| open_parenthesis_token expr_rule close_parenthesis_token
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
		context->push_node(Node::subscript_op);
	};

call_args_rule:
	call_arg_start_rule call_arg_list_rule call_arg_stop_rule
	| call_args_rule call_arg_start_rule call_arg_list_rule call_arg_stop_rule;

call_member_args_rule:
	call_member_arg_start_rule call_arg_list_rule call_member_arg_stop_rule
	| call_member_args_rule call_arg_start_rule call_arg_list_rule call_arg_stop_rule;

call_arg_start_rule:
	open_parenthesis_token {
		context->push_node(Node::init_call);
		context->start_call();
	};

call_arg_stop_rule:
	close_parenthesis_token {
		context->push_node(Node::call);
		context->resolve_call();
	};

call_member_arg_start_rule:
	symbol_token open_parenthesis_token {
		context->push_node(Node::init_member_call);
		context->push_node($1.c_str());
		context->start_call();
	}
	| operator_desc_rule open_parenthesis_token {
		context->push_node(Node::init_operator_call);
		context->push_node(context->retrieve_operator());
		context->start_call();
	}
	| var_symbol_rule open_parenthesis_token {
		context->push_node(Node::init_var_member_call);
		context->start_call();
	};

call_member_arg_stop_rule:
	close_parenthesis_token {
		context->push_node(Node::call_member);
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
	| asterisk_token expr_rule {
		context->push_node(Node::in_op);
		context->push_node(Node::load_extra_arguments);
	};

def_rule:
	def_start_rule def_capture_rule def_args_rule stmt_bloc_rule {
		if (context->is_in_generator()) {
			context->push_node(Node::exit_generator);
		}
		else if (!context->has_returned()) {
			context->push_node(Node::load_constant);
			context->push_node(Compiler::make_none());
			context->push_node(Node::exit_call);
		}
		context->resolve_jump_forward();
		context->save_definition();
	}
	| def_start_rule def_capture_rule def_no_args_rule stmt_bloc_rule {
		if (context->is_in_generator()) {
			context->push_node(Node::exit_generator);
		}
		else if (!context->has_returned()) {
			context->push_node(Node::load_constant);
			context->push_node(Compiler::make_none());
			context->push_node(Node::exit_call);
		}
		context->resolve_jump_forward();
		context->save_definition();
	};

def_start_rule:
	def_token {
		context->push_node(Node::jump);
		context->start_jump_forward();
		context->start_definition();
	};

def_capture_rule:
	def_capture_start_rule def_capture_list_rule def_capture_stop_rule
	| ;

def_capture_start_rule:
	open_bracket_token {
		context->start_capture();
	};

def_capture_stop_rule:
	close_bracket_token {
		context->resolve_capture();
	};

def_capture_list_rule:
	symbol_token equal_token expr_rule separator_rule def_capture_list_rule {
		if (!context->capture_as($1)) {
			YYERROR;
		}
	}
	| symbol_token equal_token expr_rule {
		if (!context->capture_as($1)) {
			YYERROR;
		}
	}
	| symbol_token separator_rule def_capture_list_rule {
		if (!context->capture($1)) {
			YYERROR;
		}
	}
	| symbol_token {
		if (!context->capture($1)) {
			YYERROR;
		}
	}
	| tpl_dot_token {
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
	open_parenthesis_token;

def_arg_stop_rule:
	close_parenthesis_token {
		if (!context->save_parameters()) {
			YYERROR;
		}
	};

def_arg_list_rule:
	def_arg_rule separator_rule def_arg_list_rule
	| def_arg_rule
	| ;

def_arg_rule:
	symbol_token {
		if (!context->add_parameter($1)) {
			YYERROR;
		}
	}
	| symbol_token equal_token expr_rule {
		if (!context->add_definition_signature()) {
			YYERROR;
		}
		if (!context->add_parameter($1)) {
			YYERROR;
		}
	}
	| modifier_rule symbol_token {
		if (!context->add_parameter($2, context->retrieve_modifiers())) {
			YYERROR;
		}
	}
	| modifier_rule symbol_token equal_token expr_rule {
		if (!context->add_definition_signature()) {
			YYERROR;
		}
		if (!context->add_parameter($2, context->retrieve_modifiers())) {
			YYERROR;
		}
	}
	| tpl_dot_token {
		if (!context->set_variadic()) {
			YYERROR;
		}
	};

member_ident_rule:
	expr_rule dot_token symbol_token {
		context->push_node(Node::load_member);
		context->push_node($3.c_str());
	}
	| expr_rule dot_token operator_desc_rule {
		context->push_node(Node::load_operator);
		context->push_node(context->retrieve_operator());
	}
	| expr_rule dot_token var_symbol_rule {
		context->push_node(Node::load_var_member);
	};

defined_symbol_rule:
	symbol_token {
		context->push_node(Node::find_defined_symbol);
		context->push_node($1.c_str());
	}
	| defined_symbol_rule dot_token symbol_token {
		context->push_node(Node::find_defined_member);
		context->push_node($3.c_str());
	}
	| var_symbol_rule {
		context->push_node(Node::find_defined_var_symbol);
		context->push_node($1.c_str());
	}
	| defined_symbol_rule dot_token var_symbol_rule {
		context->push_node(Node::find_defined_var_member);
		context->push_node($3.c_str());
	}
	| constant_rule {
		context->push_node(Node::load_constant);
		if (Data *data = Compiler::make_data($1, Compiler::data_unknown_hint)) {
			context->push_node(data);
		}
		else {
			error("token '" + $1 + "' is not a valid constant");
			YYERROR;
		}
	};

ident_rule:
	constant_rule {
		context->push_node(Node::load_constant);
		if (Data *data = Compiler::make_data($1, Compiler::data_unknown_hint)) {
			context->push_node(data);
		}
		else {
			error("token '" + $1 + "' is not a valid constant");
			YYERROR;
		}
	}
	| lib_token {
		context->push_node(Node::create_lib);
	}
	| var_symbol_rule {
		context->push_node(Node::load_var_symbol);
	}
	| symbol_token {
		const int index = context->fast_symbol_index($1);
		if (index != -1) {
			context->push_node(Node::load_fast);
			context->push_node($1.c_str());
			context->push_node(index);
		}
		else {
			context->push_node(Node::load_symbol);
			context->push_node($1.c_str());
		}
	}
	| let_token symbol_token {
		const int index = context->create_fast_scoped_symbol_index($2);
		if (index != -1) {
			context->push_node(Node::create_fast);
			context->push_node($2.c_str());
			context->push_node(index);
			context->push_node(Reference::standard);
		}
		else {
			context->push_node(Node::create_symbol);
			context->push_node($2.c_str());
			context->push_node(Reference::standard);
		}
	}
	| modifier_rule symbol_token {
		const int index = context->create_fast_symbol_index($2);
		if (index != -1) {
			context->push_node(Node::create_fast);
			context->push_node($2.c_str());
			context->push_node(index);
			context->push_node(context->retrieve_modifiers());
		}
		else {
			context->push_node(Node::create_symbol);
			context->push_node($2.c_str());
			context->push_node(context->retrieve_modifiers());
		}
	}
	| let_token modifier_rule symbol_token {
		const int index = context->create_fast_scoped_symbol_index($3);
		if (index != -1) {
			context->push_node(Node::create_fast);
			context->push_node($3.c_str());
			context->push_node(index);
			context->push_node(context->retrieve_modifiers());
		}
		else {
			context->push_node(Node::create_symbol);
			context->push_node($3.c_str());
			context->push_node(context->retrieve_modifiers());
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
		$$ = $1 + context->lexer.read_regex();
	} slash_token {
		$$ = $2 + $1;
	};

var_symbol_rule:
	dollar_token open_brace_token expr_rule close_brace_token;

modifier_rule:
	var_token {
		context->start_modifiers(Reference::standard);
	}
	| dollar_token {
		context->start_modifiers(Reference::const_address);
	}
	| percent_token {
		context->start_modifiers(Reference::const_value);
	}
	| const_token {
		context->start_modifiers(Reference::const_address | Reference::const_value);
	}
	| at_token {
		context->start_modifiers(Reference::global);
	}
	| modifier_rule var_token {
		context->add_modifiers(Reference::standard);
	}
	| modifier_rule dollar_token {
		context->add_modifiers(Reference::const_address);
	}
	| modifier_rule percent_token {
		context->add_modifiers(Reference::const_value);
	}
	| modifier_rule const_token {
		context->add_modifiers(Reference::const_address | Reference::const_value);
	}
	| modifier_rule at_token {
		context->add_modifiers(Reference::global);
	};

separator_rule:
	comma_token | separator_rule line_end_token {
		context->commit_line();
	};

empty_lines_rule:
	line_end_token {
		context->commit_line();
	}
	| empty_lines_rule line_end_token {
		context->commit_line();
	};

%%

void parser::error(const std::string &msg) {
	context->parse_error(msg.c_str());
}

int BuildContext::next_token(std::string *token) {

	if (lexer.at_end()) {
		return parser::token::file_end_token;
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
