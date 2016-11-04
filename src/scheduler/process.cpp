#include "scheduler/process.h"
#include "scheduler/processor.h"
#include "scheduler/scheduler.h"
#include "memory/builtin/iterator.h"
#include "memory/builtin/string.h"
#include "compiler/compiler.h"
#include "system/filestream.h"
#include "system/inputstream.h"
#include "system/output.h"

using namespace std;

Process::Process() : m_endless(false) {}

Process *Process::create(const string &file) {

	Compiler compiler;
	FileStream stream(file);

	if (stream.isValid()) {

		if (compiler.build(&stream, Module::create())) {
			return new Process;
		}

		exit(1);
	}

	return nullptr;
}

Process *Process::readInput(Process *process) {

	Compiler compiler;
	Module::Context context;

	if (InputStream::instance().isValid()) {

		if (process == nullptr) {
			context = Module::create();
			process = new Process;
			process->m_endless = true;
			process->m_ast.openPrinter(&Output::instance());
		}
		else {
			context = Module::main();
			InputStream::instance().next();
		}

		if (compiler.build(&InputStream::instance(), context)) {
			return process;
		}
		exit(1);
	}

	return nullptr;
}

void Process::parseArgument(const std::string &arg) {

	auto args = m_ast.symbols().find("va_args");
	if (args == m_ast.symbols().end()) {
		args = m_ast.symbols().insert({"va_args", Reference(Reference::standard, Reference::alloc<Iterator>())}).first;
	}

	Reference *argv = Reference::create<String>();
	((String *)argv->data())->str = arg;
	((Iterator *)args->second.data())->ctx.push_back(SharedReference::unique(argv));
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
