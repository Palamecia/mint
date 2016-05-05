#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include "Memory/reference.h"

union Instruction {
	enum Command {
		load_modul,

		load_symbol,
		load_member,
		load_constant,
		load_var_symbol,
		load_var_member,
		unload_reference,
		reduce_member,

		create_symbol,
		create_global_symbol,
		create_array,
		create_hash,
		array_insert,
		hash_insert,

		register_class,

		move,
		copy,
		add,
		sub,
		mod,
		mul,
		div,
		pow,
		is,
		eq,
		ne,
		lt,
		gt,
		le,
		ge,
		inc,
		dec,
		op_not,
		inv,
		shift_left,
		shift_right,
		subscript,
		membersof,
		defined,

		in_find,
		in_init,
		in_next,
		in_check,

		open_printer,
		close_printer,
		print,

		jump_zero,
		jump,

		call,
		call_member,
		init_call,
		init_param,
		exit_call,
		module_end
	};
	Command command;
	int parameter;
	const char *symbol;
	Reference *constant;
};

#endif // INSTRUCTION_H
