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

		if ( ! compiler.build(&stream, process->m_ast.createModul())) {
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
		{
			Reference ref = m_ast.stack().back();
			m_ast.stack().pop_back();
			m_ast.stack().push_back(get_object_member((Object*)ref.data(), m_ast.next().symbol));
		}
			break;
		case Instruction::load_constant:
			m_ast.stack().push_back(m_ast.next().constant);
			break;
		case Instruction::unload_reference:
			m_ast.stack().pop_back();
			break;

		case Instruction::create_symbol:
			create_symbol(&m_ast, m_ast.next().symbol, m_ast.next().parameter);
			break;
		case Instruction::create_global_symbol:
			create_global_symbol(&m_ast, m_ast.next().symbol, m_ast.next().parameter);
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
			GlobalData::instance().registerClass(m_ast.next().symbol, m_ast.next().parameter);
			break;
		case Instruction::register_symbol:
			GlobalData::instance().registerSymbol(m_ast.next().symbol, m_ast.next().parameter);
			break;

		case Instruction::move:
			move_operator(&m_ast);
			break;
		case Instruction::copy:
			copy_operator(&m_ast);
			break;
		case Instruction::call:
			call_operator(&m_ast);
			break;
		case Instruction::call_member:
			m_ast.stack().push_back(get_object_member((Object*)m_ast.stack().back().get().data(), m_ast.next().symbol));
			call_operator(&m_ast);
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

		case Instruction::exit_call:
			m_ast.exit_call();
			break;
		case Instruction::module_end:
			return false;
		}
	}

	return true;
}
