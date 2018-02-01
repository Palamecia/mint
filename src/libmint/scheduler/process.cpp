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
using namespace mint;

Process::Process(Cursor *cursor) :
	m_cursor(cursor),
	m_endless(false),
	m_threadId(0) {
	m_errorHandler = add_error_callback(bind(&Process::dump, this));
}

Process::~Process() {
	remove_error_callback(m_errorHandler);
	delete m_cursor;
}

Process *Process::fromFile(AbstractSyntaxTree *ast, const string &file) {

	Compiler compiler;
	FileStream stream(file);

	if (stream.isValid()) {

		Module::Infos infos = ast->createModule();
		if (compiler.build(&stream, infos)) {
			return new Process(ast->createCursor(infos.id));
		}

		exit(EXIT_FAILURE);
	}

	return nullptr;
}

Process *Process::fromBuffer(AbstractSyntaxTree *ast, const string &buffer) {

	Compiler compiler;
	BufferStream stream(buffer);

	if (stream.isValid()) {

		Module::Infos infos = ast->createModule();
		if (compiler.build(&stream, infos)) {
			return new Process(ast->createCursor(infos.id));
		}

		exit(EXIT_FAILURE);
	}

	return nullptr;
}

Process *Process::fromStandardInput(AbstractSyntaxTree *ast) {

	if (InputStream::instance().isValid()) {

		Module::Infos infos = ast->createModule();
		Process *process = new Process(ast->createCursor(infos.id));
		process->setEndless(true);
		process->installErrorHandler();
		process->cursor()->openPrinter(&Output::instance());
		return process;
	}

	return nullptr;
}

void Process::parseArgument(const string &arg) {

	auto args = m_cursor->symbols().find("va_args");
	if (args == m_cursor->symbols().end()) {

		Iterator *va_args = Reference::alloc<Iterator>();
		va_args->construct();
		args = m_cursor->symbols().emplace("va_args", Reference(Reference::standard, va_args)).first;
	}

	Reference *argv = Reference::create<String>();
	argv->data<Object>()->construct();
	argv->data<String>()->str = arg;
	args->second.data<Iterator>()->ctx.push_back(SharedReference::unique(argv));
}

bool Process::exec(size_t maxStep) {

	for (size_t i = 0; i < maxStep; ++i) {

		if (!run_step(m_cursor)) {
			return false;
		}
	}

	return true;
}

bool Process::resume() {

	while (m_endless) {
		try {
			Compiler compiler;
			m_cursor->resume();
			InputStream::instance().next();
			return compiler.build(&InputStream::instance(), Scheduler::instance()->ast()->main());
		}
		catch (MintSystemError) {
			/// \todo handle this case
		}
	}

	return false;
}

void Process::setThreadId(int id) {
	m_threadId = id;
}

Cursor *Process::cursor() {
	return m_cursor;
}

void Process::setEndless(bool endless) {
	m_endless = endless;
}

void Process::installErrorHandler() {
	set_exit_callback(bind(&Cursor::retrive, m_cursor));
}

void Process::dump() {

	fprintf(stderr, "Traceback thread %d : \n", m_threadId);

	m_cursor->dump();
}
