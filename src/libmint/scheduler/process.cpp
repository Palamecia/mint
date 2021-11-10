#include "scheduler/process.h"
#include "scheduler/processor.h"
#include "scheduler/scheduler.h"
#include "memory/builtin/iterator.h"
#include "memory/functiontool.h"
#include "compiler/compiler.h"
#include "debug/debugtool.h"
#include "system/filestream.h"
#include "system/bufferstream.h"
#include "system/inputstream.h"
#include "system/output.h"
#include "system/error.h"

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

Process *Process::fromFile(const string &file) {

	try {

		Compiler compiler;
		FileStream stream(file);

		if (stream.isValid()) {

			Module::Infos infos = AbstractSyntaxTree::instance().createModule();
			if (compiler.build(&stream, infos)) {
				return new Process(AbstractSyntaxTree::instance().createCursor(infos.id));
			}
		}
	}
	catch (const MintSystemError &) {
		return nullptr;
	}

	return nullptr;
}

Process *Process::fromBuffer(const string &buffer) {

	try {
		Compiler compiler;
		BufferStream stream(buffer);

		if (stream.isValid()) {

			Module::Infos infos = AbstractSyntaxTree::instance().createModule();
			if (compiler.build(&stream, infos)) {
				return new Process(AbstractSyntaxTree::instance().createCursor(infos.id));
			}
		}
	}
	catch (const MintSystemError &) {
		return nullptr;
	}

	return nullptr;
}

Process *Process::fromStandardInput() {

	if (InputStream::instance().isValid()) {

		Module::Infos infos = AbstractSyntaxTree::instance().createModule();
		Process *process = new Process(AbstractSyntaxTree::instance().createCursor(infos.id));
		process->setEndless(true);
		process->installErrorHandler();
		process->cursor()->openPrinter(&Output::instance());
		return process;
	}

	return nullptr;
}

void Process::cleanupAll() {
	AbstractSyntaxTree::instance().cleanup();
}

void Process::parseArgument(const string &arg) {

	auto args = m_cursor->symbols().find("va_args");
	if (args == m_cursor->symbols().end()) {

		Iterator *va_args = Reference::alloc<Iterator>();
		va_args->construct();
		args = m_cursor->symbols().emplace("va_args", StrongReference(Reference::standard, va_args)).first;
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
}

bool Process::exec() {

	try {
		return run_steps(m_cursor);
	}
	catch (const MintSystemError &) {
		unlock_processor();
		return false;
	}

	return true;
}

bool Process::debug(DebugInterface *interface) {

	try {
		return debug_steps(m_cursor, interface);
	}
	catch (const MintSystemError &) {
		unlock_processor();
		return false;
	}

	return true;
}

bool Process::resume() {

	while (m_endless) {
		try {
			Compiler compiler;
			compiler.setPrinting(true);
			m_cursor->resume();
			InputStream::instance().next();
			return compiler.build(&InputStream::instance(), AbstractSyntaxTree::instance().main());
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

	fprintf(stderr, "Traceback thread %d : \n", m_threadId);

	for (const LineInfo &call : m_cursor->dump()) {
		fprintf(stderr, "  %s\n", call.toString().c_str());
		fprintf(stderr, "  %s\n", get_module_line(call.moduleName(), call.lineNumber()).c_str());
	}
}
