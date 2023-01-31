#ifndef MINT_NODE_H
#define MINT_NODE_H

#include "ast/symbol.h"
#include "memory/reference.h"

namespace mint {

union MINT_EXPORT Node {
	enum Command {
		load_module,

		load_fast,
		load_symbol,
		load_member,
		load_operator,
		load_constant,
		load_var_symbol,
		load_var_member,
		store_reference,
		reload_reference,
		unload_reference,
		load_extra_arguments,
		reset_symbol,
		reset_fast,

		create_fast,
		create_symbol,
		create_function,
		create_iterator,
		create_array,
		create_hash,
		create_lib,
		function_overload,

		regex_match,
		regex_unmatch,

		open_package,
		close_package,
		register_class,

		move_op,
		copy_op,
		add_op,
		sub_op,
		mod_op,
		mul_op,
		div_op,
		pow_op,
		is_op,
		eq_op,
		ne_op,
		lt_op,
		gt_op,
		le_op,
		ge_op,
		inc_op,
		dec_op,
		not_op,
		and_op,
		or_op,
		band_op,
		bor_op,
		xor_op,
		compl_op,
		pos_op,
		neg_op,
		shift_left_op,
		shift_right_op,
		inclusive_range_op,
		exclusive_range_op,
		subscript_op,
		subscript_move_op,
		typeof_op,
		membersof_op,
		find_op,
		in_op,

		find_defined_symbol,
		find_defined_member,
		find_defined_var_symbol,
		find_defined_var_member,
		check_defined,

		find_init,
		find_next,
		find_check,
		range_init,
		range_next,
		range_check,
		range_iterator_check,

		open_printer,
		close_printer,
		print,

		or_pre_check,
		and_pre_check,
		case_jump,
		jump_zero,
		jump,

		set_retrieve_point,
		unset_retrieve_point,
		raise,

		yield,
		exit_generator,
		yield_exit_generator,

		capture_symbol,
		capture_as,
		capture_all,
		call,
		call_member,
		call_builtin,
		init_call,
		init_member_call,
		init_operator_call,
		init_var_member_call,
		init_exception,
		reset_exception,
		init_param,
		exit_call,
		exit_thread,
		exit_exec,
		module_end
	};

	Node(Command command);
	Node(int parameter);
	Node(Symbol *symbol);
	Node(Reference *constant);

	Command command;
	int parameter;
	Symbol *symbol;
	Reference *constant;
};

}

#endif // MINT_NODE_H
