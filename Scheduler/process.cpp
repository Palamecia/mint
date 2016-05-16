#include "Scheduler/process.h"
#include "System/filestream.h"
#include "Compiler/compiler.h"
#include "Memory/globaldata.h"
#include "Memory/memorytool.h"
#include "Memory/operatortool.h"

using namespace std;

Process::Process() {

}

Process *Process::create(const string &file) {

	Compiler compiler;
	FileStream stream(file);

	if (stream.isValid()) {

		Process *process = new Process;

		if (compiler.build(&stream, process->m_ast.createModul())) {
			process->m_ast.call(0, 0);
			return process;
		}

		delete process;
	}

	/// \todo error
	exit(1);
	return nullptr;
}

bool Process::exec(uint nbStep) {

	for (uint i = 0; i < nbStep; ++i) {
		switch (m_ast.next().command) {
		case Instruction::load_modul:
			m_ast.loadModul(m_ast.next().symbol);
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
			break;
		case Instruction::create_hash:
			m_ast.stack().push_back(SharedReference::unique(Reference::create<Hash>()));
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

		case Instruction::move:
			move_operator(&m_ast);
			break;
		case Instruction::copy:
			copy_operator(&m_ast);
			break;
		case Instruction::add:
			add_operator(&m_ast);
			break;
		case Instruction::sub:
			sub_operator(&m_ast);
			break;
		case Instruction::mod:
			mod_operator(&m_ast);
			break;
		case Instruction::mul:
			mul_operator(&m_ast);
			break;
		case Instruction::div:
			div_operator(&m_ast);
			break;
		case Instruction::pow:
			pow_operator(&m_ast);
			break;
		case Instruction::is:
			is_operator(&m_ast);
			break;
		case Instruction::eq:
			eq_operator(&m_ast);
			break;
		case Instruction::ne:
			ne_operator(&m_ast);
			break;
		case Instruction::lt:
			lt_operator(&m_ast);
			break;
		case Instruction::gt:
			gt_operator(&m_ast);
			break;
		case Instruction::le:
			le_operator(&m_ast);
			break;
		case Instruction::ge:
			ge_operator(&m_ast);
			break;
		case Instruction::inc:
			inc_operator(&m_ast);
			break;
		case Instruction::dec:
			dec_operator(&m_ast);
			break;
		case Instruction::op_not:
			not_operator(&m_ast);
			break;
		case Instruction::inv:
			inv_operator(&m_ast);
			break;
		case Instruction::shift_left:
			shift_left_operator(&m_ast);
			break;
		case Instruction::shift_right:
			shift_right_operator(&m_ast);
			break;
		case Instruction::subscript:
			subscript_operator(&m_ast);
			break;
		case Instruction::membersof:
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
		case Instruction::module_end:
			return false;
		}
	}

	return true;
}
