%{

#ifndef PARSER_HPP
#define PARSER_HPP

#include "compiler/compiler.h"

#define YYSTYPE std::string

using namespace mint;

int yylex(std::string *token);
%}

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
%left dot_token open_parenthesis_token close_parenthesis_token open_bracket_token close_bracket_token open_brace_token close_brace_token

%%

module_rule: stmt_list_rule file_end_token {
		DEBUG_STACK("END");
		Compiler::context()->pushNode(Node::module_end);
		fflush(stdout);
		YYACCEPT;
	};

stmt_list_rule: stmt_list_rule stmt_rule
	| stmt_rule;

stmt_rule: load_token module_path_rule line_end_token {
		DEBUG_STACK("LOAD MODULE %s", $2.c_str());
		Compiler::context()->pushNode(Node::load_module);
		Compiler::context()->pushNode($2.c_str());
	}
	| try_rule stmt_bloc_rule {
		DEBUG_STACK("UNTRY");
		Compiler::context()->pushNode(Node::unset_retrieve_point);

		DEBUG_STACK("LBL FWD");
		Compiler::context()->resolveJumpForward();
	}
	| try_rule stmt_bloc_rule catch_rule stmt_bloc_rule {
		DEBUG_STACK("LBL FWD");
		Compiler::context()->resolveJumpForward();
	}
	| cond_if_rule stmt_bloc_rule {
		DEBUG_STACK("LBL FWD");
		Compiler::context()->resolveJumpForward();
	}
	| cond_if_rule stmt_bloc_rule cond_else_rule stmt_bloc_rule {
		DEBUG_STACK("LBL FWD");
		Compiler::context()->resolveJumpForward();
	}
	| cond_if_rule stmt_bloc_rule elif_bloc_rule {
		DEBUG_STACK("LBL FWD");
		Compiler::context()->resolveJumpForward();
	}
	| cond_if_rule stmt_bloc_rule elif_bloc_rule cond_else_rule stmt_bloc_rule {
		DEBUG_STACK("LBL FWD");
		Compiler::context()->resolveJumpForward();
	}
	| loop_rule stmt_bloc_rule {
		DEBUG_STACK("JMP BWD");
		Compiler::context()->pushNode(Node::jump);
		Compiler::context()->resolveJumpBackward();
		DEBUG_STACK("LBL FWD");
		Compiler::context()->resolveJumpForward();
		Compiler::context()->endLoop();
	}
	| range_rule stmt_bloc_rule {
		DEBUG_STACK("JMP BWD");
		Compiler::context()->pushNode(Node::jump);
		Compiler::context()->resolveJumpBackward();
		DEBUG_STACK("LBL FWD");
		Compiler::context()->resolveJumpForward();
		Compiler::context()->endLoop();
	}
	| break_token line_end_token {
		if (!Compiler::context()->isInLoop()) {
			Compiler::context()->parse_error("break statement not within loop");
			YYERROR;
		}
		DEBUG_STACK("JMP FWD");
		Compiler::context()->pushNode(Node::jump);
		Compiler::context()->loopJumpForward();
	}
	| continue_token line_end_token {
		if (!Compiler::context()->isInLoop()) {
			Compiler::context()->parse_error("continue statement not within loop");
			YYERROR;
		}
		DEBUG_STACK("JMP BWD");
		Compiler::context()->pushNode(Node::jump);
		Compiler::context()->loopJumpBackward();
	}
	| print_rule stmt_bloc_rule {
		DEBUG_STACK("CLOSE PRINTER");
		Compiler::context()->pushNode(Node::close_printer);
	}
	| yield_token expr_rule line_end_token {
		DEBUG_STACK("YIELD");
		Compiler::context()->pushNode(Node::yield);
	}
	| return_token expr_rule line_end_token {
		DEBUG_STACK("EXIT CALL");
		Compiler::context()->pushNode(Node::exit_call);
	}
	| raise_token expr_rule line_end_token {
		DEBUG_STACK("RAISE");
		Compiler::context()->pushNode(Node::raise);
	}
	| exit_token expr_rule line_end_token {
		DEBUG_STACK("EXIT EXEC");
		Compiler::context()->pushNode(Node::exit_exec);
	}
	| exit_token line_end_token {
		DEBUG_STACK("PUSH 0");
		Compiler::context()->pushNode(Node::load_constant);
		Compiler::context()->pushNode(Compiler::makeData("0"));
		DEBUG_STACK("EXIT EXEC");
		Compiler::context()->pushNode(Node::exit_exec);
	}
	| expr_rule line_end_token {
		DEBUG_STACK("PRINT");
		Compiler::context()->pushNode(Node::print);
	}
	| modifier_rule def_start_rule def_capture_rule symbol_token def_args_rule stmt_bloc_rule {
		DEBUG_STACK("LOAD_DR");
		Compiler::context()->pushNode(Node::load_default_result);
		DEBUG_STACK("EXIT CALL");
		Compiler::context()->pushNode(Node::exit_call);
		DEBUG_STACK("LBL FWD");
		Compiler::context()->resolveJumpForward();
		DEBUG_STACK("NEW GLOABL %s", $3.c_str());
		Compiler::context()->pushNode(Node::create_symbol);
		Compiler::context()->pushNode($3.c_str());
		Compiler::context()->pushNode(Compiler::context()->getModifiers() | Reference::global);
		DEBUG_STACK("PUSH DEF");
		Compiler::context()->saveDefinition();
		DEBUG_STACK("MOVE");
		Compiler::context()->pushNode(Node::move_op);
	}
	| def_start_rule symbol_token def_capture_rule def_args_rule stmt_bloc_rule {
		DEBUG_STACK("LOAD_DR");
		Compiler::context()->pushNode(Node::load_default_result);
		DEBUG_STACK("EXIT CALL");
		Compiler::context()->pushNode(Node::exit_call);
		DEBUG_STACK("LBL FWD");
		Compiler::context()->resolveJumpForward();
		DEBUG_STACK("NEW GLOABL %s", $2.c_str());
		Compiler::context()->pushNode(Node::create_symbol);
		Compiler::context()->pushNode($2.c_str());
		Compiler::context()->pushNode(Reference::global);
		DEBUG_STACK("PUSH DEF");
		Compiler::context()->saveDefinition();
		DEBUG_STACK("MOVE");
		Compiler::context()->pushNode(Node::move_op);
		DEBUG_STACK("POP");
		Compiler::context()->pushNode(Node::unload_reference);
	}
	| class_desc_rule
	| enum_desc_rule
	| line_end_token;

module_path_rule: symbol_token {
		$$ = $1;
	}
	| module_path_rule dot_token symbol_token {
		$$ = $1 + $2 + $3;
	};

class_rule: class_token symbol_token {
		DEBUG_STACK("CLASS %s", $2.c_str());
		Compiler::context()->startClassDescription($2);
	};

parent_rule: dbldot_token parent_list_rule
	| ;

parent_list_rule: symbol_token {
		DEBUG_STACK("INHERITE %s", $1.c_str());
		Compiler::context()->classInheritance($1);
	}
	| parent_list_rule comma_token symbol_token {
		DEBUG_STACK("INHERITE %s", $3.c_str());
		Compiler::context()->classInheritance($3);
	};

class_desc_rule: class_rule parent_rule desc_bloc_rule {
		Compiler::context()->resolveClassDescription();
	};

desc_bloc_rule: open_brace_token desc_list_rule close_brace_token;

desc_list_rule: desc_list_rule desc_rule
	| desc_rule;

desc_rule: member_desc_rule line_end_token {
		if (!Compiler::context()->createMember(Compiler::context()->getModifiers(), $1)) {
			YYERROR;
		}
	}
	| member_desc_rule equal_token constant_token line_end_token {
		if (!Compiler::context()->createMember(Compiler::context()->getModifiers(), $1, Compiler::makeData($3))) {
			YYERROR;
		}
	}
	| member_desc_rule equal_token string_token line_end_token {
		if (!Compiler::context()->createMember(Compiler::context()->getModifiers(), $1, Compiler::makeData($3))) {
			YYERROR;
		}
	}
	| member_desc_rule equal_token number_token line_end_token {
		if (!Compiler::context()->createMember(Compiler::context()->getModifiers(), $1, Compiler::makeData($3))) {
			YYERROR;
		}
	}
	| member_desc_rule equal_token open_bracket_token close_bracket_token line_end_token {
		if (!Compiler::context()->createMember(Compiler::context()->getModifiers(), $1, Compiler::makeArray())) {
			YYERROR;
		}
	}
	| member_desc_rule equal_token open_brace_token close_brace_token line_end_token {
		if (!Compiler::context()->createMember(Compiler::context()->getModifiers(), $1, Compiler::makeHash())) {
			YYERROR;
		}
	}
	| member_desc_rule equal_token lib_token open_parenthesis_token string_token close_parenthesis_token line_end_token {
		if (!Compiler::context()->createMember(Compiler::context()->getModifiers(), $1, Compiler::makeLibrary($5))) {
			YYERROR;
		}
	}
	| member_desc_rule equal_token def_start_rule def_args_rule stmt_bloc_rule {
		DEBUG_STACK("LOAD_DR");
		Compiler::context()->pushNode(Node::load_default_result);
		DEBUG_STACK("EXIT CALL");
		Compiler::context()->pushNode(Node::exit_call);
		DEBUG_STACK("LBL FWD");
		Compiler::context()->resolveJumpForward();

		if (!Compiler::context()->createMember(Compiler::context()->getModifiers(), $1, Compiler::context()->retrieveDefinition())) {
			YYERROR;
		}
	}
	| member_desc_rule plus_equal_token def_start_rule def_args_rule stmt_bloc_rule {
		DEBUG_STACK("LOAD_DR");
		Compiler::context()->pushNode(Node::load_default_result);
		DEBUG_STACK("EXIT CALL");
		Compiler::context()->pushNode(Node::exit_call);
		DEBUG_STACK("LBL FWD");
		Compiler::context()->resolveJumpForward();

		if (!Compiler::context()->updateMember(Compiler::context()->getModifiers(), $1, Compiler::context()->retrieveDefinition())) {
			YYERROR;
		}
	}
	| def_start_rule symbol_token def_args_rule stmt_bloc_rule {
		DEBUG_STACK("LOAD_DR");
		Compiler::context()->pushNode(Node::load_default_result);
		DEBUG_STACK("EXIT CALL");
		Compiler::context()->pushNode(Node::exit_call);
		DEBUG_STACK("LBL FWD");
		Compiler::context()->resolveJumpForward();

		if (!Compiler::context()->updateMember(Reference::standard, $2, Compiler::context()->retrieveDefinition())) {
			YYERROR;
		}
	}
	| def_start_rule operator_desc_rule def_args_rule stmt_bloc_rule {
		DEBUG_STACK("LOAD_DR");
		Compiler::context()->pushNode(Node::load_default_result);
		DEBUG_STACK("EXIT CALL");
		Compiler::context()->pushNode(Node::exit_call);
		DEBUG_STACK("LBL FWD");
		Compiler::context()->resolveJumpForward();

		if (!Compiler::context()->updateMember(Reference::standard, $2, Compiler::context()->retrieveDefinition())) {
			YYERROR;
		}
	}
	| desc_modifier_rule def_start_rule symbol_token def_args_rule stmt_bloc_rule {
		DEBUG_STACK("LOAD_DR");
		Compiler::context()->pushNode(Node::load_default_result);
		DEBUG_STACK("EXIT CALL");
		Compiler::context()->pushNode(Node::exit_call);
		DEBUG_STACK("LBL FWD");
		Compiler::context()->resolveJumpForward();

		if (!Compiler::context()->updateMember(Compiler::context()->getModifiers(), $3, Compiler::context()->retrieveDefinition())) {
			YYERROR;
		}
	}
	| class_desc_rule
	| enum_desc_rule
	| line_end_token;

member_desc_rule: symbol_token {
		Compiler::context()->setModifiers(Reference::standard);
		$$ = $1;
	}
	| desc_modifier_rule symbol_token {
		$$ = $2;
	}
	| operator_desc_rule {
		Compiler::context()->setModifiers(Reference::standard);
		$$ = $1;
	};

desc_modifier_rule: modifier_rule
	| plus_token {
		Compiler::context()->setModifiers(Reference::standard);
	}
	| sharp_token {
		Compiler::context()->setModifiers(Reference::user_hiden);
	}
	| minus_token {
		Compiler::context()->setModifiers(Reference::child_hiden);
	}
	| plus_token modifier_rule {
		Compiler::context()->setModifiers(Compiler::context()->getModifiers() | Reference::standard);
	}
	| sharp_token modifier_rule {
		Compiler::context()->setModifiers(Compiler::context()->getModifiers() | Reference::user_hiden);
	}
	| minus_token modifier_rule {
		Compiler::context()->setModifiers(Compiler::context()->getModifiers() | Reference::child_hiden);
	};

operator_desc_rule: dbl_pipe_token { $$ = $1; }
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
	| open_bracket_token close_bracket_token { $$ = $1 + $2; };

enum_rule: enum_token symbol_token {
		DEBUG_STACK("ENUM %s", $2.c_str());
		Compiler::context()->startEnumDescription($2);
	};

enum_desc_rule: enum_rule enum_block_rule {
		Compiler::context()->resolveEnumDescription();
	};

enum_block_rule: open_brace_token enum_list_rule close_brace_token;

enum_list_rule: enum_list_rule enum_item_rule
		| enum_item_rule;

enum_item_rule: symbol_token equal_token number_token {
			Reference::Flags flags = Reference::const_value | Reference::const_address | Reference::global;
			if (!Compiler::context()->createMember(flags, $1, Compiler::makeData($3))) {
				YYERROR;
			}
			Compiler::context()->setCurrentEnumValue(atoi($3.c_str()));
		}
		| symbol_token {
			Reference::Flags flags = Reference::const_value | Reference::const_address | Reference::global;
			if (!Compiler::context()->createMember(flags, $1, Compiler::makeData(std::to_string(Compiler::context()->nextEnumValue())))) {
				YYERROR;
			}
		}
		| line_end_token;

try_rule: try_token {
		DEBUG_STACK("TRY");
		Compiler::context()->pushNode(Node::set_retrieve_point);
		Compiler::context()->startJumpForward();
	};

catch_rule: catch_token symbol_token {
		DEBUG_STACK("UNTRY");
		Compiler::context()->pushNode(Node::unset_retrieve_point);

		DEBUG_STACK("JMP FWD");
		Compiler::context()->pushNode(Node::jump);
		Compiler::context()->startJumpForward();

		DEBUG_STACK("FWD LBL");
		Compiler::context()->shiftJumpForward();
		Compiler::context()->resolveJumpForward();

		DEBUG_STACK("INIT %s", $2.c_str());
		Compiler::context()->pushNode(Node::init_param);
		Compiler::context()->pushNode($2.c_str());
	};

elif_bloc_rule: cond_elif_rule stmt_bloc_rule {
		DEBUG_STACK("LBL FWD");
		Compiler::context()->shiftJumpForward();
		Compiler::context()->resolveJumpForward();
	}
	| elif_bloc_rule cond_elif_rule stmt_bloc_rule {
		DEBUG_STACK("LBL FWD");
		Compiler::context()->shiftJumpForward();
		Compiler::context()->resolveJumpForward();
	};

stmt_bloc_rule: open_brace_token stmt_list_rule close_brace_token
	| open_brace_token expr_rule close_brace_token {
		DEBUG_STACK("PRINT");
		Compiler::context()->pushNode(Node::print);
	}
	| open_brace_token close_brace_token;

cond_if_rule: if_token expr_rule {
		DEBUG_STACK("JZR FWD");
		Compiler::context()->pushNode(Node::jump_zero);
		Compiler::context()->startJumpForward();
	}
	| if_token find_in_rule {
		DEBUG_STACK("JZR FWD");
		Compiler::context()->pushNode(Node::jump_zero);
		Compiler::context()->startJumpForward();
	};

cond_elif_rule: elif_rule expr_rule {
		DEBUG_STACK("JZR FWD");
		Compiler::context()->pushNode(Node::jump_zero);
		Compiler::context()->startJumpForward();
	}
	| elif_rule find_in_rule {
		DEBUG_STACK("JZR FWD");
		Compiler::context()->pushNode(Node::jump_zero);
		Compiler::context()->startJumpForward();
	};

elif_rule: elif_token {
		DEBUG_STACK("JMP FWD");
		Compiler::context()->pushNode(Node::jump);
		Compiler::context()->startJumpForward();
		DEBUG_STACK("LBL FWD");
		Compiler::context()->shiftJumpForward();
		Compiler::context()->resolveJumpForward();
	};

cond_else_rule: else_token {
		DEBUG_STACK("JMP FWD");
		Compiler::context()->pushNode(Node::jump);
		Compiler::context()->startJumpForward();
		DEBUG_STACK("LBL FWD");
		Compiler::context()->shiftJumpForward();
		Compiler::context()->resolveJumpForward();
	};

loop_rule: while_rule expr_rule {
		DEBUG_STACK("JZR FWD");
		Compiler::context()->pushNode(Node::jump_zero);
		Compiler::context()->startJumpForward();

		Compiler::context()->beginLoop();
	}
	| while_rule find_in_rule {
		DEBUG_STACK("JZR FWD");
		Compiler::context()->pushNode(Node::jump_zero);
		Compiler::context()->startJumpForward();

		Compiler::context()->beginLoop();
	};

while_rule: while_token {
		DEBUG_STACK("LBL BWD");
		Compiler::context()->startJumpBackward();
	};

find_in_rule: expr_rule in_token expr_rule {
		DEBUG_STACK("FIND");
		Compiler::context()->pushNode(Node::in_find);
	}
	| expr_rule exclamation_token in_token expr_rule {
		DEBUG_STACK("FIND");
		Compiler::context()->pushNode(Node::in_find);
		DEBUG_STACK("NOT");
		Compiler::context()->pushNode(Node::not_op);
	};

range_rule: for_token range_init_rule range_next_rule range_cond_rule {
		Compiler::context()->beginLoop();
	}
	| for_token ident_rule in_token expr_rule {
		DEBUG_STACK("RANGE INIT");
		Compiler::context()->pushNode(Node::in_init);
		DEBUG_STACK("JMP FWD");
		Compiler::context()->pushNode(Node::jump);
		Compiler::context()->startJumpForward();

		DEBUG_STACK("LBL BWD");
		Compiler::context()->startJumpBackward();
		DEBUG_STACK("RANGE NEXT");
		Compiler::context()->pushNode(Node::in_next);
		DEBUG_STACK("LBL FWD");
		Compiler::context()->resolveJumpForward();

		DEBUG_STACK("RANGE CHECK");
		Compiler::context()->pushNode(Node::in_check);
		DEBUG_STACK("JZR FWD");
		Compiler::context()->pushNode(Node::jump_zero);
		Compiler::context()->startJumpForward();

		Compiler::context()->beginLoop();
	};

range_init_rule: expr_rule comma_token {
		DEBUG_STACK("POP");
		Compiler::context()->pushNode(Node::unload_reference);
		DEBUG_STACK("JMP FWD");
		Compiler::context()->pushNode(Node::jump);
		Compiler::context()->startJumpForward();
		DEBUG_STACK("LBL BWD");
		Compiler::context()->startJumpBackward();
	};

range_next_rule: expr_rule comma_token {
		DEBUG_STACK("POP");
		Compiler::context()->pushNode(Node::unload_reference);
		DEBUG_STACK("LBL FWD");
		Compiler::context()->resolveJumpForward();
	};

range_cond_rule: expr_rule {
		DEBUG_STACK("JZR FWD");
		Compiler::context()->pushNode(Node::jump_zero);
		Compiler::context()->startJumpForward();
	};

start_hash_rule: open_brace_token {
		DEBUG_STACK("NEW HASH");
		Compiler::context()->pushNode(Node::create_hash);
	};

stop_hash_rule: close_brace_token;

hash_item_rule: hash_item_rule separator_rule expr_rule dbldot_token expr_rule {
		DEBUG_STACK("HASH PUSH");
		Compiler::context()->pushNode(Node::hash_insert);
	}
	| expr_rule dbldot_token expr_rule {
		DEBUG_STACK("HASH PUSH");
		Compiler::context()->pushNode(Node::hash_insert);
	};

start_array_rule: open_bracket_token {
		DEBUG_STACK("NEW ARRAY");
		Compiler::context()->pushNode(Node::create_array);
	};

stop_array_rule: close_bracket_token;

array_item_rule: array_item_rule separator_rule expr_rule {
		DEBUG_STACK("ARRAY PUSH");
		Compiler::context()->pushNode(Node::array_insert);
	}
	| expr_rule {
		DEBUG_STACK("ARRAY PUSH");
		Compiler::context()->pushNode(Node::array_insert);
	};

iterator_item_rule: iterator_item_rule expr_rule separator_rule {
		Compiler::context()->addToCall();
	}
	| expr_rule separator_rule {
		Compiler::context()->startCall();
		Compiler::context()->addToCall();
	};

iterator_end_rule: expr_rule {
		DEBUG_STACK("NEW ITERATOR");
		Compiler::context()->pushNode(Node::create_iterator);
		Compiler::context()->addToCall();
		Compiler::context()->resolveCall();
	}
	| {
		DEBUG_STACK("NEW ITERATOR");
		Compiler::context()->pushNode(Node::create_iterator);
		Compiler::context()->resolveCall();
	};

print_rule: print_token {
		DEBUG_STACK("PUSH stdout");
		Compiler::context()->pushNode(Node::load_constant);
		Compiler::context()->pushNode(Compiler::makeData("1"));
		DEBUG_STACK("OPEN PRINTER");
		Compiler::context()->pushNode(Node::open_printer);
	}
	| print_token open_parenthesis_token expr_rule close_parenthesis_token {
		DEBUG_STACK("OPEN PRINTER");
		Compiler::context()->pushNode(Node::open_printer);
	};

expr_rule: expr_rule equal_token expr_rule {
		DEBUG_STACK("MOVE");
		Compiler::context()->pushNode(Node::move_op);
	}
	| expr_rule dbldot_equal_token expr_rule {
		DEBUG_STACK("COPY");
		Compiler::context()->pushNode(Node::copy_op);
	}
	| expr_rule plus_token expr_rule {
		DEBUG_STACK("ADD");
		Compiler::context()->pushNode(Node::add_op);
	}
	| expr_rule minus_token expr_rule {
		DEBUG_STACK("SUB");
		Compiler::context()->pushNode(Node::sub_op);
	}
	| expr_rule asterisk_token expr_rule {
		DEBUG_STACK("MUL");
		Compiler::context()->pushNode(Node::mul_op);
	}
	| expr_rule slash_token expr_rule {
		DEBUG_STACK("DIV");
		Compiler::context()->pushNode(Node::div_op);
	}
	| expr_rule percent_token expr_rule {
		DEBUG_STACK("MOD");
		Compiler::context()->pushNode(Node::mod_op);
	}
	| expr_rule dbl_asterisk_token expr_rule {
		DEBUG_STACK("POW");
		Compiler::context()->pushNode(Node::pow_op);
	}
	| expr_rule is_token expr_rule {
		DEBUG_STACK("IS");
		Compiler::context()->pushNode(Node::is_op);
	}
	| expr_rule dbl_equal_token expr_rule {
		DEBUG_STACK("EQ");
		Compiler::context()->pushNode(Node::eq_op);
	}
	| expr_rule exclamation_equal_token expr_rule {
		DEBUG_STACK("NE");
		Compiler::context()->pushNode(Node::ne_op);
	}
	| expr_rule left_angled_token expr_rule {
		DEBUG_STACK("LT");
		Compiler::context()->pushNode(Node::lt_op);
	}
	| expr_rule right_angled_token expr_rule {
		DEBUG_STACK("GT");
		Compiler::context()->pushNode(Node::gt_op);
	}
	| expr_rule left_angled_equal_token expr_rule {
		DEBUG_STACK("LE");
		Compiler::context()->pushNode(Node::le_op);
	}
	| expr_rule right_angled_equal_token expr_rule {
		DEBUG_STACK("GE");
		Compiler::context()->pushNode(Node::ge_op);
	}
	| expr_rule dbl_left_angled_token expr_rule {
		DEBUG_STACK("SHIFT LEFT");
		Compiler::context()->pushNode(Node::shift_left_op);
	}
	| expr_rule dbl_right_angled_token expr_rule {
		DEBUG_STACK("SHIFT RIGHT");
		Compiler::context()->pushNode(Node::shift_right_op);
	}
	| expr_rule dot_dot_token expr_rule {
		DEBUG_STACK("INCLUSIVE RANGE");
		Compiler::context()->pushNode(Node::inclusive_range_op);
	}
	| expr_rule tpl_dot_token expr_rule {
		DEBUG_STACK("EXCLUSIVE RANGE");
		Compiler::context()->pushNode(Node::exclusive_range_op);
	}
	| expr_rule dbl_plus_token {
		DEBUG_STACK("INC");
		Compiler::context()->pushNode(Node::inc_op);
	}
	| expr_rule dbl_minus_token {
		DEBUG_STACK("DEC");
		Compiler::context()->pushNode(Node::dec_op);
	}
	| exclamation_token expr_rule {
		DEBUG_STACK("NOT");
		Compiler::context()->pushNode(Node::not_op);
	}
	| expr_rule dbl_pipe_token {
		DEBUG_STACK("OR PRE CHECK");
		Compiler::context()->pushNode(Node::or_pre_check);
		Compiler::context()->startJumpForward();
	} expr_rule {
		DEBUG_STACK("OR");
		Compiler::context()->pushNode(Node::or_op);
		DEBUG_STACK("FWD LABEL");
		Compiler::context()->resolveJumpForward();
	}
	| expr_rule dbl_amp_token {
		DEBUG_STACK("AND PRE CHECK");
		Compiler::context()->pushNode(Node::and_pre_check);
		Compiler::context()->startJumpForward();
	} expr_rule {
		DEBUG_STACK("AND");
		Compiler::context()->pushNode(Node::and_op);
		DEBUG_STACK("FWD LABEL");
		Compiler::context()->resolveJumpForward();
	}
	| expr_rule pipe_token expr_rule {
		DEBUG_STACK("BOR");
		Compiler::context()->pushNode(Node::bor_op);
	}
	| expr_rule amp_token expr_rule {
		DEBUG_STACK("BAND");
		Compiler::context()->pushNode(Node::band_op);
	}
	| expr_rule caret_token expr_rule {
		DEBUG_STACK("XOR");
		Compiler::context()->pushNode(Node::xor_op);
	}
	| tilde_token expr_rule {
		DEBUG_STACK("BNOT");
		Compiler::context()->pushNode(Node::compl_op);
	}
	| plus_token expr_rule {
		DEBUG_STACK("POS");
		Compiler::context()->pushNode(Node::pos_op);
	}
	| minus_token expr_rule {
		DEBUG_STACK("NEG");
		Compiler::context()->pushNode(Node::neg_op);
	}
	| typeof_token expr_rule {
		DEBUG_STACK("TYPEOF");
		Compiler::context()->pushNode(Node::typeof_op);
	}
	| membersof_token expr_rule {
		DEBUG_STACK("MBROF");
		Compiler::context()->pushNode(Node::membersof_op);
	}
	| defined_token defined_symbol_rule {
		DEBUG_STACK("DEFINED");
		Compiler::context()->pushNode(Node::check_defined);
	}
	| expr_rule open_bracket_token expr_rule close_bracket_token {
		DEBUG_STACK("SUBSCR");
		Compiler::context()->pushNode(Node::subscript_op);
	}
	| member_ident_rule
	| ident_rule call_args_rule
	| def_rule call_args_rule
	| expr_rule dot_token call_member_args_rule
	| expr_rule plus_equal_token {
		DEBUG_STACK("RELOAD");
		Compiler::context()->pushNode(Node::reload_reference);
	} expr_rule {
		DEBUG_STACK("ADD");
		Compiler::context()->pushNode(Node::add_op);
		DEBUG_STACK("MOVE");
		Compiler::context()->pushNode(Node::move_op);
	}
	| expr_rule minus_equal_token {
		DEBUG_STACK("RELOAD");
		Compiler::context()->pushNode(Node::reload_reference);
	} expr_rule {
		DEBUG_STACK("SUB");
		Compiler::context()->pushNode(Node::sub_op);
		DEBUG_STACK("MOVE");
		Compiler::context()->pushNode(Node::move_op);
	}
	| expr_rule asterisk_equal_token {
		DEBUG_STACK("RELOAD");
		Compiler::context()->pushNode(Node::reload_reference);
	} expr_rule {
		DEBUG_STACK("MUL");
		Compiler::context()->pushNode(Node::mul_op);
		DEBUG_STACK("MOVE");
		Compiler::context()->pushNode(Node::move_op);
	}
	| expr_rule slash_equal_token {
		DEBUG_STACK("RELOAD");
		Compiler::context()->pushNode(Node::reload_reference);
	} expr_rule {
		DEBUG_STACK("DIV");
		Compiler::context()->pushNode(Node::div_op);
		DEBUG_STACK("MOVE");
		Compiler::context()->pushNode(Node::move_op);
	}
	| expr_rule percent_equal_token {
		DEBUG_STACK("RELOAD");
		Compiler::context()->pushNode(Node::reload_reference);
	} expr_rule {
		DEBUG_STACK("MOD");
		Compiler::context()->pushNode(Node::mod_op);
		DEBUG_STACK("MOVE");
		Compiler::context()->pushNode(Node::move_op);
	}
	| expr_rule dbl_left_angled_equal_token {
		DEBUG_STACK("RELOAD");
		Compiler::context()->pushNode(Node::reload_reference);
	} expr_rule {
		DEBUG_STACK("SHIFT LEFT");
		Compiler::context()->pushNode(Node::shift_left_op);
		DEBUG_STACK("MOVE");
		Compiler::context()->pushNode(Node::move_op);
	}
	| expr_rule dbl_right_angled_equal_token {
		DEBUG_STACK("RELOAD");
		Compiler::context()->pushNode(Node::reload_reference);
	} expr_rule {
		DEBUG_STACK("SHIFT RIGHT");
		Compiler::context()->pushNode(Node::shift_right_op);
		DEBUG_STACK("MOVE");
		Compiler::context()->pushNode(Node::move_op);
	}
	| expr_rule amp_equal_token {
		DEBUG_STACK("RELOAD");
		Compiler::context()->pushNode(Node::reload_reference);
	} expr_rule {
		DEBUG_STACK("BAND");
		Compiler::context()->pushNode(Node::band_op);
		DEBUG_STACK("MOVE");
		Compiler::context()->pushNode(Node::move_op);
	}
	| expr_rule pipe_equal_token {
		DEBUG_STACK("RELOAD");
		Compiler::context()->pushNode(Node::reload_reference);
	} expr_rule {
		DEBUG_STACK("BOR");
		Compiler::context()->pushNode(Node::bor_op);
		DEBUG_STACK("MOVE");
		Compiler::context()->pushNode(Node::move_op);
	}
	| expr_rule caret_equal_token {
		DEBUG_STACK("RELOAD");
		Compiler::context()->pushNode(Node::reload_reference);
	} expr_rule {
		DEBUG_STACK("XOR");
		Compiler::context()->pushNode(Node::xor_op);
		DEBUG_STACK("MOVE");
		Compiler::context()->pushNode(Node::move_op);
	}
	| expr_rule equal_tilde_token expr_rule {
		DEBUG_STACK("MATCH");
		Compiler::context()->pushNode(Node::regex_match);
	}
	| expr_rule exclamation_tilde_token expr_rule {
		DEBUG_STACK("UNMATCH");
		Compiler::context()->pushNode(Node::regex_unmatch);
	}
	| expr_rule question_token {
		DEBUG_STACK("JZR FWD");
		Compiler::context()->pushNode(Node::jump_zero);
		Compiler::context()->startJumpForward();
	} expr_rule dbldot_token {
		DEBUG_STACK("JMP FWD");
		Compiler::context()->pushNode(Node::jump);
		Compiler::context()->startJumpForward();
		DEBUG_STACK("FWD LABEL");
		Compiler::context()->shiftJumpForward();
		Compiler::context()->resolveJumpForward();
	} expr_rule {
		DEBUG_STACK("FWD LABEL");
		Compiler::context()->resolveJumpForward();
	}
	| open_parenthesis_token close_parenthesis_token {
		DEBUG_STACK("NEW ITERATOR");
		Compiler::context()->startCall();
		Compiler::context()->pushNode(Node::create_iterator);
		Compiler::context()->resolveCall();
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

call_args_rule: call_arg_start_rule call_arg_list_rule call_arg_stop_rule;
call_member_args_rule: call_member_arg_start_rule call_arg_list_rule call_member_arg_stop_rule;

call_arg_start_rule: open_parenthesis_token {
		Compiler::context()->pushNode(Node::init_call);
		Compiler::context()->startCall();
	};

call_arg_stop_rule: close_parenthesis_token {
	DEBUG_STACK("CALL");
	Compiler::context()->pushNode(Node::call);
	Compiler::context()->resolveCall();
};

call_member_arg_start_rule: symbol_token open_parenthesis_token {
		DEBUG_STACK("LOAD MBR %s", $1.c_str());
		Compiler::context()->pushNode(Node::init_member_call);
		Compiler::context()->pushNode($1.c_str());
		Compiler::context()->startCall();
	}
	| operator_desc_rule open_parenthesis_token {
		DEBUG_STACK("LOAD MBR OP %s", $1.c_str());
		Compiler::context()->pushNode(Node::init_member_call);
		Compiler::context()->pushNode($1.c_str());
		Compiler::context()->startCall();
	}
	| var_symbol_rule open_parenthesis_token {
		DEBUG_STACK("LOAD VAR MBR");
		Compiler::context()->pushNode(Node::init_var_member_call);
		Compiler::context()->startCall();
	};

call_member_arg_stop_rule: close_parenthesis_token {
		DEBUG_STACK("CALL MBR");
		Compiler::context()->pushNode(Node::call_member);
		Compiler::context()->resolveCall();
	};

call_arg_list_rule: call_arg_list_rule separator_rule call_arg_rule
	| call_arg_rule
	| ;

call_arg_rule: expr_rule {
		Compiler::context()->addToCall();
	};

def_rule: def_start_rule def_capture_rule def_args_rule stmt_bloc_rule {
		DEBUG_STACK("LOAD_DR");
		Compiler::context()->pushNode(Node::load_default_result);
		DEBUG_STACK("EXIT CALL");
		Compiler::context()->pushNode(Node::exit_call);
		DEBUG_STACK("LBL FWD");
		Compiler::context()->resolveJumpForward();
		DEBUG_STACK("PUSH DEF");
		Compiler::context()->saveDefinition();
	};

def_start_rule: def_token {
		DEBUG_STACK("JMP FWD");
		Compiler::context()->pushNode(Node::jump);
		Compiler::context()->startJumpForward();
		Compiler::context()->startDefinition();
	};

def_capture_rule: def_capture_start_rule def_capture_list_rule def_capture_stop_rule
		| ;

def_capture_start_rule: open_bracket_token;

def_capture_stop_rule: close_bracket_token;

def_capture_list_rule: symbol_token separator_rule def_capture_list_rule {
			Compiler::context()->capture($1);
		}
		| symbol_token {
			Compiler::context()->capture($1);
		}
		| tpl_dot_token {
			Compiler::context()->captureAll();
		};

def_args_rule: def_arg_start_rule def_arg_list_rule def_arg_stop_rule;

def_arg_start_rule: open_parenthesis_token;

def_arg_stop_rule: close_parenthesis_token {
		if (!Compiler::context()->saveParameters()) {
			YYERROR;
		}
	};

def_arg_list_rule: def_arg_rule separator_rule def_arg_list_rule
	| def_arg_rule
	| ;

def_arg_rule: symbol_token {
		DEBUG_STACK("ARG %s", $1.c_str());
		if (!Compiler::context()->addParameter($1)) {
			YYERROR;
		}
	}
	| symbol_token equal_token expr_rule {
		if (!Compiler::context()->addDefinitionSignature()) {
			YYERROR;
		}

		DEBUG_STACK("ARG %s", $1.c_str());
		if (!Compiler::context()->addParameter($1)) {
			YYERROR;
		}
	}
	| tpl_dot_token {
		DEBUG_STACK("VARIADIC");
		if (!Compiler::context()->setVariadic()) {
			YYERROR;
		}
	};

member_ident_rule: expr_rule dot_token symbol_token {
			DEBUG_STACK("LOAD MBR %s", $3.c_str());
			Compiler::context()->pushNode(Node::load_member);
			Compiler::context()->pushNode($3.c_str());
		}
		| expr_rule dot_token operator_desc_rule {
			DEBUG_STACK("LOAD MBR OP %s", $3.c_str());
			Compiler::context()->pushNode(Node::load_member);
			Compiler::context()->pushNode($3.c_str());
		}
		| expr_rule dot_token var_symbol_rule {
			DEBUG_STACK("LOAD VAR MBR");
			Compiler::context()->pushNode(Node::load_var_member);
		};

defined_symbol_rule: symbol_token {
			DEBUG_STACK("SYMBOL %s", $1.c_str());
			Compiler::context()->pushNode(Node::find_defined_symbol);
			Compiler::context()->pushNode($1.c_str());
		}
		| defined_symbol_rule dot_token symbol_token {
			DEBUG_STACK("MEMBER %s", $3.c_str());
			Compiler::context()->pushNode(Node::find_defined_member);
			Compiler::context()->pushNode($3.c_str());
		}
		| var_symbol_rule {
			DEBUG_STACK("VAR SYMBOL %s", $1.c_str());
			Compiler::context()->pushNode(Node::find_defined_var_symbol);
			Compiler::context()->pushNode($1.c_str());
		}
		| defined_symbol_rule dot_token var_symbol_rule {
			DEBUG_STACK("VAR MEMBER %s", $3.c_str());
			Compiler::context()->pushNode(Node::find_defined_var_member);
			Compiler::context()->pushNode($3.c_str());
		}
		| constant_rule {
			DEBUG_STACK("PUSH %s", $1.c_str());
			Compiler::context()->pushNode(Node::load_constant);
			if (Data *data = Compiler::makeData($1.c_str())) {
				Compiler::context()->pushNode(data);
			}
			else {
				error("token '" + $1 + "' is not a valid constant");
				YYERROR;
			}
		};

ident_rule: constant_rule {
		DEBUG_STACK("PUSH %s", $1.c_str());
		Compiler::context()->pushNode(Node::load_constant);
		if (Data *data = Compiler::makeData($1.c_str())) {
			Compiler::context()->pushNode(data);
		}
		else {
			error("token '" + $1 + "' is not a valid constant");
			YYERROR;
		}
	}
	| lib_token {
		DEBUG_STACK("PUSH lib");
		Compiler::context()->pushNode(Node::create_lib);
	}
	| symbol_token {
		DEBUG_STACK("LOAD %s", $1.c_str());
		Compiler::context()->pushNode(Node::load_symbol);
		Compiler::context()->pushNode($1.c_str());
	}
	| var_symbol_rule {
		DEBUG_STACK("LOAD VAR");
		Compiler::context()->pushNode(Node::load_var_symbol);
	}
	| modifier_rule symbol_token {
		if (Compiler::context()->getModifiers() & Reference::global) {
			DEBUG_STACK("NEW GLOABL %s", $2.c_str());
		}
		else {
			DEBUG_STACK("NEW %s", $2.c_str());
		}
		Compiler::context()->pushNode(Node::create_symbol);
		Compiler::context()->pushNode($2.c_str());
		Compiler::context()->pushNode(Compiler::context()->getModifiers());
	};

constant_rule: constant_token {
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

regexp_rule: slash_token {
			$$ = $1 + Compiler::context()->lexer.readRegex();
		} slash_token {
			$$ = $2 + $1;
		};

var_symbol_rule: dollar_token open_parenthesis_token expr_rule close_parenthesis_token;

modifier_rule: dollar_token {
		Compiler::context()->setModifiers(Reference::const_address);
	}
	| percent_token {
		Compiler::context()->setModifiers(Reference::const_value);
	}
	| const_token {
		Compiler::context()->setModifiers(Reference::const_address | Reference::const_value);
	}
	| at_token {
		Compiler::context()->setModifiers(Reference::global);
	}
	| modifier_rule dollar_token {
		Compiler::context()->setModifiers(Compiler::context()->getModifiers() | Reference::const_address);
	}
	| modifier_rule percent_token {
		Compiler::context()->setModifiers(Compiler::context()->getModifiers() | Reference::const_value);
	}
	| modifier_rule const_token {
		Compiler::context()->setModifiers(Compiler::context()->getModifiers() | Reference::const_address | Reference::const_value);
	}
	| modifier_rule at_token {
		Compiler::context()->setModifiers(Compiler::context()->getModifiers() | Reference::global);
	};

separator_rule: comma_token | separator_rule line_end_token;

empty_lines_rule: line_end_token | empty_lines_rule line_end_token;

%%

int yylex(std::string *token) {

	if (Compiler::context()->lexer.atEnd()) {
		return yy::parser::token::file_end_token;
	}

	*token = Compiler::context()->lexer.nextToken();
	return Compiler::context()->lexer.tokenType(*token);
}

void yy::parser::error(const std::string &msg) {
	Compiler::context()->parse_error(msg.c_str());
}

bool Compiler::build(DataStream *stream, Module::Infos node) {

	g_ctx = new BuildContext(stream, node);
	yy::parser parser;

	bool success = !parser.parse();

	delete g_ctx;
	g_ctx = nullptr;

	return success;
}

#endif // PARSER_HPP
