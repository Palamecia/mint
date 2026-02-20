/**
 * Copyright (c) 2026 Gauvain CHERY.
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
#include "commandrunner.h"
#include "debugger.h"
#include "debugprinter.h"
#include "expressionevaluator.h"
#include "highlighter.h"
#include "mint/ast/cursor.h"
#include "mint/ast/module.h"
#include "mint/debug/debuginfo.h"
#include "mint/debug/debuginterface.h"
#include "mint/debug/debugtool.h"
#include "mint/debug/lineinfo.h"
#include "mint/memory/class.h"
#include "mint/memory/data.h"
#include "mint/memory/memorytool.h"
#include "mint/memory/object.h"
#include "mint/scheduler/process.h"
#include "mint/system/mintruntimeerror.h"
#include "mint/system/terminal.h"
#include "symbolevaluator.h"

#include <csignal>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_set>

namespace {

bool paused_by_signal = false;

}

InteractiveDebugger::InteractiveDebugger() {
	init_continue(_command_runner.add_command({"c", "continue"}, "Execute until next break point."));
	init_next(_command_runner.add_command({"n", "next"}, "Execute next line."));
	init_enter(_command_runner.add_command({"e", "enter"}, "Enter function."));
	init_return(_command_runner.add_command({"r", "return"}, "Exit function."));
	init_thread(_command_runner.add_command({"th", "thread"}, "Manage threads."));
	init_backtrace(_command_runner.add_command({"bt", "backtrace"}, "Print backtrace."));
	init_breakpoint(_command_runner.add_command({"bp", "breakpoint"}, "Manage break points."));
	init_print(_command_runner.add_command({"p", "print"}, "Print current line."));
	init_list(_command_runner.add_command({"l", "list"}, "Print defined symbols."));
	init_show(_command_runner.add_command({"s", "show"}, "Show symbol value."));
	init_eval(_command_runner.add_command({"eval"}, "Evaluate an expression."));
	init_quit(_command_runner.add_command({"q", "quit"}, "Exit program."));
}

bool InteractiveDebugger::setup(Debugger& debugger, mint::Scheduler& /*scheuler*/) {
	_terminal.set_completion_generator([this, &debugger](std::string_view context, std::string_view::size_type offset) {
		return _command_runner.complete(debugger, context.substr(0, offset));
	});
	std::signal(SIGINT, [](int /*sig*/) {
		paused_by_signal = true;
	});
	return true;
}

bool InteractiveDebugger::process_events(Debugger& debugger, mint::CursorDebugger& cursor) {
	if (paused_by_signal) {
		paused_by_signal = false;
		debugger.do_pause(cursor);
	}
	return true;
}

bool InteractiveDebugger::check(Debugger& debugger, mint::CursorDebugger& cursor) {

	_terminal.set_prompt([cursor](std::size_t row_number) {
		return MINT_TERM_STDSTR(MINT_TERM_OPT(MINT_TERM_BOLD) + cursor.module_name() + ":"
		                        + std::to_string(row_number + cursor.line_number()))
		       + " ❯❯❯ ";
	});

	auto buffer = _terminal.read_line();
	if (!buffer.has_value()) {
		return false;
	}

	auto stream = std::string_view(*buffer);
	_command_runner.run(debugger, cursor, stream);

	return _running;
}

void InteractiveDebugger::cleanup(Debugger& debugger, mint::Scheduler& scheduler) {}

void InteractiveDebugger::on_thread_started(Debugger& /*debugger*/, mint::CursorDebugger& cursor) {
	print_debug_trace("Created thread {}", cursor.get_thread_id());
}

void InteractiveDebugger::on_thread_exited(Debugger& /*debugger*/, mint::CursorDebugger& cursor) {
	print_debug_trace("Deleted thread {}", cursor.get_thread_id());
}

void InteractiveDebugger::on_breakpoint_created(Debugger& /*debugger*/, const mint::Breakpoint& breakpoint) {
	print_debug_trace("Created breakpoint {} at {}:{}", breakpoint.id, breakpoint.info.module_name(),
	    breakpoint.info.line_number());
}

void InteractiveDebugger::on_breakpoint_deleted(Debugger& /*debugger*/, const mint::Breakpoint& breakpoint) {
	print_debug_trace("Deleted breakpoint {} at {}:{}", breakpoint.id, breakpoint.info.module_name(),
	    breakpoint.info.line_number());
}

void InteractiveDebugger::on_module_loaded(Debugger& /*debugger*/, mint::CursorDebugger& cursor, mint::Module& module) {
	const auto& ast = cursor.cursor().ast();
	const auto module_id = ast.get_module_id(module);
	if (module_id != mint::Module::invalid_id) {
		const std::string& module_name = ast.get_module_name(module);
		print_debug_trace("Loaded module {}", module_name);
	}
}

bool InteractiveDebugger::on_breakpoint(Debugger& /*debugger*/, mint::CursorDebugger& /*cursor*/,
    const std::unordered_set<mint::Breakpoint::Id>& /*breakpoints*/) {
	return true;
}

bool InteractiveDebugger::on_exception(Debugger& /*debugger*/, mint::CursorDebugger& /*cursor*/) {
	return true;
}

bool InteractiveDebugger::on_pause(Debugger& /*debugger*/, mint::CursorDebugger& /*cursor*/) {
	return true;
}

bool InteractiveDebugger::on_step(Debugger& /*debugger*/, mint::CursorDebugger& /*cursor*/) {
	return true;
}

void InteractiveDebugger::on_exit(Debugger& /*debugger*/, int code) {
	print_debug_trace("Script has exited with code {}", code);
}

void InteractiveDebugger::on_error(Debugger& debugger) {}

void InteractiveDebugger::stop() {
	_running = false;
}

void InteractiveDebugger::init_continue(CommandRunner::Command& command) {
	command.add({}, "Execute until next break point.",
	    [](Debugger& debugger, mint::CursorDebugger& cursor,
	        const std::span<CommandRunner::Parameter::value_t>& /*parameters*/) {
		    debugger.do_run(cursor);
	    });
}

void InteractiveDebugger::init_next(CommandRunner::Command& command) {
	command.add({}, "Execute next line.",
	    [](Debugger& debugger, mint::CursorDebugger& cursor,
	        const std::span<CommandRunner::Parameter::value_t>& /*parameters*/) {
		    debugger.do_next(cursor);
	    });
}

void InteractiveDebugger::init_enter(CommandRunner::Command& command) {
	command.add({}, "Enter function.",
	    [](Debugger& debugger, mint::CursorDebugger& cursor,
	        const std::span<CommandRunner::Parameter::value_t>& /*parameters*/) {
		    debugger.do_enter(cursor);
	    });
}

void InteractiveDebugger::init_return(CommandRunner::Command& command) {
	command.add({}, "Exit function.",
	    [](Debugger& debugger, mint::CursorDebugger& cursor,
	        const std::span<CommandRunner::Parameter::value_t>& /*parameters*/) {
		    debugger.do_return(cursor);
	    });
}

void InteractiveDebugger::init_thread(CommandRunner::Command& command) {
	command.add({{{"list"}}}, "Lists runing threads.",
	    [](Debugger& debugger, mint::CursorDebugger& /*cursor*/,
	        const std::span<CommandRunner::Parameter::value_t>& /*parameters*/) {
		    for (const mint::ThreadList threads = debugger.get_threads(); const mint::CursorDebugger& thread : threads) {
			    print_debug_trace("{}: {}", thread.get_thread_id(), thread.line_info().to_string());
		    }
	    });
	command.add({{{"cur", "current"}}}, "Prints the current thread informations.",
	    [](Debugger& /*debugger*/, mint::CursorDebugger& cursor,
	        const std::span<CommandRunner::Parameter::value_t>& /*parameters*/) {
		    print_debug_trace("{}: {}", cursor.get_thread_id(), cursor.line_info().to_string());
	    });
}

void InteractiveDebugger::init_backtrace(CommandRunner::Command& command) {

	using Parameter = CommandRunner::Parameter;

	auto print_backtrace = [](Debugger& debugger, const mint::CursorDebugger& thread, bool with_context_lines = false,
	                           int count = 0) {
		for (const mint::LineInfo& line : thread.cursor().dump()) {

			const std::string module_name = line.module_name();
			const std::size_t line_number = line.line_number();

			print_debug_trace("{}", line.to_string());
			if (with_context_lines) {
				if (count < 0) {
					print_highlighted((line_number <= abs(count)) ? 1 : line_number + count, line_number + abs(count),
					    line_number, debugger.ast().global_data(), mint::get_module_stream(module_name));
				}
				else {
					print_highlighted(line_number, line_number + count, line_number, debugger.ast().global_data(),
					    mint::get_module_stream(module_name));
				}
			}
		}
	};

	command.add({{{"thread"}}, {Parameter::thread_id, "id"}}, "Prints the backtrace of the thread with the given *id*.",
	    [&](Debugger& debugger, mint::CursorDebugger& /*cursor*/,
	        const std::span<CommandRunner::Parameter::value_t>& parameters) {
		    const auto thread_id = get_parameter<mint::Process::ThreadId>(parameters[0]);
		    const mint::CursorDebugger* thread = debugger.find_thread(thread_id);
		    if (thread == nullptr) {
			    print_debug_trace("Can not find thread : unknown id {}", thread_id);
			    return;
		    }
		    print_backtrace(debugger, *thread);
	    });
	command.add({{Parameter::offset, "count"}},
	    "Prints the backtrace with the *count* next lines of each step. If *count* is negative, the *count* previous "
	    "lines are also printed.",
	    [&](Debugger& debugger, mint::CursorDebugger& cursor,
	        const std::span<CommandRunner::Parameter::value_t>& parameters) {
		    print_backtrace(debugger, cursor, true, get_parameter<int>(parameters[0]));
	    });
	command.add({}, "Prints the backtrace",
	    [](Debugger& /*debugger*/, mint::CursorDebugger& cursor,
	        const std::span<CommandRunner::Parameter::value_t>& /*parameters*/) {
		    for (const mint::LineInfo& line : cursor.cursor().dump()) {
			    print_debug_trace("{}", line.to_string());
		    }
	    });
}

void InteractiveDebugger::init_breakpoint(CommandRunner::Command& command) {

	using Parameter = CommandRunner::Parameter;

	command.add({{{"add"}}, {Parameter::module, "module"}, {Parameter::line_number, "line"}},
	    "Creates a new break point on the given *module* at the given *line* number.",
	    [](Debugger& debugger, mint::CursorDebugger& /*cursor*/,
	        const std::span<CommandRunner::Parameter::value_t>& parameters) {
		    const auto module = get_parameter<std::string>(parameters[0]);
		    const auto line = get_parameter<std::size_t>(parameters[1]);
		    const auto info = debugger.ast().module_info(module);
		    if (mint::DebugInfo* debug_info = info.debug_info;
		        debug_info && info.state != mint::Module::State::not_compiled) {
			    debugger.create_breakpoint(mint::LineInfo(info.id, module, debug_info->to_executable_line_number(line)));
		    }
		    else {
			    debugger.add_pending_breakpoint_from_module(module, line);
		    }
	    });
	command.add({{{"del", "delete"}}, {Parameter::module, "module"}, {Parameter::line_number, "line"}},
	    "Deletes the break point on the given *module* at the given *line* number.",
	    [](Debugger& debugger, mint::CursorDebugger& /*cursor*/,
	        const std::span<CommandRunner::Parameter::value_t>& parameters) {
		    const auto module = get_parameter<std::string>(parameters[0]);
		    const auto line = get_parameter<std::size_t>(parameters[1]);
		    debugger.remove_breakpoint(mint::LineInfo(debugger.ast(), module, line));
	    });
	command.add({{{"del", "delete"}}, {Parameter::breakpoint_id, "id"}}, "Deletes the break point with the given *id*.",
	    [](Debugger& debugger, mint::CursorDebugger& /*cursor*/,
	        const std::span<CommandRunner::Parameter::value_t>& parameters) {
		    const auto id = get_parameter<mint::Breakpoint::Id>(parameters[0]);
		    debugger.remove_breakpoint(id);
	    });
	command.add({{{"list"}}}, "Lists configured break points.",
	    [](Debugger& debugger, mint::CursorDebugger& /*cursor*/,
	        const std::span<CommandRunner::Parameter::value_t>& /*parameters*/) {
		    for (const mint::BreakpointList breakpoints = debugger.get_breakpoints();
		        const mint::Breakpoint& breakpoint : breakpoints) {
			    print_debug_trace("{}: {}", breakpoint.id, breakpoint.info.to_string());
		    }
	    });
}

void InteractiveDebugger::init_print(CommandRunner::Command& command) {

	using Parameter = CommandRunner::Parameter;

	auto print_sources = [](Debugger& debugger, const std::string& module_name, std::size_t line_number, int count = 0) {
		if (count < 0) {
			print_highlighted((line_number <= abs(count)) ? 1 : line_number + count, line_number + abs(count),
			    line_number, debugger.ast().global_data(), mint::get_module_stream(module_name));
		}
		else {
			print_highlighted(line_number, line_number + count, line_number, debugger.ast().global_data(),
			    mint::get_module_stream(module_name));
		}
	};

	command.add({{{"module"}}, {Parameter::module, "module"}, {Parameter::line_number, "line"},
	                {Parameter::offset, "count"}},
	    "Prints the *count* lines from *line* in the module *module*. If *count* is negative, the *count* previous "
	    "lines are also printed.",
	    [&](Debugger& debugger, mint::CursorDebugger& /*cursor*/,
	        const std::span<CommandRunner::Parameter::value_t>& parameters) {
		    const auto module_name = get_parameter<std::string>(parameters[0]);
		    const auto line_number = get_parameter<std::size_t>(parameters[1]);
		    const auto count = get_parameter<int>(parameters[2]);
		    print_sources(debugger, module_name, line_number, count);
	    });
	command.add({{{"module"}}, {Parameter::module, "module"}, {Parameter::line_number, "line"}},
	    "Prints the line *line* in the module *module*.",
	    [&](Debugger& debugger, mint::CursorDebugger& /*cursor*/,
	        const std::span<CommandRunner::Parameter::value_t>& parameters) {
		    const auto module_name = get_parameter<std::string>(parameters[0]);
		    const auto line_number = get_parameter<std::size_t>(parameters[1]);
		    print_sources(debugger, module_name, line_number);
	    });
	command.add({{Parameter::offset, "count"}},
	    "Prints the *count* next lines. If *count* is negative, the *count* previous lines are also printed.",
	    [&](Debugger& debugger, mint::CursorDebugger& cursor,
	        const std::span<CommandRunner::Parameter::value_t>& parameters) {
		    const auto count = get_parameter<int>(parameters[0]);
		    print_sources(debugger, cursor.module_name(), cursor.line_number(), count);
	    });
	command.add({}, "Prints the current line.",
	    [&](Debugger& debugger, mint::CursorDebugger& cursor,
	        const std::span<CommandRunner::Parameter::value_t>& /*parameters*/) {
		    print_sources(debugger, cursor.module_name(), cursor.line_number());
	    });
}

void InteractiveDebugger::init_list(CommandRunner::Command& command) {

	using Parameter = CommandRunner::Parameter;

	auto print_members = [](mint::CursorDebugger& cursor, const std::string& script, bool slots_only = false) {
		try {

			auto evaluator = SymbolEvaluator(cursor.cursor());
			auto stream = std::stringstream(script);

			if (evaluator.parse(stream)) {
				if (const auto& parent = evaluator.get_reference()) {
					switch (parent->data().format()) {
					case mint::Data::Format::object:
						if (mint::is_object(parent->data<mint::Object>())) {
							for (auto [symbol, member] : parent->data<mint::Object>().metadata.members()) {
								if (slots_only && member.get().offset == mint::Class::MemberInfo::invalid_offset) {
									continue;
								}
								auto& reference = mint::Class::MemberInfo::get(member, parent->data<mint::Object>());
								print_debug_trace("{} ({}) : {}", symbol.str(), type_name(reference),
								    reference_value(reference));
							}
						}
						else {
							for (auto [symbol, member] : parent->data<mint::Object>().metadata.globals()) {
								auto& reference = mint::Class::MemberInfo::get(member, parent->data<mint::Object>());
								print_debug_trace("{} ({}) : {}", symbol.str(), type_name(reference),
								    reference_value(reference));
							}
						}
						break;
					case mint::Data::Format::package:
						for (auto& [symbol, reference] : parent->data<mint::Package>().data.symbols()) {
							print_debug_trace("{} ({}) : {}", symbol.str(), type_name(reference),
							    reference_value(reference));
						}
						break;
					default:
						print_debug_trace("Symbol {} has no members", evaluator.get_symbol_name());
					}
				}
				else {
					print_debug_trace("No symbol found");
				}
			}
			else {
				print_debug_trace("Expression is not a valid symbol");
			}
		}
		catch (mint::MintRuntimeError& error) {
			print_debug_trace("Expression is not a valid symbol: {}", error.what());
		}
	};

	command.add({{{"members"}}, {Parameter::script, "symbol"}},
	    "Lists the members of the object identified by *symbol*.",
	    [&](Debugger& /*debugger*/, mint::CursorDebugger& cursor,
	        const std::span<CommandRunner::Parameter::value_t>& parameters) {
		    const auto symbol = get_parameter<std::string>(parameters[0]);
		    print_members(cursor, symbol);
	    });
	command.add({{{"slots"}}, {Parameter::script, "symbol"}}, "Lists the slots of the object identified by *symbol*.",
	    [&](Debugger& /*debugger*/, mint::CursorDebugger& cursor,
	        const std::span<CommandRunner::Parameter::value_t>& parameters) {
		    const auto symbol = get_parameter<std::string>(parameters[0]);
		    print_members(cursor, symbol, true);
	    });
	command.add({}, "Lists the variables of the current context.",
	    [](Debugger& /*debugger*/, mint::CursorDebugger& cursor,
	        const std::span<CommandRunner::Parameter::value_t>& /*parameters*/) {
		    for (auto& symbol : cursor.cursor().symbols()) {
			    std::string symbol_str = symbol.first.str();
			    std::string type = type_name(symbol.second);
			    std::string value = reference_value(symbol.second);
			    print_debug_trace("{} ({}) : {}", symbol_str, type, value);
		    }
	    });
}

void InteractiveDebugger::init_show(CommandRunner::Command& command) {

	using Parameter = CommandRunner::Parameter;

	auto print_value = [](mint::CursorDebugger& cursor, const std::string& script) {
		try {

			auto evaluator = SymbolEvaluator(cursor.cursor());
			auto stream = std::stringstream(script);

			if (evaluator.parse(stream)) {
				if (const auto& reference = evaluator.get_reference()) {
					print_debug_trace("{} ({}) : {}", evaluator.get_symbol_name(), type_name(*reference),
					    reference_value(*reference));
				}
				else {
					print_debug_trace("No symbol found");
				}
			}
			else {
				print_debug_trace("Expression is not a valid symbol");
			}
		}
		catch (mint::MintRuntimeError& error) {
			print_debug_trace("Expression is not a valid symbol: {}", error.what());
		}
	};

	command.add({{Parameter::script, "symbol"}}, "Prints the value of *symbol*.",
	    [&](Debugger& /*debugger*/, mint::CursorDebugger& cursor,
	        const std::span<CommandRunner::Parameter::value_t>& parameters) {
		    const auto symbol = get_parameter<std::string>(parameters[0]);
		    print_value(cursor, symbol);
	    });
}

void InteractiveDebugger::init_eval(CommandRunner::Command& command) {

	using Parameter = CommandRunner::Parameter;

	auto evaluate_script = [](Debugger& debugger, mint::CursorDebugger& cursor, const std::string& script) {
		try {

			auto evaluator = ExpressionEvaluator(debugger.ast());
			auto stream = std::stringstream(script);

			evaluator.setup_locals(cursor.cursor().symbols());
			if (evaluator.parse(stream)) {
				const auto& reference = evaluator.get_result();
				print_debug_trace("result ({}) : {}", type_name(reference), reference_value(reference));
			}
			else {
				print_debug_trace("Expression can not be evaluated");
				stream.setstate(std::istringstream::eofbit);
			}
		}
		catch (mint::MintRuntimeError& error) {
			print_debug_trace("Expression can not be evaluated: {}", error.what());
		}
	};

	command.add({{Parameter::script, "script"}}, "Evaluates the value of *script*.",
	    [&](Debugger& debugger, mint::CursorDebugger& cursor,
	        const std::span<CommandRunner::Parameter::value_t>& parameters) {
		    const auto script = get_parameter<std::string>(parameters[0]);
		    evaluate_script(debugger, cursor, script);
	    });
}

void InteractiveDebugger::init_quit(CommandRunner::Command& command) {
	command.add({}, "Exit program.",
	    [this](Debugger& /*debugger*/, mint::CursorDebugger& /*cursor*/,
	        const std::span<CommandRunner::Parameter::value_t>& /*parameters*/) {
		    stop();
	    });
}
