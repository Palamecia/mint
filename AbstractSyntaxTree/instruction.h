#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include "Memory/reference.h"

union Instruction {
	enum Command {
		load_symbol,
		load_member,
		load_constant,
		unload_reference,

		create_symbol,
		create_global_symbol,
		create_array,
		create_hash,
		array_insert,
		hash_insert,

		register_class,
		register_symbol,

		move,
		copy,
		call,
		call_member,
		add,
		sub,
		mod,
		mul,
		div,
		eq,

		open_printer,
		close_printer,
		print,

		jump_zero,
		jump,

		exit_call,
		module_end
	};
	Command command;
	int parameter;
	const char *symbol;
	Reference *constant;
};

#endif // INSTRUCTION_H
