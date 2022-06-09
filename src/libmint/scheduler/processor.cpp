#include "scheduler/processor.h"
#include "scheduler/scheduler.h"
#include "debug/debuginterface.h"
#include "ast/abstractsyntaxtree.h"
#include "ast/cursor.h"
#include "memory/builtin/library.h"
#include "memory/memorytool.h"
#include "memory/functiontool.h"
#include "memory/operatortool.h"
#include "memory/globaldata.h"

using namespace std;
using namespace mint;

static constexpr const size_t quantum = 64 * 1024;
static atomic_bool g_single_thread(true);
static mutex g_step_mutex;

static bool do_run_steps(Cursor *cursor, size_t count) {

	auto &stack = cursor->stack();
	AbstractSyntaxTree *ast = cursor->ast();

	while (count--) {
		switch (cursor->next().command) {
		case Node::load_module:
			cursor->loadModule(cursor->next().symbol->str());
			break;

		case Node::load_fast:
		{
			Symbol &symbol = *cursor->next().symbol;
			const size_t index = static_cast<size_t>(cursor->next().parameter);
			stack.emplace_back(cursor->symbols().get_fast(symbol, index));
		}
			break;
		case Node::load_symbol:
			stack.emplace_back(get_symbol_reference(&cursor->symbols(), *cursor->next().symbol));
			break;
		case Node::load_member:
			reduce_member(cursor, get_object_member(cursor, stack.back(), *cursor->next().symbol));
			break;
		case Node::load_operator:
			reduce_member(cursor, get_object_operator(cursor, stack.back(), static_cast<Class::Operator>(cursor->next().parameter)));
			break;
		case Node::load_constant:
			stack.emplace_back(WeakReference::share(*cursor->next().constant));
			break;
		case Node::load_var_symbol:
			stack.emplace_back(get_symbol_reference(&cursor->symbols(), var_symbol(cursor)));
			break;
		case Node::load_var_member:
		{
			Symbol &&symbol = var_symbol(cursor);
			reduce_member(cursor, get_object_member(cursor, stack.back(), symbol));
		}
			break;
		case Node::store_reference:
		{
			WeakReference reference = move(stack.back());
			stack.back() = WeakReference::clone(reference);
			stack.emplace_back(forward<Reference>(reference));
		}
			break;
		case Node::reload_reference:
			stack.emplace_back(WeakReference::share(stack.back()));
			break;
		case Node::unload_reference:
			stack.pop_back();
			break;
		case Node::load_extra_arguments:
			load_extra_arguments(cursor);
			break;

		case Node::create_fast:
		{
			Symbol &symbol = *cursor->next().symbol;
			const size_t index = static_cast<size_t>(cursor->next().parameter);
			const Reference::Flags flags = static_cast<Reference::Flags>(cursor->next().parameter);
			create_symbol(cursor, symbol, index, flags);
		}
			break;
		case Node::create_symbol:
		{
			Symbol &symbol = *cursor->next().symbol;
			const Reference::Flags flags = static_cast<Reference::Flags>(cursor->next().parameter);
			create_symbol(cursor, symbol, flags);
		}
			break;
		case Node::create_function:
		{
			Symbol &symbol = *cursor->next().symbol;
			const Reference::Flags flags = static_cast<Reference::Flags>(cursor->next().parameter);
			create_function(cursor, symbol, flags);
		}
			break;
		case Node::create_iterator:
			iterator_init_from_stack(cursor, static_cast<size_t>(cursor->next().parameter));
			break;
		case Node::create_array:
			stack.emplace_back(WeakReference::create<Array>());
			stack.back().data<Array>()->construct();
			break;
		case Node::create_hash:
			stack.emplace_back(WeakReference::create<Hash>());
			stack.back().data<Hash>()->construct();
			break;
		case Node::create_lib:
			stack.emplace_back(WeakReference::create<Library>());
			break;
		case Node::function_overload:
			function_overload_from_stack(cursor);
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
			cursor->symbols().getPackage()->registerClass(static_cast<ClassRegister::Id>(cursor->next().parameter));
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
			find_defined_symbol(cursor, *cursor->next().symbol);
			break;
		case Node::find_defined_member:
			find_defined_member(cursor, *cursor->next().symbol);
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
			find_check(cursor, static_cast<size_t>(cursor->next().parameter));
			break;
		case Node::range_init:
			range_init(cursor);
			break;
		case Node::range_next:
			range_next(cursor);
			break;
		case Node::range_check:
			range_check(cursor, static_cast<size_t>(cursor->next().parameter));
			break;
		case Node::range_iterator_finalize:
			range_iterator_finalize(cursor);
			break;
		case Node::range_iterator_check:
			range_iterator_check(cursor, static_cast<size_t>(cursor->next().parameter));
			break;

		case Node::open_printer:
			cursor->openPrinter(create_printer(cursor));
			break;

		case Node::close_printer:
			cursor->closePrinter();
			break;

		case Node::print:
		{
			WeakReference reference = move(stack.back());
			stack.pop_back();
			print(cursor->printer(), reference);
		}
			break;

		case Node::or_pre_check:
			or_pre_check(cursor, static_cast<size_t>(cursor->next().parameter));
			break;
		case Node::and_pre_check:
			and_pre_check(cursor, static_cast<size_t>(cursor->next().parameter));
			break;

		case Node::case_jump:
			if (to_boolean(cursor, stack.back())) {
				cursor->jmp(static_cast<size_t>(cursor->next().parameter));
				stack.pop_back();
			}
			else {
				((void)cursor->next());
			}
			stack.pop_back();
			break;

		case Node::jump_zero:
			if (to_boolean(cursor, stack.back())) {
				((void)cursor->next());
			}
			else {
				cursor->jmp(static_cast<size_t>(cursor->next().parameter));
			}
			stack.pop_back();
			break;

		case Node::jump:
			cursor->jmp(static_cast<size_t>(cursor->next().parameter));
			break;

		case Node::set_retrieve_point:
			cursor->setRetrievePoint(static_cast<size_t>(cursor->next().parameter));
			break;
		case Node::unset_retrieve_point:
			cursor->unsetRetrievePoint();
			break;
		case Node::raise:
		{
			WeakReference exception = move(stack.back());
			stack.pop_back();
			cursor->raise(move(exception));
		}
			break;

		case Node::yield:
			yield(cursor, cursor->generator());
			break;
		case Node::exit_generator:
			load_generator_result(cursor);
			cursor->exitCall();
			break;
		case Node::yield_exit_generator:
			yield(cursor, cursor->generator());
			cursor->exitCall();
			break;
		case Node::finalize_generator:
			if (stack.back().data()->format == Data::fmt_object && stack.back().data<Object>()->metadata->metatype() == Class::iterator) {
				stack.back().data<Iterator>()->ctx.finalize();
			}
			break;

		case Node::capture_symbol:
			capture_symbol(cursor, *cursor->next().symbol);
			break;
		case Node::capture_as:
			capture_as_symbol(cursor, *cursor->next().symbol);
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
		case Node::call_builtin:
			ast->callBuiltinMethode(static_cast<size_t>(cursor->next().parameter), cursor);
			break;
		case Node::init_call:
			init_call(cursor);
			break;
		case Node::init_member_call:
			init_member_call(cursor, *cursor->next().symbol);
			break;
		case Node::init_operator_call:
			init_operator_call(cursor, static_cast<Class::Operator>(cursor->next().parameter));
			break;
		case Node::init_var_member_call:
			init_member_call(cursor, var_symbol(cursor));
			break;
		case Node::init_param:
		{
			Symbol &symbol = *cursor->next().symbol;
			const Reference::Flags flags = static_cast<Reference::Flags>(cursor->next().parameter);
			const size_t index = static_cast<size_t>(cursor->next().parameter);
			init_parameter(cursor, symbol, flags, index);
		}
			break;
		case Node::exit_call:
			cursor->exitCall();
			break;
		case Node::exit_thread:
			return false;
		case Node::exit_exec:
			Scheduler::instance()->exit(static_cast<int>(to_integer(cursor, stack.back())));
			stack.pop_back();
			return false;
		case Node::module_end:
			if (UNLIKELY(!cursor->exitModule())) {
				return false;
			}
		}
	}

	return true;
}

bool mint::debug_steps(Cursor *cursor, DebugInterface *interface) {

	lock_processor();

	do {
		for (size_t i = 0; i < quantum; ++i) {
			if (!interface->debug(cursor)) {
				unlock_processor();
				return false;
			}
			if (!do_run_steps(cursor, 1)) {
				unlock_processor();
				return false;
			}
		}
	}
	while (g_single_thread);

	unlock_processor();
	return true;
}

bool mint::run_steps(Cursor *cursor) {

	lock_processor();

	do {
		if (!do_run_steps(cursor, quantum)) {
			unlock_processor();
			return false;
		}
	}
	while (g_single_thread);

	unlock_processor();
	return true;
}

bool mint::run_step(Cursor *cursor) {

	lock_processor();

	if (!do_run_steps(cursor, 1)) {
		unlock_processor();
		return false;
	}

	unlock_processor();
	return true;
}

void mint::set_multi_thread(bool enabled) {
	g_single_thread = !enabled;
}

void mint::lock_processor() {
	g_step_mutex.lock();
}

void mint::unlock_processor() {
	g_step_mutex.unlock();
}
