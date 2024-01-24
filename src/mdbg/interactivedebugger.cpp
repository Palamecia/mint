/**
 * Copyright (c) 2024 Gauvain CHERY.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "interactivedebugger.h"
#include "debugprinter.h"
#include "highlighter.h"
#include "debugger.h"

#include <mint/system/bufferstream.h>
#include <mint/scheduler/process.h>
#include <mint/compiler/lexer.h>
#include <mint/compiler/token.h>
#include <mint/memory/memorytool.h>
#include <mint/debug/debugtool.h>
#include <mint/ast/cursor.h>
#include <mint/ast/output.h>

#include <optional>
#include <sstream>
#include <cstring>

using namespace mint;
using namespace std;

static string get_script(istringstream &stream) {

	size_t pos = static_cast<size_t>(stream.tellg());
	string script = stream.str().substr(pos);

	stream.ignore(numeric_limits<streamsize>::max());
	return script;
}

vector<InteractiveDebugger::Command> InteractiveDebugger::InteractiveDebugger::g_commands = {
	{ {"c", "continue"}, "Execute until next break point", &InteractiveDebugger::on_continue },
	{ {"n", "next"}, "Execute next line", &InteractiveDebugger::on_next },
	{ {"e", "enter"}, "Enter function", &InteractiveDebugger::on_enter },
	{ {"r", "return"}, "Exit function", &InteractiveDebugger::on_return },
	{ {"bt", "backtrace"}, "Print backtrace", &InteractiveDebugger::on_backtrace },
	{ {"bp", "breakpoint"}, "Manage break points", &InteractiveDebugger::on_breakpoint },
	{ {"p", "print"}, "Print current line", &InteractiveDebugger::on_print },
	{ {"l", "list"}, "Print defined symbols", &InteractiveDebugger::on_list },
	{ {"s", "show"}, "Show symbol value", &InteractiveDebugger::on_show },
	{ {"exec"}, "Execute code", &InteractiveDebugger::on_exec },
	{ {"q", "quit"}, "Exit program", &InteractiveDebugger::on_quit }
};

InteractiveDebugger::InteractiveDebugger() {

}

InteractiveDebugger::~InteractiveDebugger() {

}

bool InteractiveDebugger::setup(Debugger *debugger, Scheduler *scheduler) {
	return true;
}

bool InteractiveDebugger::handle_events(Debugger *debugger, CursorDebugger *cursor) {
	return true;
}

bool InteractiveDebugger::check(Debugger *debugger, CursorDebugger *cursor) {
	
	m_terminal.set_prompt([cursor](size_t row_number) {
		return cursor->module_name() + ":" + to_string(row_number + cursor->line_number()) + " >>> ";
	});

	/* TODO if (size_t heigth = Terminal::get_height()) {
		size_t line_number = cursor->line_number();
		size_t count = heigth / 2 - 1;
		print_highlighted((line_number <= count) ? 1 : line_number - count, line_number + count, line_number, get_module_stream(cursor->module_name()));
	}*/
	
	auto buffer = m_terminal.read_line();
	if (!buffer.has_value()) {
		exit(EXIT_SUCCESS);
	}

	string command;
	istringstream stream(*buffer);

	for (stream >> command; !stream.eof(); stream >> command) {
		if (!call_command(command, debugger, cursor, stream)) {
			return false;
		}
	}

	return true;
}

void InteractiveDebugger::cleanup(Debugger *debugger, Scheduler *scheduler) {

}

void InteractiveDebugger::on_thread_started(Debugger *debugger, CursorDebugger *cursor) {

}

void InteractiveDebugger::on_thread_exited(Debugger *debugger, CursorDebugger *cursor) {

}

void InteractiveDebugger::on_breakpoint_created(Debugger *debugger, const Breakpoint &breakpoint) {
	print_debug_trace("New breackpoint at %s:%ld", breakpoint.info.module_name().c_str(), breakpoint.info.line_number());
}

void InteractiveDebugger::on_breakpoint_deleted(Debugger *debugger, const Breakpoint &breakpoint) {
	print_debug_trace("Deleted breackpoint at %s:%ld", breakpoint.info.module_name().c_str(), breakpoint.info.line_number());
}

bool InteractiveDebugger::on_breakpoint(Debugger *debugger, CursorDebugger *cursor, const unordered_set<Breakpoint::Id> &breakpoints) {
	return true;
}

bool InteractiveDebugger::on_exception(Debugger *debugger, CursorDebugger *cursor) {
	return true;
}

bool InteractiveDebugger::on_pause(Debugger *debugger, CursorDebugger *cursor) {
	return true;
}

bool InteractiveDebugger::on_step(Debugger *debugger, CursorDebugger *cursor) {
	return true;
}

bool InteractiveDebugger::on_continue(Debugger *debugger, CursorDebugger *cursor, istringstream &stream) {
	debugger->do_run(cursor);
	return true;
}

bool InteractiveDebugger::on_next(Debugger *debugger, CursorDebugger *cursor, istringstream &stream) {
	debugger->do_next(cursor);
	return true;
}

bool InteractiveDebugger::on_enter(Debugger *debugger, CursorDebugger *cursor, istringstream &stream) {
	debugger->do_enter(cursor);
	return true;
}

bool InteractiveDebugger::on_return(Debugger *debugger, CursorDebugger *cursor, istringstream &stream) {
	debugger->do_return(cursor);
	return true;
}

bool InteractiveDebugger::on_backtrace(Debugger *debugger, CursorDebugger *cursor, istringstream &stream) {
	for (const LineInfo &line : cursor->cursor()->dump()) {
		string line_str = line.to_string();
		print_debug_trace("%s", line_str.c_str());
	}
	return true;
}

bool InteractiveDebugger::on_breakpoint(Debugger *debugger, CursorDebugger *cursor, istringstream &stream) {
	string action, module, line;
	stream >> action;
	if (action == "add") {
		stream >> module;
		stream >> line;
		Module::Info info = Scheduler::instance()->ast()->module_info(module);
		if (DebugInfo *debug_info = info.debug_info; debug_info && info.state != Module::not_compiled) {
			size_t line_number = debug_info->to_executable_line_number(static_cast<size_t>(stol(line)));
			debugger->create_breakpoint({info.id, module, line_number});
		}
		else {
			print_debug_trace("Can not create breakpoint : unknown module %s", module.c_str());
		}
	}
	else if (action == "del" || action == "delete") {
		stream >> module;

		char *error = nullptr;
		Breakpoint::Id id = strtoul(module.c_str(), &error, 10);

		if (error) {
			stream >> line;
			size_t line_number = static_cast<size_t>(atol(line.c_str()));
			debugger->remove_breakpoint({cursor->cursor()->ast(), module, line_number});
		}
		else {
			debugger->remove_breakpoint(id);
		}
	}
	else if (action == "list") {
		const BreakpointList breakpoints = debugger->get_breakpoints();
		for (const Breakpoint &breakpoint : breakpoints) {
			string line_str = breakpoint.info.to_string();
			print_debug_trace("%ld: %s", breakpoint.id, line_str.c_str());
		}
	}
	else {
		Terminal::print(stdout, MINT_TERM_BOLD "add" MINT_TERM_RESET MINT_TERM_ITALIC " <module> <line>" MINT_TERM_RESET ":\n\tCreates a new break point on the given " MINT_TERM_ITALIC "module" MINT_TERM_RESET " at the given " MINT_TERM_ITALIC "line" MINT_TERM_RESET " number\n");
		Terminal::print(stdout, MINT_TERM_BOLD "del | delete" MINT_TERM_RESET MINT_TERM_ITALIC " <id> | <module> <line>" MINT_TERM_RESET ":\n\tDeletes the break point with the given " MINT_TERM_ITALIC "id" MINT_TERM_RESET " or on the given " MINT_TERM_ITALIC "module" MINT_TERM_RESET " at the given " MINT_TERM_ITALIC "line" MINT_TERM_RESET " number\n");
		Terminal::print(stdout, MINT_TERM_BOLD "list" MINT_TERM_RESET ":\n\tLists configured break points\n");
	}
	return true;
}

bool InteractiveDebugger::on_print(Debugger *debugger, CursorDebugger *cursor, istringstream &stream) {

	int count = 0;
	stream >> count;

	string module_name = cursor->module_name();
	size_t line_number = cursor->line_number();

	if (count < 0) {
		print_highlighted((line_number <= abs(count)) ? 1 : line_number + count, line_number + abs(count), line_number, get_module_stream(module_name));
	}
	else {
		print_highlighted(line_number, line_number + count, line_number, get_module_stream(module_name));
	}

	return true;
}

bool InteractiveDebugger::on_list(Debugger *debugger, CursorDebugger *cursor, istringstream &stream) {
	for (auto &symbol : cursor->cursor()->symbols()) {
		string symbol_str = symbol.first.str();
		string type = type_name(WeakReference::share(symbol.second));
		string value = reference_value(WeakReference::share(symbol.second));
		print_debug_trace("%s (%s) : %s", symbol_str.c_str(), type.c_str(), value.c_str());
	}
	return true;
}

bool InteractiveDebugger::on_show(Debugger *debugger, CursorDebugger *cursor, istringstream &stream) {

	enum State { reading_ident, reading_member, reading_operator };

	BufferStream token_stream(get_script(stream));
	optional<WeakReference> reference = nullopt;
	Lexer token_lexer(&token_stream);
	State state = reading_ident;
	string symbol_name;

	for (string token = token_lexer.next_token(); !token_lexer.at_end(); token = token_lexer.next_token()) {
		switch (token::from_local_id(token_lexer.token_type(token))) {
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

bool InteractiveDebugger::on_exec(Debugger *debugger, CursorDebugger *cursor, istringstream &stream) {

	/// \todo use variable specialized interpreter

	DebugPrinter *printer = new DebugPrinter;
	unique_ptr<Process> process(Process::from_buffer(cursor->cursor()->ast(), get_script(stream)));
	CursorDebugger *process_cursor = debugger->declare_thread(process.get());

	debugger->do_run(process_cursor);
	process->setup();

	for (SymbolTable::weak_symbol_type &symbol : cursor->cursor()->symbols()) {
		process->cursor()->symbols().insert(symbol);
	}

	process->cursor()->open_printer(printer);

	do {
		process->debug(debugger);
	}
	while (process->cursor()->call_in_progress());

	process->cleanup();
	debugger->do_pause(process_cursor);
	debugger->remove_thread(process.get());
	return true;
}

bool InteractiveDebugger::on_quit(Debugger *debugger, CursorDebugger *cursor, istringstream &stream) {
	return false;
}

void InteractiveDebugger::print_commands() {
	for (const Command &command : g_commands) {
		string names;
		for (auto i = command.names.begin(); i != command.names.end(); ++i) {
			if (i != command.names.begin()) {
				names += " | ";
			}
			names += *i;
		}
		Terminal::printf(stdout, MINT_TERM_BOLD "%s" MINT_TERM_RESET ":\n\t%s\n", names.c_str(), command.desc.c_str());
	}
}

bool InteractiveDebugger::call_command(const string &command, Debugger *debugger, CursorDebugger *cursor, istringstream &stream) {
	auto it = std::find_if(g_commands.begin(), g_commands.end(), [&command](const Command &entry) {
		return std::any_of(entry.names.begin(), entry.names.end(), [&command](const string &name) { return command == name; });
	});
	if (it != g_commands.end()) {
		return std::invoke(it->func, this, debugger, cursor, stream);
	}
	print_commands();
	return true;
}
