%{

#ifndef PARSER_HPP
#define PARSER_HPP

#include "compiler/compiler.h"

#define YYSTYPE std::string
#define yylex context->next_token

using namespace mint;

%}

%define api.namespace {mint}
%parse-param {mint::BuildContext *context}

%token assert_token
%token break_token
%token catch_token
%token class_token
%token const_token
%token continue_token
%token def_token
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
%token try_token
%token while_token
%token yield_token
%token constant_token
%token string_token
%token number_token
%token symbol_token

%token line_end_token
%token file_end_token
%token comment_token
%token dollar_token
%token at_token
%token sharp_token

%left comma_token
%left dbl_pipe_token
%left dbl_amp_token
%left pipe_token
%left caret_token
%left amp_token
%right equal_token question_token dbldot_token dbldot_equal_token plus_equal_token minus_equal_token asterisk_equal_token slash_equal_token percent_equal_token dbl_left_angled_equal_token dbl_right_angled_equal_token amp_equal_token pipe_equal_token caret_equal_token
%left dot_dot_token tpl_dot_token
%left dbl_equal_token exclamation_equal_token is_token equal_tilde_token exclamation_tilde_token
%left left_angled_token right_angled_token left_angled_equal_token right_angled_equal_token
%left dbl_left_angled_token dbl_right_angled_token
%left plus_token minus_token
%left asterisk_token slash_token percent_token
%right exclamation_token tilde_token typeof_token membersof_token defined_token
%left dbl_plus_token dbl_minus_token dbl_asterisk_token
%left dot_token open_parenthesis_token close_parenthesis_token open_bracket_token close_bracket_token open_brace_token close_brace_token close_bracket_equal_token

%%

module_rule:
	stmt_list_rule file_end_token {
		DEBUG_STACK(context, "END");
		context->pushNode(Node::module_end);
		fflush(stdout);
		YYACCEPT;
	};

stmt_list_rule:
	stmt_list_rule stmt_rule
	| stmt_rule;

stmt_rule:
	load_token module_path_rule line_end_token {
		DEBUG_STACK(context, "LOAD MODULE %s", $2.c_str());
		context->pushNode(Node::load_module);
		context->pushNode($2.c_str());
	}
	| try_rule stmt_bloc_rule {
		DEBUG_STACK(context, "UNTRY");
		context->pushNode(Node::unset_retrieve_point);

		DEBUG_STACK(context, "LBL FWD");
		context->resolveJumpForward();
	}
	| try_rule stmt_bloc_rule catch_rule stmt_bloc_rule {
		DEBUG_STACK(context, "LBL FWD");
		context->resolveJumpForward();
	}
	| cond_if_rule stmt_bloc_rule {
		DEBUG_STACK(context, "LBL FWD");
		context->resolveJumpForward();
	}
	| cond_if_rule stmt_bloc_rule cond_else_rule stmt_bloc_rule {
		DEBUG_STACK(context, "LBL FWD");
		context->resolveJumpForward();
	}
	| cond_if_rule stmt_bloc_rule elif_bloc_rule {
		DEBUG_STACK(context, "LBL FWD");
		context->resolveJumpForward();
	}
	| cond_if_rule stmt_bloc_rule elif_bloc_rule cond_else_rule stmt_bloc_rule {
		DEBUG_STACK(context, "LBL FWD");
		context->resolveJumpForward();
	}
	| loop_rule stmt_bloc_rule {
		DEBUG_STACK(context, "JMP BWD");
		context->pushNode(Node::jump);
		context->resolveJumpBackward();
		DEBUG_STACK(context, "LBL FWD");
		context->resolveJumpForward();
		context->endLoop();
	}
	| range_rule stmt_bloc_rule {
		DEBUG_STACK(context, "JMP BWD");
		context->pushNode(Node::jump);
		context->resolveJumpBackward();
		DEBUG_STACK(context, "LBL FWD");
		context->resolveJumpForward();
		context->endLoop();
	}
	| break_token line_end_token {
		if (!context->isInLoop()) {
			context->parse_error("break statement not within loop");
			YYERROR;
		}
		DEBUG_STACK(context, "JMP FWD");
		context->pushNode(Node::jump);
		context->loopJumpForward();
	}
	| continue_token line_end_token {
		if (!context->isInLoop()) {
			context->parse_error("continue statement not within loop");
			YYERROR;
		}
		DEBUG_STACK(context, "JMP BWD");
		context->pushNode(Node::jump);
		context->loopJumpBackward();
	}
	| print_rule stmt_bloc_rule {
		DEBUG_STACK(context, "CLOSE PRINTER");
		context->pushNode(Node::close_printer);
	}
	| yield_token expr_rule line_end_token {
		DEBUG_STACK(context, "YIELD");
		context->pushNode(Node::yield);
	}
	| return_rule expr_rule line_end_token {
		DEBUG_STACK(context, "EXIT CALL");
		context->pushNode(Node::exit_call);
	}
	| raise_token expr_rule line_end_token {
		DEBUG_STACK(context, "RAISE");
		context->pushNode(Node::raise);
	}
	| exit_token expr_rule line_end_token {
		DEBUG_STACK(context, "EXIT EXEC");
		context->pushNode(Node::exit_exec);
	}
	| exit_token line_end_token {
		DEBUG_STACK(context, "PUSH 0");
		context->pushNode(Node::load_constant);
		context->pushNode(Compiler::makeData("0"));
		DEBUG_STACK(context, "EXIT EXEC");
		context->pushNode(Node::exit_exec);
	}
	| expr_rule line_end_token {
		DEBUG_STACK(context, "PRINT");
		context->pushNode(Node::print);
	}
	| modifier_rule def_start_rule def_capture_rule symbol_token def_args_rule stmt_bloc_rule {
		DEBUG_STACK(context, "LOAD_DR");
		context->pushNode(Node::load_default_result);
		DEBUG_STACK(context, "EXIT CALL");
		context->pushNode(Node::exit_call);
		DEBUG_STACK(context, "LBL FWD");
		context->resolveJumpForward();
		DEBUG_STACK(context, "NEW GLOBAL %s", $3.c_str());
		context->pushNode(Node::create_symbol);
		context->pushNode($3.c_str());
		context->pushNode(context->getModifiers() | Reference::global);
		DEBUG_STACK(context, "PUSH DEF");
		context->saveDefinition();
		DEBUG_STACK(context, "MOVE");
		context->pushNode(Node::move_op);
	}
	| def_start_rule symbol_token def_capture_rule def_args_rule stmt_bloc_rule {
		DEBUG_STACK(context, "LOAD_DR");
		context->pushNode(Node::load_default_result);
		DEBUG_STACK(context, "EXIT CALL");
		context->pushNode(Node::exit_call);
		DEBUG_STACK(context, "LBL FWD");
		context->resolveJumpForward();
		DEBUG_STACK(context, "NEW GLOBAL %s", $2.c_str());
		context->pushNode(Node::create_symbol);
		context->pushNode($2.c_str());
		context->pushNode(Reference::global);
		DEBUG_STACK(context, "PUSH DEF");
		context->saveDefinition();
		DEBUG_STACK(context, "MOVE");
		context->pushNode(Node::move_op);
		DEBUG_STACK(context, "POP");
		context->pushNode(Node::unload_reference);
	}
	| package_block_rule
	| class_desc_rule
	| enum_desc_rule
	| line_end_token;

module_path_rule:
	symbol_token {
		$$ = $1;
	}
	| module_path_rule dot_token symbol_token {
		$$ = $1 + $2 + $3;
	}
	| module_path_rule minus_token symbol_token {
		$$ = $1 + $2 + $3;
	};

package_rule:
	package_token symbol_token {
		DEBUG_STACK(context, "PACKAGE %s", $2.c_str());
		context->openPackage($2);
	};

package_block_rule:
	package_rule open_brace_token stmt_list_rule close_brace_token {
		context->closePackage();
	};

class_rule:
	class_token symbol_token {
		DEBUG_STACK(context, "CLASS %s", $2.c_str());
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
		DEBUG_STACK(context, "CLASS %s", $3.c_str());
		context->startClassDescription($3, context->getModifiers());
	};

member_class_desc_rule:
	member_class_rule parent_rule desc_bloc_rule {
		context->resolveClassDescription();
	};

member_enum_rule:
	enum_rule
	| member_type_modifier_rule enum_token symbol_token {
		DEBUG_STACK(context, "ENUM %s", $3.c_str());
		context->startEnumDescription($3, context->getModifiers());
	};

member_enum_desc_rule:
	member_enum_rule enum_block_rule {
		context->resolveEnumDescription();
	};

member_type_modifier_rule:
	plus_token {
		context->setModifiers(Reference::standard);
	}
	| sharp_token {
		context->setModifiers(Reference::protected_visibility);
	}
	| minus_token {
		context->setModifiers(Reference::private_visibility);
	}
	| tilde_token {
		context->setModifiers(Reference::package_visibility);
	};

desc_bloc_rule:
	open_brace_token desc_list_rule close_brace_token;

desc_list_rule:
	desc_list_rule desc_rule
	| desc_rule;

desc_rule:
	member_desc_rule line_end_token {
		if (!context->createMember(context->getModifiers(), $1)) {
			YYERROR;
		}
	}
	| member_desc_rule equal_token constant_token line_end_token {
		if (!context->createMember(context->getModifiers(), $1, Compiler::makeData($3))) {
			YYERROR;
		}
	}
	| member_desc_rule equal_token string_token line_end_token {
		if (!context->createMember(context->getModifiers(), $1, Compiler::makeData($3))) {
			YYERROR;
		}
	}
	| member_desc_rule equal_token number_token line_end_token {
		if (!context->createMember(context->getModifiers(), $1, Compiler::makeData($3))) {
			YYERROR;
		}
	}
	| member_desc_rule equal_token open_bracket_token close_bracket_token line_end_token {
		if (!context->createMember(context->getModifiers(), $1, Compiler::makeArray())) {
			YYERROR;
		}
	}
	| member_desc_rule equal_token open_brace_token close_brace_token line_end_token {
		if (!context->createMember(context->getModifiers(), $1, Compiler::makeHash())) {
			YYERROR;
		}
	}
	| member_desc_rule equal_token lib_token open_parenthesis_token string_token close_parenthesis_token line_end_token {
		if (!context->createMember(context->getModifiers(), $1, Compiler::makeLibrary($5))) {
			YYERROR;
		}
	}
	| member_desc_rule equal_token def_start_rule def_args_rule stmt_bloc_rule {
		DEBUG_STACK(context, "LOAD_DR");
		context->pushNode(Node::load_default_result);
		DEBUG_STACK(context, "EXIT CALL");
		context->pushNode(Node::exit_call);
		DEBUG_STACK(context, "LBL FWD");
		context->resolveJumpForward();

		if (!context->createMember(context->getModifiers(), $1, context->retrieveDefinition())) {
			YYERROR;
		}
	}
	| member_desc_rule plus_equal_token def_start_rule def_args_rule stmt_bloc_rule {
		DEBUG_STACK(context, "LOAD_DR");
		context->pushNode(Node::load_default_result);
		DEBUG_STACK(context, "EXIT CALL");
		context->pushNode(Node::exit_call);
		DEBUG_STACK(context, "LBL FWD");
		context->resolveJumpForward();

		if (!context->updateMember(context->getModifiers(), $1, context->retrieveDefinition())) {
			YYERROR;
		}
	}
	| def_start_rule symbol_token def_args_rule stmt_bloc_rule {
		DEBUG_STACK(context, "LOAD_DR");
		context->pushNode(Node::load_default_result);
		DEBUG_STACK(context, "EXIT CALL");
		context->pushNode(Node::exit_call);
		DEBUG_STACK(context, "LBL FWD");
		context->resolveJumpForward();

		if (!context->updateMember(Reference::standard, $2, context->retrieveDefinition())) {
			YYERROR;
		}
	}
	| def_start_rule operator_desc_rule def_args_rule stmt_bloc_rule {
		DEBUG_STACK(context, "LOAD_DR");
		context->pushNode(Node::load_default_result);
		DEBUG_STACK(context, "EXIT CALL");
		context->pushNode(Node::exit_call);
		DEBUG_STACK(context, "LBL FWD");
		context->resolveJumpForward();

		if (!context->updateMember(Reference::standard, $2, context->retrieveDefinition())) {
			YYERROR;
		}
	}
	| desc_modifier_rule def_start_rule symbol_token def_args_rule stmt_bloc_rule {
		DEBUG_STACK(context, "LOAD_DR");
		context->pushNode(Node::load_default_result);
		DEBUG_STACK(context, "EXIT CALL");
		context->pushNode(Node::exit_call);
		DEBUG_STACK(context, "LBL FWD");
		context->resolveJumpForward();

		if (!context->updateMember(context->getModifiers(), $3, context->retrieveDefinition())) {
			YYERROR;
		}
	}
	| member_class_desc_rule
	| member_enum_desc_rule
	| line_end_token;

member_desc_rule:
	symbol_token {
		context->setModifiers(Reference::standard);
		$$ = $1;
	}
	| desc_modifier_rule symbol_token {
		$$ = $2;
	}
	| operator_desc_rule {
		context->setModifiers(Reference::standard);
		$$ = $1;
	};

desc_modifier_rule:
	modifier_rule
	| plus_token {
		context->setModifiers(Reference::standard);
	}
	| sharp_token {
		context->setModifiers(Reference::protected_visibility);
	}
	| minus_token {
		context->setModifiers(Reference::private_visibility);
	}
	| tilde_token {
		context->setModifiers(Reference::package_visibility);
	}
	| plus_token modifier_rule {
		context->setModifiers(context->getModifiers() | Reference::standard);
	}
	| sharp_token modifier_rule {
		context->setModifiers(context->getModifiers() | Reference::protected_visibility);
	}
	| minus_token modifier_rule {
		context->setModifiers(context->getModifiers() | Reference::private_visibility);
	}
	| tilde_token modifier_rule {
		context->setModifiers(context->getModifiers() | Reference::package_visibility);
	};

operator_desc_rule:
	in_token { $$ = $1; }
	| dbldot_equal_token { $$ = $1; }
	| dbl_pipe_token { $$ = $1; }
	| dbl_amp_token { $$ = $1; }
	| pipe_token { $$ = $1; }
	| caret_token { $$ = $1; }
	| amp_token { $$ = $1; }
	| dbl_equal_token { $$ = $1; }
	| exclamation_equal_token { $$ = $1; }
	| left_angled_token { $$ = $1; }
	| right_angled_token { $$ = $1; }
	| left_angled_equal_token { $$ = $1; }
	| right_angled_equal_token { $$ = $1; }
	| dbl_left_angled_token { $$ = $1; }
	| dbl_right_angled_token { $$ = $1; }
	| plus_token { $$ = $1; }
	| minus_token { $$ = $1; }
	| asterisk_token { $$ = $1; }
	| slash_token { $$ = $1; }
	| percent_token { $$ = $1; }
	| exclamation_token { $$ = $1; }
	| tilde_token { $$ = $1; }
	| dbl_plus_token { $$ = $1; }
	| dbl_minus_token { $$ = $1; }
	| dbl_asterisk_token { $$ = $1; }
	| open_parenthesis_token close_parenthesis_token { $$ = $1 + $2; }
	| open_bracket_token close_bracket_token { $$ = $1 + $2; }
	| open_bracket_token close_bracket_equal_token { $$ = $1 + $2; };

enum_rule:
	enum_token symbol_token {
		DEBUG_STACK(context, "ENUM %s", $2.c_str());
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
		DEBUG_STACK(context, "TRY");
		context->pushNode(Node::set_retrieve_point);
		context->startJumpForward();
	};

catch_rule:
	catch_token symbol_token {
		DEBUG_STACK(context, "UNTRY");
		context->pushNode(Node::unset_retrieve_point);

		DEBUG_STACK(context, "JMP FWD");
		context->pushNode(Node::jump);
		context->startJumpForward();

		DEBUG_STACK(context, "FWD LBL");
		context->shiftJumpForward();
		context->resolveJumpForward();

		DEBUG_STACK(context, "INIT %s", $2.c_str());
		context->pushNode(Node::init_param);
		context->pushNode($2.c_str());
	};

elif_bloc_rule:
	cond_elif_rule stmt_bloc_rule {
		DEBUG_STACK(context, "LBL FWD");
		context->shiftJumpForward();
		context->resolveJumpForward();
	}
	| elif_bloc_rule cond_elif_rule stmt_bloc_rule {
		DEBUG_STACK(context, "LBL FWD");
		context->shiftJumpForward();
		context->resolveJumpForward();
	};

stmt_bloc_rule:
	open_brace_token stmt_list_rule close_brace_token
	| open_brace_token expr_rule close_brace_token {
		DEBUG_STACK(context, "PRINT");
		context->pushNode(Node::print);
	}
	| open_brace_token close_brace_token;

cond_if_rule:
	if_token expr_rule {
		DEBUG_STACK(context, "JZR FWD");
		context->pushNode(Node::jump_zero);
		context->startJumpForward();
	}
	| if_token find_in_rule {
		DEBUG_STACK(context, "JZR FWD");
		context->pushNode(Node::jump_zero);
		context->startJumpForward();
	};

cond_elif_rule:
	elif_rule expr_rule {
		DEBUG_STACK(context, "JZR FWD");
		context->pushNode(Node::jump_zero);
		context->startJumpForward();
	}
	| elif_rule find_in_rule {
		DEBUG_STACK(context, "JZR FWD");
		context->pushNode(Node::jump_zero);
		context->startJumpForward();
	};

elif_rule:
	elif_token {
		DEBUG_STACK(context, "JMP FWD");
		context->pushNode(Node::jump);
		context->startJumpForward();
		DEBUG_STACK(context, "LBL FWD");
		context->shiftJumpForward();
		context->resolveJumpForward();
	};

cond_else_rule:
	else_token {
		DEBUG_STACK(context, "JMP FWD");
		context->pushNode(Node::jump);
		context->startJumpForward();
		DEBUG_STACK(context, "LBL FWD");
		context->shiftJumpForward();
		context->resolveJumpForward();
	};

loop_rule:
	while_rule expr_rule {
		DEBUG_STACK(context, "JZR FWD");
		context->pushNode(Node::jump_zero);
		context->startJumpForward();

		context->beginLoop(BuildContext::conditional_loop);
	}
	| while_rule find_in_rule {
		DEBUG_STACK(context, "JZR FWD");
		context->pushNode(Node::jump_zero);
		context->startJumpForward();

		context->beginLoop(BuildContext::conditional_loop);
	};

while_rule:
	while_token {
		DEBUG_STACK(context, "LBL BWD");
		context->startJumpBackward();
	};

find_in_rule:
	expr_rule in_token find_init_rule {
		DEBUG_STACK(context, "LBL BWD");
		context->startJumpBackward();
		DEBUG_STACK(context, "FIND NEXT");
		context->pushNode(Node::find_next);
		DEBUG_STACK(context, "FIND CHECK");
		context->pushNode(Node::find_check);
		context->startJumpForward();
		DEBUG_STACK(context, "JMP BWD");
		context->pushNode(Node::jump);
		context->resolveJumpBackward();
		context->resolveJumpForward();
	}
	| expr_rule exclamation_token in_token find_init_rule {
		DEBUG_STACK(context, "LBL BWD");
		context->startJumpBackward();
		DEBUG_STACK(context, "FIND NEXT");
		context->pushNode(Node::find_next);
		DEBUG_STACK(context, "FIND CHECK");
		context->pushNode(Node::find_check);
		context->startJumpForward();
		DEBUG_STACK(context, "JMP BWD");
		context->pushNode(Node::jump);
		context->resolveJumpBackward();
		context->resolveJumpForward();
		DEBUG_STACK(context, "NOT");
		context->pushNode(Node::not_op);
	};

find_init_rule:
	expr_rule {
		DEBUG_STACK(context, "IN");
		context->pushNode(Node::in_op);
		DEBUG_STACK(context, "FIND INIT");
		context->pushNode(Node::find_init);
	};

range_rule:
	for_token range_init_rule range_next_rule range_cond_rule {
		context->beginLoop(BuildContext::custom_range_loop);
	}
	| for_token ident_rule in_token expr_rule {
		DEBUG_STACK(context, "IN");
		context->pushNode(Node::in_op);
		DEBUG_STACK(context, "RANGE INIT");
		context->pushNode(Node::range_init);
		DEBUG_STACK(context, "JMP FWD");
		context->pushNode(Node::jump);
		context->startJumpForward();

		DEBUG_STACK(context, "LBL BWD");
		context->startJumpBackward();
		DEBUG_STACK(context, "RANGE NEXT");
		context->pushNode(Node::range_next);
		DEBUG_STACK(context, "LBL FWD");
		context->resolveJumpForward();

		DEBUG_STACK(context, "RANGE CHECK");
		context->pushNode(Node::range_check);
		context->startJumpForward();

		context->beginLoop(BuildContext::range_loop);
	};

range_init_rule:
	expr_rule comma_token {
		DEBUG_STACK(context, "POP");
		context->pushNode(Node::unload_reference);
		DEBUG_STACK(context, "JMP FWD");
		context->pushNode(Node::jump);
		context->startJumpForward();
		DEBUG_STACK(context, "LBL BWD");
		context->startJumpBackward();
	};

range_next_rule:
	expr_rule comma_token {
		DEBUG_STACK(context, "POP");
		context->pushNode(Node::unload_reference);
		DEBUG_STACK(context, "LBL FWD");
		context->resolveJumpForward();
	};

range_cond_rule:
	expr_rule {
		DEBUG_STACK(context, "JZR FWD");
		context->pushNode(Node::jump_zero);
		context->startJumpForward();
	};

return_rule:
	return_token {
		context->prepareReturn();
	};

start_hash_rule:
	open_brace_token {
		DEBUG_STACK(context, "NEW HASH");
		context->pushNode(Node::create_hash);
	};

stop_hash_rule:
	close_brace_token;

hash_item_rule:
	hash_item_rule separator_rule expr_rule dbldot_token expr_rule {
		DEBUG_STACK(context, "HASH PUSH");
		context->pushNode(Node::hash_insert);
	}
	| expr_rule dbldot_token expr_rule {
		DEBUG_STACK(context, "HASH PUSH");
		context->pushNode(Node::hash_insert);
	};

start_array_rule:
	open_bracket_token {
		DEBUG_STACK(context, "NEW ARRAY");
		context->pushNode(Node::create_array);
	};

stop_array_rule:
	close_bracket_token;

array_item_rule:
	array_item_rule separator_rule expr_rule {
		DEBUG_STACK(context, "ARRAY PUSH");
		context->pushNode(Node::array_insert);
	}
	| expr_rule {
		DEBUG_STACK(context, "ARRAY PUSH");
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
		DEBUG_STACK(context, "NEW ITERATOR");
		context->pushNode(Node::create_iterator);
		context->addToCall();
		context->resolveCall();
	}
	| {
		DEBUG_STACK(context, "NEW ITERATOR");
		context->pushNode(Node::create_iterator);
		context->resolveCall();
	};

print_rule:
	print_token {
		DEBUG_STACK(context, "PUSH stdout");
		context->pushNode(Node::load_constant);
		context->pushNode(Compiler::makeData("1"));
		DEBUG_STACK(context, "OPEN PRINTER");
		context->pushNode(Node::open_printer);
	}
	| print_token open_parenthesis_token expr_rule close_parenthesis_token {
		DEBUG_STACK(context, "OPEN PRINTER");
		context->pushNode(Node::open_printer);
	};

expr_rule:
	expr_rule equal_token expr_rule {
		DEBUG_STACK(context, "MOVE");
		context->pushNode(Node::move_op);
	}
	| expr_rule dbldot_equal_token expr_rule {
		DEBUG_STACK(context, "COPY");
		context->pushNode(Node::copy_op);
	}
	| expr_rule plus_token expr_rule {
		DEBUG_STACK(context, "ADD");
		context->pushNode(Node::add_op);
	}
	| expr_rule minus_token expr_rule {
		DEBUG_STACK(context, "SUB");
		context->pushNode(Node::sub_op);
	}
	| expr_rule asterisk_token expr_rule {
		DEBUG_STACK(context, "MUL");
		context->pushNode(Node::mul_op);
	}
	| expr_rule slash_token expr_rule {
		DEBUG_STACK(context, "DIV");
		context->pushNode(Node::div_op);
	}
	| expr_rule percent_token expr_rule {
		DEBUG_STACK(context, "MOD");
		context->pushNode(Node::mod_op);
	}
	| expr_rule dbl_asterisk_token expr_rule {
		DEBUG_STACK(context, "POW");
		context->pushNode(Node::pow_op);
	}
	| expr_rule is_token expr_rule {
		DEBUG_STACK(context, "IS");
		context->pushNode(Node::is_op);
	}
	| expr_rule dbl_equal_token expr_rule {
		DEBUG_STACK(context, "EQ");
		context->pushNode(Node::eq_op);
	}
	| expr_rule exclamation_equal_token expr_rule {
		DEBUG_STACK(context, "NE");
		context->pushNode(Node::ne_op);
	}
	| expr_rule left_angled_token expr_rule {
		DEBUG_STACK(context, "LT");
		context->pushNode(Node::lt_op);
	}
	| expr_rule right_angled_token expr_rule {
		DEBUG_STACK(context, "GT");
		context->pushNode(Node::gt_op);
	}
	| expr_rule left_angled_equal_token expr_rule {
		DEBUG_STACK(context, "LE");
		context->pushNode(Node::le_op);
	}
	| expr_rule right_angled_equal_token expr_rule {
		DEBUG_STACK(context, "GE");
		context->pushNode(Node::ge_op);
	}
	| expr_rule dbl_left_angled_token expr_rule {
		DEBUG_STACK(context, "SHIFT LEFT");
		context->pushNode(Node::shift_left_op);
	}
	| expr_rule dbl_right_angled_token expr_rule {
		DEBUG_STACK(context, "SHIFT RIGHT");
		context->pushNode(Node::shift_right_op);
	}
	| expr_rule dot_dot_token expr_rule {
		DEBUG_STACK(context, "INCLUSIVE RANGE");
		context->pushNode(Node::inclusive_range_op);
	}
	| expr_rule tpl_dot_token expr_rule {
		DEBUG_STACK(context, "EXCLUSIVE RANGE");
		context->pushNode(Node::exclusive_range_op);
	}
	| dbl_plus_token expr_rule {
		DEBUG_STACK(context, "PRE-INC");
		context->pushNode(Node::inc_op);
	}
	| dbl_minus_token expr_rule {
		DEBUG_STACK(context, "PRE-DEC");
		context->pushNode(Node::dec_op);
	}
	| expr_rule dbl_plus_token {
		DEBUG_STACK(context, "POST-INC");
		context->pushNode(Node::store_reference);
		context->pushNode(Node::inc_op);
		context->pushNode(Node::unload_reference);
	}
	| expr_rule dbl_minus_token {
		DEBUG_STACK(context, "POST-DEC");
		context->pushNode(Node::store_reference);
		context->pushNode(Node::dec_op);
		context->pushNode(Node::unload_reference);
	}
	| exclamation_token expr_rule {
		DEBUG_STACK(context, "NOT");
		context->pushNode(Node::not_op);
	}
	| expr_rule dbl_pipe_token {
		DEBUG_STACK(context, "OR PRE CHECK");
		context->pushNode(Node::or_pre_check);
		context->startJumpForward();
	} expr_rule {
		DEBUG_STACK(context, "OR");
		context->pushNode(Node::or_op);
		DEBUG_STACK(context, "FWD LABEL");
		context->resolveJumpForward();
	}
	| expr_rule dbl_amp_token {
		DEBUG_STACK(context, "AND PRE CHECK");
		context->pushNode(Node::and_pre_check);
		context->startJumpForward();
	} expr_rule {
		DEBUG_STACK(context, "AND");
		context->pushNode(Node::and_op);
		DEBUG_STACK(context, "FWD LABEL");
		context->resolveJumpForward();
	}
	| expr_rule pipe_token expr_rule {
		DEBUG_STACK(context, "BOR");
		context->pushNode(Node::bor_op);
	}
	| expr_rule amp_token expr_rule {
		DEBUG_STACK(context, "BAND");
		context->pushNode(Node::band_op);
	}
	| expr_rule caret_token expr_rule {
		DEBUG_STACK(context, "XOR");
		context->pushNode(Node::xor_op);
	}
	| tilde_token expr_rule {
		DEBUG_STACK(context, "BNOT");
		context->pushNode(Node::compl_op);
	}
	| plus_token expr_rule {
		DEBUG_STACK(context, "POS");
		context->pushNode(Node::pos_op);
	}
	| minus_token expr_rule {
		DEBUG_STACK(context, "NEG");
		context->pushNode(Node::neg_op);
	}
	| typeof_token expr_rule {
		DEBUG_STACK(context, "TYPEOF");
		context->pushNode(Node::typeof_op);
	}
	| membersof_token expr_rule {
		DEBUG_STACK(context, "MBROF");
		context->pushNode(Node::membersof_op);
	}
	| defined_token defined_symbol_rule {
		DEBUG_STACK(context, "DEFINED");
		context->pushNode(Node::check_defined);
	}
	| expr_rule open_bracket_token expr_rule close_bracket_token {
		DEBUG_STACK(context, "SUBSCR");
		context->pushNode(Node::subscript_op);
	}
	| expr_rule open_bracket_token expr_rule close_bracket_equal_token expr_rule {
		DEBUG_STACK(context, "SUBSCR MOVE");
		context->pushNode(Node::subscript_move_op);
	}
	| member_ident_rule
	| ident_rule call_args_rule
	| def_rule call_args_rule
	| expr_rule dot_token call_member_args_rule
	| expr_rule plus_equal_token {
		DEBUG_STACK(context, "RELOAD");
		context->pushNode(Node::reload_reference);
	} expr_rule {
		DEBUG_STACK(context, "ADD");
		context->pushNode(Node::add_op);
		DEBUG_STACK(context, "MOVE");
		context->pushNode(Node::move_op);
	}
	| expr_rule minus_equal_token {
		DEBUG_STACK(context, "RELOAD");
		context->pushNode(Node::reload_reference);
	} expr_rule {
		DEBUG_STACK(context, "SUB");
		context->pushNode(Node::sub_op);
		DEBUG_STACK(context, "MOVE");
		context->pushNode(Node::move_op);
	}
	| expr_rule asterisk_equal_token {
		DEBUG_STACK(context, "RELOAD");
		context->pushNode(Node::reload_reference);
	} expr_rule {
		DEBUG_STACK(context, "MUL");
		context->pushNode(Node::mul_op);
		DEBUG_STACK(context, "MOVE");
		context->pushNode(Node::move_op);
	}
	| expr_rule slash_equal_token {
		DEBUG_STACK(context, "RELOAD");
		context->pushNode(Node::reload_reference);
	} expr_rule {
		DEBUG_STACK(context, "DIV");
		context->pushNode(Node::div_op);
		DEBUG_STACK(context, "MOVE");
		context->pushNode(Node::move_op);
	}
	| expr_rule percent_equal_token {
		DEBUG_STACK(context, "RELOAD");
		context->pushNode(Node::reload_reference);
	} expr_rule {
		DEBUG_STACK(context, "MOD");
		context->pushNode(Node::mod_op);
		DEBUG_STACK(context, "MOVE");
		context->pushNode(Node::move_op);
	}
	| expr_rule dbl_left_angled_equal_token {
		DEBUG_STACK(context, "RELOAD");
		context->pushNode(Node::reload_reference);
	} expr_rule {
		DEBUG_STACK(context, "SHIFT LEFT");
		context->pushNode(Node::shift_left_op);
		DEBUG_STACK(context, "MOVE");
		context->pushNode(Node::move_op);
	}
	| expr_rule dbl_right_angled_equal_token {
		DEBUG_STACK(context, "RELOAD");
		context->pushNode(Node::reload_reference);
	} expr_rule {
		DEBUG_STACK(context, "SHIFT RIGHT");
		context->pushNode(Node::shift_right_op);
		DEBUG_STACK(context, "MOVE");
		context->pushNode(Node::move_op);
	}
	| expr_rule amp_equal_token {
		DEBUG_STACK(context, "RELOAD");
		context->pushNode(Node::reload_reference);
	} expr_rule {
		DEBUG_STACK(context, "BAND");
		context->pushNode(Node::band_op);
		DEBUG_STACK(context, "MOVE");
		context->pushNode(Node::move_op);
	}
	| expr_rule pipe_equal_token {
		DEBUG_STACK(context, "RELOAD");
		context->pushNode(Node::reload_reference);
	} expr_rule {
		DEBUG_STACK(context, "BOR");
		context->pushNode(Node::bor_op);
		DEBUG_STACK(context, "MOVE");
		context->pushNode(Node::move_op);
	}
	| expr_rule caret_equal_token {
		DEBUG_STACK(context, "RELOAD");
		context->pushNode(Node::reload_reference);
	} expr_rule {
		DEBUG_STACK(context, "XOR");
		context->pushNode(Node::xor_op);
		DEBUG_STACK(context, "MOVE");
		context->pushNode(Node::move_op);
	}
	| expr_rule equal_tilde_token expr_rule {
		DEBUG_STACK(context, "MATCH");
		context->pushNode(Node::regex_match);
	}
	| expr_rule exclamation_tilde_token expr_rule {
		DEBUG_STACK(context, "UNMATCH");
		context->pushNode(Node::regex_unmatch);
	}
	| expr_rule question_token {
		DEBUG_STACK(context, "JZR FWD");
		context->pushNode(Node::jump_zero);
		context->startJumpForward();
	} expr_rule dbldot_token {
		DEBUG_STACK(context, "JMP FWD");
		context->pushNode(Node::jump);
		context->startJumpForward();
		DEBUG_STACK(context, "FWD LABEL");
		context->shiftJumpForward();
		context->resolveJumpForward();
	} expr_rule {
		DEBUG_STACK(context, "FWD LABEL");
		context->resolveJumpForward();
	}
	| open_parenthesis_token close_parenthesis_token {
		DEBUG_STACK(context, "NEW ITERATOR");
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
		DEBUG_STACK(context, "CALL");
		context->pushNode(Node::call);
		context->resolveCall();
	};

call_member_arg_start_rule:
	symbol_token open_parenthesis_token {
		DEBUG_STACK(context, "LOAD MBR %s", $1.c_str());
		context->pushNode(Node::init_member_call);
		context->pushNode($1.c_str());
		context->startCall();
	}
	| operator_desc_rule open_parenthesis_token {
		DEBUG_STACK(context, "LOAD MBR OP %s", $1.c_str());
		context->pushNode(Node::init_member_call);
		context->pushNode($1.c_str());
		context->startCall();
	}
	| var_symbol_rule open_parenthesis_token {
		DEBUG_STACK(context, "LOAD VAR MBR");
		context->pushNode(Node::init_var_member_call);
		context->startCall();
	};

call_member_arg_stop_rule:
	close_parenthesis_token {
		DEBUG_STACK(context, "CALL MBR");
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
		context->pushNode(Node::load_extra_arguments);
	};

def_rule:
	def_start_rule def_capture_rule def_args_rule stmt_bloc_rule {
		DEBUG_STACK(context, "LOAD_DR");
		context->pushNode(Node::load_default_result);
		DEBUG_STACK(context, "EXIT CALL");
		context->pushNode(Node::exit_call);
		DEBUG_STACK(context, "LBL FWD");
		context->resolveJumpForward();
		DEBUG_STACK(context, "PUSH DEF");
		context->saveDefinition();
	};

def_start_rule:
	def_token {
		DEBUG_STACK(context, "JMP FWD");
		context->pushNode(Node::jump);
		context->startJumpForward();
		context->startDefinition();
	};

def_capture_rule:
	def_capture_start_rule def_capture_list_rule def_capture_stop_rule
	| ;

def_capture_start_rule:
	open_bracket_token;

def_capture_stop_rule:
	close_bracket_token;

def_capture_list_rule:
	symbol_token separator_rule def_capture_list_rule {
		context->capture($1);
	}
	| symbol_token {
		context->capture($1);
	}
	| tpl_dot_token {
		context->captureAll();
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
		DEBUG_STACK(context, "ARG %s", $1.c_str());
		if (!context->addParameter($1)) {
			YYERROR;
		}
	}
	| symbol_token equal_token expr_rule {
		if (!context->addDefinitionSignature()) {
			YYERROR;
		}

		DEBUG_STACK(context, "ARG %s", $1.c_str());
		if (!context->addParameter($1)) {
			YYERROR;
		}
	}
	| tpl_dot_token {
		DEBUG_STACK(context, "VARIADIC");
		if (!context->setVariadic()) {
			YYERROR;
		}
	};

member_ident_rule:
	expr_rule dot_token symbol_token {
		DEBUG_STACK(context, "LOAD MBR %s", $3.c_str());
		context->pushNode(Node::load_member);
		context->pushNode($3.c_str());
	}
	| expr_rule dot_token operator_desc_rule {
		DEBUG_STACK(context, "LOAD MBR OP %s", $3.c_str());
		context->pushNode(Node::load_member);
		context->pushNode($3.c_str());
	}
	| expr_rule dot_token var_symbol_rule {
		DEBUG_STACK(context, "LOAD VAR MBR");
		context->pushNode(Node::load_var_member);
	};

defined_symbol_rule:
	symbol_token {
		DEBUG_STACK(context, "SYMBOL %s", $1.c_str());
		context->pushNode(Node::find_defined_symbol);
		context->pushNode($1.c_str());
	}
	| defined_symbol_rule dot_token symbol_token {
		DEBUG_STACK(context, "MEMBER %s", $3.c_str());
		context->pushNode(Node::find_defined_member);
		context->pushNode($3.c_str());
	}
	| var_symbol_rule {
		DEBUG_STACK(context, "VAR SYMBOL %s", $1.c_str());
		context->pushNode(Node::find_defined_var_symbol);
		context->pushNode($1.c_str());
	}
	| defined_symbol_rule dot_token var_symbol_rule {
		DEBUG_STACK(context, "VAR MEMBER %s", $3.c_str());
		context->pushNode(Node::find_defined_var_member);
		context->pushNode($3.c_str());
	}
	| constant_rule {
		DEBUG_STACK(context, "PUSH %s", $1.c_str());
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
		DEBUG_STACK(context, "PUSH %s", $1.c_str());
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
		DEBUG_STACK(context, "PUSH lib");
		context->pushNode(Node::create_lib);
	}
	| symbol_token {
		DEBUG_STACK(context, "LOAD %s", $1.c_str());
		context->pushNode(Node::load_symbol);
		context->pushNode($1.c_str());
	}
	| var_symbol_rule {
		DEBUG_STACK(context, "LOAD VAR");
		context->pushNode(Node::load_var_symbol);
	}
	| modifier_rule symbol_token {
		if (context->getModifiers() & Reference::global) {
			DEBUG_STACK(context, "NEW GLOBAL %s", $2.c_str());
		}
		else {
			DEBUG_STACK(context, "NEW %s", $2.c_str());
		}
		context->pushNode(Node::create_symbol);
		context->pushNode($2.c_str());
		context->pushNode(context->getModifiers());
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
	dollar_token {
		context->setModifiers(Reference::const_address);
	}
	| percent_token {
		context->setModifiers(Reference::const_value);
	}
	| const_token {
		context->setModifiers(Reference::const_address | Reference::const_value);
	}
	| at_token {
		context->setModifiers(Reference::global);
	}
	| modifier_rule dollar_token {
		context->setModifiers(context->getModifiers() | Reference::const_address);
	}
	| modifier_rule percent_token {
		context->setModifiers(context->getModifiers() | Reference::const_value);
	}
	| modifier_rule const_token {
		context->setModifiers(context->getModifiers() | Reference::const_address | Reference::const_value);
	}
	| modifier_rule at_token {
		context->setModifiers(context->getModifiers() | Reference::global);
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

	BuildContext *context = new BuildContext(stream, node);
	parser parser(context);

	bool success = !parser.parse();

	delete context;
	return success;
}

#endif // PARSER_HPP
