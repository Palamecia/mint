#include "Scheduler/process.h"
#include "Scheduler/processor.h"
#include "Scheduler/scheduler.h"
#include "Compiler/compiler.h"
#include "System/filestream.h"
#include "System/inputstream.h"
#include "System/output.h"

using namespace std;

Process::Process() : m_endless(false) {}

Process *Process::create(const string &file) {

	Compiler compiler;
	FileStream stream(file);

	if (stream.isValid()) {

		Process *process = new Process;

		if (compiler.build(&stream, Module::create())) {
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
			context = Module::create();
			process->m_endless = true;
			process->m_ast.call(0, 0);
			process->m_ast.openPrinter(&Output::instance());
		}
		else {
			context = Module::main();
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

		if (!run_step(&m_ast)) {
			return false;
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
