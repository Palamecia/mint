#include "debugger.h"
#include "debugprinter.h"

#include <memory/memorytool.h>
#include <debug/cursordebugger.h>
#include <debug/debugtool.h>
#include <system/terminal.h>
#include <system/output.h>
#include <sstream>
#include <cstring>

using namespace std;
using namespace mint;

Debugger::Debugger(int argc, char **argv) {

	vector<char *> args;

	if (parseArguments(argc, argv, args)) {
		m_scheduler.reset(new Scheduler(static_cast<int>(args.size()), args.data()));
		m_scheduler->setDebugInterface(this);
	}
}

int Debugger::run() {

	if (m_scheduler) {
		return m_scheduler->run();
	}

	return EXIT_FAILURE;
}

bool Debugger::parseArguments(int argc, char **argv, vector<char *> &args) {

	args.push_back(argv[0]);

	for (int argn = 1; argn < argc; ++argn) {
		if (!parseArgument(argc, argn, argv, args)) {
			return false;
		}
	}

	return true;
}

bool Debugger::parseArgument(int argc, int &argn, char **argv, vector<char *> &args) {

	if (!strcmp(argv[argn], "-b") || !strcmp(argv[argn], "--breackpoint")) {

		if (++argn < argc) {
			string module = argv[argn];
			if (++argn < argc) {
				size_t line = static_cast<size_t>(atol(argv[argn]));
				createBreackpoint(LineInfo(module, line));
				return true;
			}
		}

		return false;
	}

	if (!strcmp(argv[argn], "--version")) {
		printVersion();
		return false;
	}

	if (!strcmp(argv[argn], "--help")) {
		printHelp();
		return false;
	}

	args.push_back(argv[argn]);
	return true;
}

void Debugger::printCommands() {
	printf("c | continue :\n");
	printf("\tExecute until next breackpoint\n");
	printf("n | next :\n");
	printf("\tExecute next line\n");
	printf("e | enter :\n");
	printf("\tEnter function\n");
	printf("r | return :\n");
	printf("\tExit function\n");
	printf("bt | backtrace :\n");
	printf("\tPrint backtrace\n");
	printf("bp | breackpoint :\n");
	printf("\tManage breakcpoints\n");
	printf("p | print :\n");
	printf("\tPrint current line\n");
	printf("l | list :\n");
	printf("\tPrint defined symbols\n");
	printf("s | show :\n");
	printf("\tShow symbol value\n");
	printf("exec :\n");
	printf("\tExecute code\n");
	printf("q | quit :\n");
	printf("\tExit program\n");
}

void Debugger::printVersion() {

}

void Debugger::printHelp() {

}

bool Debugger::check(CursorDebugger *cursor) {

	string prompt = cursor->moduleName() + ":" + to_string(cursor->lineNumber()) + " >>> ";

	bool running = true;
	char *command_line = term_read_line(prompt.c_str());
	term_add_history(command_line);

	istringstream stream(command_line);

	/// \todo use a command parser

	string command;
	stream >> command;

	if (command == "c" || command == "continue") {
		doRun();
	}
	else if (command == "n" || command == "next") {
		doNext();
	}
	else if (command == "e" || command == "enter") {
		doEnter();
	}
	else if (command == "r" || command == "return") {
		doReturn();
	}
	else if (command == "q" || command == "quit") {
		running = false;
	}
	else if (command == "p" || command == "print") {

		int count = 0;
		stream >> count;

		string moduleName = cursor->moduleName();
		size_t lineNumber = cursor->lineNumber();

		if (count < 0) {
			count = abs(count);
			for (size_t i = lineNumber - min(static_cast<size_t>(count), lineNumber - 1); i < lineNumber; ++i) {
				printf("    |%s\n", get_module_line(moduleName, i).c_str());
			}
		}

		printf("   >|%s\n", get_module_line(moduleName, lineNumber).c_str());

		for (size_t i = lineNumber + 1; i < lineNumber + count; ++i) {
			printf("    |%s\n", get_module_line(moduleName, i).c_str());
		}
	}
	else if (command == "bt" || command == "backtrace") {
		for (const LineInfo &line : cursor->cursor()->dump()) {
			print_debug_trace("%s", line.toString().c_str());
		}
	}
	else if (command == "bp" || command == "breakpoint") {
		string action, module, line;
		stream >> action;
		if (action == "add") {
			stream >> module;
			stream >> line;
			createBreackpoint(LineInfo(module, static_cast<size_t>(atol(line.c_str()))));
			print_debug_trace("New breackpoint at %s:%ld", module.c_str(), atol(line.c_str()));
		}
		else if (action == "del" || action == "delete") {
			stream >> module;

			char *error = nullptr;
			size_t i = strtoul(module.c_str(), &error, 10);

			if (error) {
				stream >> line;
				removeBreackpoint(LineInfo(module, static_cast<size_t>(atol(line.c_str()))));
				print_debug_trace("Deleted breackpoint at %s:%s", module.c_str(), line.c_str());
			}
			else {
				LineInfoList breakpoints = listBreakpoints();
				removeBreackpoint(breakpoints[i]);
				print_debug_trace("Deleted breackpoint at %s:%ld", breakpoints[i].moduleName().c_str(), breakpoints[i].lineNumber());
			}
		}
		else if (action == "list") {
			LineInfoList breakpoints = listBreakpoints();
			for (size_t i = 0; i < breakpoints.size(); ++i) {
				print_debug_trace("%ld: %s", i, breakpoints[i].toString().c_str());
			}
		}
		else {
			printCommands();
		}
	}
	else if (command == "l" || command == "list") {
		for (auto &symbol : cursor->cursor()->symbols()) {
			print_debug_trace("%s (%s) : %s",
							  symbol.first.c_str(),
							  type_name(SharedReference::unsafe(&symbol.second)).c_str(),
							  reference_value(SharedReference::unsafe(&symbol.second)).c_str());
		}
	}
	else if (command == "s" || command == "show") {
		string token = stream.str().substr(stream.tellg());
		/// \todo parse token
		auto symbol = cursor->cursor()->symbols().find(token);
		if (symbol != cursor->cursor()->symbols().end()) {
			print_debug_trace("%s (%s) : %s",
							  symbol->first.c_str(),
							  type_name(SharedReference::unsafe(&symbol->second)).c_str(),
							  reference_value(SharedReference::unsafe(&symbol->second)).c_str());
		}
	}
	else if (command == "exec") {

		/// \todo use variable specialized interpreter

		DebugPrinter *printer = new DebugPrinter;
		unique_ptr<Process> process(Process::fromBuffer(stream.str().substr(stream.tellg())));

		doRun();
		process->setup();
		process->cursor()->symbols() = cursor->cursor()->symbols();
		process->cursor()->openPrinter(printer);

		do {
			process->debug(Scheduler::quantum, this);
		}
		while (process->cursor()->callInProgress());

		process->cleanup();
		doPause();
	}
	else {
		printCommands();
	}

	free(command_line);
	return running;
}
