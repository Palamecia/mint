#include "Scheduler/processor.h"
#include "Scheduler/scheduler.h"
#include "AbstractSyntaxTree/abstractsyntaxtree.h"
#include "Memory/memorytool.h"
#include "Memory/operatortool.h"
#include "Memory/globaldata.h"

bool run_step(AbstractSynatxTree *ast) {

	switch (ast->next().command) {
	case Instruction::load_module:
		ast->loadModule(ast->next().symbol);
		break;

	case Instruction::load_symbol:
		ast->stack().push_back(get_symbol_reference(&ast->symbols(), ast->next().symbol));
		break;
	case Instruction::load_member:
		ast->stack().push_back(get_object_member(ast, ast->next().symbol));
		break;
	case Instruction::load_constant:
		ast->stack().push_back(ast->next().constant);
		break;
	case Instruction::load_var_symbol:
		ast->stack().push_back(get_symbol_reference(&ast->symbols(), var_symbol(ast)));
		break;
	case Instruction::load_var_member:
		ast->stack().push_back(get_object_member(ast, var_symbol(ast)));
		break;
	case Instruction::unload_reference:
		ast->stack().pop_back();
		break;
	case Instruction::reduce_member:
		reduce_member(ast);
		break;

	case Instruction::create_symbol:
	{
		const char *symbol = ast->next().symbol;
		const int flags = ast->next().parameter;
		create_symbol(ast, symbol, flags);
	}
		break;
	case Instruction::create_array:
		ast->stack().push_back(SharedReference::unique(Reference::create<Array>()));
		((Object *)ast->stack().back().get().data())->construct();
		break;
	case Instruction::create_hash:
		ast->stack().push_back(SharedReference::unique(Reference::create<Hash>()));
		((Object *)ast->stack().back().get().data())->construct();
		break;
	case Instruction::array_insert:
		array_insert(ast);
		break;
	case Instruction::hash_insert:
		hash_insert(ast);
		break;

	case Instruction::register_class:
		GlobalData::instance().registerClass(ast->next().parameter);
		break;

	case Instruction::move_op:
		move_operator(ast);
		break;
	case Instruction::copy_op:
		copy_operator(ast);
		break;
	case Instruction::add_op:
		add_operator(ast);
		break;
	case Instruction::sub_op:
		sub_operator(ast);
		break;
	case Instruction::mod_op:
		mod_operator(ast);
		break;
	case Instruction::mul_op:
		mul_operator(ast);
		break;
	case Instruction::div_op:
		div_operator(ast);
		break;
	case Instruction::pow_op:
		pow_operator(ast);
		break;
	case Instruction::is_op:
		is_operator(ast);
		break;
	case Instruction::eq_op:
		eq_operator(ast);
		break;
	case Instruction::ne_op:
		ne_operator(ast);
		break;
	case Instruction::lt_op:
		lt_operator(ast);
		break;
	case Instruction::gt_op:
		gt_operator(ast);
		break;
	case Instruction::le_op:
		le_operator(ast);
		break;
	case Instruction::ge_op:
		ge_operator(ast);
		break;
	case Instruction::inc_op:
		inc_operator(ast);
		break;
	case Instruction::dec_op:
		dec_operator(ast);
		break;
	case Instruction::not_op:
		not_operator(ast);
		break;
	case Instruction::and_op:
		and_operator(ast);
		break;
	case Instruction::or_op:
		or_operator(ast);
		break;
	case Instruction::xor_op:
		xor_operator(ast);
		break;
	case Instruction::compl_op:
		compl_operator(ast);
		break;
	case Instruction::pos_op:
		pos_operator(ast);
		break;
	case Instruction::neg_op:
		neg_operator(ast);
		break;
	case Instruction::shift_left_op:
		shift_left_operator(ast);
		break;
	case Instruction::shift_right_op:
		shift_right_operator(ast);
		break;
	case Instruction::inclusive_range_op:
		inclusive_range_operator(ast);
		break;
	case Instruction::exclusive_range_op:
		exclusive_range_operator(ast);
		break;
	case Instruction::subscript_op:
		subscript_operator(ast);
		break;
	case Instruction::typeof_op:
		typeof_operator(ast);
		break;
	case Instruction::membersof_op:
		membersof_operator(ast);
		break;

	case Instruction::find_defined_symbol:
		find_defined_symbol(ast, ast->next().symbol);
		break;
	case Instruction::find_defined_member:
		find_defined_member(ast, ast->next().symbol);
		break;
	case Instruction::check_defined:
		check_defined(ast);
		break;

	case Instruction::in_find:
		in_find(ast);
		break;
	case Instruction::in_init:
		in_init(ast);
		break;
	case Instruction::in_next:
		in_next(ast);
		break;
	case Instruction::in_check:
		in_check(ast);
		break;

	case Instruction::open_printer:
		ast->openPrinter(toPrinter(ast->stack().back()));
		ast->stack().pop_back();
		break;

	case Instruction::close_printer:
		ast->closePrinter();
		break;

	case Instruction::print:
		print(ast->printer(), ast->stack().back());
		ast->stack().pop_back();
		break;

	case Instruction::jump_zero:
		if (is_not_zero(ast->stack().back())) {
			ast->next();
		}
		else {
			ast->jmp(ast->next().parameter);
		}
		ast->stack().pop_back();
		break;

	case Instruction::jump:
		ast->jmp(ast->next().parameter);
		break;

	case Instruction::set_retrive_point:
		ast->setRetrivePoint(ast->next().parameter);
		break;
	case Instruction::unset_retrive_point:
		ast->unsetRetivePoint();
		break;
	case Instruction::raise:
		ast->raise(ast->stack().back());
		break;

	case Instruction::call:
		call_operator(ast, ast->next().parameter);
		break;
	case Instruction::call_member:
		call_member_operator(ast, ast->next().parameter);
		break;
	case Instruction::init_call:
		init_call(ast);
		break;
	case Instruction::init_param:
		init_parameter(ast, ast->next().symbol);
		break;
	case Instruction::exit_call:
		exit_call(ast);
		break;
	case Instruction::exit_exec:
		Scheduler::instance()->exit(to_number(ast, ast->stack().back()));
		ast->stack().pop_back();
		return false;
	case Instruction::module_end:
		return ast->exitModule();
	}

	return true;
}
