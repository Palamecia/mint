%{
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

#include "mint/compiler/build_tools.h"
#include "mint/compiler/compiler.h"
#include <memory>

#define yylex context.next_token

using namespace mint;

%}

%define api.namespace {mint}
%define api.value.type {std::string}
%parse-param {mint::BuildContext& context}

%code requires {
namespace mint {
class BuildContext;
}
}

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
%left dbl_equal_token exclamation_equal_token is_token in_token equal_tilde_token exclamation_tilde_token tpl_equal_token exclamation_dbl_equal_token
%left dbl_dot_token tpl_dot_token
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
		context.reduce_module_stmt_list();
		fflush(stdout); 
		YYACCEPT;
	}
	| file_end_token {
		context.reduce_module_stmt_list();
		fflush(stdout); 
		YYACCEPT;
	};

stmt_list_rule:
	stmt_list_rule stmt_rule
	| stmt_rule;

stmt_rule:
    load_token module_path_rule line_end_token {
		context.reduce_load_statement($2);
		context.commit_line();
	}
	| try_rule stmt_bloc_rule {
		context.reduce_try_bloc();
	}
	| try_bloc_rule catch_rule stmt_bloc_rule {
		context.reduce_catch_bloc();
	}
	| if_cond_rule stmt_bloc_rule {
		context.reduce_if_bloc();
	}
	| if_bloc_rule else_rule stmt_bloc_rule {
		context.reduce_else_bloc();
	}
	| if_bloc_rule elif_bloc_rule {
		context.reduce_elif_bloc();
	}
	| if_bloc_rule elif_bloc_rule else_rule stmt_bloc_rule {
		context.reduce_else_bloc();
	}
	| switch_cond_rule open_brace_token case_list_rule close_brace_token {
		context.reduce_switch_bloc();
	}
	| while_cond_rule stmt_bloc_rule {
		context.reduce_while_bloc();
	}
	| for_cond_rule stmt_bloc_rule {
		context.reduce_for_bloc();
	}
	| break_token line_end_token {
		context.reduce_break_statement();
		context.commit_line();
	}
	| continue_token line_end_token {
		context.reduce_continue_statement();
		context.commit_line();
	}
	| print_token open_parenthesis_token expr_rule print_stmt_sep_rule expr_rule close_parenthesis_token line_end_token {
		context.reduce_print_to_stream_statement();
		context.commit_line();
	}
	| print_token open_parenthesis_token expr_rule close_parenthesis_token line_end_token {
		context.reduce_print_statement();
		context.commit_line();
	}
	| print_token print_bloc_target_rule stmt_bloc_rule {
		context.reduce_print_bloc();
	}
	| yield_token generator_expr_rule line_end_token {
		context.reduce_yield_generator_expression_statement();
		context.commit_line();
	}
	| yield_token expr_rule line_end_token {
		context.reduce_yield_statement();
		context.commit_line();
	}
	| return_rule generator_expr_rule line_end_token {
		context.reduce_return_generator_expression_statement();
		context.commit_line();
	}
	| return_rule expr_rule line_end_token {
		context.reduce_return_statement();
		context.commit_line();
	}
	| raise_token expr_rule line_end_token {
		context.reduce_raise_statement();
		context.commit_line();
	}
	| raise_token in_token expr_rule line_end_token {
		context.reduce_raise_in_statement();
		context.commit_line();
	}
	| raise_token line_end_token {
		context.reduce_reraise_statement();
		context.commit_line();
	}
	| exit_token expr_rule line_end_token {
		context.reduce_exit_with_code_statement();
		context.commit_line();
	}
	| exit_token line_end_token {
		context.reduce_exit_statement();
		context.commit_line();
	}
	| ident_iterator_item_rule ident_iterator_end_rule equal_token expr_rule line_end_token {
		context.reduce_identifier_iterator_assignment_expression();
		context.commit_line();
	}
	| ident_iterator_item_rule ident_iterator_end_rule equal_token generator_expr_rule line_end_token {
		context.reduce_identifier_iterator_assignment_generator_expression();
		context.commit_line();
	}
	| create_ident_iterator_rule equal_token expr_rule line_end_token {
		context.reduce_create_identifier_iterator_assignment_expression();
		context.commit_line();
	}
	| create_ident_iterator_rule equal_token generator_expr_rule line_end_token {
		context.reduce_create_identifier_iterator_assignment_generator_expression();
		context.commit_line();
	}
	| expr_rule line_end_token {
		context.reduce_expression_statement();
		context.commit_line();
	}
	| modifier_rule def_start_rule def_capture_rule symbol_token def_args_rule stmt_bloc_rule {
		context.reduce_function_definition_with_modifiers($4);
	}
	| def_start_rule def_capture_rule symbol_token def_args_rule stmt_bloc_rule {
		context.reduce_function_definition($3);
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
		context.reduce_package_declaration($2);
	};

package_block_rule:
    package_rule open_brace_token stmt_list_rule close_brace_token {
		context.reduce_package_block();
	};

class_rule:
    type_modifier_rule class_token symbol_token {
		context.reduce_class_declaration_with_modifiers($3);
	}
	| class_token symbol_token {
		context.reduce_class_declaration($2);
	};

parent_rule:
    colon_token parent_list_rule
	| ;

parent_list_rule:
	parent_ident_rule {
		context.reduce_parent_class();
	}
	| parent_list_rule comma_token parent_ident_rule {
		context.reduce_add_parent_class();
	};

parent_ident_rule:
    symbol_token {
		context.reduce_parent_class_name($1);
	}
	| parent_ident_rule dot_token symbol_token {
		context.reduce_qualified_parent_class_name($3);
	};

class_desc_rule:
	class_rule parent_rule desc_bloc_rule {
		context.reduce_class_definition();
	};

member_class_rule:
	class_token symbol_token {
		context.reduce_nested_class_declaration($2);
	
	}
	| member_type_modifier_rule class_token symbol_token {
		context.reduce_modified_nested_class_declaration($3);
	
	};

member_class_desc_rule:
	member_class_rule parent_rule desc_bloc_rule {
		context.reduce_nested_class_definition();
	
	};

member_enum_rule:
	enum_token symbol_token {
		context.reduce_nested_enum_declaration($2);
	
	}
	| member_type_modifier_rule enum_token symbol_token {
		context.reduce_modified_nested_enum_declaration($3);
	
	};

member_enum_desc_rule:
	member_enum_rule enum_block_rule {
		context.reduce_nested_enum_definition();
	
	};

member_type_modifier_rule:
    plus_token {
		context.reduce_public_modifier();
	}
	| sharp_token {
		context.reduce_protected_modifier();
	}
	| minus_token {
		context.reduce_private_modifier();
	}
	| tilde_token {
		context.reduce_package_modifier();
	}
	| at_token {
		context.reduce_global_modifier();
	};

desc_bloc_rule:
    open_brace_token desc_list_rule close_brace_token
	| open_brace_token close_brace_token;

desc_list_rule:
	desc_list_rule desc_rule
	| desc_rule;

desc_rule:
    member_desc_rule line_end_token {
		context.reduce_member_without_initializer($1);
		context.commit_line();
	
	}
	| member_desc_rule equal_token constant_token line_end_token {
		context.reduce_member_with_constant_initializer($1, $3);
		context.commit_line();
	
	}
	| member_desc_rule equal_token string_token line_end_token {
		context.reduce_member_with_string_initializer($1, $3);
		context.commit_line();
	
	}
	| member_desc_rule equal_token regex_rule line_end_token {
		context.reduce_member_with_regex_initializer($1, $3);
		context.commit_line();
	
	}
	| member_desc_rule equal_token regex_rule regex_rule symbol_token line_end_token {
		context.reduce_member_with_flagged_regex_initializer($1, $3, $4);
		context.commit_line();
	
	}
	| member_desc_rule equal_token number_token line_end_token {
		context.reduce_member_with_number_initializer($1, $3);
		context.commit_line();
	
	}
	| member_desc_rule equal_token open_bracket_token close_bracket_token line_end_token {
		context.reduce_member_with_empty_array_initializer($1);
		context.commit_line();
	
	}
	| member_desc_rule equal_token open_brace_token close_brace_token line_end_token {
		context.reduce_member_with_empty_hash_initializer($1);
		context.commit_line();
	
	}
	| member_desc_rule equal_token lib_token open_parenthesis_token string_token close_parenthesis_token line_end_token {
		context.reduce_member_with_library_initializer($1, $5);
		context.commit_line();
	
	}
	| member_desc_rule equal_token def_start_rule def_args_rule stmt_bloc_rule {
		context.reduce_member_function_definition($1);
	
	}
	| member_desc_rule plus_equal_token def_start_rule def_args_rule stmt_bloc_rule {
		context.reduce_member_function_update($1);
	
	}
	| def_start_rule symbol_token def_args_rule stmt_bloc_rule {
		context.reduce_named_function_definition($2);
	
	}
	| def_start_rule await_token def_args_rule stmt_bloc_rule {
		context.reduce_async_function_definition($2);
	
	}
	| def_start_rule operator_desc_rule def_args_rule stmt_bloc_rule {
		context.reduce_operator_function_definition();
	
	}
	| desc_modifier_rule def_start_rule symbol_token def_args_rule stmt_bloc_rule {
		context.reduce_modified_named_function_definition($3);
	
	}
	| desc_modifier_rule def_start_rule await_token def_args_rule stmt_bloc_rule {
		context.reduce_modified_async_function_definition($3);
	
	}
	| desc_modifier_rule def_start_rule operator_desc_rule def_args_rule stmt_bloc_rule {
		context.reduce_modified_operator_function_definition();
	
	}
	| member_class_desc_rule
	| member_enum_desc_rule
	| line_end_token {
		context.reduce_empty_descriptor_line();
		context.commit_line();
	
	};

member_desc_rule:
    symbol_token {
		context.reduce_default_modifier();
		$$ = $1;
	}
	| desc_modifier_rule symbol_token {
		$$ = $2;
	};

desc_base_modifier_rule:
	modifier_rule
	| final_token {
		context.reduce_final_modifier();
	
	}
	| override_token {
		context.reduce_override_modifier();
	
	}
	| final_token modifier_rule {
		context.reduce_add_final_modifier();
	
	}
	| override_token modifier_rule {
		context.reduce_add_override_modifier();
	
	};

desc_modifier_rule:
	desc_base_modifier_rule
	| plus_token {
		context.reduce_public_modifier();
	
	}
	| sharp_token {
		context.reduce_protected_modifier();
	
	}
	| minus_token {
		context.reduce_private_modifier();
	
	}
	| tilde_token {
		context.reduce_package_modifier();
	
	}
	| plus_token desc_base_modifier_rule {
		context.reduce_add_public_modifier();
	
	}
	| sharp_token desc_base_modifier_rule {
		context.reduce_add_protected_modifier();
	
	}
	| minus_token desc_base_modifier_rule {
		context.reduce_add_private_modifier();
	
	}
	| tilde_token desc_base_modifier_rule {
		context.reduce_add_package_modifier();
	
	};

operator_desc_rule:
    in_token {
		context.reduce_in_operator();
	
	}
	| colon_equal_token {
		context.reduce_copy_operator();
	
	}
	| dbl_pipe_token {
		context.reduce_or_operator();
	
	}
	| dbl_amp_token {
		context.reduce_and_operator();
	
	}
	| pipe_token {
		context.reduce_bor_operator();
	
	}
	| caret_token {
		context.reduce_xor_operator();
	
	}
	| amp_token {
		context.reduce_band_operator();
	
	}
	| dbl_equal_token {
		context.reduce_eq_operator();
	
	}
	| exclamation_equal_token {
		context.reduce_ne_operator();
	
	}
	| left_angled_token {
		context.reduce_lt_operator();
	
	}
	| right_angled_token {
		context.reduce_gt_operator();
	
	}
	| left_angled_equal_token {
		context.reduce_le_operator();
	
	}
	| right_angled_equal_token {
		context.reduce_ge_operator();
	
	}
	| dbl_left_angled_token {
		context.reduce_shift_left_operator();
	
	}
	| dbl_right_angled_token {
		context.reduce_shift_right_operator();
	
	}
	| plus_token {
		context.reduce_add_operator();
	
	}
	| minus_token {
		context.reduce_sub_operator();
	
	}
	| asterisk_token {
		context.reduce_mul_operator();
	
	}
	| slash_token {
		context.reduce_div_operator();
	
	}
	| percent_token {
		context.reduce_mod_operator();
	
	}
	| exclamation_token {
		context.reduce_not_operator();
	
	}
	| tilde_token {
		context.reduce_compl_operator();
	
	}
	| dbl_plus_token {
		context.reduce_inc_operator();
	
	}
	| dbl_minus_token {
		context.reduce_dec_operator();
	
	}
	| dbl_asterisk_token {
		context.reduce_pow_operator();
	
	}
	| dbl_dot_token {
		context.reduce_inclusive_range_operator();
	
	}
	| tpl_dot_token {
		context.reduce_exclusive_range_operator();
	
	}
	| open_parenthesis_token close_parenthesis_token {
		context.reduce_call_operator();
	
	}
	| open_bracket_token close_bracket_token {
		context.reduce_subscript_operator();
	
	}
	| open_bracket_token close_bracket_equal_token {
		context.reduce_subscript_move_operator();
	
	};

enum_rule:
    type_modifier_rule enum_token symbol_token {
		context.reduce_modified_enum_declaration($3);
	
	}
	| enum_token symbol_token {
		context.reduce_enum_declaration($2);
	
	};

enum_desc_rule:
	enum_rule enum_block_rule {
		context.reduce_enum_definition();
	
	};

enum_block_rule:
    open_brace_token enum_list_rule close_brace_token;

enum_list_rule:
	enum_list_rule enum_item_rule
	| enum_item_rule;

enum_item_rule:
    symbol_token equal_token number_token {
		context.reduce_enum_item_with_value($1, $3);
	
	}
	| symbol_token {
		context.reduce_enum_item_with_implicit_value($1);
	
	}
	| line_end_token {
		context.reduce_empty_enum_item();
		context.commit_line();
	
	};

type_modifier_rule:
    at_token {
		context.reduce_global_modifier();
	
	};

generator_expr_rule:
	if_cond_generator_rule generator_stmt_bloc_rule {
		context.reduce_conditional_generator_expression();
	
	}
	| open_parenthesis_token if_generator_bloc_rule else_rule generator_stmt_bloc_rule close_parenthesis_token {
		context.reduce_if_else_generator_expression();
	
	}
	| open_parenthesis_token if_generator_bloc_rule elif_generator_bloc_rule close_parenthesis_token {
		context.reduce_if_elif_generator_expression();
	
	}
	| open_parenthesis_token if_generator_bloc_rule elif_generator_bloc_rule else_rule generator_stmt_bloc_rule close_parenthesis_token {
		context.reduce_if_elif_else_generator_expression();
	
	}
	| switch_cond_generator_rule open_brace_token case_list_rule close_brace_token {
		context.reduce_switch_generator_expression();
	
	}
	| while_cond_generator_rule generator_stmt_bloc_rule {
		context.reduce_while_generator_expression();
	
	}
	| for_cond_generator_rule generator_stmt_bloc_rule {
		context.reduce_for_generator_expression();
	
	};

try_rule:
    try_token {
		context.reduce_try_keyword();
	
	};

catch_rule:
    catch_token symbol_token {
		context.reduce_catch_clause($2);
	
	};

try_bloc_rule:
	try_rule stmt_bloc_rule {
		context.reduce_try_block();
	
	};

if_bloc_rule:
	if_cond_rule stmt_bloc_rule {
		context.reduce_if_block();
	
	};

if_generator_bloc_rule:
	if_cond_generator_rule generator_stmt_bloc_rule {
		context.reduce_if_generator_block();
	
	};

elif_bloc_rule:
	elif_cond_rule stmt_bloc_rule {
		context.reduce_elif_block();
	
	}
	| elif_bloc_rule elif_cond_rule stmt_bloc_rule {
		context.reduce_add_elif_block();
	
	};

elif_generator_bloc_rule:
	elif_cond_rule generator_stmt_bloc_rule {
		context.reduce_elif_generator_block();
	
	}
	| elif_generator_bloc_rule elif_cond_rule generator_stmt_bloc_rule {
		context.reduce_add_elif_generator_block();
	
	};

stmt_bloc_rule:
    open_brace_token stmt_list_rule close_brace_token
	| open_brace_token yield_token generator_expr_rule close_brace_token {
		context.reduce_yield_generator_block();
	
	}
	| open_brace_token yield_token expr_rule close_brace_token {
		context.reduce_yield_value_block();
	
	}
	| open_brace_token return_rule generator_expr_rule close_brace_token {
		context.reduce_return_generator_block();
	
	}
	| open_brace_token return_rule expr_rule close_brace_token {
		context.reduce_return_value_block();
	
	}
	| open_brace_token raise_token expr_rule close_brace_token {
		context.reduce_raise_block();
	
	}
	| open_brace_token raise_token in_token expr_rule close_brace_token {
		context.reduce_reraise_block();
	
	}
	| open_brace_token raise_token close_brace_token {
		context.reduce_bare_raise_block();
	
	}
	| open_brace_token expr_rule close_brace_token {
		context.reduce_expression_block();
	
	}
	| open_brace_token close_brace_token;

generator_stmt_bloc_rule:
	equal_right_angled_token expr_rule {
		context.reduce_generator_expression_body();
	
	}
	| equal_right_angled_token generator_expr_rule
	| stmt_bloc_rule;

if_cond_rule:
	if_rule expr_rule {
		context.reduce_if_condition();
	
	};

if_cond_generator_rule:
	if_generator_rule expr_rule {
		context.reduce_if_generator_condition();
	
	};

elif_cond_rule:
	elif_rule expr_rule {
		context.reduce_elif_condition();
	
	};

if_rule:
    if_token {
		context.reduce_if_keyword();
	
	};

if_generator_rule:
    if_token {
		context.reduce_generator_if_keyword();
	
	};

elif_rule:
    elif_token {
		context.reduce_elif_keyword();
	
	};

else_rule:
    else_token {
		context.reduce_else_keyword();
	
	};

switch_cond_rule:
	switch_rule expr_rule {
		context.reduce_switch_condition();
	
	};

switch_cond_generator_rule:
	switch_expr_rule expr_rule {
		context.reduce_generator_switch_condition();
	
	};

switch_expr_rule:
    switch_token {
		context.reduce_generator_switch_keyword();
	
	};

switch_rule:
    switch_token {
		context.reduce_switch_keyword();
	
	};

case_rule:
    case_token {
		context.reduce_case_keyword();
	
	};

case_symbol_rule:
    symbol_token {
		$$ = context.reduce_case_symbol($1);
	
	}
	| case_symbol_rule dot_token symbol_token {
		$$ = context.reduce_qualified_case_symbol($1, $2, $3);
	
	};

case_constant_rule:
	constant_rule {
		$$ = context.reduce_case_constant($1);
	
	}
	| plus_token number_token {
		$$ = context.reduce_positive_case_number($2);
	
	}
	| minus_token number_token {
		$$ = context.reduce_negative_case_number($1, $2);
	
	};
case_constant_list_rule:
    case_constant_list_rule case_constant_rule comma_token {
		$$ = context.reduce_append_case_constant($1, $2, $3);
	
	}
	| case_constant_rule comma_token {
		$$ = context.reduce_start_case_constant_list($1, $2);
	
	};

case_constant_list_end_rule:
	case_constant_rule {
		$$ = context.reduce_finish_case_constant_list($1);
	
	}
	| {
		context.reduce_empty_case_constant_list();
	
	};

case_label_rule:
    case_rule in_token case_constant_rule dbl_dot_token case_constant_rule {
		context.reduce_inclusive_case_range_label($3, $4, $5);
	
	}
	| case_rule in_token case_constant_rule tpl_dot_token case_constant_rule {
		context.reduce_exclusive_case_range_label($3, $4, $5);
	
	}
	| case_rule in_token case_constant_list_rule case_constant_list_end_rule {
		context.reduce_case_constant_list_label($3, $4);
	
	}
	| case_rule in_token case_constant_rule {
		context.reduce_case_constant_membership_label($3);
	
	}
	| case_rule in_token case_symbol_rule {
		context.reduce_case_symbol_membership_label($3);
	
	}
	| case_rule is_token case_constant_rule {
		context.reduce_case_constant_identity_label($3);
	
	}
	| case_rule is_token case_symbol_rule {
		context.reduce_case_symbol_identity_label($3);
	
	}
	| case_rule case_constant_rule {
		context.reduce_case_constant_label($2);
	
	}
	| case_rule case_symbol_rule {
		context.reduce_case_symbol_label($2);
	
	};

default_rule:
    default_token {
		context.reduce_default_case_keyword();
	
	};

case_list_rule:
    line_end_token {
		context.reduce_empty_case_line();
		context.commit_line();
	
	}
	| case_label_rule colon_token stmt_list_rule
	| case_list_rule case_label_rule colon_token stmt_list_rule
	| default_rule colon_token stmt_list_rule
	| case_list_rule default_rule colon_token stmt_list_rule
	| case_label_rule equal_right_angled_token expr_rule line_end_token {
		context.reduce_case_expression_body();
		context.commit_line();
	
	}
	| case_list_rule case_label_rule equal_right_angled_token expr_rule line_end_token {
		context.reduce_add_case_expression_body();
		context.commit_line();
	
	}
	| default_rule equal_right_angled_token expr_rule line_end_token {
		context.reduce_default_expression_body();
		context.commit_line();
	
	}
	| case_list_rule default_rule equal_right_angled_token expr_rule line_end_token {
		context.reduce_add_default_expression_body();
		context.commit_line();
	
	};

while_cond_rule:
    while_rule expr_rule {
		context.reduce_while_condition();
	
	};

while_cond_generator_rule:
	while_expr_rule expr_rule {
		context.reduce_generator_while_condition();
	
	};

while_expr_rule:
    while_token {
		context.reduce_generator_while_keyword();
	
	};

while_rule:
    while_token {
		context.reduce_while_keyword();
	
	};

for_cond_rule:
    for_rule open_parenthesis_token range_init_rule range_cond_rule range_next_rule close_parenthesis_token {
		context.reduce_range_for_condition();
	
	}
	| for_iterator_in_rule expr_rule {
		context.reduce_iterator_for_condition();
	
	}
	| for_in_rule expr_rule {
		context.reduce_for_in_condition();
	
	};

for_cond_generator_rule:
    for_expr_rule open_parenthesis_token range_init_rule range_cond_rule range_next_rule close_parenthesis_token {
		context.reduce_generator_range_for_condition();
	
	}
	| for_iterator_in_expr_rule expr_rule {
		context.reduce_generator_iterator_for_condition();
	
	}
	| for_in_expr_rule expr_rule {
		context.reduce_generator_for_in_condition();
	
	};

for_expr_rule:
    for_token {
		context.reduce_generator_for_keyword();
	
	};

for_rule:
    for_token {
		context.reduce_for_keyword();
	
	};

for_in_expr_rule:
    for_expr_rule ident_rule in_token {
		context.reduce_generator_for_identifier();
	
	};

for_in_rule:
    for_rule ident_rule in_token {
		context.reduce_for_identifier();
	
	};

for_iterator_in_expr_rule:
    for_expr_rule ident_iterator_item_rule ident_iterator_end_rule in_token {
		context.reduce_generator_for_iterator_identifier();
	
	}
	| for_expr_rule create_ident_iterator_rule in_token {
		context.reduce_generator_for_created_iterator();
	
	};

for_iterator_in_rule:
    for_rule ident_iterator_item_rule ident_iterator_end_rule in_token {
		context.reduce_for_iterator_identifier();
	
	}
	| for_rule create_ident_iterator_rule in_token {
		context.reduce_for_created_iterator();
	
	};

range_init_rule:
    expr_rule comma_token {
		context.reduce_range_initial_value();
	
	};

range_cond_rule:
	expr_rule comma_token {
		context.reduce_range_condition_value();
	
	};

range_next_rule:
    expr_rule {
		context.reduce_range_step_value();
	
	};

return_rule:
    return_token {
		context.reduce_return_keyword();
	
	};

start_hash_rule:
    open_brace_token {
		context.reduce_hash_literal_start();
	
	};

stop_hash_rule:
    close_brace_token {
		context.reduce_hash_literal_end();
	
	};

hash_item_rule:
    hash_item_rule separator_rule expr_rule colon_token expr_rule {
		context.reduce_add_hash_entry();
	
	}
	| expr_rule colon_token expr_rule {
		context.reduce_hash_entry();
	
	};

start_array_rule:
    open_bracket_token {
		context.reduce_array_literal_start();
	
	};

stop_array_rule:
    close_bracket_token {
		context.reduce_array_literal_end();
	
	};

array_item_list_rule:
	array_item_list_rule separator_rule array_item_rule
	| array_item_rule;

array_item_rule:
	expr_rule {
		context.reduce_array_value();
	
	}
	| asterisk_token expr_rule {
		context.reduce_array_spread();
	
	}
	| tpl_dot_token expr_rule {
		context.reduce_array_unpack();
	
	}
	| generator_expr_rule {
		context.reduce_array_generator();
	
	};

iterator_item_rule:
	iterator_item_rule expr_rule separator_rule {
		context.reduce_add_iterator_value();
	
	}
	| expr_rule separator_rule {
		context.reduce_iterator_value();
	
	}
	| iterator_item_rule asterisk_token expr_rule separator_rule {
		context.reduce_add_iterator_spread();
	
	}
	| iterator_item_rule tpl_dot_token expr_rule separator_rule {
		context.reduce_add_iterator_unpack();
	
	}
	| asterisk_token expr_rule separator_rule {
		context.reduce_iterator_spread();
	
	}
	| tpl_dot_token expr_rule separator_rule {
		context.reduce_iterator_unpack();
	
	};

iterator_end_rule:
	expr_rule {
		context.reduce_iterator_end_expression();
	
	}
	| {
		context.reduce_empty_iterator_end();
	
	};

ident_iterator_item_rule:
	ident_iterator_item_rule ident_rule separator_rule {
		context.reduce_add_identifier_iterator_target();
	
	}
	| ident_rule separator_rule {
		context.reduce_identifier_iterator_target();
	
	};

ident_iterator_end_rule:
	ident_rule {
		context.reduce_identifier_iterator_end();
	
	}
	| {
		context.reduce_empty_identifier_iterator_end();
	
	};

let_modifier_rule:
    let_token {
		context.reduce_let_modifier();
	
	};

create_ident_iterator_rule:
    let_token modifier_rule create_ident_iterator_scoped_item_rule create_ident_iterator_scoped_end_rule
	| let_modifier_rule create_ident_iterator_scoped_item_rule create_ident_iterator_scoped_end_rule
	| modifier_rule create_ident_iterator_item_rule create_ident_iterator_end_rule;

create_ident_iterator_scoped_item_rule:
    create_ident_iterator_scoped_item_rule symbol_token comma_token {
		context.reduce_add_scoped_iterator_name($2);
	
	}
	| open_parenthesis_token symbol_token comma_token {
		context.reduce_first_scoped_iterator_name($2);
	
	};

create_ident_iterator_scoped_end_rule:
    symbol_token close_parenthesis_token {
		context.reduce_scoped_iterator_end_name($1);
	
	};

create_ident_iterator_item_rule:
    create_ident_iterator_item_rule symbol_token comma_token {
		context.reduce_add_iterator_name($2);
	
	}
	| open_parenthesis_token symbol_token comma_token {
		context.reduce_first_iterator_name($2);
	
	};

create_ident_iterator_end_rule:
    symbol_token close_parenthesis_token {
		context.reduce_iterator_end_name($1);
	
	};

print_stmt_sep_rule:
	comma_token {
		context.reduce_print_argument_separator();
	
	};

print_bloc_target_rule:
	open_parenthesis_token expr_rule close_parenthesis_token {
		context.reduce_print_block_target();
	
	}
	| {
		context.reduce_print_block_without_target();
	
	};

expr_rule:
    expr_rule equal_token generator_expr_rule {
		context.reduce_assignment_from_generator_expression();
	
	}
	| expr_rule equal_token expr_rule {
		context.reduce_assignment_expression();
	
	}
	| expr_rule colon_equal_token generator_expr_rule {
		context.reduce_binding_from_generator_expression();
	
	}
	| expr_rule colon_equal_token expr_rule {
		context.reduce_binding_expression();
	
	}
	| expr_rule equal_colon_token {
		context.begin_iterator_initialization_expression();
	} generator_expr_rule {
		context.reduce_iterator_initialization_expression();
	
	}
	| expr_rule plus_token expr_rule {
		context.reduce_addition_expression();
	
	}
	| expr_rule minus_token expr_rule {
		context.reduce_subtraction_expression();
	
	}
	| expr_rule asterisk_token expr_rule {
		context.reduce_multiplication_expression();
	
	}
	| expr_rule slash_token expr_rule {
		context.reduce_division_expression();
	
	}
	| expr_rule percent_token expr_rule {
		context.reduce_modulo_expression();
	
	}
	| expr_rule dbl_asterisk_token expr_rule {
		context.reduce_exponentiation_expression();
	
	}
	| expr_rule is_token expr_rule {
		context.reduce_identity_expression();
	
	}
	| expr_rule in_token expr_rule {
		context.reduce_membership_expression();
	
	}
	| expr_rule exclamation_token in_token expr_rule %prec in_token {
		context.reduce_negated_membership_expression();
	
	}
	| expr_rule dbl_equal_token expr_rule {
		context.reduce_equality_expression();
	
	}
	| expr_rule exclamation_equal_token expr_rule {
		context.reduce_inequality_expression();
	
	}
	| expr_rule left_angled_token expr_rule {
		context.reduce_less_than_expression();
	
	}
	| expr_rule right_angled_token expr_rule {
		context.reduce_greater_than_expression();
	
	}
	| expr_rule left_angled_equal_token expr_rule {
		context.reduce_less_than_or_equal_expression();
	
	}
	| expr_rule right_angled_equal_token expr_rule {
		context.reduce_greater_than_or_equal_expression();
	
	}
	| expr_rule dbl_left_angled_token expr_rule {
		context.reduce_left_shift_expression();
	
	}
	| expr_rule dbl_right_angled_token expr_rule {
		context.reduce_right_shift_expression();
	
	}
	| expr_rule dbl_dot_token expr_rule {
		context.reduce_inclusive_range_expression();
	
	}
	| expr_rule tpl_dot_token expr_rule {
		context.reduce_exclusive_range_expression();
	
	}
	| dbl_plus_token expr_rule %prec prefix_dbl_plus_token {
		context.reduce_prefix_increment_expression();
	
	}
	| dbl_minus_token expr_rule %prec prefix_dbl_minus_token {
		context.reduce_prefix_decrement_expression();
	
	}
	| expr_rule dbl_plus_token {
		context.reduce_postfix_increment_expression();
	
	}
	| expr_rule dbl_minus_token {
		context.reduce_postfix_decrement_expression();
	
	}
	| exclamation_token expr_rule {
		context.reduce_logical_not_expression();
	
	}
	| expr_rule dbl_pipe_token {
		context.begin_logical_or_expression();
	} expr_rule {
		context.reduce_logical_or_expression();
	
	}
	| expr_rule dbl_amp_token {
		context.begin_logical_and_expression();
	} expr_rule {
		context.reduce_logical_and_expression();
	
	}
	| expr_rule pipe_token expr_rule {
		context.reduce_bitwise_or_expression();
	
	}
	| expr_rule amp_token expr_rule {
		context.reduce_bitwise_and_expression();
	
	}
	| expr_rule caret_token expr_rule {
		context.reduce_bitwise_xor_expression();
	
	}
	| tilde_token expr_rule {
		context.reduce_bitwise_not_expression();
	
	}
	| plus_token expr_rule %prec prefix_plus_token {
		context.reduce_unary_plus_expression();
	
	}
	| minus_token expr_rule %prec prefix_minus_token {
		context.reduce_unary_minus_expression();
	
	}
	| await_token expr_rule {
		context.reduce_await_expression();
	
	}
	| typeof_token expr_rule {
		context.reduce_typeof_expression();
	
	}
	| membersof_token expr_rule {
		context.reduce_membersof_expression();
	
	}
	| defined_token defined_symbol_rule {
		context.reduce_defined_expression();
	
	}
	| expr_rule open_bracket_token expr_rule close_bracket_equal_token expr_rule {
		context.reduce_slice_assignment_expression();
	
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
		context.begin_addition_assignment_expression();
	} expr_rule {
		context.reduce_addition_assignment_expression();
	
	}
	| expr_rule minus_equal_token {
		context.begin_subtraction_assignment_expression();
	} expr_rule {
		context.reduce_subtraction_assignment_expression();
	
	}
	| expr_rule asterisk_equal_token {
		context.begin_multiplication_assignment_expression();
	} expr_rule {
		context.reduce_multiplication_assignment_expression();
	
	}
	| expr_rule slash_equal_token {
		context.begin_division_assignment_expression();
	} expr_rule {
		context.reduce_division_assignment_expression();
	
	}
	| expr_rule percent_equal_token {
		context.begin_modulo_assignment_expression();
	} expr_rule {
		context.reduce_modulo_assignment_expression();
	
	}
	| expr_rule dbl_left_angled_equal_token {
		context.begin_left_shift_assignment_expression();
	} expr_rule {
		context.reduce_left_shift_assignment_expression();
	
	}
	| expr_rule dbl_right_angled_equal_token {
		context.begin_right_shift_assignment_expression();
	} expr_rule {
		context.reduce_right_shift_assignment_expression();
	
	}
	| expr_rule amp_equal_token {
		context.begin_bitwise_and_assignment_expression();
	} expr_rule {
		context.reduce_bitwise_and_assignment_expression();
	
	}
	| expr_rule pipe_equal_token {
		context.begin_bitwise_or_assignment_expression();
	} expr_rule {
		context.reduce_bitwise_or_assignment_expression();
	
	}
	| expr_rule caret_equal_token {
		context.begin_bitwise_xor_assignment_expression();
	} expr_rule {
		context.reduce_bitwise_xor_assignment_expression();
	
	}
	| expr_rule equal_tilde_token expr_rule {
		context.reduce_regex_match_expression();
	
	}
	| expr_rule exclamation_tilde_token expr_rule {
		context.reduce_regex_non_match_expression();
	
	}
	| expr_rule tpl_equal_token expr_rule {
		context.reduce_strict_equality_expression();
	
	}
	| expr_rule exclamation_dbl_equal_token expr_rule {
		context.reduce_strict_inequality_expression();
	
	}
	| expr_rule question_token {
		context.begin_conditional_expression();
	} expr_rule colon_token {
		context.continue_conditional_expression();
	} expr_rule {
		context.reduce_conditional_expression();
	
	}
	| open_parenthesis_token close_parenthesis_token {
		context.reduce_empty_parenthesized_expression();
	
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
		context.reduce_subscript_expression();
	
	};

call_args_rule:
	call_arg_start_rule call_arg_list_rule call_arg_stop_rule
	| call_args_rule call_arg_start_rule call_arg_list_rule call_arg_stop_rule;

call_member_args_rule:
	call_member_arg_start_rule call_arg_list_rule call_member_arg_stop_rule
	| call_member_args_rule call_arg_start_rule call_arg_list_rule call_arg_stop_rule;

call_defined_member_args_rule:
	call_defined_member_arg_start_rule call_arg_list_rule call_member_arg_stop_rule {
		context.reduce_defined_member_call_arguments();
	
	}
	| call_defined_member_args_rule call_arg_start_rule call_arg_list_rule call_arg_stop_rule;

call_arg_start_rule:
    open_parenthesis_token {
		context.reduce_call_arguments_start();
	
	};

call_arg_stop_rule:
    close_parenthesis_token {
		context.reduce_call_arguments_end();
	
	};

call_member_arg_start_rule:
    symbol_token open_parenthesis_token {
		context.reduce_member_call_start($1);
	
	}
	| operator_desc_rule open_parenthesis_token {
		context.reduce_operator_member_call_start();
	
	}
	| var_symbol_rule open_parenthesis_token {
		context.reduce_variable_member_call_start();
	
	};

call_defined_member_arg_start_rule:
    symbol_token open_parenthesis_token {
		context.reduce_defined_member_call_start($1);
	
	}
	| operator_desc_rule open_parenthesis_token {
		context.reduce_defined_operator_member_call_start();
	
	}
	| var_symbol_rule open_parenthesis_token {
		context.reduce_defined_variable_member_call_start();
	
	};

call_member_arg_stop_rule:
    close_parenthesis_token {
		context.reduce_member_call_arguments_end();
	
	};

call_arg_list_rule:
	call_arg_list_rule separator_rule call_arg_rule
	| call_arg_rule
	| ;

call_arg_rule:
	expr_rule {
		context.reduce_positional_call_argument();
	
	}
	| def_arrow_rule {
		context.reduce_callable_call_argument();
	
	}
	| generator_expr_rule {
		context.reduce_generator_call_argument();
	
	}
	| asterisk_token expr_rule {
		context.reduce_spread_call_argument();
	
	}
	| tpl_dot_token expr_rule {
		context.reduce_unpack_call_argument();
	
	};

def_rule:
	def_start_rule def_capture_rule def_args_rule stmt_bloc_rule {
		context.reduce_function_definition_with_arguments();
	
	}
	| def_start_rule def_capture_rule def_no_args_rule stmt_bloc_rule {
		context.reduce_function_definition_without_arguments();
	
	};

def_arrow_rule:
    def_start_rule def_capture_rule def_args_rule def_arrow_stmt_rule {
		context.reduce_arrow_function_definition();
	
	};

def_start_rule:
    def_token {
		context.reduce_function_definition_start();
	
	}
	| async_token def_token {
		context.reduce_async_function_definition_start();
	
	};

def_capture_rule:
	def_capture_start_rule def_capture_list_rule def_capture_stop_rule
	| ;

def_capture_start_rule:
    open_bracket_token {
		context.reduce_capture_list_start();
	
	};

def_capture_stop_rule:
    close_bracket_token {
		context.reduce_capture_list_end();
	
	};

def_capture_list_rule:
    symbol_token equal_token expr_rule separator_rule def_capture_list_rule {
		context.reduce_capture_with_initializer($1);
	
	}
	| symbol_token equal_token expr_rule {
		context.reduce_final_capture_with_initializer($1);
	
	}
	| symbol_token separator_rule def_capture_list_rule {
		context.reduce_capture($1);
	
	}
	| symbol_token {
		context.reduce_final_capture($1);
	
	}
	| tpl_dot_token {
		context.reduce_capture_all();
	
	};

def_no_args_rule:
	{
		context.reduce_no_function_arguments();
	
	};

def_args_rule:
	def_arg_start_rule def_arg_list_rule def_arg_stop_rule;

def_arg_start_rule:
    open_parenthesis_token;

def_arg_stop_rule:
    close_parenthesis_token {
		context.reduce_function_arguments_end();
	
	};

def_arg_list_rule:
	def_arg_rule separator_rule def_arg_list_rule
	| def_arg_rule
	| ;

def_arg_rule:
    symbol_token {
		context.reduce_function_argument($1);
	
	}
	| symbol_token equal_token expr_rule {
		context.reduce_function_argument_with_default($1);
	
	}
	| modifier_rule symbol_token {
		context.reduce_modified_function_argument($2);
	
	}
	| modifier_rule symbol_token equal_token expr_rule {
		context.reduce_modified_function_argument_with_default($2);
	
	}
	| tpl_dot_token {
		context.reduce_function_argument_unpack();
	
	};

def_arrow_stmt_rule:
    def_arrow_stmt_start_rule expr_rule;

def_arrow_stmt_start_rule:
    equal_right_angled_token {
		context.reduce_arrow_function_body_start();
	
	};

member_ident_rule:
    expr_rule dot_token symbol_token {
		context.reduce_member_access($3);
	
	}
	| expr_rule dot_token operator_desc_rule {
		context.reduce_operator_member_access();
	
	}
	| expr_rule dot_token var_symbol_rule {
		context.reduce_variable_member_access();
	
	}
	| expr_rule question_dot_token symbol_token {
		context.reduce_optional_member_access($3);
	
	}
	| expr_rule question_dot_token operator_desc_rule {
		context.reduce_optional_operator_member_access();
	
	}
	| expr_rule question_dot_token var_symbol_rule {
		context.reduce_optional_variable_member_access();
	
	};

defined_symbol_rule:
    symbol_token {
		context.reduce_defined_symbol($1);
	
	}
	| defined_symbol_rule dot_token symbol_token {
		context.reduce_qualified_defined_symbol($3);
	
	}
	| var_symbol_rule {
		context.reduce_defined_variable_symbol();
	
	}
	| defined_symbol_rule dot_token var_symbol_rule {
		context.reduce_qualified_defined_variable_symbol();
	
	}
	| constant_rule {
		context.reduce_defined_constant_symbol($1);
	
	};

ident_rule:
	constant_rule {
		context.reduce_constant_identifier($1);
	
	}
	| lib_token {
		context.reduce_library_identifier();
	
	}
	| var_symbol_rule {
		context.reduce_variable_identifier();
	
	}
	| symbol_token {
		context.reduce_identifier($1);
	
	}
	| let_token symbol_token {
		context.reduce_let_identifier($2);
	
	}
	| modifier_rule symbol_token {
		context.reduce_modified_identifier($2);
	
	}
	| let_token modifier_rule symbol_token {
		context.reduce_let_modified_identifier($3);
	
	};

constant_rule:
    constant_token {
		$$ = context.reduce_constant_value($1);
	
	}
	| regex_rule {
		$$ = context.reduce_regular_expression($1);
	
	}
	| regex_rule symbol_token {
		$$ = context.reduce_regular_expression_with_symbol($1, $2);
	
	}
	| string_token {
		$$ = context.reduce_string_literal($1);
	
	}
	| number_token {
		$$ = context.reduce_number_literal($1);
	
	};

regex_rule:
    slash_token {
		$$ = context.reduce_regular_expression_start($1);
	} slash_token {
		$$ = context.reduce_regular_expression_end($1, $2);
	
	};

var_symbol_rule:
    dollar_token open_brace_token expr_rule close_brace_token;

modifier_rule:
    var_token {
		context.reduce_default_modifier();
	
	}
	| dollar_token {
		context.reduce_const_address_modifier();
	
	}
	| percent_token {
		context.reduce_const_value_modifier();
	
	}
	| const_token {
		context.reduce_const_modifier();
	
	}
	| at_token {
		context.reduce_global_modifier();
	
	}
	| modifier_rule var_token {
		context.reduce_add_default_modifier();
	
	}
	| modifier_rule dollar_token {
		context.reduce_add_const_address_modifier();
	
	}
	| modifier_rule percent_token {
		context.reduce_add_const_value_modifier();
	
	}
	| modifier_rule const_token {
		context.reduce_add_const_modifier();
	
	}
	| modifier_rule at_token {
		context.reduce_add_global_modifier();
	
	};

separator_rule:
    comma_token
	| separator_rule line_end_token {
		context.reduce_statement_separator();
		context.commit_line();
	
	};

empty_lines_rule:
    line_end_token {
		context.reduce_empty_line();
		context.commit_line();
	
	}
	| empty_lines_rule line_end_token {
		context.reduce_add_empty_line();
		context.commit_line();
	
	};

%%

void parser::error(const std::string &msg) {
	context.parse_error(msg);
}
