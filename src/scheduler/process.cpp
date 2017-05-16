#include "scheduler/process.h"
#include "scheduler/processor.h"
#include "scheduler/scheduler.h"
#include "memory/builtin/iterator.h"
#include "memory/builtin/string.h"
#include "compiler/compiler.h"
#include "system/filestream.h"
#include "system/bufferstream.h"
#include "system/inputstream.h"
#include "system/output.h"
#include "system/error.h"

using namespace std;

Process::Process(size_t moduleId) : m_ast(moduleId), m_endless(false) {}

Process *Process::fromFile(const string &file) {

	Compiler compiler;
	FileStream stream(file);

	if (stream.isValid()) {

		Module::Context context = Module::create();
		if (compiler.build(&stream, context)) {
			return new Process(context.moduleId);
		}

		exit(EXIT_FAILURE);
	}

	return nullptr;
}

Process *Process::fromBuffer(const std::string &buffer) {

	Compiler compiler;
	BufferStream stream(buffer);

	if (stream.isValid()) {

		Module::Context context = Module::create();
		if (compiler.build(&stream, context)) {
			return new Process(context.moduleId);
		}

		exit(EXIT_FAILURE);
	}

	return nullptr;
}

Process *Process::fromStandardInput() {

	if (InputStream::instance().isValid()) {

		Process *process = new Process(Module::main().moduleId);
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

		Iterator *va_args = Reference::alloc<Iterator>();
		va_args->construct();
		args = m_ast.symbols().emplace("va_args", Reference(Reference::standard, va_args)).first;
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
