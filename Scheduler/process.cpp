#include "Scheduler/process.h"
#include "Scheduler/scheduler.h"
#include "System/filestream.h"
#include "System/inputstream.h"
#include "System/output.h"
#include "Compiler/compiler.h"
#include "Memory/globaldata.h"
#include "Memory/memorytool.h"
#include "Memory/operatortool.h"

using namespace std;

Process::Process() : m_endless(false) {

}

Process *Process::create(const string &file) {

	Compiler compiler;
	FileStream stream(file);

	if (stream.isValid()) {

		Process *process = new Process;

		if (compiler.build(&stream, process->m_ast.createModule())) {
			process->m_ast.call(0, 0);
			return process;
		}

		delete process;
	}

	exit(1);
	return nullptr;
}

Process *Process::readInput(Process *process) {

	Compiler compiler;
	Module::Context context;

	if (InputStream::instance().isValid()) {

		if (process == nullptr) {
			process = new Process;
			context = process->m_ast.createModule();
			process->m_endless = true;
			process->m_ast.call(0, 0);
			process->m_ast.openPrinter(&Output::instance());
		}
		else {
			context = process->m_ast.continueModule();
			InputStream::instance().next();
		}

		if (compiler.build(&InputStream::instance(), context)) {
			return process;
		}
	}

	exit(1);
	return nullptr;
}

bool Process::exec(uint nbStep) {

	for (uint i = 0; i < nbStep; ++i) {
		switch (m_ast.next().command) {
		case Instruction::load_module:
			m_ast.loadModule(m_ast.next().symbol);
			break;

		case Instruction::load_symbol:
			m_ast.stack().push_back(get_symbol_reference(&m_ast.symbols(), m_ast.next().symbol));
			break;
		case Instruction::load_member:
			m_ast.stack().push_back(get_object_member(&m_ast, m_ast.next().symbol));
			break;
		case Instruction::load_constant:
			m_ast.stack().push_back(m_ast.next().constant);
			break;
		case Instruction::load_var_symbol:
			m_ast.stack().push_back(get_symbol_reference(&m_ast.symbols(), var_symbol(&m_ast)));
			break;
		case Instruction::load_var_member:
			m_ast.stack().push_back(get_object_member(&m_ast, var_symbol(&m_ast)));
			break;
		case Instruction::unload_reference:
			m_ast.stack().pop_back();
			break;
		case Instruction::reduce_member:
			reduce_member(&m_ast);
			break;

		case Instruction::create_symbol:
		{
			const char *symbol = m_ast.next().symbol;
			const int flags = m_ast.next().parameter;
			create_symbol(&m_ast, symbol, flags);
		}
			break;
		case Instruction::create_global_symbol:
		{
			const char *symbol = m_ast.next().symbol;
			const int flags = m_ast.next().parameter;
			create_global_symbol(&m_ast, symbol, flags);
		}
			break;
		case Instruction::create_array:
			m_ast.stack().push_back(SharedReference::unique(Reference::create<Array>()));
			((Object *)m_ast.stack().back().get().data())->construct();
			break;
		case Instruction::create_hash:
			m_ast.stack().push_back(SharedReference::unique(Reference::create<Hash>()));
			((Object *)m_ast.stack().back().get().data())->construct();
			break;
		case Instruction::array_insert:
			array_insert(&m_ast);
			break;
		case Instruction::hash_insert:
			hash_insert(&m_ast);
			break;

		case Instruction::register_class:
			GlobalData::instance().registerClass(m_ast.next().parameter);
			break;

		case Instruction::move_op:
			move_operator(&m_ast);
			break;
		case Instruction::copy_op:
			copy_operator(&m_ast);
			break;
		case Instruction::add_op:
			add_operator(&m_ast);
			break;
		case Instruction::sub_op:
			sub_operator(&m_ast);
			break;
		case Instruction::mod_op:
			mod_operator(&m_ast);
			break;
		case Instruction::mul_op:
			mul_operator(&m_ast);
			break;
		case Instruction::div_op:
			div_operator(&m_ast);
			break;
		case Instruction::pow_op:
			pow_operator(&m_ast);
			break;
		case Instruction::is_op:
			is_operator(&m_ast);
			break;
		case Instruction::eq_op:
			eq_operator(&m_ast);
			break;
		case Instruction::ne_op:
			ne_operator(&m_ast);
			break;
		case Instruction::lt_op:
			lt_operator(&m_ast);
			break;
		case Instruction::gt_op:
			gt_operator(&m_ast);
			break;
		case Instruction::le_op:
			le_operator(&m_ast);
			break;
		case Instruction::ge_op:
			ge_operator(&m_ast);
			break;
		case Instruction::inc_op:
			inc_operator(&m_ast);
			break;
		case Instruction::dec_op:
			dec_operator(&m_ast);
			break;
		case Instruction::not_op:
			not_operator(&m_ast);
			break;
		case Instruction::and_op:
			and_operator(&m_ast);
			break;
		case Instruction::or_op:
			or_operator(&m_ast);
			break;
		case Instruction::xor_op:
			xor_operator(&m_ast);
			break;
		case Instruction::compl_op:
			compl_operator(&m_ast);
			break;
		case Instruction::shift_left_op:
			shift_left_operator(&m_ast);
			break;
		case Instruction::shift_right_op:
			shift_right_operator(&m_ast);
			break;
		case Instruction::subscript_op:
			subscript_operator(&m_ast);
			break;
		case Instruction::typeof_op:
			typeof_operator(&m_ast);
			break;
		case Instruction::membersof_op:
			membersof_operator(&m_ast);
			break;
		case Instruction::defined:
			/// \todo
			break;

		case Instruction::in_find:
			in_find(&m_ast);
			break;
		case Instruction::in_init:
			in_init(&m_ast);
			break;
		case Instruction::in_next:
			in_next(&m_ast);
			break;
		case Instruction::in_check:
			in_check(&m_ast);
			break;

		case Instruction::open_printer:
			m_ast.openPrinter(toPrinter(m_ast.stack().back()));
			m_ast.stack().pop_back();
			break;

		case Instruction::close_printer:
			m_ast.closePrinter();
			break;

		case Instruction::print:
			print(m_ast.printer(), m_ast.stack().back());
			m_ast.stack().pop_back();
			break;

		case Instruction::jump_zero:
			if (is_not_zero(m_ast.stack().back())) {
				m_ast.next();
			}
			else {
				m_ast.jmp(m_ast.next().parameter);
			}
			m_ast.stack().pop_back();
			break;

		case Instruction::jump:
			m_ast.jmp(m_ast.next().parameter);
			break;

		case Instruction::set_retrive_point:
			m_ast.setRetrivePoint(m_ast.next().parameter);
			break;
		case Instruction::unset_retrive_point:
			m_ast.unsetRetivePoint();
			break;
		case Instruction::raise:
			m_ast.raise(m_ast.stack().back());
			break;

		case Instruction::call:
			call_operator(&m_ast, m_ast.next().parameter);
			break;
		case Instruction::call_member:
			call_member_operator(&m_ast, m_ast.next().parameter);
			break;
		case Instruction::init_call:
			init_call(&m_ast);
			break;
		case Instruction::init_param:
			init_parameter(&m_ast, m_ast.next().symbol);
			break;
		case Instruction::exit_call:
			if (!m_ast.stack().back().isUnique()) {
				Reference &lvalue = m_ast.stack().back().get();
				Reference *rvalue = new Reference(lvalue);
				m_ast.stack().pop_back();
				m_ast.stack().push_back(SharedReference::unique(rvalue));
			}
			m_ast.exit_call();
			break;
		case Instruction::exit_exec:
			Scheduler::instance()->exit(to_number(&m_ast, m_ast.stack().back()));
			m_ast.stack().pop_back();
		case Instruction::module_end:
			return m_ast.exitModule();
		}
	}

	return true;
}

bool Process::isOver() {
	if (m_endless) {
		Process::readInput(this);
		return false;
	}
	return true;
}
