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

#include "dapdebugger.h"
#include "dapmessage.h"
#include "debugprinter.h"
#include "debugger.h"
#include "json.h"
#include "log.h"
#include "mint/ast/module.h"
#include "mint/debug/debuginfo.h"
#include "mint/debug/debuginterface.h"
#include "mint/debug/lineinfo.h"
#include "mint/memory/class.h"
#include "mint/memory/data.h"
#include "mint/memory/object.h"
#include "mint/scheduler/process.h"
#include "mint/scheduler/scheduler.h"
#include "utils.h"

#include "mint/ast/cursor.h"
#include "mint/debug/debugtool.h"
#include "mint/memory/memorytool.h"
#include "mint/system/terminal.h"

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <print>
#include <ranges>
#include <regex>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#ifdef MINT_OS_UNIX
#include <sys/file.h>
#include <unistd.h>
#include <poll.h>
#endif

namespace {

std::size_t to_stack_frame_id(std::size_t thread_id, std::size_t frame_index) {
	return (thread_id * 0xFFFF) + (frame_index % 0xFFFF);
}

std::tuple<mint::Process::ThreadId, std::size_t> from_stack_frame_id(std::size_t frame_id) {
	return {static_cast<mint::Process::ThreadId>(frame_id / 0xFFFF), frame_id % 0xFFFF};
}

std::string format_pii(std::string format, const JsonObject& variables) {

	static const std::regex g_format_pii_regexp("{([^}]+)}");

	std::smatch match;
	while (regex_search(format, match, g_format_pii_regexp)) {
		if (const JsonString* value = variables.get_string(match.str(1))) {
			format.replace(match.position(), match.length(), *value);
		}
	}
	return format;
}

}

std::unordered_map<std::string, DapDebugger::GlobalCommand> DapDebugger::g_commands = {
    {"setBreakpoints", DapDebugger::GlobalCommand {.func = &DapDebugger::on_set_breakpoints}},
    {"threads", DapDebugger::GlobalCommand {.func = &DapDebugger::on_threads}},
    {"stackTrace", DapDebugger::GlobalCommand {.func = &DapDebugger::on_stack_trace}},
    {"breakpointLocations", DapDebugger::GlobalCommand {.func = &DapDebugger::on_breakpoint_locations}},
    {"scopes", DapDebugger::GlobalCommand {.func = &DapDebugger::on_scopes}},
    {"variables", DapDebugger::GlobalCommand {.func = &DapDebugger::on_variables}},
    {"continue", DapDebugger::GlobalCommand {.func = &DapDebugger::on_continue}},
    {"next", DapDebugger::GlobalCommand {.func = &DapDebugger::on_next}},
    {"stepIn", DapDebugger::GlobalCommand {.func = &DapDebugger::on_step_in}},
    {"stepOut", DapDebugger::GlobalCommand {.func = &DapDebugger::on_step_out}},
    {"pause", DapDebugger::GlobalCommand {.func = &DapDebugger::on_pause}},
    {"disconnect", DapDebugger::GlobalCommand {.func = &DapDebugger::on_disconnect}},
    {"terminate", DapDebugger::GlobalCommand {.func = &DapDebugger::on_terminate}},
};

std::unordered_map<std::string, DapDebugger::SetupCommand> DapDebugger::g_setup_commands = {
    {"initialize", DapDebugger::SetupCommand {.func = &DapDebugger::on_initialize}},
    {"launch", DapDebugger::SetupCommand {.func = &DapDebugger::on_launch}},
    {"configurationDone", DapDebugger::SetupCommand {.func = &DapDebugger::on_configuration_done}},
};

std::unordered_map<std::string, DapDebugger::RuntimeCommand> DapDebugger::g_runtime_commands = {

};

DapDebugger::DapDebugger(std::unique_ptr<DapMessageReader>&& reader, std::unique_ptr<DapMessageWriter>&& writer) :
    _reader(std::move(reader)),
    _writer(std::move(writer)),
    _stdin(mint::stdin_file_no),
    _stdout(mint::stdout_file_no),
    _stderr(mint::stderr_file_no) {
	std::println(Logger::default_logger(), "Start debugger");
}

DapDebugger::~DapDebugger() {
	std::println(Logger::default_logger(), "Stop debugger");
}

bool DapDebugger::setup(Debugger& debugger, mint::Scheduler& scheduler) {

	while (_running && _configuring) {
		if (const auto message = _reader->next_message()) {
			std::println(Logger::default_logger(), "From client: {}", message->encode());
			visit(Overloaded {
			          [&](const DapRequestMessage& message) {
				          if (!dispatch_request(message, debugger, scheduler)) {
					          std::println(Logger::default_logger(), "Unknown request");
				          }
			          },
			          [](const DapResponseMessage& message) {},
			          [](const DapEventMessage& message) {},
			      },
			    *message);
		}
	}

	return _running;
}

bool DapDebugger::process_events(Debugger& debugger, mint::CursorDebugger& cursor) {

	if (_stdout.can_read()) {
		send_event("output", JsonObject {
		                         {"category", JsonString("stdout")},
		                         {"output", JsonString(_stdout.read())},
		                     });
	}

	if (_stderr.can_read()) {
		send_event("output", JsonObject {
		                         {"category", JsonString("stderr")},
		                         {"output", JsonString(_stderr.read())},
		                     });
	}

	while (const auto message = _reader->next_message()) {
		std::println(Logger::default_logger(), "From client: {}", message->encode());
		visit(Overloaded {
		          [&](const DapRequestMessage& message) {
			          if (!dispatch_request(message, debugger, cursor)) {
				          std::println(Logger::default_logger(), "Unknown request");
			          }
		          },
		          [](const DapResponseMessage& message) {},
		          [](const DapEventMessage& message) {},
		      },
		    *message);
	}

	return _running;
}

bool DapDebugger::check(Debugger& debugger, mint::CursorDebugger& cursor) {

	if (_stdout.can_read()) {
		send_event("output", JsonObject {
		                         {"category", JsonString("stdout")},
		                         {"output", JsonString(_stdout.read())},
		                     });
	}

	if (_stderr.can_read()) {
		send_event("output", JsonObject {
		                         {"category", JsonString("stderr")},
		                         {"output", JsonString(_stderr.read())},
		                     });
	}

	while (const auto message = _reader->next_message()) {
		std::println(Logger::default_logger(), "From client: {}", message->encode());
		switch (message->get_type()) {
		case DapMessage::Type::request:
			if (!dispatch_request(static_cast<DapRequestMessage&>(*message), debugger, cursor)) {
				std::println(Logger::default_logger(), "Unknown request");
			}
			break;
		case DapMessage::Type::response:
		case DapMessage::Type::event:
			break;
		}
	}

	return _running;
}

void DapDebugger::cleanup(Debugger& /*debugger*/, mint::Scheduler& /*scheduler*/) {

	while (_stdout.can_read()) {
		send_event("output", JsonObject {
		                         {"category", JsonString("stdout")},
		                         {"output", JsonString(_stdout.read())},
		                     });
	}

	while (_stderr.can_read()) {
		send_event("output", JsonObject {
		                         {"category", JsonString("stderr")},
		                         {"output", JsonString(_stderr.read())},
		                     });
	}

	send_event("terminated");
}

void DapDebugger::on_thread_started(Debugger& /*debugger*/, mint::CursorDebugger& cursor) {
	send_event("thread", JsonObject {
	                         {"reason", JsonString("started")},
	                         {"threadId", JsonNumber(to_client_id(cursor.get_thread_id()))},
	                     });
}

void DapDebugger::on_thread_exited(Debugger& /*debugger*/, mint::CursorDebugger& cursor) {
	send_event("thread", JsonObject {
	                         {"reason", JsonString("exited")},
	                         {"threadId", JsonNumber(to_client_id(cursor.get_thread_id()))},
	                     });
}

void DapDebugger::on_breakpoint_created(Debugger& /*debugger*/, const mint::Breakpoint& breakpoint) {
	send_event("breakpoint", JsonObject {
	                             {"reason", JsonString("new")},
	                             {
	                                 "breakpoint",
	                                 JsonObject {
	                                     {"verified", JsonBoolean(true)},
	                                     {"id", JsonNumber(to_client_id(breakpoint.id))},
	                                     {"line", JsonNumber(to_client_line_number(breakpoint.info.line_number()))},
	                                 },
	                             },
	                         });
}

void DapDebugger::on_breakpoint_deleted(Debugger& /*debugger*/, const mint::Breakpoint& breakpoint) {
	send_event("breakpoint", JsonObject {
	                             {"reason", JsonString("removed")},
	                             {
	                                 "breakpoint",
	                                 JsonObject {
	                                     {"verified", JsonBoolean(true)},
	                                     {"id", JsonNumber(to_client_id(breakpoint.id))},
	                                     {"line", JsonNumber(to_client_line_number(breakpoint.info.line_number()))},
	                                 },
	                             },
	                         });
}

void DapDebugger::on_module_loaded(Debugger& /*debugger*/, mint::CursorDebugger& cursor, mint::Module& module) {
	const auto& ast = cursor.cursor().ast();
	const auto module_id = ast.get_module_id(module);
	if (module_id != mint::Module::invalid_id) {
		const std::string module_name = ast.get_module_name(module);
		const std::filesystem::path system_path = mint::to_system_path(module_name);
		if (!system_path.empty()) {
			send_event("loadedSource", JsonObject {
			                               {"reason", JsonString("new")},
			                               {
			                                   "source",
			                                   JsonObject {
			                                       {"name", JsonString(system_path.filename().generic_string())},
			                                       {"path", JsonString(system_path.generic_string())},
			                                   },
			                               },
			                           });
		}
		send_event("module", JsonObject {
		                         {"reason", JsonString("new")},
		                         {
		                             "module",
		                             JsonObject {
		                                 {"id", JsonNumber(to_client_id(module_id))},
		                                 {"name", JsonString(module_name)},
		                             },
		                         },
		                     });
	}
}

bool DapDebugger::on_breakpoint(Debugger& /*debugger*/, mint::CursorDebugger& cursor,
    const std::unordered_set<mint::Breakpoint::Id>& breakpoints) {
	send_event("stopped", JsonObject {
	                          {"reason", JsonString("breakpoint")},
	                          {"threadId", JsonNumber(to_client_id(cursor.get_thread_id()))},
	                          {"preserveFocusHint", JsonBoolean(false)},
	                          {"allThreadsStopped", JsonBoolean(true)},
	                          {"hitBreakpointIds", JsonArray(std::from_range, std::views::transform(breakpoints,
	                                                                              [](mint::Breakpoint::Id id) {
		                                                                              return JsonNumber(id);
	                                                                              }))},
	                      });
	_variables.clear();
	return true;
}

bool DapDebugger::on_exception(Debugger& /*debugger*/, mint::CursorDebugger& cursor) {
	send_event("stopped", JsonObject {
	                          {"reason", JsonString("exception")},
	                          {"threadId", JsonNumber(to_client_id(cursor.get_thread_id()))},
	                          {"preserveFocusHint", JsonBoolean(false)},
	                          {"allThreadsStopped", JsonBoolean(true)},
	                          {"hitBreakpointIds", JsonArray()},
	                      });
	_variables.clear();
	return true;
}

bool DapDebugger::on_pause(Debugger& /*debugger*/, mint::CursorDebugger& cursor) {
	send_event("stopped", JsonObject {
	                          {"reason", JsonString("pause")},
	                          {"threadId", JsonNumber(to_client_id(cursor.get_thread_id()))},
	                          {"preserveFocusHint", JsonBoolean(false)},
	                          {"allThreadsStopped", JsonBoolean(true)},
	                      });
	_variables.clear();
	return true;
}

bool DapDebugger::on_step(Debugger& /*debugger*/, mint::CursorDebugger& cursor) {
	send_event("stopped", JsonObject {
	                          {"reason", JsonString("step")},
	                          {"threadId", JsonNumber(to_client_id(cursor.get_thread_id()))},
	                          {"preserveFocusHint", JsonBoolean(false)},
	                          {"allThreadsStopped", JsonBoolean(true)},
	                          {"hitBreakpointIds", JsonArray()},
	                      });
	_variables.clear();
	return true;
}

void DapDebugger::on_exit(Debugger& /*debugger*/, int code) {
	send_event("exited", JsonObject {
	                         {"exitCode", JsonNumber(code)},
	                     });
}

void DapDebugger::on_error(Debugger& /*debugger*/) {
	_configuring = false;
}

void DapDebugger::shutdown() {
	_running = false;
}

bool DapDebugger::dispatch_request(const DapRequestMessage& message, Debugger& debugger, mint::Scheduler& scheduler) {
	if (auto it = g_commands.find(message.get_command()); it != g_commands.end()) {
		call_command(it->second, message, debugger);
		return true;
	}
	if (auto it = g_setup_commands.find(message.get_command()); it != g_setup_commands.end()) {
		call_command(it->second, message, debugger, scheduler);
		return true;
	}
	return false;
}

bool DapDebugger::dispatch_request(const DapRequestMessage& message, Debugger& debugger, mint::CursorDebugger& cursor) {
	if (auto it = g_commands.find(message.get_command()); it != g_commands.end()) {
		call_command(it->second, message, debugger);
		return true;
	}
	if (auto it = g_runtime_commands.find(message.get_command()); it != g_runtime_commands.end()) {
		call_command(it->second, message, debugger, cursor);
		return true;
	}
	return false;
}

void DapDebugger::send_event(const std::string& event, std::optional<JsonObject> body) {

	auto message = std::make_unique<DapEventMessage>(event, std::move(body));
	std::println(Logger::default_logger(), "To client: {}", message->encode());

	const std::unique_lock _(_write_mutex);
	_writer->append_message(std::move(message));
}

void DapDebugger::send_response(const DapRequestMessage& request, std::optional<JsonObject> body) {

	auto message = std::make_unique<DapResponseMessage>(request, std::move(body));
	std::println(Logger::default_logger(), "To client: {}", message->encode());

	const std::unique_lock _(_write_mutex);
	_writer->append_message(std::move(message));
}

void DapDebugger::send_error(const DapRequestMessage& request, int code, const std::string& format,
    std::optional<JsonObject> variables, ErrorDestination destination) {

	auto error = JsonObject {
	    {"id", JsonNumber(code)},
	    {"format", JsonString(format)},
	};

	auto error_message = format;

	if (variables) {
		error_message = format_pii(format, *variables);
		error.emplace("variables", std::move(*variables));
	}
	if (destination & user) {
		error.emplace("showUser", JsonBoolean(true));
	}
	if (destination & telemetry) {
		error.emplace("sendTelemetry", JsonBoolean(true));
	}

	auto message = std::make_unique<DapResponseMessage>(request, error_message, std::move(error));
	std::println(Logger::default_logger(), "To client: {}", message->encode());

	const std::unique_lock _(_write_mutex);
	_writer->append_message(std::move(message));
}

std::size_t DapDebugger::to_column_number(std::size_t number) const {
	return _client_columns_start_at_1 ? number : number + 1;
}

std::size_t DapDebugger::to_client_column_number(std::size_t number) const {
	return _client_columns_start_at_1 ? number : number - 1;
}

std::size_t DapDebugger::to_line_number(std::size_t number) const {
	return _client_lines_start_at_1 ? number : number + 1;
}

std::size_t DapDebugger::to_client_line_number(std::size_t number) const {
	return _client_lines_start_at_1 ? number : number - 1;
}

std::size_t DapDebugger::to_frame_id(std::size_t id) {
	return id - 1;
}

std::size_t DapDebugger::to_client_id(std::size_t id) {
	return id + 1;
}

mint::Process::ThreadId DapDebugger::to_thread_id(std::size_t id) {
	return static_cast<mint::Process::ThreadId>(id - 1);
}

std::size_t DapDebugger::to_client_id(mint::Process::ThreadId id) {
	return id + 1;
}

void DapDebugger::on_set_breakpoints(const DapRequestMessage& request, const JsonObject& arguments, Debugger& debugger) {
	const auto file_path = static_cast<std::string>(*arguments.get_object("source")->get_string("path"));
	const auto module = mint::to_module_path(file_path);
	for (const mint::Breakpoint& breakpoint : debugger.get_breakpoints()) {
		if (module == breakpoint.info.module_name()) {
			debugger.remove_breakpoint(breakpoint.id);
		}
	}
	const auto info = debugger.ast().module_info(module);
	if (const JsonArray* breakpoints = arguments.get_array("breakpoints")) {
		for (const Json& breakpoint : *breakpoints) {
			if (mint::DebugInfo* debug_info = info.debug_info;
			    debug_info && info.state != mint::Module::State::not_compiled) {
				const std::size_t line_number = debug_info->to_executable_line_number(
				    to_line_number(*breakpoint.if_object()->get_number("line")));
				debugger.create_breakpoint({info.id, module, line_number});
			}
			else {
				const std::size_t line_number = to_line_number(*breakpoint.if_object()->get_number("line"));
				std::println(Logger::default_logger(), "New pending breakpoint {}:{}", file_path, line_number);
				debugger.add_pending_breakpoint_from_file(file_path, line_number);
			}
		}
	}
	else if (const JsonArray* lines = arguments.get_array("lines")) {
		for (const Json& line : *lines) {
			if (mint::DebugInfo* debug_info = info.debug_info;
			    debug_info && info.state != mint::Module::State::not_compiled) {
				const std::size_t line_number = debug_info->to_executable_line_number(to_line_number(*line.if_number()));
				debugger.create_breakpoint({info.id, module, line_number});
			}
			else {
				const std::size_t line_number = to_line_number(*line.if_number());
				std::println(Logger::default_logger(), "New pending breakpoint {}:{}", file_path, line_number);
				debugger.add_pending_breakpoint_from_file(file_path, line_number);
			}
		}
	}
	auto actual_breakpoints = JsonArray();
	for (const mint::BreakpointList breakpoints = debugger.get_breakpoints();
	    const mint::Breakpoint& breakpoint : breakpoints) {
		if (module == breakpoint.info.module_name()) {
			actual_breakpoints.push_back(JsonObject {
			    {"verified", JsonBoolean(true)},
			    {"id", JsonNumber(to_client_id(breakpoint.id))},
			    {"line", JsonNumber(to_client_line_number(breakpoint.info.line_number()))},
			});
		}
	}
	send_response(request, JsonObject {
	                           {"breakpoints", actual_breakpoints},
	                       });
}

void DapDebugger::on_threads(const DapRequestMessage& request, const JsonObject& /*arguments*/, Debugger& debugger) {
	auto threads = JsonArray(std::from_range,
	    std::views::transform(debugger.get_threads(), [](const mint::CursorDebugger& thread) {
		    return JsonObject {
		        {"id", JsonNumber(to_client_id(thread.get_thread_id()))},
		        {"name", JsonString("Thread " + std::to_string(thread.get_thread_id()))},
		    };
	    }));
	send_response(request, JsonObject {
	                           {"threads", threads},
	                       });
}

void DapDebugger::on_stack_trace(const DapRequestMessage& request, const JsonObject& arguments, Debugger& debugger) {
	if (const mint::CursorDebugger* cursor = debugger.find_thread(to_thread_id(*arguments.get_number("threadId")))) {
		const mint::LineInfoList& call_stack = cursor->cursor().dump();
		std::size_t i = 0;
		if (const JsonNumber* start_frame = arguments.get_number("startFrame")) {
			i = *start_frame;
		}
		std::size_t count = call_stack.size();
		if (const JsonNumber* levels = arguments.get_number("levels")) {
			if (auto value = static_cast<std::size_t>(*levels)) {
				count = std::min(i + value, count);
			}
		}
		auto stack_frames = JsonArray();
		if (count && i == 0) {
			const std::filesystem::path system_path = mint::to_system_path(cursor->module_name());
			auto stack_frame = JsonObject {
			    {"id", JsonNumber(to_client_id(to_stack_frame_id(cursor->get_thread_id(), i)))},
			    {"name", JsonString("Stack frame " + std::to_string(i) + ": module '" + cursor->module_name()
			                        + "', line " + std::to_string(cursor->line_number()))},
			    {"moduleId", JsonNumber(to_client_id(cursor->module_id()))},
			};
			if (!system_path.empty()) {
				stack_frame.emplace("source", JsonObject {
				                                  {"name", JsonString(cursor->system_file_name().generic_string())},
				                                  {"path", JsonString(cursor->system_path().generic_string())},
				                              });
				stack_frame.emplace("line", JsonNumber(to_client_line_number(cursor->line_number())));
				stack_frame.emplace("column", JsonNumber(to_client_column_number(1)));
			}
			stack_frames.push_back(stack_frame);
			++i;
		}
		for (; i < count; ++i) {
			const std::filesystem::path system_path = mint::to_system_path(call_stack[i].module_name());
			auto stack_frame = JsonObject {
			    {"id", JsonNumber(to_client_id(to_stack_frame_id(cursor->get_thread_id(), i)))},
			    {"name", JsonString("Stack frame " + std::to_string(i) + ": module '" + call_stack[i].module_name()
			                        + "', line " + std::to_string(call_stack[i].line_number()))},
			    {"moduleId", JsonNumber(to_client_id(call_stack[i].module_id()))},
			};
			if (!system_path.empty()) {
				stack_frame.emplace("source",
				    JsonObject {
				        {"name", JsonString(call_stack[i].system_file_name().generic_string())},
				        {"path", JsonString(call_stack[i].system_path().generic_string())},
				    });
				stack_frame.emplace("line", JsonNumber(to_client_line_number(call_stack[i].line_number())));
				stack_frame.emplace("column", JsonNumber(to_client_column_number(1)));
			}
			stack_frames.push_back(stack_frame);
		}
		send_response(request, JsonObject {
		                           {"stackFrames", stack_frames},
		                           {"totalFrames", JsonNumber(call_stack.size())},
		                       });
	}
}

void DapDebugger::on_breakpoint_locations(const DapRequestMessage& request, const JsonObject& arguments,
    Debugger& debugger) {
	auto breakpoints = JsonArray();
	const auto from_line = to_line_number(*arguments.get_number("line"));
	const auto to_line = attribute_or_default(arguments.get_number("endLine"), from_line);
	const std::string module = mint::to_module_path(*arguments.get_object("source")->get_string("path"));
	if (mint::DebugInfo* info = debugger.ast().module_info(module).debug_info) {
		for (std::size_t line = info->to_executable_line_number(from_line); line >= from_line && line <= to_line;
		    line = info->to_executable_line_number(line + 1)) {
			breakpoints.push_back(JsonObject {
			    {"line", JsonNumber(to_client_line_number(line))},
			});
		}
	}
	send_response(request, JsonObject {
	                           {"breakpoints", breakpoints},
	                       });
}

void DapDebugger::on_scopes(const DapRequestMessage& request, const JsonObject& arguments, Debugger& debugger) {
	const auto frame_id = to_frame_id(*arguments.get_number("frameId"));
	const auto [thread_id, frame_index] = from_stack_frame_id(frame_id);
	if (const mint::CursorDebugger* thread = debugger.find_thread(thread_id)) {
		auto scopes = JsonArray();
		if (const mint::SymbolTable* symbols = thread->symbols(frame_index)) {
			const mint::LineInfo& state = thread->line_info(frame_index);
			scopes.push_back(JsonObject {
			    {"name", JsonString("Locals")},
			    {"presentationHint", JsonString("locals")},
			    {"variablesReference", JsonNumber(to_client_id(register_frame_variables_reference(frame_id)))},
			    {"namedVariables", JsonNumber(symbols->size())},
			    {"expensive", JsonBoolean(false)},
			    {
			        "source",
			        JsonObject {
			            {"name", JsonString(state.system_file_name().generic_string())},
			            {"path", JsonString(state.system_path().generic_string())},
			        },
			    },
			});
		}
		send_response(request, JsonObject {
		                           {"scopes", scopes},
		                       });
	}
}

void DapDebugger::on_variables(const DapRequestMessage& request, const JsonObject& arguments, Debugger& debugger) {

	const auto variables_reference_id = to_thread_id(*arguments.get_number("variablesReference"));
	if (variables_reference_id >= _variables.size()) {
		return;
	}

	const auto& variables_reference = _variables[variables_reference_id];
	const auto [thread_id, frame_index] = from_stack_frame_id(variables_reference.frame_id);
	if (const auto* thread = debugger.find_thread(thread_id)) {
		auto variables = JsonArray();
		if (const mint::SymbolTable* symbols = thread->symbols(frame_index)) {
			if (mint::Object* object = variables_reference.object) {
				for (auto [symbol, member] : object->metadata.members()) {
					if (member.get().offset == mint::Class::MemberInfo::invalid_offset) {
						continue;
					}
					auto& reference = mint::Class::MemberInfo::get(member.get(), *object);
					if (is_instance_of(reference, mint::Data::Format::object)
					    && !reference.data<mint::Object>().metadata.slots().empty()) {
						variables.push_back(JsonObject {
						    {"name", JsonString(symbol.str())},
						    {"value", JsonString(reference_value(reference))},
						    {"type", JsonString(type_name(reference))},
						    {"variablesReference",
						        JsonNumber(to_client_id(register_frame_variables_reference(variables_reference.frame_id,
						            &reference.data<mint::Object>())))},
						});
					}
					else {
						variables.push_back(JsonObject {
						    {"name", JsonString(symbol.str())},
						    {"value", JsonString(reference_value(reference))},
						    {"type", JsonString(type_name(reference))},
						    {"variablesReference", JsonNumber(0)},
						});
					}
				}
			}
			else {
				for (const auto& [symbol, reference] : *symbols) {
					if (is_instance_of(reference, mint::Data::Format::object)
					    && !reference.data<mint::Object>().metadata.slots().empty()) {
						variables.push_back(JsonObject {
						    {"name", JsonString(symbol.str())},
						    {"value", JsonString(reference_value(reference))},
						    {"type", JsonString(type_name(reference))},
						    {"variablesReference",
						        JsonNumber(to_client_id(register_frame_variables_reference(variables_reference.frame_id,
						            &reference.data<mint::Object>())))},
						});
					}
					else {
						variables.push_back(JsonObject {
						    {"name", JsonString(symbol.str())},
						    {"value", JsonString(reference_value(reference))},
						    {"type", JsonString(type_name(reference))},
						    {"variablesReference", JsonNumber(0)},
						});
					}
				}
			}
		}
		send_response(request, JsonObject {
		                           {"variables", variables},
		                       });
	}
}

void DapDebugger::on_continue(const DapRequestMessage& request, const JsonObject& arguments, Debugger& debugger) {
	if (mint::CursorDebugger* cursor = debugger.find_thread(to_thread_id(*arguments.get_number("threadId")))) {
		debugger.do_run(*cursor);
		send_response(request);
	}
}

void DapDebugger::on_next(const DapRequestMessage& request, const JsonObject& arguments, Debugger& debugger) {
	if (mint::CursorDebugger* cursor = debugger.find_thread(to_thread_id(*arguments.get_number("threadId")))) {
		debugger.do_next(*cursor);
		send_response(request);
	}
}

void DapDebugger::on_step_in(const DapRequestMessage& request, const JsonObject& arguments, Debugger& debugger) {
	if (mint::CursorDebugger* cursor = debugger.find_thread(to_thread_id(*arguments.get_number("threadId")))) {
		debugger.do_enter(*cursor);
		send_response(request);
	}
}

void DapDebugger::on_step_out(const DapRequestMessage& request, const JsonObject& arguments, Debugger& debugger) {
	if (mint::CursorDebugger* cursor = debugger.find_thread(to_thread_id(*arguments.get_number("threadId")))) {
		debugger.do_return(*cursor);
		send_response(request);
	}
}

void DapDebugger::on_pause(const DapRequestMessage& request, const JsonObject& arguments, Debugger& debugger) {
	if (mint::CursorDebugger* cursor = debugger.find_thread(to_thread_id(*arguments.get_number("threadId")))) {
		debugger.do_pause(*cursor);
		send_response(request);
		send_event("stopped", JsonObject {
		                          {"reason", JsonString("pause")},
		                          {"threadId", JsonNumber(to_client_id(cursor->get_thread_id()))},
		                          {"preserveFocusHint", JsonBoolean(false)},
		                          {"allThreadsStopped", JsonBoolean(true)},
		                      });
		_variables.clear();
	}
}

void DapDebugger::on_disconnect(const DapRequestMessage& request, const JsonObject& arguments, Debugger& /*debugger*/) {
	if (const JsonBoolean* restart = arguments.get_boolean("restart")) {
		if (*restart) {
			/// \todo handle restart
		}
		else {
			shutdown();
		}
	}
	else {
		shutdown();
	}
	send_response(request);
}

void DapDebugger::on_terminate(const DapRequestMessage& request, const JsonObject& arguments, Debugger& /*debugger*/) {
	if (const JsonBoolean* restart = arguments.get_boolean("restart")) {
		if (*restart) {
			/// \todo handle restart
		}
		else {
			shutdown();
		}
	}
	else {
		shutdown();
	}
	send_response(request);
}

void DapDebugger::on_initialize(const DapRequestMessage& request, const JsonObject& arguments, Debugger& /*debugger*/,
    mint::Scheduler& /*scheduler*/) {
	if (const JsonBoolean* columns_start_at1 = arguments.get_boolean("columnsStartAt1")) {
		_client_columns_start_at_1 = *columns_start_at1;
	}
	if (const JsonBoolean* lines_start_at1 = arguments.get_boolean("linesStartAt1")) {
		_client_lines_start_at_1 = *lines_start_at1;
	}
	if (const JsonString* path_format = arguments.get_string("pathFormat")) {
		if (*path_format != "path") {
			send_error(request, 2018, "debug adapter only supports native paths", std::nullopt, telemetry);
			return;
		}
	}
	send_response(request, JsonObject {
	                           {"supportsConfigurationDoneRequest", JsonBoolean(true)},
	                           {"supportsFunctionBreakpoints", JsonBoolean(false)},
	                           {"supportsConditionalBreakpoints", JsonBoolean(false)},
	                           {"supportsHitConditionalBreakpoints", JsonBoolean(false)},
	                           {"supportsEvaluateForHovers", JsonBoolean(false)},
	                           {"supportsStepBack", JsonBoolean(false)},
	                           {"supportsSetVariable", JsonBoolean(false)},
	                           {"supportsRestartFrame", JsonBoolean(false)},
	                           {"supportsStepInTargetsRequest", JsonBoolean(true)},
	                           {"supportsGotoTargetsRequest", JsonBoolean(false)},
	                           {"supportsCompletionsRequest", JsonBoolean(false)},
	                           {"supportsRestartRequest", JsonBoolean(false)},
	                           {"supportsExceptionOptions", JsonBoolean(false)},
	                           {"supportsValueFormattingOptions", JsonBoolean(false)},
	                           {"supportsExceptionInfoRequest", JsonBoolean(false)},
	                           {"supportTerminateDebuggee", JsonBoolean(false)},
	                           {"supportsDelayedStackTraceLoading", JsonBoolean(false)},
	                           {"supportsLoadedSourcesRequest", JsonBoolean(false)},
	                           {"supportsLogPoints", JsonBoolean(false)},
	                           {"supportsTerminateThreadsRequest", JsonBoolean(false)},
	                           {"supportsSetExpression", JsonBoolean(false)},
	                           {"supportsTerminateRequest", JsonBoolean(true)},
	                           {"supportsDataBreakpoints", JsonBoolean(false)},
	                           {"supportsReadMemoryRequest", JsonBoolean(false)},
	                           {"supportsDisassembleRequest", JsonBoolean(false)},
	                           {"supportsCancelRequest", JsonBoolean(false)},
	                           {"supportsBreakpointLocationsRequest", JsonBoolean(true)},
	                           {"supportsClipboardContext", JsonBoolean(false)},
	                           {"supportsSteppingGranularity", JsonBoolean(false)},
	                           {"supportsInstructionBreakpoints", JsonBoolean(false)},
	                           {"supportsExceptionFilterOptions", JsonBoolean(false)},
	                       });
	send_event("initialized");
}

void DapDebugger::on_launch(const DapRequestMessage& request, const JsonObject& arguments, Debugger& debugger,
    mint::Scheduler& scheduler) {

	if (const JsonString* program = arguments.get_string("program")) {
		if (auto process = mint::Process::from_main_file(scheduler, *program)) {
			process->parse_argument(*program);
			if (const JsonArray* args = arguments.get_array("args")) {
				for (const Json& argv : *args) {
					process->parse_argument(*argv.if_string());
				}
			}
			if (const JsonBoolean* stop_on_entry = arguments.get_boolean("stopOnEntry")) {
				if (*stop_on_entry) {
					debugger.pause_on_next_step();
				}
			}
			scheduler.push_waiting_process(std::move(process));
			send_response(request);
			_configuring = false;
		}
		else {
			send_error(request, 1001, "compile error.", std::nullopt, user);
			_configuring = false;
		}
	}
}

void DapDebugger::on_configuration_done(const DapRequestMessage& request, const JsonObject& /*arguments*/,
    Debugger& /*debugger*/, mint::Scheduler& /*scheduler*/) {
	send_response(request);
}

std::size_t DapDebugger::register_frame_variables_reference(std::size_t frame_id, mint::Object* object) {
	const auto variables_reference_id = _variables.size();
	_variables.push_back({
	    .frame_id = frame_id,
	    .object = object,
	});
	return variables_reference_id;
}
