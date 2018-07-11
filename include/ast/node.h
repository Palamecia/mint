#ifndef NODE_H
#define NODE_H

#include "memory/reference.h"

namespace mint {

union MINT_EXPORT Node {
	enum Command {
		load_module,

		load_symbol,
		load_member,
		load_constant,
		load_var_symbol,
		load_var_member,
		reload_reference,
		unload_reference,

		create_symbol,
		create_iterator,
		create_array,
		create_hash,
		create_lib,
		array_insert,
		hash_insert,

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

		or_pre_check,
		and_pre_check,
		jump_zero,
		jump,

		set_retrieve_point,
		unset_retrieve_point,
		raise,

		yield,
		load_default_result,

		capture_symbol,
		capture_all,
		call,
		call_member,
		init_call,
		init_member_call,
		init_var_member_call,
		init_param,
		exit_call,
		exit_thread,
		exit_exec,
		module_end
	};
	Command command;
	int parameter;
	const char *symbol;
	Reference *constant;
};

}

#endif // NODE_H
