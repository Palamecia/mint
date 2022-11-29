#ifndef MINT_TOKEN_H
#define MINT_TOKEN_H

#include "config.h"

namespace mint {
namespace token {

enum Type {
	assert_token,
	break_token,
	case_token,
	catch_token,
	class_token,
	const_token,
	continue_token,
	def_token,
	default_token,
	elif_token,
	else_token,
	enum_token,
	exit_token,
	for_token,
	if_token,
	in_token,
	let_token,
	lib_token,
	load_token,
	package_token,
	print_token,
	raise_token,
	return_token,
	switch_token,
	try_token,
	while_token,
	yield_token,
	var_token,
	constant_token,
	string_token,
	number_token,
	symbol_token,
	no_line_end_token,
	line_end_token,
	file_end_token,
	comment_token,
	dollar_token,
	at_token,
	sharp_token,
	back_slash_token,
	comma_token,
	dbl_pipe_token,
	dbl_amp_token,
	pipe_token,
	caret_token,
	amp_token,
	equal_token,
	question_token,
	dbldot_token,
	dbldot_equal_token,
	close_bracket_equal_token,
	plus_equal_token,
	minus_equal_token,
	asterisk_equal_token,
	slash_equal_token,
	percent_equal_token,
	dbl_left_angled_equal_token,
	dbl_right_angled_equal_token,
	amp_equal_token,
	pipe_equal_token,
	caret_equal_token,
	dot_dot_token,
	tpl_dot_token,
	dbl_equal_token,
	exclamation_equal_token,
	is_token,
	equal_tilde_token,
	exclamation_tilde_token,
	left_angled_token,
	right_angled_token,
	left_angled_equal_token,
	right_angled_equal_token,
	dbl_left_angled_token,
	dbl_right_angled_token,
	plus_token,
	minus_token,
	asterisk_token,
	slash_token,
	percent_token,
	exclamation_token,
	tilde_token,
	typeof_token,
	membersof_token,
	defined_token,
	dbl_plus_token,
	dbl_minus_token,
	dbl_asterisk_token,
	dot_token,
	open_parenthesis_token,
	close_parenthesis_token,
	open_bracket_token,
	close_bracket_token,
	open_brace_token,
	close_brace_token
};

MINT_EXPORT Type fromLocalId(int id);

}
}

#endif // MINT_TOKEN_H
