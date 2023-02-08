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

InteractiveDebugger::InteractiveDebugger() {
	m_commands = {
		Command {
			{"c", "continue"},
			"Execute until next breackpoint",
			[] (DebugInterface *debugger, CursorDebugger *cursor, istringstream &) {
				debugger->do_run(cursor);
				return true;
			}
		},
		Command {
			{"n", "next"},
			"Execute next line",
			[] (DebugInterface *debugger, CursorDebugger *cursor, istringstream &) {
				debugger->do_next(cursor);
				return true;
			}
		},
		Command {
			{"e", "enter"},
			"Enter function",
			[] (DebugInterface *debugger, CursorDebugger *cursor, istringstream &) {
				debugger->do_enter(cursor);
				return true;
			}
		},
		Command {
			{"r", "return"},
			"Exit function",
			[] (DebugInterface *debugger, CursorDebugger *cursor, istringstream &) {
				debugger->do_return(cursor);
				return true;
			}
		},
		Command {
			{"bt", "backtrace"},
			"Print backtrace",
			[] (DebugInterface *debugger, CursorDebugger *cursor, istringstream &) {
				for (const LineInfo &line : cursor->cursor()->dump()) {
					string line_str = line.to_string();
					print_debug_trace("%s", line_str.c_str());
				}
				return true;
			}
		},
		Command {
			{"bp", "breackpoint"},
			"Manage breakcpoints",
			[this] (DebugInterface *debugger, CursorDebugger *cursor, istringstream &stream) {
				string action, module, line;
				stream >> action;
				if (action == "add") {
					stream >> module;
					stream >> line;
					Module::Info info = Scheduler::instance()->ast()->module_info(module);
					if (DebugInfo *debug_info = info.debug_info) {
						size_t line_number = debug_info->to_executable_line_number(static_cast<size_t>(stol(line)));
						print_debug_trace("New breackpoint at %s:%ld", module.c_str(), line_number);
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
						LineInfo info(cursor->cursor()->ast(), module, static_cast<size_t>(atol(line.c_str())));
						print_debug_trace("Deleted breackpoint at %s:%ld", module.c_str(), info.line_number());
						debugger->remove_breakpoint(info);
					}
					else {
						LineInfo info = debugger->get_breakpoint(id).info;
						module = info.module_name();
						print_debug_trace("Deleted breackpoint at %s:%ld", module.c_str(), info.line_number());
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
					print_commands();
				}
				return true;
			}
		},
		Command {
			{"p", "print"},
			"Print current line",
			[] (DebugInterface *debugger, CursorDebugger *cursor, istringstream &stream) {

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
		},
		Command {
			{"l", "list"},
			"Print defined symbols",
			[] (DebugInterface *debugger, CursorDebugger *cursor, istringstream &) {
				for (auto &symbol : cursor->cursor()->symbols()) {
					string symbol_str = symbol.first.str();
					string type = type_name(WeakReference::share(symbol.second));
					string value = reference_value(WeakReference::share(symbol.second));
					print_debug_trace("%s (%s) : %s", symbol_str.c_str(), type.c_str(), value.c_str());
				}
				return true;
			}
		},
		Command {
			{"s", "show"},
			"Show symbol value",
			[] (DebugInterface *debugger, CursorDebugger *cursor, istringstream &stream) {

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
		},
		Command {
			{"exec"},
			"Execute code",
			[] (DebugInterface *debugger, CursorDebugger *cursor, istringstream &stream) {

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
		},
		Command {
			{"q", "quit"},
			"Exit program",
			[] (DebugInterface *, CursorDebugger *, istringstream &) {
				return false;
			}
		}
	};
}

InteractiveDebugger::~InteractiveDebugger() {

}

bool InteractiveDebugger::setup(DebugInterface *debugger, Scheduler *scheduler) {
	return true;
}

bool InteractiveDebugger::handle_events(DebugInterface *debugger, CursorDebugger *cursor) {
	return true;
}

bool InteractiveDebugger::check(DebugInterface *debugger, CursorDebugger *cursor) {
	
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
		if (!run_command(command, debugger, cursor, stream)) {
			return false;
		}
	}

	return true;
}

void InteractiveDebugger::cleanup(DebugInterface *debugger, Scheduler *scheduler) {

}

void InteractiveDebugger::on_thread_started(DebugInterface *debugger, CursorDebugger *cursor) {

}

void InteractiveDebugger::on_thread_exited(DebugInterface *debugger, CursorDebugger *cursor) {

}

bool InteractiveDebugger::on_breakpoint(DebugInterface *debugger, CursorDebugger *cursor, const unordered_set<Breakpoint::Id> &breakpoints) {
	return true;
}

bool InteractiveDebugger::on_exception(DebugInterface *debugger, CursorDebugger *cursor) {
	return true;
}

bool InteractiveDebugger::on_step(DebugInterface *debugger, CursorDebugger *cursor) {
	return true;
}

void InteractiveDebugger::print_commands() {
	for (const Command &command : m_commands) {
		string names;
		for (auto i = command.names.begin(); i != command.names.end(); ++i) {
			if (i != command.names.begin()) {
				names += " | ";
			}
			names += *i;
		}
		Terminal::printf(stdout, "%s :\n\t%s\n", names.c_str(), command.desc.c_str());
	}
}

bool InteractiveDebugger::run_command(const string &command, DebugInterface *debugger, CursorDebugger *cursor, istringstream &stream) {

	for (const Command &entry : m_commands) {
		for (const string &name : entry.names) {
			if (command == name) {
				return entry.func(debugger, cursor, stream);
			}
		}
	}

	print_commands();
	return true;
}
