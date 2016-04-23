%{

#ifndef PARSER_HPP
#define PARSER_HPP

#include "Compiler/compiler.h"

#define YYSTYPE std::string

#if !defined(NDEBUG) && !defined(_DEBUG)
#define DEBUG_STACK(msg, ...) printf(msg, ##__VA_ARGS__);
#else
#define DEBUG_STACK(msg, ...)
#endif


int yylex(std::string *token);
%}

%token assert_token
%token break_token
%token catch_token
%token class_token
%token continue_token
%token def_token
%token elif_token
%token else_token
%token exec_token
%token for_token
%token if_token
%token in_token
%token load_token
%token print_token
%token raise_token
%token return_token
%token try_token
%token while_token
%token yield_token
%token constant_token
%token symbol_token

%token line_end_token
%token comment_token
%token dollar_token
%token at_token

%left comma_token
%left dbl_pipe_token
%left dbl_amp_token
%left pipe_token
%left caret_token
%left amp_token
%right equal_token dbldot_token dbldot_equal_token
%left dbl_equal_token exclamation_equal_token is_token
%left left_angled_token right_angled_token left_angled_equal_token right_angled_equal_token
%left dbl_left_angled_token dbl_right_angled_token
%left plus_token minus_token
%left asterisk_token slash_token percent_token
%right exclamation_token tilde_token membersof_token defined_token
%left dbl_plus_token dbl_minus_token dbl_asterisk_token
%left dot_token open_parenthesis_token close_parenthesis_token open_bracket_token close_bracket_token open_brace_token close_brace_token

%%

modul_rule: stmt_list_rule {
		DEBUG_STACK("END\n")
		Compiler::context()->pushInstruction(Instruction::module_end);
		fflush(stdout);
	};

stmt_list_rule: stmt_list_rule stmt_rule
	| stmt_rule;

stmt_rule: load_token modul_path_rule line_end_token {
		DEBUG_STACK("LOAD MODUL %s\n", $2.c_str())
		Compiler::context()->pushInstruction(Instruction::load_modul);
		Compiler::context()->pushInstruction($2.c_str());
	}
	| cond_if_rule stmt_bloc_rule {
		DEBUG_STACK("LBL FWD\n")
		Compiler::context()->resolveJumpForward();
	}
	| cond_if_rule stmt_bloc_rule cond_else_rule stmt_bloc_rule {
		DEBUG_STACK("LBL FWD\n")
		Compiler::context()->resolveJumpForward();
	}
	| cond_if_rule stmt_bloc_rule elif_bloc_rule {
		DEBUG_STACK("LBL FWD\n")
		Compiler::context()->resolveJumpForward();
	}
	| cond_if_rule stmt_bloc_rule elif_bloc_rule cond_else_rule stmt_bloc_rule {
		DEBUG_STACK("LBL FWD\n")
		Compiler::context()->resolveJumpForward();
	}
	| loop_rule stmt_bloc_rule {
		DEBUG_STACK("JMP BWD\n")
		Compiler::context()->pushInstruction(Instruction::jump);
		Compiler::context()->resolveJumpBackward();
		DEBUG_STACK("LBL FWD\n")
		Compiler::context()->resolveJumpForward();
	}
	| range_rule stmt_bloc_rule {
		DEBUG_STACK("JMP BWD\n")
		Compiler::context()->pushInstruction(Instruction::jump);
		Compiler::context()->resolveJumpBackward();
		DEBUG_STACK("LBL FWD\n")
		Compiler::context()->resolveJumpForward();
		DEBUG_STACK(" --- 4 : range done ---\n")
	}
	| print_rule stmt_bloc_rule {
		DEBUG_STACK("CLOSE PRINTER\n")
		Compiler::context()->pushInstruction(Instruction::close_printer);
	}
	| return_token expr_rule line_end_token {
		DEBUG_STACK("EXIT CALL\n")
	}
	| expr_rule line_end_token {
		DEBUG_STACK("PRINT\n")
		Compiler::context()->pushInstruction(Instruction::print);
	}
	| line_end_token;

modul_path_rule: symbol_token {
		$$ = $1;
	}
	| modul_path_rule dot_token symbol_token {
		$$ = $1 + $2 + $3;
	};

elif_bloc_rule: cond_elif_rule stmt_bloc_rule {
		DEBUG_STACK("LBL FWD\n")
		Compiler::context()->shiftJumpForward();
		Compiler::context()->resolveJumpForward();
	}
	| elif_bloc_rule cond_elif_rule stmt_bloc_rule {
		DEBUG_STACK("LBL FWD\n")
		Compiler::context()->shiftJumpForward();
		Compiler::context()->resolveJumpForward();
	};

stmt_bloc_rule: open_brace_token stmt_list_rule close_brace_token
	| open_brace_token expr_rule close_brace_token {
		DEBUG_STACK("PRINT\n")
		Compiler::context()->pushInstruction(Instruction::print);
	};

cond_if_rule: if_token expr_rule {
		DEBUG_STACK("JZR FWD\n")
		Compiler::context()->pushInstruction(Instruction::jump_zero);
		Compiler::context()->startJumpForward();
	};

cond_elif_rule: elif_rule expr_rule {
		DEBUG_STACK("JZR FWD\n")
		Compiler::context()->pushInstruction(Instruction::jump_zero);
		Compiler::context()->startJumpForward();
	};

elif_rule: elif_token {
		DEBUG_STACK("JMP FWD\n")
		Compiler::context()->pushInstruction(Instruction::jump);
		Compiler::context()->startJumpForward();
		DEBUG_STACK("LBL FWD\n")
		Compiler::context()->shiftJumpForward();
		Compiler::context()->resolveJumpForward();
	};

cond_else_rule: else_token {
		DEBUG_STACK("JMP FWD\n")
		Compiler::context()->pushInstruction(Instruction::jump);
		Compiler::context()->startJumpForward();
		DEBUG_STACK("LBL FWD\n")
		Compiler::context()->shiftJumpForward();
		Compiler::context()->resolveJumpForward();
	};

loop_rule: while_rule expr_rule {
		DEBUG_STACK("JZR FWD\n")
		Compiler::context()->pushInstruction(Instruction::jump_zero);
		Compiler::context()->startJumpForward();
	};

while_rule: while_token {
		DEBUG_STACK("LBL BWD\n")
		Compiler::context()->startJumpBackward();
	};

range_rule: for_token range_init_rule comma_token range_next_rule comma_token range_cond_rule
	| for_token ident_rule in_token expr_rule {

		Compiler::context()->pushInstruction(Instruction::in_init);
		Compiler::context()->pushInstruction(Instruction::jump);
		Compiler::context()->startJumpForward();

		Compiler::context()->startJumpBackward();
		Compiler::context()->pushInstruction(Instruction::in_next);
		Compiler::context()->resolveJumpForward();

		Compiler::context()->pushInstruction(Instruction::in_check);
		Compiler::context()->pushInstruction(Instruction::jump_zero);
		Compiler::context()->startJumpForward();
	};

range_init_rule: expr_rule {
		DEBUG_STACK("POP\n")
		Compiler::context()->pushInstruction(Instruction::unload_reference);
		DEBUG_STACK("JMP FWD\n")
		Compiler::context()->pushInstruction(Instruction::jump);
		Compiler::context()->startJumpForward();
		Compiler::context()->startJumpBackward();
		DEBUG_STACK(" --- 1 : range pre condition ---\n")
	};

range_next_rule: expr_rule {
		DEBUG_STACK("POP\n")
		Compiler::context()->pushInstruction(Instruction::unload_reference);
		DEBUG_STACK("LBL FWD\n")
		Compiler::context()->resolveJumpForward();
		DEBUG_STACK(" --- 2 : range post condition ---\n")
	};

range_cond_rule: expr_rule {
		DEBUG_STACK("JZR FWD\n")
		Compiler::context()->pushInstruction(Instruction::jump_zero);
		Compiler::context()->startJumpForward();
		DEBUG_STACK(" --- 3 : range condition ---\n")
	};

start_hash_rule: open_brace_token {
		DEBUG_STACK("NEW HASH\n")
		Compiler::context()->pushInstruction(Instruction::create_hash);
	};

stop_hash_rule: close_brace_token;

hash_item_rule: hash_item_rule comma_token expr_rule dbldot_token expr_rule {
		DEBUG_STACK("HASH PUSH\n")
		Compiler::context()->pushInstruction(Instruction::hash_insert);
	}
	| expr_rule dbldot_token expr_rule {
		DEBUG_STACK("HASH PUSH\n")
		Compiler::context()->pushInstruction(Instruction::hash_insert);
	}
	| ;

start_array_rule: open_bracket_token {
		DEBUG_STACK("NEW ARRAY\n")
		Compiler::context()->pushInstruction(Instruction::create_array);
	};

stop_array_rule: close_bracket_token;

array_item_rule: array_item_rule comma_token expr_rule {
		DEBUG_STACK("ARRAY PUSH\n")
		Compiler::context()->pushInstruction(Instruction::array_insert);
	}
	| expr_rule {
		DEBUG_STACK("ARRAY PUSH\n")
		Compiler::context()->pushInstruction(Instruction::array_insert);
	}
	| ;

print_rule: print_token {
		DEBUG_STACK("PUSH 0\n")
		Compiler::context()->pushInstruction(Instruction::load_constant);
		Compiler::context()->pushInstruction(Compiler::makeData("1"));
		DEBUG_STACK("OPEN PRINTER\n")
		Compiler::context()->pushInstruction(Instruction::open_printer);
	}
	| print_token open_parenthesis_token expr_rule close_parenthesis_token {
		DEBUG_STACK("OPEN PRINTER\n")
		Compiler::context()->pushInstruction(Instruction::open_printer);
	};

expr_rule: expr_rule equal_token expr_rule {
		DEBUG_STACK("MOVE\n")
		Compiler::context()->pushInstruction(Instruction::move);
	}
	| expr_rule dbldot_equal_token expr_rule {
		DEBUG_STACK("COPY\n")
		Compiler::context()->pushInstruction(Instruction::copy);
	}
	| expr_rule plus_token expr_rule {
		DEBUG_STACK("ADD\n")
		Compiler::context()->pushInstruction(Instruction::add);
	}
	| expr_rule minus_token expr_rule {
		DEBUG_STACK("SUB\n")
		Compiler::context()->pushInstruction(Instruction::sub);
	}
	| expr_rule asterisk_token expr_rule {
		DEBUG_STACK("MUL\n")
		Compiler::context()->pushInstruction(Instruction::mul);
	}
	| expr_rule slash_token expr_rule {
		DEBUG_STACK("DIV\n")
		Compiler::context()->pushInstruction(Instruction::div);
	}
	| expr_rule percent_token expr_rule {
		DEBUG_STACK("MOD\n")
		Compiler::context()->pushInstruction(Instruction::mod);
	}
	| expr_rule dbl_asterisk_token expr_rule {
		DEBUG_STACK("POW\n")
		Compiler::context()->pushInstruction(Instruction::pow);
	}
	| expr_rule is_token expr_rule {
		DEBUG_STACK("IS\n")
		Compiler::context()->pushInstruction(Instruction::is);
	}
	| expr_rule dbl_equal_token expr_rule {
		DEBUG_STACK("EQ\n")
		Compiler::context()->pushInstruction(Instruction::eq);
	}
	| expr_rule exclamation_equal_token expr_rule {
		DEBUG_STACK("NE\n")
		Compiler::context()->pushInstruction(Instruction::ne);
	}
	| expr_rule left_angled_token expr_rule {
		DEBUG_STACK("LT\n")
		Compiler::context()->pushInstruction(Instruction::lt);
	}
	| expr_rule right_angled_token expr_rule {
		DEBUG_STACK("GT\n")
		Compiler::context()->pushInstruction(Instruction::gt);
	}
	| expr_rule left_angled_equal_token expr_rule {
		DEBUG_STACK("LE\n")
		Compiler::context()->pushInstruction(Instruction::le);
	}
	| expr_rule right_angled_equal_token expr_rule {
		DEBUG_STACK("GE\n")
		Compiler::context()->pushInstruction(Instruction::ge);
	}
	| expr_rule dbl_left_angled_token expr_rule {
		DEBUG_STACK("SHIFT LEFT\n")
		Compiler::context()->pushInstruction(Instruction::shift_left);
	}
	| expr_rule dbl_right_angled_token expr_rule {
		DEBUG_STACK("SHIFT RIGHT\n")
		Compiler::context()->pushInstruction(Instruction::shift_right);
	}
	| expr_rule dbl_plus_token {
		DEBUG_STACK("INC\n")
		Compiler::context()->pushInstruction(Instruction::inc);
	}
	| expr_rule dbl_minus_token {
		DEBUG_STACK("DEC\n")
		Compiler::context()->pushInstruction(Instruction::dec);
	}
	| exclamation_token expr_rule {
		DEBUG_STACK("NOT\n")
		Compiler::context()->pushInstruction(Instruction::op_not);
	}
	| tilde_token expr_rule {
		DEBUG_STACK("BNOT\n")
		Compiler::context()->pushInstruction(Instruction::inv);
	}
	| membersof_token expr_rule {
		DEBUG_STACK("MBROF\n")
		Compiler::context()->pushInstruction(Instruction::membersof);
	}
	/// \todo work with symbols
	| defined_token expr_rule {
		DEBUG_STACK("DEFINED\n")
		Compiler::context()->pushInstruction(Instruction::defined);
	}
	/*| expr_rule in_token expr_rule {
		DEBUG_STACK("FIND\n")
		Compiler::context()->pushInstruction(Instruction::in_find);
	}*/
	| expr_rule open_bracket_token expr_rule close_bracket_token {
		DEBUG_STACK("SUBSCR\n")
		Compiler::context()->pushInstruction(Instruction::subscript);
	}
	| open_parenthesis_token expr_rule close_parenthesis_token
	| start_array_rule array_item_rule stop_array_rule
	| start_hash_rule hash_item_rule stop_hash_rule
	| ident_rule;

ident_rule: constant_token {
		DEBUG_STACK("PUSH %s\n", $1.c_str())
		Compiler::context()->pushInstruction(Instruction::load_constant);
		Compiler::context()->pushInstruction(Compiler::makeData($1.c_str()));
	}
	| symbol_token {
		DEBUG_STACK("LOAD %s\n", $1.c_str())
		Compiler::context()->pushInstruction(Instruction::load_symbol);
		Compiler::context()->pushInstruction($1.c_str());
	}
	| modifier_rule symbol_token {
		DEBUG_STACK("NEW GLOABL %s\n", $2.c_str())
		Compiler::context()->pushInstruction(Instruction::create_symbol);
		Compiler::context()->pushInstruction($2.c_str());
		Compiler::context()->pushInstruction(Reference::standard);
	}
	| at_token symbol_token {
		DEBUG_STACK("NEW %s\n", $2.c_str())
		Compiler::context()->pushInstruction(Instruction::create_global_symbol);
		Compiler::context()->pushInstruction($2.c_str());
		Compiler::context()->pushInstruction(Compiler::context()->getModifiers());
	}
	| modifier_rule at_token symbol_token {
		DEBUG_STACK("NEW GLOABL %s\n", $3.c_str())
		Compiler::context()->pushInstruction(Instruction::create_symbol);
		Compiler::context()->pushInstruction($3.c_str());
		Compiler::context()->pushInstruction(Compiler::context()->getModifiers());
	}
	| at_token modifier_rule symbol_token {
		DEBUG_STACK("NEW GLOABL %s\n", $3.c_str())
		Compiler::context()->pushInstruction(Instruction::create_symbol);
		Compiler::context()->pushInstruction($3.c_str());
		Compiler::context()->pushInstruction(Compiler::context()->getModifiers());
	}
	| expr_rule dot_token symbol_token {
		DEBUG_STACK("LOAD MBR %s\n", $3.c_str())
		Compiler::context()->pushInstruction(Instruction::load_member);
		Compiler::context()->pushInstruction($3.c_str());
	};

modifier_rule: dollar_token {
		Compiler::context()->setModifiers(Reference::const_ref);
	}
	| percent_token {
		Compiler::context()->setModifiers(Reference::const_value);
	}
	| modifier_rule dollar_token {
		Compiler::context()->setModifiers(Compiler::context()->getModifiers() | Reference::const_ref);
	}
	| modifier_rule percent_token {
		Compiler::context()->setModifiers(Compiler::context()->getModifiers() | Reference::const_value);
	};
%%

int yylex(std::string *token) {

    *token = Compiler::context()->lexer.nextToken();
    return Compiler::context()->lexer.tokenType(*token);
}

void yy::parser::error(const std::string &msg) {

	size_t lineNumber = Compiler::context()->lexer.lineNumber();
	fflush(stdout);
}

bool Compiler::build(DataStream *stream, Modul::Context node) {

    g_ctx = new BuildContext(stream, node);
    yy::parser parser;

    bool success = !parser.parse();

    delete g_ctx;
    g_ctx = nullptr;

    return success;
}

#endif // PARSER_HPP
