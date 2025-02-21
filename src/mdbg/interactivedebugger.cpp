/**
 * Copyright (c) 2025 Gauvain CHERY.
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
#include "expressionevaluator.h"
#include "symbolevaluator.h"
#include "debugprinter.h"
#include "highlighter.h"
#include "debugger.h"

#include <mint/system/mintsystemerror.hpp>
#include <mint/system/bufferstream.h>
#include <mint/scheduler/process.h>
#include <mint/scheduler/output.h>
#include <mint/memory/memorytool.h>
#include <mint/debug/debugtool.h>
#include <mint/ast/cursor.h>

#include <optional>
#include <sstream>
#include <cstring>

using namespace mint;

namespace {

std::string get_script(std::istringstream &stream) {

	size_t pos = static_cast<size_t>(stream.tellg());
	std::string script = stream.str().substr(pos);

	stream.ignore(std::numeric_limits<std::streamsize>::max());
	return script;
}

}

std::vector<InteractiveDebugger::Command> InteractiveDebugger::InteractiveDebugger::g_commands = {
	{{"c", "continue"}, "Execute until next break point", &InteractiveDebugger::on_continue},
	{{"n", "next"}, "Execute next line", &InteractiveDebugger::on_next},
	{{"e", "enter"}, "Enter function", &InteractiveDebugger::on_enter},
	{{"r", "return"}, "Exit function", &InteractiveDebugger::on_return},
	{{"th", "thread"}, "Manage threads", &InteractiveDebugger::on_thread},
	{{"bt", "backtrace"}, "Print backtrace", &InteractiveDebugger::on_backtrace},
	{{"bp", "breakpoint"}, "Manage break points", &InteractiveDebugger::on_breakpoint},
	{{"p", "print"}, "Print current line", &InteractiveDebugger::on_print},
	{{"l", "list"}, "Print defined symbols", &InteractiveDebugger::on_list},
	{{"s", "show"}, "Show symbol value", &InteractiveDebugger::on_show},
	{{"eval"}, "Evaluate an expression", &InteractiveDebugger::on_eval},
	{{"q", "quit"}, "Exit program", &InteractiveDebugger::on_quit},
};

InteractiveDebugger::~InteractiveDebugger() {}

bool InteractiveDebugger::setup(Debugger *debugger, Scheduler *scheduler) {
	return true;
}

bool InteractiveDebugger::handle_events(Debugger *debugger, CursorDebugger *cursor) {
	return true;
}

bool InteractiveDebugger::check(Debugger *debugger, CursorDebugger *cursor) {

	m_terminal.set_prompt([cursor](size_t row_number) {
		return cursor->module_name() + ":" + std::to_string(row_number + cursor->line_number()) + " >>> ";
	});

	auto buffer = m_terminal.read_line();
	if (!buffer.has_value()) {
		return false;
	}

	std::string command;
	std::istringstream stream(*buffer);

	for (stream >> command; !stream.eof() && !stream.fail(); stream >> command) {
		if (!call_command(command, debugger, cursor, stream)) {
			return false;
		}
	}

	return true;
}

void InteractiveDebugger::cleanup(Debugger *debugger, Scheduler *scheduler) {}

void InteractiveDebugger::on_thread_started(Debugger *debugger, CursorDebugger *cursor) {
	print_debug_trace("Created thread %d", cursor->get_thread_id());
}

void InteractiveDebugger::on_thread_exited(Debugger *debugger, CursorDebugger *cursor) {
	print_debug_trace("Deleted thread %d", cursor->get_thread_id());
}

void InteractiveDebugger::on_breakpoint_created(Debugger *debugger, const Breakpoint &breakpoint) {
	print_debug_trace("Created breakpoint %zu at %s:%ld", breakpoint.id, breakpoint.info.module_name().c_str(),
					  breakpoint.info.line_number());
}

void InteractiveDebugger::on_breakpoint_deleted(Debugger *debugger, const Breakpoint &breakpoint) {
	print_debug_trace("Deleted breakpoint %zu at %s:%ld", breakpoint.id, breakpoint.info.module_name().c_str(),
					  breakpoint.info.line_number());
}

void InteractiveDebugger::on_module_loaded(Debugger *debugger, CursorDebugger *cursor, Module *module) {
	AbstractSyntaxTree *ast = cursor->cursor()->ast();
	Module::Id module_id = ast->get_module_id(module);
	if (module_id != Module::INVALID_ID) {
		const std::string &module_name = ast->get_module_name(module);
		print_debug_trace("Loaded module %s", module_name.c_str());
	}
}

bool InteractiveDebugger::on_breakpoint(Debugger *debugger, CursorDebugger *cursor,
										const std::unordered_set<Breakpoint::Id> &breakpoints) {
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

void InteractiveDebugger::on_exit(Debugger *debugger, int code) {
	print_debug_trace("Script has exited with code %d", code);
}

void InteractiveDebugger::on_error(Debugger *debugger) {}

bool InteractiveDebugger::on_continue(Debugger *debugger, CursorDebugger *cursor, std::istringstream &stream) {
	debugger->do_run(cursor);
	return true;
}

bool InteractiveDebugger::on_next(Debugger *debugger, CursorDebugger *cursor, std::istringstream &stream) {
	debugger->do_next(cursor);
	return true;
}

bool InteractiveDebugger::on_enter(Debugger *debugger, CursorDebugger *cursor, std::istringstream &stream) {
	debugger->do_enter(cursor);
	return true;
}

bool InteractiveDebugger::on_return(Debugger *debugger, CursorDebugger *cursor, std::istringstream &stream) {
	debugger->do_return(cursor);
	return true;
}

bool InteractiveDebugger::on_thread(Debugger *debugger, mint::CursorDebugger *cursor, std::istringstream &stream) {
	std::string action;
	stream >> action;
	if (action == "list") {
		const ThreadList threads = debugger->get_threads();
		for (const CursorDebugger *thread : threads) {
			print_debug_trace("%d: %s", thread->get_thread_id(), thread->line_info().to_string().c_str());
		}
	}
	else if (action == "cur" || action == "current") {
		print_debug_trace("%d: %s", cursor->get_thread_id(), cursor->line_info().to_string().c_str());
	}
	else {
		Terminal::print(stdout, MINT_TERM_BOLD "thread list" MINT_TERM_RESET ":\n\tLists runing threads\n");
		Terminal::print(stdout, MINT_TERM_BOLD "thread cur | current" MINT_TERM_RESET
											   ":\n\tPrints the current thread informations\n");
	}
	return true;
}

bool InteractiveDebugger::on_backtrace(Debugger *debugger, CursorDebugger *cursor, std::istringstream &stream) {

	while (std::isspace(stream.peek())) {
		stream.get();
	}

	switch (stream.peek()) {
	case '\n':
	case EOF:
		for (const LineInfo &line : cursor->cursor()->dump()) {
			print_debug_trace("%s", line.to_string().c_str());
		}
		break;
	default:

		const CursorDebugger *thread = cursor;
		bool with_context_lines = false;
		int count = 0;

		do {
			std::string option;
			stream >> option;
			if (option == "--thread") {
				Process::ThreadId thread_id;
				stream >> thread_id;
				thread = debugger->get_thread(thread_id);
				if (thread == nullptr) {
					print_debug_trace("Can not find thread : unknown id %d", thread_id);
					return true;
				}
			}
			else if (option.front() == '-' || option.front() == '+' || std::isdigit(option.front())) {
				with_context_lines = true;
				try {
					count = stoi(option);
				}
				catch (...) {
					print_debug_trace("Invalid line count : %s", option.c_str());
					return true;
				}
			}
			else {
				Terminal::print(stdout, MINT_TERM_BOLD
								"backtrace --thread" MINT_TERM_RESET MINT_TERM_ITALIC " <id>" MINT_TERM_RESET
								":\n\tPrints the backtrace of the thread with the given " MINT_TERM_ITALIC
								"id" MINT_TERM_RESET "\n");
				Terminal::print(stdout, MINT_TERM_BOLD
								"backtrace" MINT_TERM_RESET MINT_TERM_ITALIC " <count> | +<count>" MINT_TERM_RESET
								":\n\tPrints the backtrace with the " MINT_TERM_ITALIC "count" MINT_TERM_RESET
								" next lines of each step\n");
				Terminal::print(stdout,
								MINT_TERM_BOLD "backtrace" MINT_TERM_RESET MINT_TERM_ITALIC " -<count>" MINT_TERM_RESET
											   ":\n\tPrints the backtrace with the " MINT_TERM_ITALIC
											   "count" MINT_TERM_RESET " previous and next lines of each step\n");
				Terminal::print(stdout, MINT_TERM_BOLD "backtrace" MINT_TERM_RESET ":\n\tPrints the backtrace\n");
				return true;
			}
		}
		while (stream.peek() != '\n' && stream.peek() != EOF);

		for (const LineInfo &line : thread->cursor()->dump()) {

			const std::string module_name = line.module_name();
			const size_t line_number = line.line_number();

			print_debug_trace("%s", line.to_string().c_str());
			if (with_context_lines) {
				if (count < 0) {
					print_highlighted((line_number <= abs(count)) ? 1 : line_number + count, line_number + abs(count),
									  line_number, get_module_stream(module_name));
				}
				else {
					print_highlighted(line_number, line_number + count, line_number, get_module_stream(module_name));
				}
			}
		}
	}

	return true;
}

bool InteractiveDebugger::on_breakpoint(Debugger *debugger, CursorDebugger *cursor, std::istringstream &stream) {
	std::string action, module, line;
	stream >> action;
	if (action == "add") {
		stream >> module;
		stream >> line;
		Module::Info info = Scheduler::instance()->ast()->module_info(module);
		if (DebugInfo *debug_info = info.debug_info; debug_info && info.state != Module::NOT_COMPILED) {
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
			print_debug_trace("%ld: %s", breakpoint.id, breakpoint.info.to_string().c_str());
		}
	}
	else {
		Terminal::print(stdout, MINT_TERM_BOLD
						"breakpoint add" MINT_TERM_RESET MINT_TERM_ITALIC " <module> <line>" MINT_TERM_RESET
						":\n\tCreates a new break point on the given " MINT_TERM_ITALIC "module" MINT_TERM_RESET
						" at the given " MINT_TERM_ITALIC "line" MINT_TERM_RESET " number\n");
		Terminal::print(stdout, MINT_TERM_BOLD "breakpoint del | delete" MINT_TERM_RESET MINT_TERM_ITALIC
											   " <id> | <module> <line>" MINT_TERM_RESET
											   ":\n\tDeletes the break point with the given " MINT_TERM_ITALIC
											   "id" MINT_TERM_RESET " or on the given " MINT_TERM_ITALIC
											   "module" MINT_TERM_RESET " at the given " MINT_TERM_ITALIC
											   "line" MINT_TERM_RESET " number\n");
		Terminal::print(stdout,
						MINT_TERM_BOLD "breakpoint list" MINT_TERM_RESET ":\n\tLists configured break points\n");
	}
	return true;
}

bool InteractiveDebugger::on_print(Debugger *debugger, CursorDebugger *cursor, std::istringstream &stream) {

	const std::string module_name = cursor->module_name();
	const size_t line_number = cursor->line_number();

	while (std::isspace(stream.peek())) {
		stream.get();
	}

	switch (int next = stream.peek()) {
	case '\n':
	case EOF:
		print_highlighted(line_number, line_number, line_number, get_module_stream(module_name));
		break;
	default:

		int count = 0;

		std::string option;
		stream >> option;
		if (option.front() == '-' || option.front() == '+' || std::isdigit(option.front())) {
			try {
				count = stoi(option);
			}
			catch (...) {
				print_debug_trace("Invalid line count : %s", option.c_str());
				return true;
			}
		}
		else {
			Terminal::print(stdout, MINT_TERM_BOLD
							"print" MINT_TERM_RESET MINT_TERM_ITALIC " <count> | +<count>" MINT_TERM_RESET
							":\n\tPrints the " MINT_TERM_ITALIC "count" MINT_TERM_RESET " next lines\n");
			Terminal::print(stdout, MINT_TERM_BOLD "print" MINT_TERM_RESET MINT_TERM_ITALIC " -<count>" MINT_TERM_RESET
												   ":\n\tPrints the " MINT_TERM_ITALIC "count" MINT_TERM_RESET
												   " previous and next lines\n");
			Terminal::print(stdout, MINT_TERM_BOLD "print" MINT_TERM_RESET ":\n\tPrints the current line\n");
			return true;
		}
		if (count < 0) {
			print_highlighted((line_number <= abs(count)) ? 1 : line_number + count, line_number + abs(count),
							  line_number, get_module_stream(module_name));
		}
		else {
			print_highlighted(line_number, line_number + count, line_number, get_module_stream(module_name));
		}
	}

	return true;
}

bool InteractiveDebugger::on_list(Debugger *debugger, CursorDebugger *cursor, std::istringstream &stream) {

	bool slots_only = false;

	while (std::isspace(stream.peek())) {
		stream.get();
	}

	switch (stream.peek()) {
	case '\n':
	case EOF:
		for (auto &symbol : cursor->cursor()->symbols()) {
			std::string symbol_str = symbol.first.str();
			std::string type = type_name(WeakReference::share(symbol.second));
			std::string value = reference_value(WeakReference::share(symbol.second));
			print_debug_trace("%s (%s) : %s", symbol_str.c_str(), type.c_str(), value.c_str());
		}
		break;
	case '-':
		do {
			std::string option;
			stream >> option;
			if (option == "--slots") {
				slots_only = true;
			}
			else {
				Terminal::print(stdout, MINT_TERM_BOLD
								"list --slots" MINT_TERM_RESET MINT_TERM_ITALIC " <symbol>" MINT_TERM_RESET
								":\n\tLists the slots of the object identified by " MINT_TERM_ITALIC
								"symbol" MINT_TERM_RESET "\n");
				Terminal::print(stdout,
								MINT_TERM_BOLD "list" MINT_TERM_RESET MINT_TERM_ITALIC " <symbol>" MINT_TERM_RESET
											   ":\n\tLists the members of the object identified by " MINT_TERM_ITALIC
											   "symbol" MINT_TERM_RESET "\n");
				return true;
			}
		}
		while (stream.peek() == '-');
		[[fallthrough]];
	default:
		try {

			SymbolEvaluator evaluator(cursor->cursor());

			if (evaluator.parse(stream)) {
				if (const std::optional<WeakReference> &parent = evaluator.get_reference()) {
					switch (parent->data()->format) {
					case Data::FMT_OBJECT:
						if (mint::is_object(parent->data<Object>())) {
							for (auto &[symbol, member] : parent->data<Object>()->metadata->members()) {
								if (slots_only && member->offset == Class::MemberInfo::INVALID_OFFSET) {
									continue;
								}
								Reference &reference = Class::MemberInfo::get(member, parent->data<Object>());
								print_debug_trace("%s (%s) : %s", symbol.str().c_str(), type_name(reference).c_str(),
												  reference_value(reference).c_str());
							}
						}
						else {
							for (auto &[symbol, member] : parent->data<Object>()->metadata->globals()) {
								Reference &reference = Class::MemberInfo::get(member, parent->data<Object>());
								print_debug_trace("%s (%s) : %s", symbol.str().c_str(), type_name(reference).c_str(),
												  reference_value(reference).c_str());
							}
						}
						break;
					case Data::FMT_PACKAGE:
						for (auto &[symbol, reference] : parent->data<Package>()->data->symbols()) {
							print_debug_trace("%s (%s) : %s", symbol.str().c_str(), type_name(reference).c_str(),
											  reference_value(reference).c_str());
						}
						break;
					default:
						print_debug_trace("Symbol %s has no members", evaluator.get_symbol_name().c_str());
					}
				}
				else {
					print_debug_trace("No symbol found");
				}
			}
			else {
				print_debug_trace("Expression is not a valid symbol");
				stream.setstate(std::istringstream::eofbit);
			}
		}
		catch (MintSystemError &error) {
			print_debug_trace("Expression is not a valid symbol: %s", error.what());
			stream.setstate(std::istringstream::eofbit);
		}
	}

	return true;
}

bool InteractiveDebugger::on_show(Debugger *debugger, CursorDebugger *cursor, std::istringstream &stream) {

	SymbolEvaluator evaluator(cursor->cursor());

	try {
		if (evaluator.parse(stream)) {
			if (const std::optional<WeakReference> &reference = evaluator.get_reference()) {
				print_debug_trace("%s (%s) : %s", evaluator.get_symbol_name().c_str(), type_name(*reference).c_str(),
								  reference_value(*reference).c_str());
			}
			else {
				print_debug_trace("No symbol found");
			}
		}
		else {
			print_debug_trace("Expression is not a valid symbol");
			stream.setstate(std::istringstream::eofbit);
		}
	}
	catch (MintSystemError &error) {
		print_debug_trace("Expression is not a valid symbol: %s", error.what());
		stream.setstate(std::istringstream::eofbit);
	}

	return true;
}

bool InteractiveDebugger::on_eval(Debugger *debugger, CursorDebugger *cursor, std::istringstream &stream) {

	ExpressionEvaluator evaluator(cursor->cursor()->ast());

	try {
		evaluator.setup_locals(cursor->cursor()->symbols());
		if (evaluator.parse(stream)) {
			Reference &reference = evaluator.get_result();
			print_debug_trace("result (%s) : %s", type_name(reference).c_str(), reference_value(reference).c_str());
		}
		else {
			print_debug_trace("Expression can not be evaluated");
			stream.setstate(std::istringstream::eofbit);
		}
	}
	catch (MintSystemError &error) {
		print_debug_trace("Expression can not be evaluated: %s", error.what());
		stream.setstate(std::istringstream::eofbit);
	}

	return true;
}

bool InteractiveDebugger::on_quit(Debugger *debugger, CursorDebugger *cursor, std::istringstream &stream) {
	return false;
}

void InteractiveDebugger::print_commands() {
	for (const Command &command : g_commands) {
		std::string names;
		for (auto i = command.names.begin(); i != command.names.end(); ++i) {
			if (i != command.names.begin()) {
				names += " | ";
			}
			names += *i;
		}
		Terminal::printf(stdout, MINT_TERM_BOLD "%s" MINT_TERM_RESET ":\n\t%s\n", names.c_str(), command.desc.c_str());
	}
}

bool InteractiveDebugger::call_command(const std::string &command, Debugger *debugger, CursorDebugger *cursor,
									   std::istringstream &stream) {
	auto it = std::find_if(g_commands.begin(), g_commands.end(), [&command](const Command &entry) {
		return std::any_of(entry.names.begin(), entry.names.end(), [&command](const std::string &name) {
			return command == name;
		});
	});
	if (it != g_commands.end()) {
		return std::invoke(it->func, this, debugger, cursor, stream);
	}
	print_commands();
	return true;
}
