#include "scheduler/process.h"
#include "scheduler/processor.h"
#include "scheduler/scheduler.h"
#include "memory/builtin/iterator.h"
#include "memory/builtin/string.h"
#include "compiler/compiler.h"
#include "system/filestream.h"
#include "system/inputstream.h"
#include "system/output.h"
#include "system/error.h"

using namespace std;

Process::Process() : m_endless(false) {}

Process *Process::create(const string &file) {

	Compiler compiler;
	FileStream stream(file);

	if (stream.isValid()) {

		if (compiler.build(&stream, Module::create())) {
			return new Process;
		}

		exit(EXIT_FAILURE);
	}

	return nullptr;
}

Process *Process::fromStandardInput() {

	if (InputStream::instance().isValid()) {

		Process *process = new Process;
		process->m_endless = true;
		process->m_ast.installErrorHandler();
		process->m_ast.openPrinter(&Output::instance());
		return process;
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

bool Process::exec(size_t maxStep) {

	try {
		for (size_t i = 0; i < maxStep; ++i) {

			if (!run_step(&m_ast)) {
				return false;
			}
		}
	}
	catch (MintSystemError) {
		return false;
	}

	return true;
}

bool Process::resume() {

	while (m_endless) {
		try {
			Compiler compiler;
			InputStream::instance().next();
			return compiler.build(&InputStream::instance(), Module::main());
		}
		catch (MintSystemError) {}
	}

	return false;
}
