#include "scheduler/process.h"
#include "scheduler/processor.h"
#include "scheduler/scheduler.h"
#include "scheduler/exception.h"
#include "memory/builtin/iterator.h"
#include "memory/functiontool.h"
#include "compiler/compiler.h"
#include "debug/debugtool.h"
#include "system/filestream.h"
#include "system/bufferstream.h"
#include "system/inputstream.h"
#include "system/terminal.h"
#include "system/error.h"
#include "ast/output.h"

#include <thread>
#include <chrono>

using namespace std;
using namespace mint;

Process::Process(Cursor *cursor) :
	m_cursor(cursor),
	m_endless(false),
	m_threadId(0),
	m_errorHandler(0) {

}

Process::~Process() {
	lock_processor();
	delete m_cursor;
	unlock_processor();
}

Process *Process::fromFile(AbstractSyntaxTree *ast, const string &file) {

	try {

		Compiler compiler;
		FileStream stream(file);

		if (stream.isValid()) {

			Module::Infos infos = ast->createModule();
			if (compiler.build(&stream, infos)) {
				return new Process(ast->createCursor(infos.id));
			}
		}
	}
	catch (const MintSystemError &) {
		return nullptr;
	}

	return nullptr;
}

Process *Process::fromBuffer(AbstractSyntaxTree *ast, const string &buffer) {

	try {
		Compiler compiler;
		BufferStream stream(buffer);

		if (stream.isValid()) {

			Module::Infos infos = ast->createModule();
			if (compiler.build(&stream, infos)) {
				return new Process(ast->createCursor(infos.id));
			}
		}
	}
	catch (const MintSystemError &) {
		return nullptr;
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
		args = m_cursor->symbols().emplace("va_args", WeakReference(Reference::standard, va_args)).first;
	}

	iterator_insert(args->second.data<Iterator>(), create_string(arg));
}

void Process::setup() {
	if (!m_cursor->parent()) {
		m_errorHandler = add_error_callback(bind(&Process::dump, this));
	}
}

void Process::cleanup() {
	if (m_errorHandler) {
		remove_error_callback(m_errorHandler);
	}
	lock_processor();
	m_cursor->cleanup();
	unlock_processor();
}

bool Process::collectOnExit() const {
	return true;
}

bool Process::exec() {
	try {
		return run_steps(m_cursor);
	}
	catch (MintException &raised) {
		if (m_cursor == raised.cursor()) {
			m_cursor->raise(raised.takeException());
			unlock_processor();
			return true;
		}
		else {
			throw;
		}
	}
	catch (const MintSystemError &) {
		unlock_processor();
		return false;
	}
}

bool Process::debug(DebugInterface *debugInterface) {
	try {
		return debug_steps(m_cursor, debugInterface);
	}
	catch (MintException &raised) {
		if (m_cursor == raised.cursor()) {
			m_cursor->raise(raised.takeException());
			unlock_processor();
			return true;
		}
		else {
			throw;
		}
	}
	catch (const MintSystemError &) {
		unlock_processor();
		return false;
	}
}

bool Process::resume() {

	while (m_endless) {
		try {
			Compiler compiler;
			compiler.setPrinting(true);
			m_cursor->resume();
			InputStream::instance().next();
			return compiler.build(&InputStream::instance(), m_cursor->ast()->main());
		}
		catch (const MintSystemError &) {
			continue;
		}
	}

	return false;
}

void Process::wait() {
	this_thread::yield();
}

void Process::sleep(uint64_t msec) {
	this_thread::sleep_for(chrono::duration<uint64_t, milli>(msec));
}

void Process::setThreadId(int id) {
	m_threadId = id;
}

int Process::getThreadId() const {
	return m_threadId;
}

Cursor *Process::cursor() {
	return m_cursor;
}

void Process::setEndless(bool endless) {
	m_endless = endless;
}

void Process::installErrorHandler() {
	set_exit_callback(bind(&Cursor::retrieve, m_cursor));
}

void Process::dump() {

	term_printf(stderr, "Traceback thread %d : \n", m_threadId);

	for (const LineInfo &call : m_cursor->dump()) {
		string call_str = call.toString();
		string line_str = get_module_line(call.moduleName(), call.lineNumber());
		term_printf(stderr, "  %s\n", call_str.c_str());
		term_printf(stderr, "  %s\n", line_str.c_str());
	}
}
