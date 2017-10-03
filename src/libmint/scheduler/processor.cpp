#include "scheduler/processor.h"
#include "scheduler/scheduler.h"
#include "ast/cursor.h"
#include "memory/builtin/library.h"
#include "memory/memorytool.h"
#include "memory/operatortool.h"
#include "memory/globaldata.h"

bool run_step(Cursor *cursor) {

	switch (cursor->next().command) {
	case Instruction::load_module:
		cursor->loadModule(cursor->next().symbol);
		break;

	case Instruction::load_symbol:
		cursor->stack().push_back(get_symbol_reference(&cursor->symbols(), cursor->next().symbol));
		break;
	case Instruction::load_member:
		cursor->stack().push_back(get_object_member(cursor, cursor->next().symbol));
		break;
	case Instruction::load_constant:
		cursor->stack().push_back(cursor->next().constant);
		break;
	case Instruction::load_var_symbol:
		cursor->stack().push_back(get_symbol_reference(&cursor->symbols(), var_symbol(cursor)));
		break;
	case Instruction::load_var_member:
		cursor->stack().push_back(get_object_member(cursor, var_symbol(cursor)));
		break;
	case Instruction::reload_reference:
		if (cursor->stack().back().isUnique()) {
			Reference *clone = new Reference();
			clone->clone(*cursor->stack().back());
			cursor->stack().push_back(SharedReference::unique(clone));
		}
		else {
			cursor->stack().push_back(cursor->stack().back());
		}
		break;
	case Instruction::unload_reference:
		cursor->stack().pop_back();
		break;
	case Instruction::reduce_member:
		reduce_member(cursor);
		break;

	case Instruction::create_symbol:
	{
		const char *symbol = cursor->next().symbol;
		const int flags = cursor->next().parameter;
		create_symbol(cursor, symbol, flags);
	}
		break;
	case Instruction::create_iterator:
		iterator_init(cursor, cursor->next().parameter);
		break;
	case Instruction::create_array:
		cursor->stack().push_back(SharedReference::unique(Reference::create<Array>()));
		((Object *)cursor->stack().back()->data())->construct();
		break;
	case Instruction::create_hash:
		cursor->stack().push_back(SharedReference::unique(Reference::create<Hash>()));
		((Object *)cursor->stack().back()->data())->construct();
		break;
	case Instruction::create_lib:
		cursor->stack().push_back(SharedReference::unique(Reference::create<Library>()));
		break;
	case Instruction::array_insert:
		array_append(cursor);
		break;
	case Instruction::hash_insert:
		hash_insert(cursor);
		break;

	case Instruction::regex_match:
		regex_match(cursor);
		break;
	case Instruction::regex_unmatch:
		regex_unmatch(cursor);
		break;

	case Instruction::register_class:
		GlobalData::instance().registerClass(cursor->next().parameter);
		break;

	case Instruction::move_op:
		move_operator(cursor);
		break;
	case Instruction::copy_op:
		copy_operator(cursor);
		break;
	case Instruction::add_op:
		add_operator(cursor);
		break;
	case Instruction::sub_op:
		sub_operator(cursor);
		break;
	case Instruction::mod_op:
		mod_operator(cursor);
		break;
	case Instruction::mul_op:
		mul_operator(cursor);
		break;
	case Instruction::div_op:
		div_operator(cursor);
		break;
	case Instruction::pow_op:
		pow_operator(cursor);
		break;
	case Instruction::is_op:
		is_operator(cursor);
		break;
	case Instruction::eq_op:
		eq_operator(cursor);
		break;
	case Instruction::ne_op:
		ne_operator(cursor);
		break;
	case Instruction::lt_op:
		lt_operator(cursor);
		break;
	case Instruction::gt_op:
		gt_operator(cursor);
		break;
	case Instruction::le_op:
		le_operator(cursor);
		break;
	case Instruction::ge_op:
		ge_operator(cursor);
		break;
	case Instruction::inc_op:
		inc_operator(cursor);
		break;
	case Instruction::dec_op:
		dec_operator(cursor);
		break;
	case Instruction::not_op:
		not_operator(cursor);
		break;
	case Instruction::and_op:
		and_operator(cursor);
		break;
	case Instruction::or_op:
		or_operator(cursor);
		break;
	case Instruction::band_op:
		band_operator(cursor);
		break;
	case Instruction::bor_op:
		bor_operator(cursor);
		break;
	case Instruction::xor_op:
		xor_operator(cursor);
		break;
	case Instruction::compl_op:
		compl_operator(cursor);
		break;
	case Instruction::pos_op:
		pos_operator(cursor);
		break;
	case Instruction::neg_op:
		neg_operator(cursor);
		break;
	case Instruction::shift_left_op:
		shift_left_operator(cursor);
		break;
	case Instruction::shift_right_op:
		shift_right_operator(cursor);
		break;
	case Instruction::inclusive_range_op:
		inclusive_range_operator(cursor);
		break;
	case Instruction::exclusive_range_op:
		exclusive_range_operator(cursor);
		break;
	case Instruction::subscript_op:
		subscript_operator(cursor);
		break;
	case Instruction::typeof_op:
		typeof_operator(cursor);
		break;
	case Instruction::membersof_op:
		membersof_operator(cursor);
		break;

	case Instruction::find_defined_symbol:
		find_defined_symbol(cursor, cursor->next().symbol);
		break;
	case Instruction::find_defined_member:
		find_defined_member(cursor, cursor->next().symbol);
		break;
	case Instruction::find_defined_var_symbol:
		find_defined_symbol(cursor, var_symbol(cursor));
		break;
	case Instruction::find_defined_var_member:
		find_defined_member(cursor, var_symbol(cursor));
		break;
	case Instruction::check_defined:
		check_defined(cursor);
		break;

	case Instruction::in_find:
		in_find(cursor);
		break;
	case Instruction::in_init:
		in_init(cursor);
		break;
	case Instruction::in_next:
		in_next(cursor);
		break;
	case Instruction::in_check:
		in_check(cursor);
		break;

	case Instruction::open_printer:
		cursor->openPrinter(to_printer(cursor->stack().back()));
		cursor->stack().pop_back();
		break;

	case Instruction::close_printer:
		cursor->closePrinter();
		break;

	case Instruction::print:
		print(cursor->printer(), cursor->stack().back());
		cursor->stack().pop_back();
		break;

	case Instruction::or_pre_check:
		if (to_boolean(cursor, *cursor->stack().back())) {
			cursor->jmp(cursor->next().parameter);
		}
		else {
			cursor->next();
		}
		break;
	case Instruction::and_pre_check:
		if (to_boolean(cursor, *cursor->stack().back())) {
			cursor->next();
		}
		else {
			cursor->jmp(cursor->next().parameter);
		}
		break;

	case Instruction::jump_zero:
		if (to_boolean(cursor, *cursor->stack().back())) {
			cursor->next();
		}
		else {
			cursor->jmp(cursor->next().parameter);
		}
		cursor->stack().pop_back();
		break;

	case Instruction::jump:
		cursor->jmp(cursor->next().parameter);
		break;

	case Instruction::set_retrive_point:
		cursor->setRetrivePoint(cursor->next().parameter);
		break;
	case Instruction::unset_retrive_point:
		cursor->unsetRetivePoint();
		break;
	case Instruction::raise:
		cursor->raise(cursor->stack().back());
		break;

	case Instruction::yield:
		yield(cursor);
		break;
	case Instruction::load_default_result:
		load_default_result(cursor);
		break;

	case Instruction::capture_symbol:
		capture_symbol(cursor, cursor->next().symbol);
		break;
	case Instruction::capture_all:
		capture_all_symbols(cursor);
		break;
	case Instruction::call:
		call_operator(cursor, cursor->next().parameter);
		break;
	case Instruction::call_member:
		call_member_operator(cursor, cursor->next().parameter);
		break;
	case Instruction::init_call:
		init_call(cursor);
		break;
	case Instruction::init_param:
		init_parameter(cursor, cursor->next().symbol);
		break;
	case Instruction::exit_call:
		exit_call(cursor);
		break;
	case Instruction::exit_exec:
		Scheduler::instance()->exit(to_number(cursor, *cursor->stack().back()));
		cursor->stack().pop_back();
		return false;
	case Instruction::module_end:
		return cursor->exitModule();
	}

	return true;
}
