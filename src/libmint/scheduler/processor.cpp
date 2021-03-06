#include "scheduler/processor.h"
#include "scheduler/scheduler.h"
#include "ast/cursor.h"
#include "memory/builtin/library.h"
#include "memory/memorytool.h"
#include "memory/functiontool.h"
#include "memory/operatortool.h"
#include "memory/globaldata.h"

using namespace std;
using namespace mint;

static mutex g_step_mutex;

bool mint::run_step(Cursor *cursor) {

	lock_guard<mutex> lock(g_step_mutex);

	switch (cursor->next().command) {
	case Node::load_module:
		cursor->loadModule(cursor->next().symbol);
		break;

	case Node::load_symbol:
		cursor->stack().emplace_back(get_symbol_reference(&cursor->symbols(), cursor->next().symbol));
		break;
	case Node::load_member:
		reduce_member(cursor, get_object_member(cursor, *cursor->stack().back(), cursor->next().symbol));
		break;
	case Node::load_constant:
		cursor->stack().emplace_back(SharedReference::unsafe(cursor->next().constant));
		break;
	case Node::load_var_symbol:
		cursor->stack().emplace_back(get_symbol_reference(&cursor->symbols(), var_symbol(cursor)));
		break;
	case Node::load_var_member:
		reduce_member(cursor, get_object_member(cursor, *cursor->stack().back(), var_symbol(cursor)));
		break;
	case Node::store_reference:
		if (SharedReference reference = move(cursor->stack().back())) {
			Reference *clone = new StrongReference();
			clone->clone(*reference);
			cursor->stack().pop_back();
			cursor->stack().emplace_back(SharedReference::unique(clone));
			cursor->stack().emplace_back(move(reference));
		}
		break;
	case Node::reload_reference:
		if (cursor->stack().back().isUnique()) {
			Reference *clone = new StrongReference();
			clone->clone(*cursor->stack().back());
			cursor->stack().emplace_back(SharedReference::unique(clone));
		}
		else {
			cursor->stack().emplace_back(move(cursor->stack().back()));
		}
		break;
	case Node::unload_reference:
		cursor->stack().pop_back();
		break;
	case Node::load_extra_arguments:
		load_extra_arguments(cursor);
		break;

	case Node::create_symbol:
	{
		const char *symbol = cursor->next().symbol;
		const int flags = cursor->next().parameter;
		create_symbol(cursor, symbol, flags);
	}
		break;
	case Node::create_iterator:
		iterator_init_from_stack(cursor, cursor->next().parameter);
		break;
	case Node::create_array:
		cursor->stack().emplace_back(create_array({}));
		break;
	case Node::create_hash:
		cursor->stack().emplace_back(create_hash({}));
		break;
	case Node::create_lib:
		cursor->stack().emplace_back(SharedReference::unique(StrongReference::create<Library>()));
		break;
	case Node::array_insert:
		array_append_from_stack(cursor);
		break;
	case Node::hash_insert:
		hash_insert_from_stack(cursor);
		break;

	case Node::regex_match:
		regex_match(cursor);
		break;
	case Node::regex_unmatch:
		regex_unmatch(cursor);
		break;

	case Node::open_package:
		cursor->symbols().openPackage(cursor->next().constant->data<Package>()->data);
		break;
	case Node::close_package:
		cursor->symbols().closePackage();
		break;
	case Node::register_class:
		cursor->symbols().getPackage()->registerClass(cursor->next().parameter);
		break;

	case Node::move_op:
		move_operator(cursor);
		break;
	case Node::copy_op:
		copy_operator(cursor);
		break;
	case Node::add_op:
		add_operator(cursor);
		break;
	case Node::sub_op:
		sub_operator(cursor);
		break;
	case Node::mod_op:
		mod_operator(cursor);
		break;
	case Node::mul_op:
		mul_operator(cursor);
		break;
	case Node::div_op:
		div_operator(cursor);
		break;
	case Node::pow_op:
		pow_operator(cursor);
		break;
	case Node::is_op:
		is_operator(cursor);
		break;
	case Node::eq_op:
		eq_operator(cursor);
		break;
	case Node::ne_op:
		ne_operator(cursor);
		break;
	case Node::lt_op:
		lt_operator(cursor);
		break;
	case Node::gt_op:
		gt_operator(cursor);
		break;
	case Node::le_op:
		le_operator(cursor);
		break;
	case Node::ge_op:
		ge_operator(cursor);
		break;
	case Node::inc_op:
		inc_operator(cursor);
		break;
	case Node::dec_op:
		dec_operator(cursor);
		break;
	case Node::not_op:
		not_operator(cursor);
		break;
	case Node::and_op:
		and_operator(cursor);
		break;
	case Node::or_op:
		or_operator(cursor);
		break;
	case Node::band_op:
		band_operator(cursor);
		break;
	case Node::bor_op:
		bor_operator(cursor);
		break;
	case Node::xor_op:
		xor_operator(cursor);
		break;
	case Node::compl_op:
		compl_operator(cursor);
		break;
	case Node::pos_op:
		pos_operator(cursor);
		break;
	case Node::neg_op:
		neg_operator(cursor);
		break;
	case Node::shift_left_op:
		shift_left_operator(cursor);
		break;
	case Node::shift_right_op:
		shift_right_operator(cursor);
		break;
	case Node::inclusive_range_op:
		inclusive_range_operator(cursor);
		break;
	case Node::exclusive_range_op:
		exclusive_range_operator(cursor);
		break;
	case Node::subscript_op:
		subscript_operator(cursor);
		break;
	case Node::subscript_move_op:
		subscript_move_operator(cursor);
		break;
	case Node::typeof_op:
		typeof_operator(cursor);
		break;
	case Node::membersof_op:
		membersof_operator(cursor);
		break;
	case Node::find_op:
		find_operator(cursor);
		break;
	case Node::in_op:
		in_operator(cursor);
		break;

	case Node::find_defined_symbol:
		find_defined_symbol(cursor, cursor->next().symbol);
		break;
	case Node::find_defined_member:
		find_defined_member(cursor, cursor->next().symbol);
		break;
	case Node::find_defined_var_symbol:
		find_defined_symbol(cursor, var_symbol(cursor));
		break;
	case Node::find_defined_var_member:
		find_defined_member(cursor, var_symbol(cursor));
		break;
	case Node::check_defined:
		check_defined(cursor);
		break;

	case Node::find_init:
		find_init(cursor);
		break;
	case Node::find_next:
		find_next(cursor);
		break;
	case Node::find_check:
		find_check(cursor, cursor->next().parameter);
		break;
	case Node::range_init:
		range_init(cursor);
		break;
	case Node::range_next:
		range_next(cursor);
		break;
	case Node::range_check:
		range_check(cursor, cursor->next().parameter);
		break;
	case Node::range_iterator_check:
		range_iterator_check(cursor, cursor->next().parameter);
		break;

	case Node::open_printer:
		cursor->openPrinter(create_printer(cursor));
		break;

	case Node::close_printer:
		cursor->closePrinter();
		break;

	case Node::print:
	{
		SharedReference reference = move(cursor->stack().back());
		cursor->stack().pop_back();
		print(cursor->printer(), reference);
	}
		break;

	case Node::or_pre_check:
		if (to_boolean(cursor, cursor->stack().back())) {
			cursor->jmp(cursor->next().parameter);
		}
		else {
			cursor->next();
		}
		break;
	case Node::and_pre_check:
		if (to_boolean(cursor, cursor->stack().back())) {
			cursor->next();
		}
		else {
			cursor->jmp(cursor->next().parameter);
		}
		break;

	case Node::case_jump:
		if (to_boolean(cursor, cursor->stack().back())) {
			cursor->jmp(cursor->next().parameter);
			cursor->stack().pop_back();
		}
		else {
			cursor->next();
		}
		cursor->stack().pop_back();
		break;

	case Node::jump_zero:
		if (to_boolean(cursor, cursor->stack().back())) {
			cursor->next();
		}
		else {
			cursor->jmp(cursor->next().parameter);
		}
		cursor->stack().pop_back();
		break;

	case Node::jump:
		cursor->jmp(cursor->next().parameter);
		break;

	case Node::set_retrieve_point:
		cursor->setRetrievePoint(cursor->next().parameter);
		break;
	case Node::unset_retrieve_point:
		cursor->unsetRetrievePoint();
		break;
	case Node::raise:
		cursor->raise(move(cursor->stack().back()));
		break;

	case Node::yield:
		yield(cursor);
		break;
	case Node::finalize_iterator:
		iterator_finalize(cursor->stack().back());
		break;
	case Node::load_current_result:
		load_current_result(cursor);
		break;
	case Node::load_default_result:
		load_default_result(cursor);
		break;

	case Node::capture_symbol:
		capture_symbol(cursor, cursor->next().symbol);
		break;
	case Node::capture_all:
		capture_all_symbols(cursor);
		break;
	case Node::call:
		call_operator(cursor, cursor->next().parameter);
		break;
	case Node::call_member:
		call_member_operator(cursor, cursor->next().parameter);
		break;
	case Node::init_call:
		init_call(cursor);
		break;
	case Node::init_member_call:
		init_member_call(cursor, cursor->next().symbol);
		break;
	case Node::init_var_member_call:
		init_member_call(cursor, var_symbol(cursor));
		break;
	case Node::init_param:
		init_parameter(cursor, cursor->next().symbol);
		break;
	case Node::exit_call:
		exit_call(cursor);
		break;
	case Node::exit_thread:
		return false;
	case Node::exit_exec:
		Scheduler::instance()->exit(static_cast<int>(to_number(cursor, cursor->stack().back())));
		cursor->stack().pop_back();
		return false;
	case Node::module_end:
		return cursor->exitModule();
	}

	return true;
}

void mint::lock_processor() {
	g_step_mutex.lock();
}

void mint::unlock_processor() {
	g_step_mutex.unlock();
}
