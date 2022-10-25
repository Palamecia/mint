%{

#ifndef PARSER_HPP
#define PARSER_HPP

#include "compiler/compiler.h"
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
%token for_token
%token if_token
%token in_token
%token lib_token
%token load_token
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
%left dbl_equal_token exclamation_equal_token is_token equal_tilde_token exclamation_tilde_token
%left left_angled_token right_angled_token left_angled_equal_token right_angled_equal_token
%left dbl_left_angled_token dbl_right_angled_token
%left plus_token minus_token
%left asterisk_token slash_token percent_token
%right exclamation_token tilde_token typeof_token membersof_token defined_token
%left dbl_plus_token dbl_minus_token dbl_asterisk_token
%left dot_token open_parenthesis_token close_parenthesis_token open_bracket_token close_bracket_token open_brace_token close_brace_token

%%

module_rule:
	stmt_list_rule file_end_token {
		context->pushNode(Node::module_end);
		fflush(stdout);
		YYACCEPT;
	};

stmt_list_rule:
	stmt_list_rule stmt_rule
	| stmt_rule;

stmt_rule:
	load_token module_path_rule line_end_token {
		context->pushNode(Node::load_module);
		context->pushNode($2.c_str());
	}
	| try_rule stmt_bloc_rule {
		context->pushNode(Node::unset_retrieve_point);
		context->resolveJumpForward();
		context->closeBlock();
	}
	| try_rule stmt_bloc_rule catch_rule stmt_bloc_rule {
		context->resetException();
		context->resolveJumpForward();
		context->closeBlock();
	}
	| cond_if_rule stmt_bloc_rule {
		context->resolveJumpForward();
		context->closeBlock();
	}
	| cond_if_rule stmt_bloc_rule cond_else_rule stmt_bloc_rule {
		context->resolveJumpForward();
		context->closeBlock();
	}
	| cond_if_rule stmt_bloc_rule elif_bloc_rule {
		context->resolveJumpForward();
		context->closeBlock();
	}
	| cond_if_rule stmt_bloc_rule elif_bloc_rule cond_else_rule stmt_bloc_rule {
		context->resolveJumpForward();
		context->closeBlock();
	}
	| switch_rule open_brace_token case_list_rule close_brace_token {
		context->pushNode(Node::jump);
		context->startJumpForward();
		context->buildCaseTable();
		context->resolveJumpForward();
		context->closeBlock();
	}
	| loop_rule stmt_bloc_rule {
		context->pushNode(Node::jump);
		context->resolveJumpBackward();
		context->resolveJumpForward();
		context->closeBlock();
	}
	| range_rule stmt_bloc_rule {
		context->pushNode(Node::jump);
		context->resolveJumpBackward();
		context->resolveJumpForward();
		context->closeBlock();
	}
	| break_token line_end_token {
		if (!context->isInLoop() && !context->isInSwitch()) {
			context->parse_error("break statement not within loop or switch");
			YYERROR;
		}
		context->prepareBreak();
		context->pushNode(Node::jump);
		context->blocJumpForward();
	}
	| continue_token line_end_token {
		if (!context->isInLoop()) {
			context->parse_error("continue statement not within loop");
			YYERROR;
		}
		context->prepareContinue();
		context->pushNode(Node::jump);
		context->blocJumpBackward();
	}
	| print_rule stmt_bloc_rule {
		context->closePrinter();
	}
	| yield_token expr_rule line_end_token {
		if (!context->isInFunction()) {
			context->parse_error("unexpected 'yield' statement outside of function");
			YYERROR;
		}
		context->setGenerator();
		context->pushNode(Node::yield);
	}
	| return_rule expr_rule line_end_token {
		context->setExitPoint();
		if (context->isInGenerator()) {
			context->pushNode(Node::yield_exit_generator);
		}
		else {
			context->pushNode(Node::exit_call);
		}
	}
	| raise_token expr_rule line_end_token {
		context->pushNode(Node::raise);
	}
	| exit_token expr_rule line_end_token {
		context->pushNode(Node::exit_exec);
	}
	| exit_token line_end_token {
		context->pushNode(Node::load_constant);
		context->pushNode(Compiler::makeData("0"));
		context->pushNode(Node::exit_exec);
	}
	| ident_iterator_item_rule ident_iterator_end_rule equal_token expr_rule line_end_token {
		context->pushNode(Node::copy_op);
		if (context->hasPrinter()) {
			context->pushNode(Node::print);
		}
		else {
			context->pushNode(Node::unload_reference);
		}
	}
	| expr_rule line_end_token {
		if (context->hasPrinter()) {
			context->pushNode(Node::print);
		}
		else {
			context->pushNode(Node::unload_reference);
		}
	}
	| modifier_rule def_start_rule def_capture_rule symbol_token def_args_rule stmt_bloc_rule {
		if (context->isInGenerator()) {
			context->pushNode(Node::exit_generator);
		}
		else if (!context->hasReturned()) {
			context->pushNode(Node::load_constant);
			context->pushNode(Compiler::makeNone());
			context->pushNode(Node::exit_call);
		}
		context->resolveJumpForward();
		context->pushNode(Node::create_function);
		context->pushNode($4.c_str());
		context->pushNode(Reference::global | context->retrieveModifiers());
		context->saveDefinition();
		context->pushNode(Node::function_overload);
		context->pushNode(Node::unload_reference);
	}
	| def_start_rule def_capture_rule symbol_token def_args_rule stmt_bloc_rule {
		if (context->isInGenerator()) {
			context->pushNode(Node::exit_generator);
		}
		else if (!context->hasReturned()) {
			context->pushNode(Node::load_constant);
			context->pushNode(Compiler::makeNone());
			context->pushNode(Node::exit_call);
		}
		context->resolveJumpForward();
		context->pushNode(Node::create_function);
		context->pushNode($3.c_str());
		context->pushNode(Reference::global);
		context->saveDefinition();
		context->pushNode(Node::function_overload);
		context->pushNode(Node::unload_reference);
	}
	| package_block_rule
	| class_desc_rule
	| enum_desc_rule
	| line_end_token;

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
		context->openPackage($2);
	};

package_block_rule:
	package_rule open_brace_token stmt_list_rule close_brace_token {
		context->closePackage();
	};

class_rule:
	class_token symbol_token {
		context->startClassDescription($2);
	};

parent_rule:
	dbldot_token parent_list_rule
	| ;

parent_list_rule:
	parent_ident_rule {
		context->saveBaseClassPath();
	}
	| parent_list_rule comma_token parent_ident_rule {
		context->saveBaseClassPath();
	};

parent_ident_rule:
	symbol_token {
		context->appendSymbolToBaseClassPath($1);
	}
	| parent_ident_rule dot_token symbol_token {
		context->appendSymbolToBaseClassPath($3);
	};

class_desc_rule:
	class_rule parent_rule desc_bloc_rule {
		context->resolveClassDescription();
	};

member_class_rule:
	class_rule
	| member_type_modifier_rule class_token symbol_token {
		context->startClassDescription($3, context->retrieveModifiers());
	};

member_class_desc_rule:
	member_class_rule parent_rule desc_bloc_rule {
		context->resolveClassDescription();
	};

member_enum_rule:
	enum_rule
	| member_type_modifier_rule enum_token symbol_token {
		context->startEnumDescription($3, context->retrieveModifiers());
	};

member_enum_desc_rule:
	member_enum_rule enum_block_rule {
		context->resolveEnumDescription();
	};

member_type_modifier_rule:
	plus_token {
		context->startModifiers(Reference::standard);
	}
	| sharp_token {
		context->startModifiers(Reference::protected_visibility);
	}
	| minus_token {
		context->startModifiers(Reference::private_visibility);
	}
	| tilde_token {
		context->startModifiers(Reference::package_visibility);
	};

desc_bloc_rule:
	open_brace_token desc_list_rule close_brace_token;

desc_list_rule:
	desc_list_rule desc_rule
	| desc_rule;

desc_rule:
	member_desc_rule line_end_token {
		if (!context->createMember(context->retrieveModifiers(), $1)) {
			YYERROR;
		}
	}
	| member_desc_rule equal_token constant_token line_end_token {
		if (!context->createMember(context->retrieveModifiers(), $1, Compiler::makeData($3))) {
			YYERROR;
		}
	}
	| member_desc_rule equal_token string_token line_end_token {
		if (!context->createMember(context->retrieveModifiers(), $1, Compiler::makeData($3))) {
			YYERROR;
		}
	}
	| member_desc_rule equal_token regexp_rule line_end_token {
		if (!context->createMember(context->retrieveModifiers(), $1, Compiler::makeData($3))) {
			YYERROR;
		}
	}
	| member_desc_rule equal_token regexp_rule regexp_rule symbol_token line_end_token {
		if (!context->createMember(context->retrieveModifiers(), $1, Compiler::makeData($3 + $4))) {
			YYERROR;
		}
	}
	| member_desc_rule equal_token number_token line_end_token {
		if (!context->createMember(context->retrieveModifiers(), $1, Compiler::makeData($3))) {
			YYERROR;
		}
	}
	| member_desc_rule equal_token open_bracket_token close_bracket_token line_end_token {
		if (!context->createMember(context->retrieveModifiers(), $1, Compiler::makeArray())) {
			YYERROR;
		}
	}
	| member_desc_rule equal_token open_brace_token close_brace_token line_end_token {
		if (!context->createMember(context->retrieveModifiers(), $1, Compiler::makeHash())) {
			YYERROR;
		}
	}
	| member_desc_rule equal_token lib_token open_parenthesis_token string_token close_parenthesis_token line_end_token {
		if (!context->createMember(context->retrieveModifiers(), $1, Compiler::makeLibrary($5))) {
			YYERROR;
		}
	}
	| member_desc_rule equal_token def_start_rule def_args_rule stmt_bloc_rule {
		if (context->isInGenerator()) {
			context->pushNode(Node::exit_generator);
		}
		else if (!context->hasReturned()) {
			context->pushNode(Node::load_constant);
			context->pushNode(Compiler::makeNone());
			context->pushNode(Node::exit_call);
		}
		context->resolveJumpForward();

		if (!context->createMember(context->retrieveModifiers(), $1, context->retrieveDefinition())) {
			YYERROR;
		}
	}
	| member_desc_rule plus_equal_token def_start_rule def_args_rule stmt_bloc_rule {
		if (context->isInGenerator()) {
			context->pushNode(Node::exit_generator);
		}
		else if (!context->hasReturned()) {
			context->pushNode(Node::load_constant);
			context->pushNode(Compiler::makeNone());
			context->pushNode(Node::exit_call);
		}
		context->resolveJumpForward();

		if (!context->updateMember(context->retrieveModifiers(), $1, context->retrieveDefinition())) {
			YYERROR;
		}
	}
	| def_start_rule symbol_token def_args_rule stmt_bloc_rule {
		if (context->isInGenerator()) {
			context->pushNode(Node::exit_generator);
		}
		else if (!context->hasReturned()) {
			context->pushNode(Node::load_constant);
			context->pushNode(Compiler::makeNone());
			context->pushNode(Node::exit_call);
		}
		context->resolveJumpForward();

		if (!context->updateMember(Reference::standard, $2, context->retrieveDefinition())) {
			YYERROR;
		}
	}
	| def_start_rule operator_desc_rule def_args_rule stmt_bloc_rule {
		if (context->isInGenerator()) {
			context->pushNode(Node::exit_generator);
		}
		else if (!context->hasReturned()) {
			context->pushNode(Node::load_constant);
			context->pushNode(Compiler::makeNone());
			context->pushNode(Node::exit_call);
		}
		context->resolveJumpForward();

		if (!context->updateMember(Reference::standard, context->getOperator(), context->retrieveDefinition())) {
			YYERROR;
		}
	}
	| desc_modifier_rule def_start_rule symbol_token def_args_rule stmt_bloc_rule {
		if (context->isInGenerator()) {
			context->pushNode(Node::exit_generator);
		}
		else if (!context->hasReturned()) {
			context->pushNode(Node::load_constant);
			context->pushNode(Compiler::makeNone());
			context->pushNode(Node::exit_call);
		}
		context->resolveJumpForward();

		if (!context->updateMember(context->retrieveModifiers(), $3, context->retrieveDefinition())) {
			YYERROR;
		}
	}
	| desc_modifier_rule def_start_rule operator_desc_rule def_args_rule stmt_bloc_rule {
		if (context->isInGenerator()) {
			context->pushNode(Node::exit_generator);
		}
		else if (!context->hasReturned()) {
			context->pushNode(Node::load_constant);
			context->pushNode(Compiler::makeNone());
			context->pushNode(Node::exit_call);
		}
		context->resolveJumpForward();

		if (!context->updateMember(context->retrieveModifiers(), context->getOperator(), context->retrieveDefinition())) {
			YYERROR;
		}
	}
	| member_class_desc_rule
	| member_enum_desc_rule
	| line_end_token;

member_desc_rule:
	symbol_token {
		context->startModifiers(Reference::standard);
		$$ = $1;
	}
	| desc_modifier_rule symbol_token {
		$$ = $2;
	};

desc_modifier_rule:
	modifier_rule
	| plus_token {
		context->startModifiers(Reference::standard);
	}
	| sharp_token {
		context->startModifiers(Reference::protected_visibility);
	}
	| minus_token {
		context->startModifiers(Reference::private_visibility);
	}
	| tilde_token {
		context->startModifiers(Reference::package_visibility);
	}
	| plus_token modifier_rule {
		context->addModifiers(Reference::standard);
	}
	| sharp_token modifier_rule {
		context->addModifiers(Reference::protected_visibility);
	}
	| minus_token modifier_rule {
		context->addModifiers(Reference::private_visibility);
	}
	| tilde_token modifier_rule {
		context->addModifiers(Reference::package_visibility);
	};

operator_desc_rule:
	in_token {
		context->setOperator(Class::in_operator);
		$$ = $1;
	}
	| dbldot_equal_token {
		context->setOperator(Class::copy_operator);
		$$ = $1;
	}
	| dbl_pipe_token {
		context->setOperator(Class::or_operator);
		$$ = $1;
	}
	| dbl_amp_token {
		context->setOperator(Class::and_operator);
		$$ = $1;
	}
	| pipe_token {
		context->setOperator(Class::bor_operator);
		$$ = $1;
	}
	| caret_token {
		context->setOperator(Class::xor_operator);
		$$ = $1;
	}
	| amp_token {
		context->setOperator(Class::band_operator);
		$$ = $1;
	}
	| dbl_equal_token {
		context->setOperator(Class::eq_operator);
		$$ = $1;
	}
	| exclamation_equal_token {
		context->setOperator(Class::ne_operator);
		$$ = $1;
	}
	| left_angled_token {
		context->setOperator(Class::lt_operator);
		$$ = $1;
	}
	| right_angled_token {
		context->setOperator(Class::gt_operator);
		$$ = $1;
	}
	| left_angled_equal_token {
		context->setOperator(Class::le_operator);
		$$ = $1;
	}
	| right_angled_equal_token {
		context->setOperator(Class::ge_operator);
		$$ = $1;
	}
	| dbl_left_angled_token {
		context->setOperator(Class::shift_left_operator);
		$$ = $1;
	}
	| dbl_right_angled_token {
		context->setOperator(Class::shift_right_operator);
		$$ = $1;
	}
	| plus_token {
		context->setOperator(Class::add_operator);
		$$ = $1;
	}
	| minus_token {
		context->setOperator(Class::sub_operator);
		$$ = $1;
	}
	| asterisk_token {
		context->setOperator(Class::mul_operator);
		$$ = $1;
	}
	| slash_token {
		context->setOperator(Class::div_operator);
		$$ = $1;
	}
	| percent_token {
		context->setOperator(Class::mod_operator);
		$$ = $1;
	}
	| exclamation_token {
		context->setOperator(Class::not_operator);
		$$ = $1;
	}
	| tilde_token {
		context->setOperator(Class::compl_operator);
		$$ = $1;
	}
	| dbl_plus_token {
		context->setOperator(Class::inc_operator);
		$$ = $1;
	}
	| dbl_minus_token {
		context->setOperator(Class::dec_operator);
		$$ = $1;
	}
	| dbl_asterisk_token {
		context->setOperator(Class::pow_operator);
		$$ = $1;
	}
	| dot_dot_token {
		context->setOperator(Class::inclusive_range_operator);
		$$ = $1;
	}
	| tpl_dot_token {
		context->setOperator(Class::exclusive_range_operator);
		$$ = $1;
	}
	| open_parenthesis_token close_parenthesis_token {
		context->setOperator(Class::call_operator);
		$$ = $1 + $2;
	}
	| open_bracket_token close_bracket_token {
		context->setOperator(Class::subscript_operator);
		$$ = $1 + $2;
	}
	| open_bracket_token close_bracket_equal_token {
		context->setOperator(Class::subscript_move_operator);
		$$ = $1 + $2;
	};

enum_rule:
	enum_token symbol_token {
		context->startEnumDescription($2);
	};

enum_desc_rule:
	enum_rule enum_block_rule {
		context->resolveEnumDescription();
	};

enum_block_rule:
	open_brace_token enum_list_rule close_brace_token;

enum_list_rule:
	enum_list_rule enum_item_rule
	| enum_item_rule;

enum_item_rule:
	symbol_token equal_token number_token {
		Reference::Flags flags = Reference::const_value | Reference::const_address | Reference::global;
		if (!context->createMember(flags, $1, Compiler::makeData($3))) {
			YYERROR;
		}
		context->setCurrentEnumValue(atoi($3.c_str()));
	}
	| symbol_token {
		Reference::Flags flags = Reference::const_value | Reference::const_address | Reference::global;
		if (!context->createMember(flags, $1, Compiler::makeData(std::to_string(context->nextEnumValue())))) {
			YYERROR;
		}
	}
	| line_end_token;

try_rule:
	try_token {
		context->registerRetrievePoint();
		context->pushNode(Node::set_retrieve_point);
		context->startJumpForward();
		context->openBlock(BuildContext::try_type);
	};

catch_rule:
	catch_token symbol_token {
		context->closeBlock();
		context->unregisterRetrievePoint();
		context->pushNode(Node::unset_retrieve_point);
		context->pushNode(Node::jump);
		context->startJumpForward();
		context->shiftJumpForward();
		context->resolveJumpForward();
		context->openBlock(BuildContext::catch_type);
		context->pushNode(Node::init_exception);
		context->pushNode($2.c_str());
		context->setExceptionSymbol($2);
	};

elif_bloc_rule:
	cond_elif_rule stmt_bloc_rule {
		context->shiftJumpForward();
		context->resolveJumpForward();
	}
	| elif_bloc_rule cond_elif_rule stmt_bloc_rule {
		context->shiftJumpForward();
		context->resolveJumpForward();
	};

stmt_bloc_rule:
	open_brace_token stmt_list_rule close_brace_token
	| open_brace_token yield_token expr_rule close_brace_token {
		if (!context->isInFunction()) {
			context->parse_error("unexpected 'yield' statement outside of function");
			YYERROR;
		}
		context->setGenerator();
		context->pushNode(Node::yield);
	}
	| open_brace_token return_rule expr_rule close_brace_token {
		context->setExitPoint();
		if (context->isInGenerator()) {
			context->pushNode(Node::yield_exit_generator);
		}
		else {
			context->pushNode(Node::exit_call);
		}
	}
	| open_brace_token expr_rule close_brace_token {
		if (context->hasPrinter()) {
			context->pushNode(Node::print);
		}
		else {
			context->pushNode(Node::unload_reference);
		}
	}
	| open_brace_token close_brace_token;

cond_if_rule:
	if_token expr_rule {
		context->pushNode(Node::jump_zero);
		context->startJumpForward();
		context->openBlock(BuildContext::if_type);
	}
	| if_token find_rule {
		context->pushNode(Node::jump_zero);
		context->startJumpForward();
		context->openBlock(BuildContext::if_type);
	};

cond_elif_rule:
	elif_rule expr_rule {
		context->closeBlock();
		context->pushNode(Node::jump_zero);
		context->startJumpForward();
		context->openBlock(BuildContext::elif_type);
	}
	| elif_rule find_rule {
		context->closeBlock();
		context->pushNode(Node::jump_zero);
		context->startJumpForward();
		context->openBlock(BuildContext::elif_type);
	};

elif_rule:
	elif_token {
		context->pushNode(Node::jump);
		context->startJumpForward();
		context->shiftJumpForward();
		context->resolveJumpForward();
	};

cond_else_rule:
	else_token {
		context->pushNode(Node::jump);
		context->startJumpForward();

		context->closeBlock();
		context->shiftJumpForward();
		context->resolveJumpForward();
		context->openBlock(BuildContext::else_type);
	};

switch_rule:
	switch_token expr_rule {
		context->openBlock(BuildContext::switch_type);
	};

case_rule:
	case_token {
		context->startCaseLabel();
	};

case_symbol_rule:
	symbol_token {
		context->pushNode(Node::load_symbol);
		context->pushNode($1.c_str());
		$$ = $1;
	}
	| case_symbol_rule dot_token symbol_token {
		context->pushNode(Node::load_member);
		context->pushNode($3.c_str());
		$$ = $1 + $2 + $3;
	};

case_constant_rule:
	constant_rule {
		if (Data *data = Compiler::makeData($1)) {
			context->pushNode(Node::load_constant);
			context->pushNode(data);
			$$ = $1;
		}
		else {
			error("token '" + $1 + "' is not a valid constant");
			YYERROR;
		}
	}
	| plus_token number_token {
		if (Data *data = Compiler::makeData($2)) {
			context->pushNode(Node::load_constant);
			context->pushNode(data);
			context->pushNode(Node::pos_op);
			$$ = $2;
		}
		else {
			error("token '" + $2 + "' is not a valid constant");
			YYERROR;
		}
	}
	| minus_token number_token {
		if (Data *data = Compiler::makeData($2)) {
			context->pushNode(Node::load_constant);
			context->pushNode(data);
			context->pushNode(Node::neg_op);
			$$ = $1 + $2;
		}
		else {
			error("token '" + $2 + "' is not a valid constant");
			YYERROR;
		}
	};

case_constant_list_rule:
	case_constant_list_rule case_constant_rule comma_token {
		context->addToCall();
		$$ = $1 + $2 + $3;
	}
	| case_constant_rule comma_token {
		context->startCall();
		context->addToCall();
		$$ = $1 + $2;
	};

case_constant_list_end_rule:
	case_constant_rule {
		context->pushNode(Node::create_iterator);
		context->addToCall();
		context->resolveCall();
		$$ = $1;
	}
	| {
		context->pushNode(Node::create_iterator);
		context->resolveCall();
	};

case_label_rule:
	case_rule in_token case_constant_rule dot_dot_token case_constant_rule dbldot_token {
		context->pushNode(Node::inclusive_range_op);
		context->startJumpBackward();
		context->pushNode(Node::find_next);
		context->pushNode(Node::find_check);
		context->startJumpForward();
		context->pushNode(Node::jump);
		context->resolveJumpBackward();
		context->resolveJumpForward();
		context->resolveCaseLabel($3 + $4 + $5);
	}
	| case_rule in_token case_constant_rule tpl_dot_token case_constant_rule dbldot_token {
		context->pushNode(Node::exclusive_range_op);
		context->startJumpBackward();
		context->pushNode(Node::find_next);
		context->pushNode(Node::find_check);
		context->startJumpForward();
		context->pushNode(Node::jump);
		context->resolveJumpBackward();
		context->resolveJumpForward();
		context->resolveCaseLabel($3 + $4 + $5);
	}
	| case_rule in_token case_constant_list_rule case_constant_list_end_rule dbldot_token {
		context->startJumpBackward();
		context->pushNode(Node::find_next);
		context->pushNode(Node::find_check);
		context->startJumpForward();
		context->pushNode(Node::jump);
		context->resolveJumpBackward();
		context->resolveJumpForward();
		context->resolveCaseLabel($3 + $4);
	}
	| case_rule in_token case_constant_rule dbldot_token {
		context->pushNode(Node::find_op);
		context->pushNode(Node::find_init);
		context->startJumpBackward();
		context->pushNode(Node::find_next);
		context->pushNode(Node::find_check);
		context->startJumpForward();
		context->pushNode(Node::jump);
		context->resolveJumpBackward();
		context->resolveJumpForward();
		context->resolveCaseLabel($3);
	}
	| case_rule in_token case_symbol_rule dbldot_token {
		context->pushNode(Node::find_op);
		context->pushNode(Node::find_init);
		context->startJumpBackward();
		context->pushNode(Node::find_next);
		context->pushNode(Node::find_check);
		context->startJumpForward();
		context->pushNode(Node::jump);
		context->resolveJumpBackward();
		context->resolveJumpForward();
		context->resolveCaseLabel($3);
	}
	| case_rule is_token case_constant_rule dbldot_token {
		context->pushNode(Node::is_op);
		context->resolveCaseLabel($3);
	}
	| case_rule is_token case_symbol_rule dbldot_token {
		context->pushNode(Node::is_op);
		context->resolveCaseLabel($3);
	}
	| case_rule case_constant_rule dbldot_token {
		context->pushNode(Node::eq_op);
		context->resolveCaseLabel($2);
	}
	| case_rule case_symbol_rule dbldot_token {
		context->pushNode(Node::eq_op);
		context->resolveCaseLabel($2);
	};

default_rule:
	default_token dbldot_token {
		context->setDefaultLabel();
	};

case_list_rule:
	line_end_token
	| case_label_rule stmt_list_rule
	| case_list_rule case_label_rule stmt_list_rule
	| default_rule stmt_list_rule
	| case_list_rule default_rule stmt_list_rule;

loop_rule:
	while_rule expr_rule {
		context->pushNode(Node::jump_zero);
		context->startJumpForward();

		context->openBlock(BuildContext::conditional_loop_type);
	}
	| while_rule find_rule {
		context->pushNode(Node::jump_zero);
		context->startJumpForward();

		context->openBlock(BuildContext::conditional_loop_type);
	};

while_rule:
	while_token {
		context->startJumpBackward();
	};

find_rule:
	expr_rule in_token find_init_rule {
		context->startJumpBackward();
		context->pushNode(Node::find_next);
		context->pushNode(Node::find_check);
		context->startJumpForward();
		context->pushNode(Node::jump);
		context->resolveJumpBackward();
		context->resolveJumpForward();
	}
	| expr_rule exclamation_token in_token find_init_rule {
		context->startJumpBackward();
		context->pushNode(Node::find_next);
		context->pushNode(Node::find_check);
		context->startJumpForward();
		context->pushNode(Node::jump);
		context->resolveJumpBackward();
		context->resolveJumpForward();
		context->pushNode(Node::not_op);
	};

find_init_rule:
	expr_rule {
		context->pushNode(Node::find_op);
		context->pushNode(Node::find_init);
	};

range_rule:
	for_token open_parenthesis_token range_init_rule range_next_rule range_cond_rule close_parenthesis_token {
		context->openBlock(BuildContext::custom_range_loop_type);
	}
	| for_token ident_iterator_item_rule ident_iterator_end_rule in_token expr_rule {
		context->pushNode(Node::in_op);
		context->pushNode(Node::range_init);
		context->pushNode(Node::jump);
		context->startJumpForward();
		context->startJumpBackward();
		context->pushNode(Node::range_next);
		context->resolveJumpForward();
		context->pushNode(Node::range_iterator_finalize);
		context->pushNode(Node::range_iterator_check);
		context->startJumpForward();

		context->openBlock(BuildContext::range_loop_type);
	}
	| for_token ident_rule in_token expr_rule {
		context->pushNode(Node::in_op);
		context->pushNode(Node::range_init);
		context->pushNode(Node::jump);
		context->startJumpForward();
		context->startJumpBackward();
		context->pushNode(Node::range_next);
		context->resolveJumpForward();
		context->pushNode(Node::range_check);
		context->startJumpForward();

		context->openBlock(BuildContext::range_loop_type);
	};

range_init_rule:
	expr_rule comma_token {
		context->pushNode(Node::unload_reference);
		context->pushNode(Node::jump);
		context->startJumpForward();
		context->startJumpBackward();
	};

range_next_rule:
	expr_rule comma_token {
		context->pushNode(Node::unload_reference);
		context->resolveJumpForward();
	};

range_cond_rule:
	expr_rule {
		context->pushNode(Node::jump_zero);
		context->startJumpForward();
	};

return_rule:
	return_token {
		if (!context->isInFunction()) {
			context->parse_error("unexpected 'return' statement outside of function");
			YYERROR;
		}
		context->prepareReturn();
	};

start_hash_rule:
	open_brace_token {
		context->pushNode(Node::create_hash);
	};

stop_hash_rule:
	close_brace_token;

hash_item_rule:
	hash_item_rule separator_rule expr_rule dbldot_token expr_rule {
		context->pushNode(Node::hash_insert);
	}
	| expr_rule dbldot_token expr_rule {
		context->pushNode(Node::hash_insert);
	};

start_array_rule:
	open_bracket_token {
		context->pushNode(Node::create_array);
	};

stop_array_rule:
	close_bracket_token;

array_item_rule:
	array_item_rule separator_rule expr_rule {
		context->pushNode(Node::array_insert);
	}
	| expr_rule {
		context->pushNode(Node::array_insert);
	};

iterator_item_rule:
	iterator_item_rule expr_rule separator_rule {
		context->addToCall();
	}
	| expr_rule separator_rule {
		context->startCall();
		context->addToCall();
	};

iterator_end_rule:
	expr_rule {
		context->pushNode(Node::create_iterator);
		context->addToCall();
		context->resolveCall();
	}
	| {
		context->pushNode(Node::create_iterator);
		context->resolveCall();
	};

ident_iterator_item_rule:
	ident_iterator_item_rule ident_rule separator_rule {
		context->addToCall();
	}
	| ident_rule separator_rule {
		context->startCall();
		context->addToCall();
	};

ident_iterator_end_rule:
	ident_rule {
		context->pushNode(Node::create_iterator);
		context->addToCall();
		context->resolveCall();
	}
	| {
		context->pushNode(Node::create_iterator);
		context->resolveCall();
	};

print_rule:
	print_token {
		context->pushNode(Node::load_constant);
		context->pushNode(Compiler::makeData("1"));
		context->openPrinter();
	}
	| print_token open_parenthesis_token expr_rule close_parenthesis_token {
		context->openPrinter();
	};

expr_rule:
	expr_rule equal_token expr_rule {
		context->pushNode(Node::move_op);
	}
	| expr_rule dbldot_equal_token expr_rule {
		context->pushNode(Node::copy_op);
	}
	| expr_rule plus_token expr_rule {
		context->pushNode(Node::add_op);
	}
	| expr_rule minus_token expr_rule {
		context->pushNode(Node::sub_op);
	}
	| expr_rule asterisk_token expr_rule {
		context->pushNode(Node::mul_op);
	}
	| expr_rule slash_token expr_rule {
		context->pushNode(Node::div_op);
	}
	| expr_rule percent_token expr_rule {
		context->pushNode(Node::mod_op);
	}
	| expr_rule dbl_asterisk_token expr_rule {
		context->pushNode(Node::pow_op);
	}
	| expr_rule is_token expr_rule {
		context->pushNode(Node::is_op);
	}
	| expr_rule dbl_equal_token expr_rule {
		context->pushNode(Node::eq_op);
	}
	| expr_rule exclamation_equal_token expr_rule {
		context->pushNode(Node::ne_op);
	}
	| expr_rule left_angled_token expr_rule {
		context->pushNode(Node::lt_op);
	}
	| expr_rule right_angled_token expr_rule {
		context->pushNode(Node::gt_op);
	}
	| expr_rule left_angled_equal_token expr_rule {
		context->pushNode(Node::le_op);
	}
	| expr_rule right_angled_equal_token expr_rule {
		context->pushNode(Node::ge_op);
	}
	| expr_rule dbl_left_angled_token expr_rule {
		context->pushNode(Node::shift_left_op);
	}
	| expr_rule dbl_right_angled_token expr_rule {
		context->pushNode(Node::shift_right_op);
	}
	| expr_rule dot_dot_token expr_rule {
		context->pushNode(Node::inclusive_range_op);
	}
	| expr_rule tpl_dot_token expr_rule {
		context->pushNode(Node::exclusive_range_op);
	}
	| dbl_plus_token expr_rule {
		context->pushNode(Node::inc_op);
	}
	| dbl_minus_token expr_rule {
		context->pushNode(Node::dec_op);
	}
	| expr_rule dbl_plus_token {
		context->pushNode(Node::store_reference);
		context->pushNode(Node::inc_op);
		context->pushNode(Node::unload_reference);
	}
	| expr_rule dbl_minus_token {
		context->pushNode(Node::store_reference);
		context->pushNode(Node::dec_op);
		context->pushNode(Node::unload_reference);
	}
	| exclamation_token expr_rule {
		context->pushNode(Node::not_op);
	}
	| expr_rule dbl_pipe_token {
		context->pushNode(Node::or_pre_check);
		context->startJumpForward();
	} expr_rule {
		context->pushNode(Node::or_op);
		context->resolveJumpForward();
	}
	| expr_rule dbl_amp_token {
		context->pushNode(Node::and_pre_check);
		context->startJumpForward();
	} expr_rule {
		context->pushNode(Node::and_op);
		context->resolveJumpForward();
	}
	| expr_rule pipe_token expr_rule {
		context->pushNode(Node::bor_op);
	}
	| expr_rule amp_token expr_rule {
		context->pushNode(Node::band_op);
	}
	| expr_rule caret_token expr_rule {
		context->pushNode(Node::xor_op);
	}
	| tilde_token expr_rule {
		context->pushNode(Node::compl_op);
	}
	| plus_token expr_rule {
		context->pushNode(Node::pos_op);
	}
	| minus_token expr_rule {
		context->pushNode(Node::neg_op);
	}
	| typeof_token expr_rule {
		context->pushNode(Node::typeof_op);
	}
	| membersof_token expr_rule {
		context->pushNode(Node::membersof_op);
	}
	| defined_token defined_symbol_rule {
		context->pushNode(Node::check_defined);
	}
	| expr_rule open_bracket_token expr_rule close_bracket_equal_token expr_rule {
		context->pushNode(Node::subscript_move_op);
	}
	| expr_rule subscript_rule
	| member_ident_rule
	| ident_rule call_args_rule
	| def_rule call_args_rule
	| expr_rule subscript_rule call_args_rule
	| expr_rule dot_token call_member_args_rule
	| open_parenthesis_token expr_rule close_parenthesis_token call_args_rule
	| expr_rule plus_equal_token {
		context->pushNode(Node::reload_reference);
	} expr_rule {
		context->pushNode(Node::add_op);
		context->pushNode(Node::move_op);
	}
	| expr_rule minus_equal_token {
		context->pushNode(Node::reload_reference);
	} expr_rule {
		context->pushNode(Node::sub_op);
		context->pushNode(Node::move_op);
	}
	| expr_rule asterisk_equal_token {
		context->pushNode(Node::reload_reference);
	} expr_rule {
		context->pushNode(Node::mul_op);
		context->pushNode(Node::move_op);
	}
	| expr_rule slash_equal_token {
		context->pushNode(Node::reload_reference);
	} expr_rule {
		context->pushNode(Node::div_op);
		context->pushNode(Node::move_op);
	}
	| expr_rule percent_equal_token {
		context->pushNode(Node::reload_reference);
	} expr_rule {
		context->pushNode(Node::mod_op);
		context->pushNode(Node::move_op);
	}
	| expr_rule dbl_left_angled_equal_token {
		context->pushNode(Node::reload_reference);
	} expr_rule {
		context->pushNode(Node::shift_left_op);
		context->pushNode(Node::move_op);
	}
	| expr_rule dbl_right_angled_equal_token {
		context->pushNode(Node::reload_reference);
	} expr_rule {
		context->pushNode(Node::shift_right_op);
		context->pushNode(Node::move_op);
	}
	| expr_rule amp_equal_token {
		context->pushNode(Node::reload_reference);
	} expr_rule {
		context->pushNode(Node::band_op);
		context->pushNode(Node::move_op);
	}
	| expr_rule pipe_equal_token {
		context->pushNode(Node::reload_reference);
	} expr_rule {
		context->pushNode(Node::bor_op);
		context->pushNode(Node::move_op);
	}
	| expr_rule caret_equal_token {
		context->pushNode(Node::reload_reference);
	} expr_rule {
		context->pushNode(Node::xor_op);
		context->pushNode(Node::move_op);
	}
	| expr_rule equal_tilde_token expr_rule {
		context->pushNode(Node::regex_match);
	}
	| expr_rule exclamation_tilde_token expr_rule {
		context->pushNode(Node::regex_unmatch);
	}
	| expr_rule question_token {
		context->pushNode(Node::jump_zero);
		context->startJumpForward();
	} expr_rule dbldot_token {
		context->pushNode(Node::jump);
		context->startJumpForward();
		context->shiftJumpForward();
		context->resolveJumpForward();
	} expr_rule {
		context->resolveJumpForward();
	}
	| open_parenthesis_token close_parenthesis_token {
		context->startCall();
		context->pushNode(Node::create_iterator);
		context->resolveCall();
	}
	| open_parenthesis_token expr_rule close_parenthesis_token
	| open_parenthesis_token iterator_item_rule iterator_end_rule close_parenthesis_token
	| start_array_rule empty_lines_rule array_item_rule empty_lines_rule stop_array_rule
	| start_array_rule empty_lines_rule array_item_rule stop_array_rule
	| start_array_rule array_item_rule empty_lines_rule stop_array_rule
	| start_array_rule array_item_rule stop_array_rule
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
		context->pushNode(Node::subscript_op);
	};

call_args_rule:
	call_arg_start_rule call_arg_list_rule call_arg_stop_rule
	| call_args_rule call_arg_start_rule call_arg_list_rule call_arg_stop_rule;

call_member_args_rule:
	call_member_arg_start_rule call_arg_list_rule call_member_arg_stop_rule
	| call_member_args_rule call_arg_start_rule call_arg_list_rule call_arg_stop_rule;

call_arg_start_rule:
	open_parenthesis_token {
		context->pushNode(Node::init_call);
		context->startCall();
	};

call_arg_stop_rule:
	close_parenthesis_token {
		context->pushNode(Node::call);
		context->resolveCall();
	};

call_member_arg_start_rule:
	symbol_token open_parenthesis_token {
		context->pushNode(Node::init_member_call);
		context->pushNode($1.c_str());
		context->startCall();
	}
	| operator_desc_rule open_parenthesis_token {
		context->pushNode(Node::init_operator_call);
		context->pushNode(context->getOperator());
		context->startCall();
	}
	| var_symbol_rule open_parenthesis_token {
		context->pushNode(Node::init_var_member_call);
		context->startCall();
	};

call_member_arg_stop_rule:
	close_parenthesis_token {
		context->pushNode(Node::call_member);
		context->resolveCall();
	};

call_arg_list_rule:
	call_arg_list_rule separator_rule call_arg_rule
	| call_arg_rule
	| ;

call_arg_rule:
	expr_rule {
		context->addToCall();
	}
	| asterisk_token expr_rule {
		context->pushNode(Node::in_op);
		context->pushNode(Node::finalize_generator);
		context->pushNode(Node::load_extra_arguments);
	};

def_rule:
	def_start_rule def_capture_rule def_args_rule stmt_bloc_rule {
		if (context->isInGenerator()) {
			context->pushNode(Node::exit_generator);
		}
		else if (!context->hasReturned()) {
			context->pushNode(Node::load_constant);
			context->pushNode(Compiler::makeNone());
			context->pushNode(Node::exit_call);
		}
		context->resolveJumpForward();
		context->saveDefinition();
	}
	| def_start_rule def_capture_rule def_no_args_rule stmt_bloc_rule {
		if (context->isInGenerator()) {
			context->pushNode(Node::exit_generator);
		}
		else if (!context->hasReturned()) {
			context->pushNode(Node::load_constant);
			context->pushNode(Compiler::makeNone());
			context->pushNode(Node::exit_call);
		}
		context->resolveJumpForward();
		context->saveDefinition();
	};

def_start_rule:
	def_token {
		context->pushNode(Node::jump);
		context->startJumpForward();
		context->startDefinition();
	};

def_capture_rule:
	def_capture_start_rule def_capture_list_rule def_capture_stop_rule
	| ;

def_capture_start_rule:
	open_bracket_token {
		context->startCapture();
	};

def_capture_stop_rule:
	close_bracket_token {
		context->resolveCapture();
	};

def_capture_list_rule:
	symbol_token equal_token expr_rule separator_rule def_capture_list_rule {
		if (!context->captureAs($1)) {
			YYERROR;
		}
	}
	| symbol_token equal_token expr_rule {
		if (!context->captureAs($1)) {
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
		if (!context->captureAll()) {
			YYERROR;
		}
	};

def_no_args_rule:
	{
		if (!context->saveParameters()) {
			YYERROR;
		}
	};

def_args_rule:
	def_arg_start_rule def_arg_list_rule def_arg_stop_rule;

def_arg_start_rule:
	open_parenthesis_token;

def_arg_stop_rule:
	close_parenthesis_token {
		if (!context->saveParameters()) {
			YYERROR;
		}
	};

def_arg_list_rule:
	def_arg_rule separator_rule def_arg_list_rule
	| def_arg_rule
	| ;

def_arg_rule:
	symbol_token {
		if (!context->addParameter($1)) {
			YYERROR;
		}
	}
	| symbol_token equal_token expr_rule {
		if (!context->addDefinitionSignature()) {
			YYERROR;
		}
		if (!context->addParameter($1)) {
			YYERROR;
		}
	}
	| modifier_rule symbol_token {
		if (!context->addParameter($2, context->retrieveModifiers())) {
			YYERROR;
		}
	}
	| modifier_rule symbol_token equal_token expr_rule {
		if (!context->addDefinitionSignature()) {
			YYERROR;
		}
		if (!context->addParameter($2, context->retrieveModifiers())) {
			YYERROR;
		}
	}
	| tpl_dot_token {
		if (!context->setVariadic()) {
			YYERROR;
		}
	};

member_ident_rule:
	expr_rule dot_token symbol_token {
		context->pushNode(Node::load_member);
		context->pushNode($3.c_str());
	}
	| expr_rule dot_token operator_desc_rule {
		context->pushNode(Node::load_operator);
		context->pushNode(context->getOperator());
	}
	| expr_rule dot_token var_symbol_rule {
		context->pushNode(Node::load_var_member);
	};

defined_symbol_rule:
	symbol_token {
		context->pushNode(Node::find_defined_symbol);
		context->pushNode($1.c_str());
	}
	| defined_symbol_rule dot_token symbol_token {
		context->pushNode(Node::find_defined_member);
		context->pushNode($3.c_str());
	}
	| var_symbol_rule {
		context->pushNode(Node::find_defined_var_symbol);
		context->pushNode($1.c_str());
	}
	| defined_symbol_rule dot_token var_symbol_rule {
		context->pushNode(Node::find_defined_var_member);
		context->pushNode($3.c_str());
	}
	| constant_rule {
		context->pushNode(Node::load_constant);
		if (Data *data = Compiler::makeData($1.c_str())) {
			context->pushNode(data);
		}
		else {
			error("token '" + $1 + "' is not a valid constant");
			YYERROR;
		}
	};

ident_rule:
	constant_rule {
		context->pushNode(Node::load_constant);
		if (Data *data = Compiler::makeData($1.c_str())) {
			context->pushNode(data);
		}
		else {
			error("token '" + $1 + "' is not a valid constant");
			YYERROR;
		}
	}
	| lib_token {
		context->pushNode(Node::create_lib);
	}
	| symbol_token {
		int index = context->fastSymbolIndex($1);
		if (index != -1) {
			context->pushNode(Node::load_fast);
			context->pushNode($1.c_str());
			context->pushNode(index);
		}
		else {
			context->pushNode(Node::load_symbol);
			context->pushNode($1.c_str());
		}
	}
	| var_symbol_rule {
		context->pushNode(Node::load_var_symbol);
	}
	| modifier_rule symbol_token {
		int index = context->fastSymbolIndex($2);
		if (index != -1) {
			context->pushNode(Node::create_fast);
			context->pushNode($2.c_str());
			context->pushNode(index);
			context->pushNode(context->retrieveModifiers());
		}
		else {
			context->pushNode(Node::create_symbol);
			context->pushNode($2.c_str());
			context->pushNode(context->retrieveModifiers());
		}
	};

constant_rule:
	constant_token {
		$$ = $1;
	}
	| regexp_rule {
		$$ = $1;
	}
	| regexp_rule symbol_token {
		$$ = $1 + $2;
	}
	| string_token {
		$$ = $1;
	}
	| number_token {
		$$ = $1;
	};

regexp_rule:
	slash_token {
		$$ = $1 + context->lexer.readRegex();
	} slash_token {
		$$ = $2 + $1;
	};

var_symbol_rule:
	dollar_token open_parenthesis_token expr_rule close_parenthesis_token;

modifier_rule:
	var_token {
		context->startModifiers(Reference::standard);
	}
	| dollar_token {
		context->startModifiers(Reference::const_address);
	}
	| percent_token {
		context->startModifiers(Reference::const_value);
	}
	| const_token {
		context->startModifiers(Reference::const_address | Reference::const_value);
	}
	| at_token {
		context->startModifiers(Reference::global);
	}
	| modifier_rule var_token {
		context->addModifiers(Reference::standard);
	}
	| modifier_rule dollar_token {
		context->addModifiers(Reference::const_address);
	}
	| modifier_rule percent_token {
		context->addModifiers(Reference::const_value);
	}
	| modifier_rule const_token {
		context->addModifiers(Reference::const_address | Reference::const_value);
	}
	| modifier_rule at_token {
		context->addModifiers(Reference::global);
	};

separator_rule:
	comma_token | separator_rule line_end_token;

empty_lines_rule:
	line_end_token | empty_lines_rule line_end_token;

%%

void parser::error(const std::string &msg) {
	context->parse_error(msg.c_str());
}

int BuildContext::next_token(std::string *token) {

	if (lexer.atEnd()) {
		return parser::token::file_end_token;
	}

	*token = lexer.nextToken();
	return lexer.tokenType(*token);
}

bool Compiler::build(DataStream *stream, Module::Infos node) {

	std::unique_ptr<BuildContext> context(new BuildContext(stream, node));
	parser parser(context.get());

	if (isPrinting()) {
		context->forcePrinter();
	}

	return !parser.parse();
}

#endif // PARSER_HPP
