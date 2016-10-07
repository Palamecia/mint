#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include "Memory/reference.h"

union Instruction {
	enum Command {
		load_module,

		load_symbol,
		load_member,
		load_constant,
		load_var_symbol,
		load_var_member,
		unload_reference,
		reduce_member,

		create_symbol,
		create_array,
		create_hash,
		array_insert,
		hash_insert,

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
		xor_op,
		compl_op,
		pos_op,
		neg_op,
		shift_left_op,
		shift_right_op,
		inclusive_range_op,
		exclusive_range_op,
		subscript_op,
		typeof_op,
		membersof_op,

		find_defined_symbol,
		find_defined_member,
		find_defined_var_symbol,
		find_defined_var_member,
		check_defined,

		in_find,
		in_init,
		in_next,
		in_check,

		open_printer,
		close_printer,
		print,

		jump_zero,
		jump,

		set_retrive_point,
		unset_retrive_point,
		raise,

		call,
		call_member,
		init_call,
		init_param,
		exit_call,
		exit_exec,
		module_end
	};
	Command command;
	int parameter;
	const char *symbol;
	Reference *constant;
};

#endif // INSTRUCTION_H
