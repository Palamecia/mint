#include "debugger.h"
#include "highlighter.h"
#include "debugprinter.h"

#include <compiler/lexer.h>
#include <compiler/token.h>
#include <memory/memorytool.h>
#include <debug/cursordebugger.h>
#include <debug/debugtool.h>
#include <system/bufferstream.h>
#include <system/optional.hpp>
#include <system/terminal.h>
#include <ast/output.h>
#include <algorithm>
#include <sstream>
#include <cstring>

using namespace std;
using namespace mint;

static string get_script(istringstream &stream) {

	size_t pos = static_cast<size_t>(stream.tellg());
	string script = stream.str().substr(pos);

	stream.ignore(numeric_limits<streamsize>::max());
	return script;
}

Debugger::Debugger(int argc, char **argv) {

	vector<char *> args;

	m_commands = {
		{
			{"c", "continue"},
			"Execute until next breackpoint",
			[this] (CursorDebugger *, istringstream &) {
				doRun();
				return true;
			}
		},
		{
			{"n", "next"},
			"Execute next line",
			[this] (CursorDebugger *, istringstream &) {
				doNext();
				return true;
			}
		},
		{
			{"e", "enter"},
			"Enter function",
			[this] (CursorDebugger *, istringstream &) {
				doEnter();
				return true;
			}
		},
		{
			{"r", "return"},
			"Exit function",
			[this] (CursorDebugger *, istringstream &) {
				doReturn();
				return true;
			}
		},
		{
			{"bt", "backtrace"},
			"Print backtrace",
			[] (CursorDebugger *cursor, istringstream &) {
				for (const LineInfo &line : cursor->cursor()->dump()) {
					string line_str = line.toString();
					print_debug_trace("%s", line_str.c_str());
				}
				return true;
			}
		},
		{
			{"bp", "breackpoint"},
			"Manage breakcpoints",
			[this] (CursorDebugger *, istringstream &stream) {
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
						string module_name = breakpoints[i].moduleName();
						print_debug_trace("Deleted breackpoint at %s:%ld", module_name.c_str(), breakpoints[i].lineNumber());
					}
				}
				else if (action == "list") {
					LineInfoList breakpoints = listBreakpoints();
					for (size_t i = 0; i < breakpoints.size(); ++i) {
						string line_str = breakpoints[i].toString();
						print_debug_trace("%ld: %s", i, line_str.c_str());
					}
				}
				else {
					printCommands();
				}
				return true;
			}
		},
		{
			{"p", "print"},
			"Print current line", [] (CursorDebugger *cursor, istringstream &stream) {

				int count = 0;
				stream >> count;

				string module_name = cursor->moduleName();
				size_t line_number = cursor->lineNumber();

				if (count < 0) {
					print_highlighted((line_number <= abs(count)) ? 1 : line_number + count, line_number + abs(count), line_number, get_module_stream(module_name));
				}
				else {
					print_highlighted(line_number, line_number + count, line_number, get_module_stream(module_name));
				}
				return true;
			}
		},
		{
			{"l", "list"},
			"Print defined symbols",
			[] (CursorDebugger *cursor, istringstream &) {
				for (auto &symbol : cursor->cursor()->symbols()) {
					string symbol_str = symbol.first.str();
					string type = type_name(WeakReference::share(symbol.second));
					string value = reference_value(WeakReference::share(symbol.second));
					print_debug_trace("%s (%s) : %s", symbol_str.c_str(), type.c_str(), value.c_str());
				}
				return true;
			}
		},
		{
			{"s", "show"},
			"Show symbol value",
			[] (CursorDebugger *cursor, istringstream &stream) {

				enum State { reading_ident, reading_member, reading_operator };

				BufferStream token_stream(get_script(stream));
				optional<WeakReference> reference = nullopt;
				Lexer token_lexer(&token_stream);
				State state = reading_ident;
				string symbol_name;

				for (string token = token_lexer.nextToken(); !token_lexer.atEnd(); token = token_lexer.nextToken()) {
					switch (token::fromLocalId(token_lexer.tokenType(token))) {
					case token::symbol_token:
						switch (state) {
						case reading_ident:
							reference = get_symbol_reference(&cursor->cursor()->symbols(), Symbol(token));
							state = reading_operator;
							symbol_name += token;
							break;

						case reading_member:
							reference = get_object_member(cursor->cursor(), *reference, Symbol(token));
							state = reading_operator;
							symbol_name += token;
							break;

						default:
							print_debug_trace("Unexpected token `%s`", token.c_str());
							break;
						}
						break;

					case token::dot_token:
						switch (state) {
						case reading_operator:
							state = reading_member;
							symbol_name += token;
							break;

						default:
							print_debug_trace("Unexpected token `%s`", token.c_str());
							break;
						}
						break;

					default:
						print_debug_trace("Unexpected token `%s`", token.c_str());
						break;
					}
				}
				if (reference) {
					string type = type_name(WeakReference::share(*reference));
					string value = reference_value(WeakReference::share(*reference));
					print_debug_trace("%s (%s) : %s", symbol_name.c_str(), type.c_str(), value.c_str());
				}
				return true;
			}
		},
		{
			{"exec"},
			"Execute code",
			[this] (CursorDebugger *cursor, istringstream &stream) {

				/// \todo use variable specialized interpreter

				DebugPrinter *printer = new DebugPrinter;
				unique_ptr<Process> process(Process::fromBuffer(cursor->cursor()->ast(), get_script(stream)));

				doRun();
				process->setup();

				for (SymbolTable::weak_symbol_type &symbol : cursor->cursor()->symbols()) {
					process->cursor()->symbols().insert(symbol);
				}

				process->cursor()->openPrinter(printer);

				do {
					process->debug(this);
				}
				while (process->cursor()->callInProgress());

				process->cleanup();
				doPause();
				return true;
			}
		},
		{
			{"q", "quit"},
			"Exit program",
			[] (CursorDebugger *, istringstream &) {
				return false;
			}
		}
	};

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

	if (m_configuring) {

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

		if (!strcmp(argv[argn], "--")) {
			m_configuring = false;
			return true;
		}
	}

	args.push_back(argv[argn]);
	return true;
}

void Debugger::printCommands() {
	for (const Command &command : m_commands) {
		string names;
		for (auto i = command.names.begin(); i != command.names.end(); ++i) {
			if (i != command.names.begin()) {
				names += " | ";
			}
			names += *i;
		}
		term_printf(stdout, "%s :\n\t%s\n", names.c_str(), command.desc.c_str());
	}
}

void Debugger::printVersion() {

}

void Debugger::printHelp() {

}

bool Debugger::check(CursorDebugger *cursor) {

	string prompt = cursor->moduleName() + ":" + to_string(cursor->lineNumber()) + " >>> ";

	char *command_line = term_read_line(prompt.c_str());
	term_add_history(command_line);

	string command;
	istringstream stream(command_line);

	for (stream >> command; !stream.eof(); stream >> command) {
		if (!runCommand(command, cursor, stream)) {
			free(command_line);
			return false;
		}
	}

	free(command_line);
	return true;
}

bool Debugger::runCommand(const string &command, CursorDebugger *cursor, istringstream &stream) {

	for (const Command &entry : m_commands) {
		for (const string &name : entry.names) {
			if (command == name) {
				return entry.func(cursor, stream);
			}
		}
	}

	printCommands();
	return true;
}
