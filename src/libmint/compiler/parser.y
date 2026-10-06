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

%define api.namespace {mint}
%define api.value.type variant
%define api.token.constructor

%parse-param {mint::BuildContext& context}
%lex-param {mint::BuildContext& context}

%code requires {
#include "mint/compiler/build_context.h"
#include "mint/compiler/parser_data.h"

namespace mint {

using token_t = std::string;
using data_t = ParserData;
using call_t = BuildContext::Call;

}
}

%code {
#include "mint/compiler/build_context.h"
#include "mint/compiler/compiler.h"
#include "mint/compiler/parser_data.h"
#include <memory>

using namespace mint;

parser::symbol_type yylex(mint::BuildContext& context);
}

%token <token_t> assert_token
%token <token_t> async_token
%token <token_t> break_token
%token <token_t> case_token
%token <token_t> catch_token
%token <token_t> class_token
%token <token_t> const_token
%token <token_t> continue_token
%token <token_t> def_token
%token <token_t> default_token
%token <token_t> elif_token
%token <token_t> else_token
%token <token_t> enum_token
%token <token_t> exit_token
%token <token_t> final_token
%token <token_t> for_token
%token <token_t> if_token
%token <token_t> let_token
%token <token_t> lib_token
%token <token_t> load_token
%token <token_t> override_token
%token <token_t> package_token
%token <token_t> print_token
%token <token_t> raise_token
%token <token_t> return_token
%token <token_t> switch_token
%token <token_t> try_token
%token <token_t> while_token
%token <token_t> yield_token
%token <token_t> var_token
%token <token_t> constant_token
%token <token_t> string_token
%token <token_t> number_token
%token <token_t> symbol_token

%token <token_t> no_line_end_token
%token <token_t> line_end_token
%token <token_t> file_end_token
%token <token_t> comment_token
%token <token_t> dollar_token
%token <token_t> at_token
%token <token_t> sharp_token
%token <token_t> back_slash_token
%token <token_t> minus_right_angled_token

%left <token_t> comma_token
%left <token_t> dbl_pipe_token
%left <token_t> dbl_amp_token
%left <token_t> pipe_token
%left <token_t> caret_token
%left <token_t> amp_token
%right <token_t> equal_token question_token colon_token colon_equal_token equal_colon_token close_bracket_equal_token plus_equal_token minus_equal_token asterisk_equal_token slash_equal_token percent_equal_token dbl_left_angled_equal_token dbl_right_angled_equal_token amp_equal_token pipe_equal_token caret_equal_token equal_right_angled_token
%left <token_t> dbl_equal_token exclamation_equal_token is_token in_token equal_tilde_token exclamation_tilde_token tpl_equal_token exclamation_dbl_equal_token
%left <token_t> dbl_dot_token tpl_dot_token
%left <token_t> left_angled_token right_angled_token left_angled_equal_token right_angled_equal_token
%left <token_t> dbl_left_angled_token dbl_right_angled_token
%left <token_t> plus_token minus_token
%left <token_t> asterisk_token slash_token percent_token
%right <token_t> prefix_dbl_plus_token prefix_dbl_minus_token prefix_plus_token prefix_minus_token exclamation_token tilde_token await_token typeof_token membersof_token defined_token
%left <token_t> dbl_plus_token dbl_minus_token dbl_asterisk_token
%left <token_t> dot_token question_dot_token open_parenthesis_token close_parenthesis_token open_bracket_token close_bracket_token open_brace_token close_brace_token

%type <token_t> module_name_rule module_path_rule
%type <token_t> type_name_rule
%type <token_t> constant_rule regex_rule
%type <token_t> member_desc_rule

%type <data_t> def_expr_rule expr_with_decl_rule cond_expr_rule range_cond_rule generator_expr_rule def_arrow_rule def_arrow_stmt_rule expr_rule
%type <data_t> def_signature_rule def_expr_signature_rule def_arrow_signature_rule
%type <data_t> decl_rule explicit_decl_rule implicit_decl_rule
%type <data_t> ident_rule member_ident_rule defined_symbol_rule
%type <data_t> case_constant_rule
%type <data_t> subscript_rule

%type <call_t> stop_hash_rule stop_array_rule iterator_end_rule ident_iterator_end_rule create_ident_iterator_scoped_end_rule create_ident_iterator_end_rule
%type <call_t> call_args_rule call_member_args_rule call_arg_stop_rule call_member_arg_stop_rule
%type <call_t> create_ident_iterator_rule
%type <call_t> case_constant_list_end_rule

%%

module_rule:
    stmt_list_rule file_end_token {
	    context.push_node(Node::Command::exit_module);
		context.resolve_deferred_checks();
		fflush(stdout);
		YYACCEPT;
	}
	| file_end_token {
	    context.push_node(Node::Command::exit_module);
		context.resolve_deferred_checks();
		fflush(stdout);
		YYACCEPT;
	};

stmt_list_rule:
	stmt_list_rule stmt_rule
	| stmt_rule;

stmt_rule:
    load_token module_path_rule line_end_token {
		context.load_module($2);
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
		context.push_node(Node::Command::reset_uncaught_exception);
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
		}
		context.prepare_break();
		context.push_node(Node::Command::jump);
		context.bloc_jump_forward();
		context.commit_line();
	}
	| continue_token line_end_token {
		if (!context.is_in_loop()) {
			context.parse_error("continue statement not within loop");
		}
		context.prepare_continue();
		context.push_node(Node::Command::jump);
		context.bloc_jump_backward();
		context.commit_line();
	}
	| print_token open_parenthesis_token expr_rule print_stmt_sep_rule expr_rule close_parenthesis_token line_end_token {
		context.check_expression($3);
		context.commit_expr_result($5);
		context.close_printer();
		context.commit_line();
	}
	| print_token open_parenthesis_token expr_rule close_parenthesis_token line_end_token {
		context.push_node(Node::Command::load_constant);
		context.push_node(context.compiler().make_constant<Number>(1.));
		context.open_printer();
		context.commit_expr_result($3);
		context.close_printer();
		context.commit_line();
	}
	| print_token print_bloc_target_rule stmt_bloc_rule {
		context.reset_scoped_symbols();
		context.close_block();
		context.close_printer();
	}
	| yield_token generator_expr_rule line_end_token {
		context.check_expression($2);
		context.set_generator();
		context.push_node(Node::Command::jump);
		context.start_jump_forward();
		context.start_jump_backward();
		context.push_node(Node::Command::range_next);
		context.resolve_jump_forward();
		context.push_node(Node::Command::range_expression_check);
		context.start_jump_forward();
		context.push_node(Node::Command::yield);
		context.push_node(Node::Command::jump);
		context.resolve_jump_backward();
		context.resolve_jump_forward();
		context.commit_line();
	}
	| yield_token expr_rule line_end_token {
		context.check_expression($2);
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
		}
	}
	| return_rule generator_expr_rule line_end_token {
		context.set_exit_point($2);
		context.push_node(Node::Command::unpack_generator_expression);
		if (context.is_in_generator()) {
			if (context.is_in_async_function()) {
				context.push_node(Node::Command::yield_exit_async_generator);
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
	| return_rule expr_rule line_end_token {
		context.set_exit_point($2);
		if (context.is_in_generator()) {
			if (context.is_in_async_function()) {
				context.push_node(Node::Command::yield_exit_async_generator);
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
		context.check_expression($2);
	    context.reset_scoped_symbols_until(BuildContext::BlockType::try_type);
		context.push_node(Node::Command::raise);
		context.commit_line();
	}
	| raise_token in_token expr_rule line_end_token {
		context.check_expression($3);
		if (context.is_in_catch()) {
			context.reset_scoped_symbols_until(BuildContext::BlockType::try_type);
			context.push_node(Node::Command::reraise_in);
			context.commit_line();
		}
		else {
			context.parse_error("no active exception to reraise");
		}
	}
	| raise_token line_end_token {
		if (context.is_in_catch()) {
			context.reset_scoped_symbols_until(BuildContext::BlockType::try_type);
			context.push_node(Node::Command::reraise);
			context.commit_line();
		}
		else {
			context.parse_error("no active exception to reraise");
		}
	    
	}
	| exit_token expr_rule line_end_token {
		context.check_expression($2);
		context.push_node(Node::Command::exit_exec);
		context.commit_line();
	}
	| exit_token line_end_token {
		context.push_node(Node::Command::load_constant);
		context.push_node(context.compiler().make_constant<Number>(0.));
		context.push_node(Node::Command::exit_exec);
		context.commit_line();
	}
	| ident_iterator_item_rule ident_iterator_end_rule equal_token expr_rule line_end_token {
		context.push_node(Node::Command::copy_operator);
		context.commit_expr_result(context.check_operation(BinaryOperation::copy_operator, context.check_init_iterator($2), $4));
		context.commit_line();
	}
	| ident_iterator_item_rule ident_iterator_end_rule equal_token generator_expr_rule line_end_token {
		context.push_node(Node::Command::copy_operator);
		context.commit_expr_result(context.check_operation(BinaryOperation::copy_operator, context.check_init_iterator($2), $4));
		context.commit_line();
	}
	| create_ident_iterator_rule equal_token expr_rule line_end_token {
		context.push_node(Node::Command::copy_operator);
		context.commit_expr_result(context.check_operation(BinaryOperation::copy_operator, context.check_init_iterator($1), $3));
		context.commit_line();
	}
	| create_ident_iterator_rule equal_token generator_expr_rule line_end_token {
		context.push_node(Node::Command::copy_operator);
		context.commit_expr_result(context.check_operation(BinaryOperation::copy_operator, context.check_init_iterator($1), $3));
		context.commit_line();
	}
	| expr_with_decl_rule line_end_token {
		context.commit_expr_result($1);
		context.commit_line();
	}
	| def_signature_rule stmt_bloc_rule {
		if (context.is_in_generator()) {
			if (!context.is_in_async_function()) {
				context.push_node(Node::Command::exit_generator);
			}
			else if (!context.has_returned()) {
				context.push_node(Node::Command::exit_async_generator);
			}
		}
		else if (!context.has_returned()) {
			if (context.is_in_async_function()) {
				context.push_node(Node::Command::load_constant);
				context.push_node(context.compiler().make_constant<None>());
				context.push_node(Node::Command::resume_coroutine);
			}
			else {
				context.push_node(Node::Command::load_constant);
				context.push_node(context.compiler().make_constant<None>());
				context.push_node(Node::Command::exit_call);
			}
		}
		context.resolve_jump_forward();
		context.register_function_description();
		context.resolve_function_description();
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

type_name_rule:
	symbol_token {
		$$ = $1;
	}
	| type_name_rule dot_token symbol_token {
		$$ = $1 + $2 + $3;
	};

type_annotation_rule:
	type_name_rule {
		context.start_type_annotation(make_type_annotation($1));
	}
	| type_annotation_rule question_token {
		context.add_type_annotation(PrimitiveTypeKind::none);
	}
	| type_annotation_rule pipe_token type_name_rule {
		context.add_type_annotation(make_type_annotation($3));
	};

def_signature_rule:
	modifier_rule def_start_rule def_capture_rule symbol_token def_args_rule {
		const auto type = context.retrieve_type_annotation();
		const auto flags = Reference::const_address | context.retrieve_modifiers();
		$$ = context.save_function_signatures($4, type, flags);
	}
	| def_start_rule def_capture_rule symbol_token def_args_rule {
		const auto type = context.retrieve_type_annotation();
		const auto flags = Reference::const_address;
		$$ = context.save_function_signatures($3, type, flags);
	};

decl_type_rule:
	colon_token type_annotation_rule
	| %empty {
		context.start_type_annotation({});
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
    type_modifier_rule class_token symbol_token {
		context.start_class_description($3, Reference::const_address | Reference::const_value | context.retrieve_modifiers());
	}
	| class_token symbol_token {
		context.start_class_description($2, Reference::const_address | Reference::const_value);
	};

parent_rule:
    colon_token parent_list_rule
	| %empty;

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
	class_token symbol_token {
		context.start_class_description($2, Reference::global | Reference::const_address | Reference::const_value);
	}
	| member_type_modifier_rule class_token symbol_token {
		context.start_class_description($3, Reference::global | Reference::const_address | Reference::const_value | context.retrieve_modifiers());
	};

member_class_desc_rule:
	member_class_rule parent_rule desc_bloc_rule {
		context.resolve_class_description();
	};

member_enum_rule:
	enum_token symbol_token {
		context.start_enum_description($2, Reference::global | Reference::const_address | Reference::const_value);
	}
	| member_type_modifier_rule enum_token symbol_token {
		context.start_enum_description($3, Reference::global | Reference::const_address | Reference::const_value | context.retrieve_modifiers());
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
	}
	| at_token {
		context.start_modifiers(Reference::global);
	};

desc_bloc_rule:
    open_brace_token desc_list_rule close_brace_token
	| open_brace_token close_brace_token;

desc_list_rule:
	desc_list_rule desc_rule
	| desc_rule;

desc_rule:
    member_desc_rule line_end_token {
		context.create_attribute($1, context.retrieve_type_annotation(), context.retrieve_modifiers(),
		    context.compiler().make_data<None>());
		context.commit_line();
	}
	| member_desc_rule equal_token constant_token line_end_token {
		context.create_attribute($1, context.retrieve_type_annotation(), context.retrieve_modifiers(),
		    context.compiler().make_data($3, Compiler::DataHint::data_unknown_hint));
		context.commit_line();
	}
	| member_desc_rule equal_token string_token line_end_token {
		context.create_attribute($1, context.retrieve_type_annotation(), context.retrieve_modifiers(),
		    context.compiler().make_data($3, Compiler::DataHint::data_string_hint));
		context.commit_line();
	}
	| member_desc_rule equal_token regex_rule line_end_token {
		context.create_attribute($1, context.retrieve_type_annotation(), context.retrieve_modifiers(),
		    context.compiler().make_data($3, Compiler::DataHint::data_regex_hint));
		context.commit_line();
	}
	| member_desc_rule equal_token regex_rule regex_rule symbol_token line_end_token {
		context.create_attribute($1, context.retrieve_type_annotation(), context.retrieve_modifiers(),
		    context.compiler().make_data($3 + $4, Compiler::DataHint::data_regex_hint));
		context.commit_line();
	}
	| member_desc_rule equal_token number_token line_end_token {
		context.create_attribute($1, context.retrieve_type_annotation(), context.retrieve_modifiers(),
		    context.compiler().make_data($3, Compiler::DataHint::data_number_hint));
		context.commit_line();
	}
	| member_desc_rule equal_token open_bracket_token close_bracket_token line_end_token {
		context.create_attribute($1, context.retrieve_type_annotation(), context.retrieve_modifiers(),
		    context.compiler().make_data<Array>());
		context.commit_line();
	}
	| member_desc_rule equal_token open_brace_token close_brace_token line_end_token {
		context.create_attribute($1, context.retrieve_type_annotation(), context.retrieve_modifiers(),
		    context.compiler().make_data<Hash>());
		context.commit_line();
	}
	| member_desc_rule equal_token lib_token open_parenthesis_token string_token close_parenthesis_token line_end_token {
		context.create_attribute($1, context.retrieve_type_annotation(), context.retrieve_modifiers(),
		    context.compiler().make_data<Library>($5));
		context.commit_line();
	}
	| member_def_signature_rule stmt_bloc_rule {
		if (context.is_in_generator()) {
			if (!context.is_in_async_function()) {
				context.push_node(Node::Command::exit_generator);
			}
			else if (!context.has_returned()) {
				context.push_node(Node::Command::exit_async_generator);
			}
		}
		else if (!context.has_returned()) {
			if (context.is_in_async_function()) {
				context.push_node(Node::Command::load_constant);
				context.push_node(context.compiler().make_constant<None>());
				context.push_node(Node::Command::resume_coroutine);
			}
			else {
				context.push_node(Node::Command::load_constant);
				context.push_node(context.compiler().make_constant<None>());
				context.push_node(Node::Command::exit_call);
			}
		}
		context.resolve_jump_forward();
		context.resolve_method_description();
	}
	| member_class_desc_rule
	| member_enum_desc_rule
	| line_end_token {
		context.commit_line();
	};

member_def_signature_rule:
	member_desc_rule equal_token def_start_rule def_args_rule {
		// TODO: reset method signature ?
		context.save_method_signatures($1, context.retrieve_type_annotation(), context.retrieve_modifiers());
	}
	| member_desc_rule plus_equal_token def_start_rule def_args_rule {
		context.save_method_signatures($1, context.retrieve_type_annotation(), context.retrieve_modifiers());
	}
	| def_start_rule symbol_token def_args_rule {
		context.save_method_signatures($2, context.retrieve_type_annotation());
	}
	| def_start_rule await_token def_args_rule {
		context.save_method_signatures($2, context.retrieve_type_annotation());
	}
	| def_start_rule operator_desc_rule def_args_rule {
		context.save_method_signatures(context.retrieve_operator(), context.retrieve_type_annotation());
	}
	| desc_modifier_rule def_start_rule symbol_token def_args_rule {
		context.save_method_signatures($3, context.retrieve_type_annotation(), context.retrieve_modifiers());
	}
	| desc_modifier_rule def_start_rule await_token def_args_rule {
		context.save_method_signatures($3, context.retrieve_type_annotation(), context.retrieve_modifiers());
	}
	| desc_modifier_rule def_start_rule operator_desc_rule def_args_rule {
		context.save_method_signatures(context.retrieve_operator(), context.retrieve_type_annotation(),
		    context.retrieve_modifiers());
	};

member_desc_rule:
    symbol_token decl_type_rule {
		context.start_modifiers(Reference::default_flags);
		$$ = $1;
	}
	| desc_modifier_rule symbol_token decl_type_rule {
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
    type_modifier_rule enum_token symbol_token {
		context.start_enum_description($3, Reference::const_address | Reference::const_value | context.retrieve_modifiers());
	}
	| enum_token symbol_token {
		context.start_enum_description($2, Reference::const_address | Reference::const_value);
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
		context.create_attribute($1, PrimitiveTypeKind::number, flags,
		    context.compiler().make_data($3, Compiler::DataHint::data_number_hint));
		context.set_current_enum_value(std::stoi($3));
	}
	| symbol_token {
		constexpr auto flags = Reference::const_value | Reference::const_address | Reference::global;
		context.create_attribute($1, PrimitiveTypeKind::number, flags,
		    context.compiler().make_data(std::to_string(context.next_enum_value()),
			Compiler::DataHint::data_number_hint));
	}
	| line_end_token {
		context.commit_line();
	};

type_modifier_rule:
    at_token {
		context.start_modifiers(Reference::global);
	};

generator_expr_rule:
	if_cond_generator_rule generator_stmt_bloc_rule {
		context.reset_scoped_symbols();
		context.resolve_jump_forward();
		context.close_block();
		context.close_generator_expression();
		$$ = {}; // TODO: build from statements
	}
	| open_parenthesis_token if_generator_bloc_rule else_rule generator_stmt_bloc_rule close_parenthesis_token {
		context.reset_scoped_symbols();
		context.resolve_jump_forward();
		context.close_block();
		context.close_generator_expression();
		$$ = {}; // TODO: build from statements
	}
	| open_parenthesis_token if_generator_bloc_rule elif_generator_bloc_rule close_parenthesis_token {
		context.resolve_jump_forward();
		context.close_block();
		context.close_generator_expression();
		$$ = {}; // TODO: build from statements
	}
	| open_parenthesis_token if_generator_bloc_rule elif_generator_bloc_rule else_rule generator_stmt_bloc_rule close_parenthesis_token {
		context.reset_scoped_symbols();
		context.resolve_jump_forward();
		context.close_block();
		context.close_generator_expression();
		$$ = {}; // TODO: build from statements
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
		$$ = {}; // TODO: build from statements
	}
	| while_cond_generator_rule generator_stmt_bloc_rule {
		context.reset_scoped_symbols();
		context.push_node(Node::Command::jump);
		context.resolve_jump_backward();
		context.resolve_jump_forward();
		context.close_block();
		context.close_generator_expression();
		$$ = {}; // TODO: build from statements
	}
	| for_cond_generator_rule generator_stmt_bloc_rule {
		context.reset_scoped_symbols();
		context.push_node(Node::Command::jump);
		context.resolve_jump_backward();
		context.resolve_jump_forward();
		context.close_block();
		context.close_generator_expression();
		$$ = {}; // TODO: build from statements
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
	| open_brace_token yield_token generator_expr_rule close_brace_token {
		context.check_expression($3);
		context.set_generator();
		context.push_node(Node::Command::jump);
		context.start_jump_forward();
		context.start_jump_backward();
		context.push_node(Node::Command::range_next);
		context.resolve_jump_forward();
		context.push_node(Node::Command::range_expression_check);
		context.start_jump_forward();
		context.push_node(Node::Command::yield);
		context.push_node(Node::Command::jump);
		context.resolve_jump_backward();
		context.resolve_jump_forward();
	}
	| open_brace_token yield_token expr_rule close_brace_token {
		context.check_expression($3);
		if (context.is_in_generator_expression()) {
			context.push_node(Node::Command::yield);
		}
		else if (context.is_in_function()) {
			context.set_generator();
			context.push_node(Node::Command::yield);
		}
		else {	
			context.parse_error("unexpected 'yield' statement outside of function");
		}
	}
	| open_brace_token return_rule generator_expr_rule close_brace_token {
		context.set_exit_point($3);
		context.push_node(Node::Command::unpack_generator_expression);
		if (context.is_in_generator()) {
			if (context.is_in_async_function()) {
				context.push_node(Node::Command::yield_exit_async_generator);
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
	| open_brace_token return_rule expr_rule close_brace_token {
		context.set_exit_point($3);
		if (context.is_in_generator()) {
			if (context.is_in_async_function()) {
				context.push_node(Node::Command::yield_exit_async_generator);
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
		context.check_expression($3);
		context.reset_scoped_symbols_until(BuildContext::BlockType::try_type);
		context.push_node(Node::Command::raise);
	}
	| open_brace_token raise_token in_token expr_rule close_brace_token {
		context.check_expression($4);
		if (context.is_in_catch()) {
			context.reset_scoped_symbols_until(BuildContext::BlockType::try_type);
			context.push_node(Node::Command::reraise_in);
		}
		else {
			context.parse_error("no active exception to reraise");
		}
	}
	| open_brace_token raise_token close_brace_token {
		if (context.is_in_catch()) {
			context.reset_scoped_symbols_until(BuildContext::BlockType::try_type);
			context.push_node(Node::Command::reraise);
		}
		else {
			context.parse_error("no active exception to reraise");
		}
	}
	| open_brace_token expr_rule close_brace_token {
		context.commit_expr_result($2);
	}
	| open_brace_token close_brace_token;

generator_stmt_bloc_rule:
	equal_right_angled_token expr_rule {
		context.check_expression($2);
		context.push_node(Node::Command::yield);
	}
	| equal_right_angled_token generator_expr_rule {
		context.check_expression($2);
	}
	| stmt_bloc_rule;

if_cond_rule:
	if_rule cond_expr_rule {
		context.resolve_condition($2);
		context.push_node(Node::Command::zero_jump);
		context.start_jump_forward();
		context.open_block(BuildContext::BlockType::if_type);
	};

if_cond_generator_rule:
	if_generator_rule cond_expr_rule {
		context.resolve_condition($2);
		context.push_node(Node::Command::zero_jump);
		context.start_jump_forward();
		context.open_block(BuildContext::BlockType::if_type);
	};

elif_cond_rule:
	elif_rule cond_expr_rule {
		context.resolve_condition($2);
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
		context.close_block();
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
	switch_rule cond_expr_rule {
		context.resolve_condition($2);
		context.open_block(BuildContext::BlockType::switch_type);
	};

switch_cond_generator_rule:
	switch_expr_rule cond_expr_rule {
		context.resolve_condition($2);
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
		context.add_to_case_label($1);
	}
	| case_symbol_rule dot_token symbol_token {
		context.push_node(Node::Command::load_member);
		context.push_node($3.c_str());
		context.add_to_case_label($2);
		context.add_to_case_label($3);
	};

case_constant_rule:
	constant_rule {
		auto* data = context.compiler().make_data($1, Compiler::DataHint::data_unknown_hint);
		context.push_node(Node::Command::load_constant);
		context.push_node(data);
		context.add_to_case_label($1);
		$$ = to_parser_data(data);
	}
	| plus_token number_token {
		auto* data = context.compiler().make_data($2, Compiler::DataHint::data_number_hint);
		context.push_node(Node::Command::load_constant);
		context.push_node(data);
		context.push_node(Node::Command::pos_operator);
		context.add_to_case_label($2);
		$$ = to_parser_data(data);
	}
	| minus_token number_token {
		auto* data = context.compiler().make_data($2, Compiler::DataHint::data_number_hint);
		context.push_node(Node::Command::load_constant);
		context.push_node(data);
		context.push_node(Node::Command::neg_operator);
		context.add_to_case_label($1);
		context.add_to_case_label($2);
		$$ = to_parser_data(data);
	};

case_in_rule:
	in_token {
		context.add_to_case_label($1);
	};

case_inclusive_range_rule:
	dbl_dot_token {
		context.add_to_case_label($1);
	};

case_exclusive_range_rule:
	tpl_dot_token {
		context.add_to_case_label($1);
	};

case_is_rule:
	is_token {
		context.add_to_case_label($1);
	};

case_constant_list_rule:
    case_constant_list_rule case_constant_rule comma_token {
		context.add_to_call($2);
		context.add_to_case_label($3);
	}
	| case_constant_rule comma_token {
		context.push_node(Node::Command::alloc_iterator);
		context.start_call();
		context.add_to_call($1);
		context.add_to_case_label($2);
	};

case_constant_list_end_rule:
	case_constant_rule {
		context.push_node(Node::Command::init_iterator);
		context.add_to_call($1);
		$$ = context.resolve_call();
	}
	| %empty {
		context.push_node(Node::Command::init_iterator);
		$$ = context.resolve_call();
	};

case_label_rule:
    case_rule case_in_rule case_constant_rule case_inclusive_range_rule case_constant_rule {
		context.push_node(Node::Command::inclusive_range_operator);
		context.start_jump_backward();
		context.push_node(Node::Command::find_next);
		context.push_node(Node::Command::find_check);
		context.start_jump_forward();
		context.push_node(Node::Command::jump);
		context.resolve_jump_backward();
		context.resolve_jump_forward();
		context.resolve_case_label();
	}
	| case_rule case_in_rule case_constant_rule case_exclusive_range_rule case_constant_rule {
		context.push_node(Node::Command::exclusive_range_operator);
		context.start_jump_backward();
		context.push_node(Node::Command::find_next);
		context.push_node(Node::Command::find_check);
		context.start_jump_forward();
		context.push_node(Node::Command::jump);
		context.resolve_jump_backward();
		context.resolve_jump_forward();
		context.resolve_case_label();
	}
	| case_rule case_in_rule case_constant_list_rule case_constant_list_end_rule {
		context.start_jump_backward();
		context.push_node(Node::Command::find_next);
		context.push_node(Node::Command::find_check);
		context.start_jump_forward();
		context.push_node(Node::Command::jump);
		context.resolve_jump_backward();
		context.resolve_jump_forward();
		context.resolve_case_label();
	}
	| case_rule case_in_rule case_constant_rule {
		context.push_node(Node::Command::find_operator);
		context.push_node(Node::Command::find_init);
		context.start_jump_backward();
		context.push_node(Node::Command::find_next);
		context.push_node(Node::Command::find_check);
		context.start_jump_forward();
		context.push_node(Node::Command::jump);
		context.resolve_jump_backward();
		context.resolve_jump_forward();
		context.resolve_case_label();
	}
	| case_rule case_in_rule case_symbol_rule {
		context.push_node(Node::Command::find_operator);
		context.push_node(Node::Command::find_init);
		context.start_jump_backward();
		context.push_node(Node::Command::find_next);
		context.push_node(Node::Command::find_check);
		context.start_jump_forward();
		context.push_node(Node::Command::jump);
		context.resolve_jump_backward();
		context.resolve_jump_forward();
		context.resolve_case_label();
	}
	| case_rule case_is_rule case_constant_rule {
		context.push_node(Node::Command::is_operator);
		context.resolve_case_label();
	}
	| case_rule case_is_rule case_symbol_rule {
		context.push_node(Node::Command::is_operator);
		context.resolve_case_label();
	}
	| case_rule case_constant_rule {
		context.push_node(Node::Command::eq_operator);
		context.resolve_case_label();
	}
	| case_rule case_symbol_rule {
		context.push_node(Node::Command::eq_operator);
		context.resolve_case_label();
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
			context.check_expression($3);
			context.push_node(Node::Command::yield);
		}
		else {
			context.commit_expr_result($3);
		}
		context.prepare_break();
		context.push_node(Node::Command::jump);
		context.bloc_jump_forward();
		context.commit_line();
	}
	| case_list_rule case_label_rule equal_right_angled_token expr_rule line_end_token {
	    if (context.is_in_generator_expression()) {
			context.check_expression($4);
			context.push_node(Node::Command::yield);
		}
		else {
			context.commit_expr_result($4);
		}
		context.prepare_break();
		context.push_node(Node::Command::jump);
		context.bloc_jump_forward();
		context.commit_line();
	}
	| default_rule equal_right_angled_token expr_rule line_end_token {
	    if (context.is_in_generator_expression()) {
			context.check_expression($3);
			context.push_node(Node::Command::yield);
		}
		else {
			context.commit_expr_result($3);
		}
		context.prepare_break();
		context.push_node(Node::Command::jump);
		context.bloc_jump_forward();
		context.commit_line();
	}
	| case_list_rule default_rule equal_right_angled_token expr_rule line_end_token {
	    if (context.is_in_generator_expression()) {
			context.check_expression($4);
			context.push_node(Node::Command::yield);
		}
		else {
			context.commit_expr_result($4);
		}
		context.prepare_break();
		context.push_node(Node::Command::jump);
		context.bloc_jump_forward();
		context.commit_line();
	};

while_cond_rule:
    while_rule cond_expr_rule {
		context.resolve_condition($2);
		context.push_node(Node::Command::zero_jump);
		context.start_jump_forward();
		context.open_block(BuildContext::BlockType::conditional_loop_type);
	};

while_cond_generator_rule:
	while_expr_rule cond_expr_rule {
		context.resolve_condition($2);
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

for_cond_rule:
    for_rule open_parenthesis_token range_init_rule range_cond_rule range_next_rule close_parenthesis_token {
		context.resolve_condition($4);
		context.open_block(BuildContext::BlockType::custom_range_loop_type);
	}
	| for_iterator_in_rule expr_rule {
		context.push_node(Node::Command::in_operator);
		context.push_node(Node::Command::range_init);
		context.resolve_condition($2);
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
		context.resolve_condition($2);
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
    for_expr_rule open_parenthesis_token range_init_rule range_cond_rule range_next_rule close_parenthesis_token {
		context.resolve_condition($4);
		context.open_block(BuildContext::BlockType::custom_range_loop_type);
	}
	| for_iterator_in_expr_rule expr_rule {
		context.push_node(Node::Command::in_operator);
		context.push_node(Node::Command::range_init);
		context.resolve_condition($2);
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
		context.resolve_condition($2);
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
    for_expr_rule decl_rule in_token {
		context.resolve_range_loop();
		context.start_condition();
	};

for_in_rule:
    for_rule decl_rule in_token {
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
    expr_with_decl_rule comma_token {
		context.check_expression($1);
		context.push_node(Node::Command::unload_reference);
		context.push_node(Node::Command::jump);
		context.start_jump_forward();
		context.start_jump_backward();
		context.resolve_range_loop();
		context.start_condition();
		context.open_sub_branch();
	};

range_cond_rule:
	cond_expr_rule comma_token {
		context.push_node(Node::Command::zero_jump);
		context.start_jump_forward();
		context.close_sub_branch();
		$$ = $1;
	};

range_next_rule:
    expr_rule {
		context.push_node(Node::Command::unload_reference);
		context.resolve_jump_forward();
		context.build_sub_branch();
	};

return_rule:
    return_token {
		if (!context.is_in_function()) {
			context.parse_error("unexpected 'return' statement outside of function");
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
		$$ = context.resolve_call();
	};

hash_item_rule:
    hash_item_rule separator_rule expr_rule colon_token expr_rule {
		context.add_to_call($3);
	}
	| expr_rule colon_token expr_rule {
		context.add_to_call($1);
	};

start_array_rule:
    open_bracket_token {
		context.push_node(Node::Command::alloc_array);
		context.start_call();
	};

stop_array_rule:
    close_bracket_token {
		context.push_node(Node::Command::init_array);
		$$ = context.resolve_call();
	};

array_item_list_rule:
	array_item_list_rule separator_rule array_item_rule
	| array_item_rule;

array_item_rule:
	expr_rule {
		context.add_to_call($1);
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
		context.add_to_call($2);
	}
	| expr_rule separator_rule {
		context.push_node(Node::Command::alloc_iterator);
		context.start_call();
		context.add_to_call($1);
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
		context.add_to_call($1);
		$$ = context.resolve_call();
	}
	| %empty {
		context.push_node(Node::Command::init_iterator);
		$$ = context.resolve_call();
	};

ident_iterator_item_rule:
	ident_iterator_item_rule decl_rule separator_rule {
		context.add_to_call($2);
	}
	| decl_rule separator_rule {
		context.push_node(Node::Command::alloc_iterator);
		context.start_call();
		context.add_to_call($1);
	};

ident_iterator_end_rule:
	decl_rule {
		context.push_node(Node::Command::init_iterator);
		context.add_to_call($1);
		$$ = context.resolve_call();
	}
	| %empty {
		context.push_node(Node::Command::init_iterator);
		$$ = context.resolve_call();
	};

let_modifier_rule:
    let_token {
	    context.start_modifiers(Reference::default_flags);
	};

create_ident_iterator_rule:
    let_token modifier_rule create_ident_iterator_scoped_item_rule create_ident_iterator_scoped_end_rule {
		$$ = $4;
	}
	| let_modifier_rule create_ident_iterator_scoped_item_rule create_ident_iterator_scoped_end_rule {
		$$ = $3;
	}
	| modifier_rule create_ident_iterator_item_rule create_ident_iterator_end_rule {
		$$ = $3;
	};

create_ident_iterator_scoped_item_rule:
    create_ident_iterator_scoped_item_rule symbol_token decl_type_rule comma_token {
		auto param = context.create_fast_scoped_symbol($2, context.retrieve_type_annotation(), context.get_modifiers());
		context.add_to_call(param);
	}
	| open_parenthesis_token symbol_token decl_type_rule comma_token {
		context.push_node(Node::Command::alloc_iterator);
		context.start_call();
		auto param = context.create_fast_scoped_symbol($2, context.retrieve_type_annotation(), context.get_modifiers());
		context.add_to_call(param);
	};

create_ident_iterator_scoped_end_rule:
    symbol_token decl_type_rule close_parenthesis_token {
		auto param = context.create_fast_scoped_symbol($1, context.retrieve_type_annotation(), context.retrieve_modifiers());
		context.push_node(Node::Command::init_iterator);
		context.add_to_call(param);
		$$ = context.resolve_call();
	};

create_ident_iterator_item_rule:
    create_ident_iterator_item_rule symbol_token decl_type_rule comma_token {
		auto param = context.create_fast_symbol($2, context.retrieve_type_annotation(), context.get_modifiers());
		context.add_to_call(param);
	}
	| open_parenthesis_token symbol_token decl_type_rule comma_token {
		context.push_node(Node::Command::alloc_iterator);
		context.start_call();
		auto param = context.create_fast_symbol($2, context.retrieve_type_annotation(), context.get_modifiers());
		context.add_to_call(param);
	};

create_ident_iterator_end_rule:
    symbol_token decl_type_rule close_parenthesis_token {
		auto param = context.create_fast_symbol($1, context.retrieve_type_annotation(), context.retrieve_modifiers());
		context.push_node(Node::Command::init_iterator);
		context.add_to_call(param);
		$$ = context.resolve_call();
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
	| %empty {
		context.push_node(Node::Command::load_constant);
		context.push_node(context.compiler().make_constant<Number>(1.));
		context.open_printer();
		context.open_block(BuildContext::BlockType::print_type);
	};

expr_with_decl_rule:
	explicit_decl_rule equal_token generator_expr_rule {
		$$ = context.check_operation(BinaryOperation::move_operator, $1, $3);
		context.push_node(Node::Command::move_operator);
		context.check_for_alias($1, $3);
	}
	| explicit_decl_rule equal_token expr_rule {
		$$ = context.check_operation(BinaryOperation::move_operator, $1, $3);
		context.push_node(Node::Command::move_operator);
		context.check_for_alias($1, $3);
	}
	| explicit_decl_rule colon_equal_token generator_expr_rule {
		$$ = context.check_operation(BinaryOperation::copy_operator, $1, $3);
		context.push_node(Node::Command::copy_operator);
	}
	| explicit_decl_rule colon_equal_token expr_rule {
		$$ = context.check_operation(BinaryOperation::copy_operator, $1, $3);
		context.push_node(Node::Command::copy_operator);
	}
	| explicit_decl_rule equal_colon_token <data_t> {
	    context.push_node(Node::Command::alloc_iterator);
		context.start_call();
		context.add_to_call($1);
		context.push_node(Node::Command::init_iterator);
		$$ = context.check_init_iterator(context.resolve_call());
	} generator_expr_rule {
		$$ = context.check_operation(BinaryOperation::copy_operator, $3, $4);
	    context.push_node(Node::Command::copy_operator);
	}
	| expr_rule {
		$$ = $1;
	};

cond_expr_rule:
	defined_token explicit_decl_rule equal_token generator_expr_rule {
		$$ = context.check_operation(BinaryOperation::move_operator, $2, $4);
		context.push_node(Node::Command::move_operator);
		context.push_node(Node::Command::check_defined);
		context.check_for_alias($2, $4);
	}
	| defined_token explicit_decl_rule equal_token expr_rule {
		$$ = context.check_operation(BinaryOperation::move_operator, $2, $4);
		context.push_node(Node::Command::move_operator);
		context.push_node(Node::Command::check_defined);
		context.check_for_alias($2, $4);
	}
	| defined_token explicit_decl_rule colon_equal_token generator_expr_rule {
		$$ = context.check_operation(BinaryOperation::copy_operator, $2, $4);
		context.push_node(Node::Command::copy_operator);
		context.push_node(Node::Command::check_defined);
	}
	| defined_token explicit_decl_rule colon_equal_token expr_rule {
		$$ = context.check_operation(BinaryOperation::copy_operator, $2, $4);
		context.push_node(Node::Command::copy_operator);
		context.push_node(Node::Command::check_defined);
	}
	| defined_token explicit_decl_rule equal_colon_token <data_t> {
	    context.push_node(Node::Command::alloc_iterator);
		context.start_call();
		context.add_to_call($2);
		context.push_node(Node::Command::init_iterator);
		$$ = context.check_init_iterator(context.resolve_call());
	} generator_expr_rule {
		$$ = context.check_operation(BinaryOperation::copy_operator, $4, $5);
	    context.push_node(Node::Command::copy_operator);
		context.push_node(Node::Command::check_defined);
	}
	| explicit_decl_rule equal_token generator_expr_rule {
		$$ = context.check_operation(BinaryOperation::move_operator, $1, $3);
		context.push_node(Node::Command::move_operator);
		context.check_for_alias($1, $3);
	}
	| explicit_decl_rule equal_token expr_rule {
		$$ = context.check_operation(BinaryOperation::move_operator, $1, $3);
		context.push_node(Node::Command::move_operator);
		context.check_for_alias($1, $3);
	}
	| explicit_decl_rule colon_equal_token generator_expr_rule {
		$$ = context.check_operation(BinaryOperation::copy_operator, $1, $3);
		context.push_node(Node::Command::copy_operator);
	}
	| explicit_decl_rule colon_equal_token expr_rule {
		$$ = context.check_operation(BinaryOperation::copy_operator, $1, $3);
		context.push_node(Node::Command::copy_operator);
	}
	| explicit_decl_rule equal_colon_token <data_t> {
	    context.push_node(Node::Command::alloc_iterator);
		context.start_call();
		context.add_to_call($1);
		context.push_node(Node::Command::init_iterator);
		$$ = context.check_init_iterator(context.resolve_call());
	} generator_expr_rule {
		$$ = context.check_operation(BinaryOperation::copy_operator, $3, $4);
	    context.push_node(Node::Command::copy_operator);
	}
	| expr_rule {
		$$ = $1;
	};

expr_rule:
    expr_rule equal_token generator_expr_rule {
		$$ = context.check_operation(BinaryOperation::move_operator, $1, $3);
		context.push_node(Node::Command::move_operator);
	}
	| expr_rule equal_token expr_rule {
		$$ = context.check_operation(BinaryOperation::move_operator, $1, $3);
		context.push_node(Node::Command::move_operator);
	}
	| expr_rule colon_equal_token generator_expr_rule {
		$$ = context.check_operation(BinaryOperation::copy_operator, $1, $3);
		context.push_node(Node::Command::copy_operator);
	}
	| expr_rule colon_equal_token expr_rule {
		$$ = context.check_operation(BinaryOperation::copy_operator, $1, $3);
		context.push_node(Node::Command::copy_operator);
	}
	| expr_rule equal_colon_token <data_t> {
	    context.push_node(Node::Command::alloc_iterator);
		context.start_call();
		context.add_to_call($1);
		context.push_node(Node::Command::init_iterator);
		$$ = context.check_init_iterator(context.resolve_call());
	} generator_expr_rule {
		$$ = context.check_operation(BinaryOperation::copy_operator, $3, $4);
	    context.push_node(Node::Command::copy_operator);
	}
	| expr_rule plus_token expr_rule {
		$$ = context.check_operation(BinaryOperation::add_operator, $1, $3);
		context.push_node(Node::Command::add_operator);
	}
	| expr_rule minus_token expr_rule {
		$$ = context.check_operation(BinaryOperation::sub_operator, $1, $3);
		context.push_node(Node::Command::sub_operator);
	}
	| expr_rule asterisk_token expr_rule {
		$$ = context.check_operation(BinaryOperation::mul_operator, $1, $3);
		context.push_node(Node::Command::mul_operator);
	}
	| expr_rule slash_token expr_rule {
		$$ = context.check_operation(BinaryOperation::div_operator, $1, $3);
		context.push_node(Node::Command::div_operator);
	}
	| expr_rule percent_token expr_rule {
		$$ = context.check_operation(BinaryOperation::mod_operator, $1, $3);
		context.push_node(Node::Command::mod_operator);
	}
	| expr_rule dbl_asterisk_token expr_rule {
		$$ = context.check_operation(BinaryOperation::pow_operator, $1, $3);
		context.push_node(Node::Command::pow_operator);
	}
	| expr_rule is_token expr_rule {
		$$ = context.check_operation(BinaryOperation::is_operator, $1, $3);
		context.push_node(Node::Command::is_operator);
	}
	| expr_rule in_token expr_rule {
		$$ = context.check_operation(BinaryOperation::in_operator, $3, $1);
		context.push_node(Node::Command::find_operator);
		context.push_node(Node::Command::find_init);
		context.start_jump_backward();
		context.push_node(Node::Command::find_next);
		context.push_node(Node::Command::find_check);
		context.start_jump_forward();
		context.push_node(Node::Command::jump);
		context.resolve_jump_backward();
		context.resolve_jump_forward();
	}
	| expr_rule exclamation_token in_token expr_rule %prec in_token {
		$$ = context.check_operation(BinaryOperation::in_operator, $4, $1);
		context.push_node(Node::Command::find_operator);
		context.push_node(Node::Command::find_init);
		context.start_jump_backward();
		context.push_node(Node::Command::find_next);
		context.push_node(Node::Command::find_check);
		context.start_jump_forward();
		context.push_node(Node::Command::jump);
		context.resolve_jump_backward();
		context.resolve_jump_forward();
		context.push_node(Node::Command::not_operator);
	}
	| expr_rule dbl_equal_token expr_rule {
		$$ = context.check_operation(BinaryOperation::eq_operator, $1, $3);
		context.push_node(Node::Command::eq_operator);
	}
	| expr_rule exclamation_equal_token expr_rule {
		$$ = context.check_operation(BinaryOperation::ne_operator, $1, $3);
		context.push_node(Node::Command::ne_operator);
	}
	| expr_rule left_angled_token expr_rule {
		$$ = context.check_operation(BinaryOperation::lt_operator, $1, $3);
		context.push_node(Node::Command::lt_operator);
	}
	| expr_rule right_angled_token expr_rule {
		$$ = context.check_operation(BinaryOperation::gt_operator, $1, $3);
		context.push_node(Node::Command::gt_operator);
	}
	| expr_rule left_angled_equal_token expr_rule {
		$$ = context.check_operation(BinaryOperation::le_operator, $1, $3);
		context.push_node(Node::Command::le_operator);
	}
	| expr_rule right_angled_equal_token expr_rule {
		$$ = context.check_operation(BinaryOperation::ge_operator, $1, $3);
		context.push_node(Node::Command::ge_operator);
	}
	| expr_rule dbl_left_angled_token expr_rule {
		$$ = context.check_operation(BinaryOperation::shift_left_operator, $1, $3);
		context.push_node(Node::Command::shift_left_operator);
	}
	| expr_rule dbl_right_angled_token expr_rule {
		$$ = context.check_operation(BinaryOperation::shift_right_operator, $1, $3);
		context.push_node(Node::Command::shift_right_operator);
	}
	| expr_rule dbl_dot_token expr_rule {
		$$ = context.check_operation(BinaryOperation::inclusive_range_operator, $1, $3);
		context.push_node(Node::Command::inclusive_range_operator);
	}
	| expr_rule tpl_dot_token expr_rule {
		$$ = context.check_operation(BinaryOperation::exclusive_range_operator, $1, $3);
		context.push_node(Node::Command::exclusive_range_operator);
	}
	| dbl_plus_token expr_rule %prec prefix_dbl_plus_token {
		$$ = context.check_operation(UnaryOperation::inc_operator, $2);
		context.push_node(Node::Command::inc_operator);
	}
	| dbl_minus_token expr_rule %prec prefix_dbl_minus_token {
		$$ = context.check_operation(UnaryOperation::dec_operator, $2);
		context.push_node(Node::Command::dec_operator);
	}
	| expr_rule dbl_plus_token {
		$$ = context.check_operation(UnaryOperation::inc_operator, $1);
	    context.push_node(Node::Command::clone_reference);
		context.push_node(Node::Command::inc_operator);
		context.push_node(Node::Command::unload_reference);
	}
	| expr_rule dbl_minus_token {
		$$ = context.check_operation(UnaryOperation::dec_operator, $1);
	    context.push_node(Node::Command::clone_reference);
		context.push_node(Node::Command::dec_operator);
		context.push_node(Node::Command::unload_reference);
	}
	| exclamation_token expr_rule {
		$$ = context.check_operation(UnaryOperation::not_operator, $2);
		context.push_node(Node::Command::not_operator);
	}
	| expr_rule dbl_pipe_token {
		context.push_node(Node::Command::or_pre_check);
		context.start_jump_forward();
	} expr_rule {
		$$ = context.check_operation(BinaryOperation::or_operator, $1, $4);
		context.push_node(Node::Command::or_operator);
		context.resolve_jump_forward();
	}
	| expr_rule dbl_amp_token {
		context.push_node(Node::Command::and_pre_check);
		context.start_jump_forward();
	} expr_rule {
		$$ = context.check_operation(BinaryOperation::and_operator, $1, $4);
		context.push_node(Node::Command::and_operator);
		context.resolve_jump_forward();
	}
	| expr_rule pipe_token expr_rule {
		$$ = context.check_operation(BinaryOperation::bor_operator, $1, $3);
		context.push_node(Node::Command::bor_operator);
	}
	| expr_rule amp_token expr_rule {
		$$ = context.check_operation(BinaryOperation::band_operator, $1, $3);
		context.push_node(Node::Command::band_operator);
	}
	| expr_rule caret_token expr_rule {
		$$ = context.check_operation(BinaryOperation::xor_operator, $1, $3);
		context.push_node(Node::Command::xor_operator);
	}
	| tilde_token expr_rule {
		$$ = context.check_operation(UnaryOperation::compl_operator, $2);
		context.push_node(Node::Command::compl_operator);
	}
	| plus_token expr_rule %prec prefix_plus_token {
		$$ = context.check_operation(UnaryOperation::pos_operator, $2);
		context.push_node(Node::Command::pos_operator);
	}
	| minus_token expr_rule %prec prefix_minus_token {
		$$ = context.check_operation(UnaryOperation::neg_operator, $2);
		context.push_node(Node::Command::neg_operator);
	}
	| await_token expr_rule {
		$$ = context.check_operation(UnaryOperation::await_operator, $2);
		if (context.is_in_async_function()) {
			context.push_node(Node::Command::await);
		}
		else {
			context.parse_error("unexpected 'await' statement outside of async function");
		}
	}
	| typeof_token expr_rule {
		$$ = context.check_operation(UnaryOperation::typeof_operator, $2);
		context.push_node(Node::Command::typeof_operator);
	}
	| membersof_token expr_rule {
		$$ = context.check_operation(UnaryOperation::membersof_operator, $2);
		context.push_node(Node::Command::membersof_operator);
	}
	| defined_token defined_symbol_rule {
		$$ = context.check_operation(UnaryOperation::defined_operator, $2);
		context.push_node(Node::Command::check_defined);
	}
	| expr_rule open_bracket_token expr_rule close_bracket_equal_token expr_rule {
		$$ = context.check_operation(TernaryOperation::subscript_move_operator, $1, $3, $5);
		context.push_node(Node::Command::subscript_move_operator);
	}
	| expr_rule subscript_rule {
		$$ = context.check_operation(BinaryOperation::subscript_operator, $1, $2);
	}
	| member_ident_rule {
		$$ = $1;
	}
	| lib_start_rule call_args_rule {
		$$ = context.check_function_call(BuiltinTypeKind::library, $2);
	}
	| ident_rule call_args_rule {
		$$ = context.check_function_call($1, $2);
	}
	| def_expr_rule call_args_rule {
		$$ = context.check_function_call($1, $2);
	}
	| expr_rule subscript_rule call_args_rule {
		$$ = context.check_function_call(context.check_operation(BinaryOperation::subscript_operator, $1, $2), $3);
	}
	| expr_rule dot_token symbol_token {
		context.push_node(Node::Command::init_member_call);
		context.push_node($3.c_str());
	} call_member_args_rule {
		$$ = context.check_method_call($1, $3, $5);
	}
	| expr_rule dot_token operator_desc_rule {
		context.push_node(Node::Command::init_operator_call);
		context.push_node(context.get_operator());
	} call_member_args_rule {
		$$ = context.check_operator_call($1, context.retrieve_operator(), $5);
	}
	| expr_rule dot_token var_symbol_rule {
		context.push_node(Node::Command::init_var_member_call);
	} call_member_args_rule {
		$$ = context.check_method_call($1, {}, $5);
	}
    | expr_rule question_dot_token symbol_token {
		context.push_node(Node::Command::init_defined_member_call);
		context.push_node($3.c_str());
		context.start_jump_forward();
	} call_member_args_rule {
		$$ = context.check_method_call($1, $3, $5);
		context.resolve_jump_forward();
	}
	| expr_rule question_dot_token operator_desc_rule {
		context.push_node(Node::Command::init_defined_operator_call);
		context.push_node(context.get_operator());
		context.start_jump_forward();
	} call_member_args_rule {
		$$ = context.check_operator_call($1, context.retrieve_operator(), $5);
		context.resolve_jump_forward();
	}
	| expr_rule question_dot_token var_symbol_rule {
		context.push_node(Node::Command::init_defined_var_member_call);
		context.start_jump_forward();
	} call_member_args_rule {
		$$ = context.check_method_call($1, {}, $5);
		context.resolve_jump_forward();
	}
	| open_parenthesis_token expr_rule close_parenthesis_token call_args_rule {
		$$ = context.check_function_call($2, $4);
	}
	| expr_rule plus_equal_token {
		context.push_node(Node::Command::reload_reference);
	} expr_rule {
		$$ = context.check_operation(BinaryOperation::move_operator, $1,
		    context.check_operation(BinaryOperation::add_operator, $1, $4));
		context.push_node(Node::Command::add_operator);
		context.push_node(Node::Command::move_operator);
	}
	| expr_rule minus_equal_token {
		context.push_node(Node::Command::reload_reference);
	} expr_rule {
		$$ = context.check_operation(BinaryOperation::move_operator, $1,
		    context.check_operation(BinaryOperation::sub_operator, $1, $4));
		context.push_node(Node::Command::sub_operator);
		context.push_node(Node::Command::move_operator);
	}
	| expr_rule asterisk_equal_token {
		context.push_node(Node::Command::reload_reference);
	} expr_rule {
		$$ = context.check_operation(BinaryOperation::move_operator, $1,
		    context.check_operation(BinaryOperation::mul_operator, $1, $4));
		context.push_node(Node::Command::mul_operator);
		context.push_node(Node::Command::move_operator);
	}
	| expr_rule slash_equal_token {
		context.push_node(Node::Command::reload_reference);
	} expr_rule {
		$$ = context.check_operation(BinaryOperation::move_operator, $1,
		    context.check_operation(BinaryOperation::div_operator, $1, $4));
		context.push_node(Node::Command::div_operator);
		context.push_node(Node::Command::move_operator);
	}
	| expr_rule percent_equal_token {
		context.push_node(Node::Command::reload_reference);
	} expr_rule {
		$$ = context.check_operation(BinaryOperation::move_operator, $1,
		    context.check_operation(BinaryOperation::mod_operator, $1, $4));
		context.push_node(Node::Command::mod_operator);
		context.push_node(Node::Command::move_operator);
	}
	| expr_rule dbl_left_angled_equal_token {
		context.push_node(Node::Command::reload_reference);
	} expr_rule {
		$$ = context.check_operation(BinaryOperation::move_operator, $1,
		    context.check_operation(BinaryOperation::shift_left_operator, $1, $4));
		context.push_node(Node::Command::shift_left_operator);
		context.push_node(Node::Command::move_operator);
	}
	| expr_rule dbl_right_angled_equal_token {
		context.push_node(Node::Command::reload_reference);
	} expr_rule {
		$$ = context.check_operation(BinaryOperation::move_operator, $1,
		    context.check_operation(BinaryOperation::shift_right_operator, $1, $4));
		context.push_node(Node::Command::shift_right_operator);
		context.push_node(Node::Command::move_operator);
	}
	| expr_rule amp_equal_token {
		context.push_node(Node::Command::reload_reference);
	} expr_rule {
		$$ = context.check_operation(BinaryOperation::move_operator, $1,
		    context.check_operation(BinaryOperation::band_operator, $1, $4));
		context.push_node(Node::Command::band_operator);
		context.push_node(Node::Command::move_operator);
	}
	| expr_rule pipe_equal_token {
		context.push_node(Node::Command::reload_reference);
	} expr_rule {
		$$ = context.check_operation(BinaryOperation::move_operator, $1,
		    context.check_operation(BinaryOperation::bor_operator, $1, $4));
		context.push_node(Node::Command::bor_operator);
		context.push_node(Node::Command::move_operator);
	}
	| expr_rule caret_equal_token {
		context.push_node(Node::Command::reload_reference);
	} expr_rule {
		$$ = context.check_operation(BinaryOperation::move_operator, $1,
		    context.check_operation(BinaryOperation::xor_operator, $1, $4));
		context.push_node(Node::Command::xor_operator);
		context.push_node(Node::Command::move_operator);
	}
	| expr_rule equal_tilde_token expr_rule {
		$$ = context.check_operation(BinaryOperation::regex_match_operator, $1, $3);
		context.push_node(Node::Command::regex_match);
	}
	| expr_rule exclamation_tilde_token expr_rule {
		$$ = context.check_operation(BinaryOperation::regex_unmatch_operator, $1, $3);
		context.push_node(Node::Command::regex_unmatch);
	}
	| expr_rule tpl_equal_token expr_rule {
		$$ = context.check_operation(BinaryOperation::strict_eq_operator, $1, $3);
		context.push_node(Node::Command::strict_eq_operator);
	}
	| expr_rule exclamation_dbl_equal_token expr_rule {
		$$ = context.check_operation(BinaryOperation::strict_ne_operator, $1, $3);
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
		$$ = context.check_operation(TernaryOperation::conditional_operator, $1, $4, $7);
		context.resolve_jump_forward();
	}
	| open_parenthesis_token close_parenthesis_token {
		context.push_node(Node::Command::alloc_iterator);
		context.start_call();
		context.push_node(Node::Command::init_iterator);
		$$ = context.check_init_iterator(context.resolve_call());
	}
	| open_parenthesis_token expr_rule close_parenthesis_token {
		$$ = $2;
	}
	| open_parenthesis_token generator_expr_rule close_parenthesis_token {
		$$ = $2;
	}
	| open_parenthesis_token iterator_item_rule iterator_end_rule close_parenthesis_token {
		$$ = BuiltinTypeKind::iterator;
	}
	| start_array_rule empty_lines_rule array_item_list_rule empty_lines_rule stop_array_rule {
		$$ = BuiltinTypeKind::array;
	}
	| start_array_rule empty_lines_rule array_item_list_rule stop_array_rule {
		$$ = BuiltinTypeKind::array;
	}
	| start_array_rule array_item_list_rule empty_lines_rule stop_array_rule {
		$$ = BuiltinTypeKind::array;
	}
	| start_array_rule array_item_list_rule stop_array_rule {
		$$ = BuiltinTypeKind::array;
	}
	| start_array_rule stop_array_rule {
		$$ = BuiltinTypeKind::array;
	}
	| start_hash_rule empty_lines_rule hash_item_rule empty_lines_rule stop_hash_rule {
		$$ = BuiltinTypeKind::hash;
	}
	| start_hash_rule empty_lines_rule hash_item_rule stop_hash_rule {
		$$ = BuiltinTypeKind::hash;
	}
	| start_hash_rule hash_item_rule empty_lines_rule stop_hash_rule {
		$$ = BuiltinTypeKind::hash;
	}
	| start_hash_rule hash_item_rule stop_hash_rule {
		$$ = BuiltinTypeKind::hash;
	}
	| start_hash_rule stop_hash_rule {
		$$ = BuiltinTypeKind::hash;
	}
	| def_expr_rule {
		$$ = $1;
	}
	| ident_rule {
		$$ = $1;
	};

subscript_rule:
    open_bracket_token expr_rule close_bracket_token {
		$$ = $2;
		context.push_node(Node::Command::subscript_operator);
	};

call_args_rule:
	call_arg_start_rule call_arg_list_rule call_arg_stop_rule {
		$$ = std::move($3);
	}
	| call_args_rule call_arg_start_rule call_arg_list_rule call_arg_stop_rule {
		// TODO: check subsequent calls
		$$ = std::move($1);
	};

call_member_args_rule:
	call_member_arg_start_rule call_arg_list_rule call_member_arg_stop_rule {
		$$ = std::move($3);
	}
	| call_member_args_rule call_arg_start_rule call_arg_list_rule call_arg_stop_rule {
		// TODO: check subsequent calls
		$$ = std::move($1);
	};

call_arg_start_rule:
    open_parenthesis_token {
		context.push_node(Node::Command::init_call);
		context.start_call();
	};

call_arg_stop_rule:
    close_parenthesis_token {
		context.push_node(Node::Command::call);
		$$ = context.resolve_call();
	};

call_member_arg_start_rule:
    open_parenthesis_token {
		context.start_call();
	};

call_member_arg_stop_rule:
    close_parenthesis_token {
		context.push_node(Node::Command::call_member);
		$$ = context.resolve_call();
	};

call_arg_list_rule:
	call_arg_list_rule separator_rule call_arg_rule
	| call_arg_rule
	| %empty;

call_arg_rule:
	expr_rule {
		context.add_to_call($1);
	}
	| def_arrow_rule {
	    context.add_to_call($1);
	}
	| generator_expr_rule {
	    context.add_to_call($1);
	}
	| asterisk_token expr_rule {
		context.add_to_call_unpacked($2);
		context.push_node(Node::Command::in_operator);
		context.push_node(Node::Command::load_extra_arguments);
	}
	| tpl_dot_token expr_rule {
		context.add_to_call_unpacked($2);
		context.push_node(Node::Command::in_operator);
		context.push_node(Node::Command::load_extra_arguments);
	};

def_expr_rule:
	def_expr_signature_rule stmt_bloc_rule {
		if (context.is_in_generator()) {
			if (!context.is_in_async_function()) {
				context.push_node(Node::Command::exit_generator);
			}
			else if (!context.has_returned()) {
				context.push_node(Node::Command::exit_async_generator);
			}
		}
		else if (!context.has_returned()) {
			if (context.is_in_async_function()) {
				context.push_node(Node::Command::load_constant);
				context.push_node(context.compiler().make_constant<None>());
				context.push_node(Node::Command::resume_coroutine);
			}
			else {
				context.push_node(Node::Command::load_constant);
				context.push_node(context.compiler().make_constant<None>());
				context.push_node(Node::Command::exit_call);
			}
		}
		context.resolve_jump_forward();
		context.resolve_function_description();
		$$ = $1;
	};

def_expr_signature_rule:
	def_start_rule def_capture_rule def_args_rule {
		$$ = context.save_function_signatures(context.retrieve_type_annotation());
	}
	| def_start_rule def_capture_rule {
		$$ = context.save_function_signatures(TypeAnnotation());
	};

def_arrow_rule:
    def_arrow_signature_rule def_arrow_stmt_rule {
	    context.set_exit_point($2);
		if (context.is_in_async_function()) {
			context.push_node(Node::Command::resume_coroutine);
		}
		else {
			context.push_node(Node::Command::exit_call);
		}
		context.resolve_jump_forward();
		context.resolve_function_description();
		$$ = $1;
	};

def_arrow_signature_rule:
	def_start_rule def_capture_rule def_args_rule {
		$$ = context.save_function_signatures(context.retrieve_type_annotation());
	};

def_start_rule:
    def_token {
		context.push_node(Node::Command::jump);
		context.start_jump_forward();
		context.start_function_description();
	}
	| async_token def_token {
		context.push_node(Node::Command::jump);
		context.start_jump_forward();
		context.start_async_function_description();
	};

def_capture_rule:
	def_capture_start_rule def_capture_list_rule def_capture_stop_rule
	| %empty;

def_capture_start_rule:
    open_bracket_token {
		context.start_capture();
	};

def_capture_stop_rule:
    close_bracket_token {
		context.resolve_capture();
	};

def_capture_list_rule:
    symbol_token decl_type_rule equal_token expr_rule separator_rule def_capture_list_rule {
		context.capture_as($1, context.retrieve_type_annotation());
	}
	| symbol_token decl_type_rule equal_token expr_rule {
		context.capture_as($1, context.retrieve_type_annotation());
	}
	| symbol_token separator_rule def_capture_list_rule {
		context.capture($1);
	}
	| symbol_token {
		context.capture($1);
	}
	| tpl_dot_token {
		context.capture_all();
	};

def_args_rule:
	def_arg_start_rule def_arg_list_rule def_arg_stop_rule;

def_arg_start_rule:
    open_parenthesis_token;

def_arg_stop_rule:
	close_parenthesis_token minus_right_angled_token type_annotation_rule
	| close_parenthesis_token {
		context.start_type_annotation({});
	};

def_arg_list_rule:
	def_arg_rule separator_rule def_arg_list_rule
	| def_arg_rule
	| %empty;

def_arg_rule:
    symbol_token decl_type_rule {
		context.add_parameter($1, context.retrieve_type_annotation());
	}
	| symbol_token decl_type_rule equal_token expr_rule {
		context.add_function_signature();
		context.add_parameter($1, context.retrieve_type_annotation());
	}
	| modifier_rule symbol_token decl_type_rule {
		context.add_parameter($2, context.retrieve_type_annotation(), context.retrieve_modifiers());
	}
	| modifier_rule symbol_token decl_type_rule equal_token expr_rule {
		context.add_function_signature();
		context.add_parameter($2, context.retrieve_type_annotation(), context.retrieve_modifiers());
	}
	| tpl_dot_token {
		context.set_variadic();
	};

def_arrow_stmt_rule:
    def_arrow_stmt_start_rule expr_rule {
		$$ = $2;
	};

def_arrow_stmt_start_rule:
    equal_right_angled_token {
	    context.prepare_return();
	};

member_ident_rule:
    expr_rule dot_token symbol_token {
		$$ = context.get_member($1, $3);
		context.push_node(Node::Command::load_member);
		context.push_node($3.c_str());
	}
	| expr_rule dot_token operator_desc_rule {
		$$ = context.get_operator($1, context.get_operator());
		context.push_node(Node::Command::load_operator);
		context.push_node(context.retrieve_operator());
	}
	| expr_rule dot_token var_symbol_rule {
		$$ = {};
		context.push_node(Node::Command::load_var_member);
	}
	| expr_rule question_dot_token symbol_token {
		$$ = context.get_member($1, $3);
		context.push_node(Node::Command::load_defined_member);
		context.push_node($3.c_str());
	}
	| expr_rule question_dot_token operator_desc_rule {
		$$ = context.get_operator($1, context.get_operator());
		context.push_node(Node::Command::load_defined_operator);
		context.push_node(context.retrieve_operator());
	}
	| expr_rule question_dot_token var_symbol_rule {
		$$ = {};
		context.push_node(Node::Command::load_defined_var_member);
	};

defined_symbol_rule:
    symbol_token {
		context.push_node(Node::Command::find_defined_symbol);
		context.push_node($1.c_str());
		$$ = {};
	}
	| defined_symbol_rule dot_token symbol_token {
		context.push_node(Node::Command::find_defined_member);
		context.push_node($3.c_str());
		$$ = {};
	}
	| var_symbol_rule {
		context.push_node(Node::Command::find_defined_var_symbol);
		$$ = {};
	}
	| defined_symbol_rule dot_token var_symbol_rule {
		context.push_node(Node::Command::find_defined_var_member);
		$$ = {};
	}
	| constant_rule {
		context.push_node(Node::Command::load_constant);
		auto* data = context.compiler().make_data($1, Compiler::DataHint::data_unknown_hint);
		context.push_node(data);
		$$ = to_parser_data(data);
	};

decl_rule:
	explicit_decl_rule {
		$$ = $1;
	}
	| implicit_decl_rule {
		$$ = $1;
	};

explicit_decl_rule:
	let_token modifier_rule symbol_token decl_type_rule {
		$$ = context.create_fast_scoped_symbol($3, context.retrieve_type_annotation(), context.retrieve_modifiers());
	}
	| modifier_rule symbol_token decl_type_rule {
		$$ = context.create_fast_symbol($2, context.retrieve_type_annotation(), context.retrieve_modifiers());
	}
	| let_token symbol_token decl_type_rule {
		$$ = context.create_fast_scoped_symbol($2, context.retrieve_type_annotation());
	};

implicit_decl_rule:
	var_symbol_rule {
		context.push_node(Node::Command::load_var_symbol);
		$$ = {};
	}
	| symbol_token {
		$$ = context.load_fast_symbol($1);
	};

ident_rule:
	constant_rule {
		context.push_node(Node::Command::load_constant);
		auto* data = context.compiler().make_data($1, Compiler::DataHint::data_unknown_hint);
		context.push_node(data);
		$$ = to_parser_data(data);
	}
	| var_symbol_rule {
		context.push_node(Node::Command::load_var_symbol);
		$$ = {};
	}
	| symbol_token {
		$$ = context.load_fast_symbol($1);
	};

lib_start_rule:
	lib_token {
		context.push_node(Node::Command::create_lib);
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
    slash_token <token_t> {
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

parser::symbol_type yylex(mint::BuildContext& context) {
	return std::make_from_tuple<parser::symbol_type>(context.next_token());
}
