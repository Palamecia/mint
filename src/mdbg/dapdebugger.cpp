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

#include "dapdebugger.h"
#include "debugprinter.h"

#include <mint/debug/debugtool.h>
#include <mint/memory/memorytool.h>
#include <mint/system/filesystem.h>
#include <mint/system/stdio.h>

#include <cstdarg>
#include <regex>

#ifdef OS_UNIX
#include <sys/file.h>
#include <unistd.h>
#include <poll.h>
#endif

using namespace mint;
using namespace std;

unordered_map<string, DapDebugger::Command> DapDebugger::g_commands = {
	{ "setBreakpoints", { &DapDebugger::set_breakpoints, Async } },
	{ "threads", { &DapDebugger::threads, Async } },
	{ "stackTrace", { &DapDebugger::stack_trace, Async } },
	{ "breakpointLocations", { &DapDebugger::breakpoint_locations, Async } },
	{ "scopes", { &DapDebugger::scopes, Async } },
	{ "variables", { &DapDebugger::variables, Async } },
	{ "continue", { &DapDebugger::_continue } },
	{ "next", { &DapDebugger::_next } },
	{ "stepIn", { &DapDebugger::_stepIn } },
	{ "stepOut", { &DapDebugger::_stepOut } },
	{ "pause", { &DapDebugger::_pause } },
	{ "disconnect", { &DapDebugger::disconnect } },
	{ "terminate", { &DapDebugger::terminate } },
};

unordered_map<string, DapDebugger::SetupCommand> DapDebugger::g_setup_commands = {
	{ "initialize", { &DapDebugger::initialize } },
	{ "launch", { &DapDebugger::launch, Async } },
	{ "configurationDone", { &DapDebugger::configuration_done } },
};

unordered_map<string, DapDebugger::RuntimeCommand> DapDebugger::g_runtime_commands = {

};

static size_t to_stack_frame_id(size_t thread_id, size_t frame_index) {
	return thread_id * 0xFFFF + frame_index % 0xFFFF;
}

static std::tuple<Process::ThreadId, size_t> from_stack_frame_id(size_t frame_id) {
	return { static_cast<Process::ThreadId>(frame_id / 0xFFFF), frame_id % 0xFFFF };
}

static const char *log_file_path() {
#ifdef OS_WINDOWS
	return "C:/mint/mdbg.log";
#else
	return "/tmp/mdbg.log";
#endif
}

DapDebugger::DapDebugger(DapMessageReader *reader, DapMessageWriter *writer) :
	m_reader(reader), m_writer(writer),
	m_logger(fopen(log_file_path(), "w")),
	m_stdin(stdin_fileno),
	m_stdout(stdout_fileno),
	m_stderr(stderr_fileno) {
	write_log("Start debugger");
}

DapDebugger::~DapDebugger() {
	write_log("Stop debugger");
	fclose(m_logger);
}

bool DapDebugger::setup(DebugInterface *debugger, Scheduler *scheduler) {

	while (m_running && m_configuring) {
		
		if (m_stderr.can_read()) {
			send_event("output", new JsonObject({
				{ "category", new JsonString("stderr") },
				{ "output", new JsonString(m_stderr.read()) }
			}));
		}

		update_async_commands();

		if (unique_ptr<DapMessage> message = m_reader->nextMessage()) {
			write_log("From client: %s", message->encode().c_str());
			switch (message->get_type()) {
			case DapMessage::request:
				if (!dispatch_request(unique_ptr<DapRequestMessage>(static_cast<DapRequestMessage *>(message.release())), debugger, scheduler)) {
					write_log("Unknown request");
				}
				break;
			case DapMessage::response:
			case DapMessage::event:
				break;
			}
		}
	}

	return m_running;
}

bool DapDebugger::handle_events(DebugInterface *debugger, CursorDebugger *cursor) {

	AbstractSyntaxTree *ast = cursor->cursor()->ast();
	
	while (DebugInfo *info = ast->get_debug_info(m_module_count)) {
		Module *module = ast->get_module(m_module_count);
		Module::Id module_id = ast->get_module_id(module);
		if (module_id != Module::invalid_id) {
			const string &module_name = ast->get_module_name(module);
			const string &system_path = to_system_path(module_name);
			if (!system_path.empty()) {
				send_event("loadedSource", new JsonObject({
					{ "reason", new JsonString("new") },
					{ "source", new JsonObject({
						{ "name", new JsonString(system_path.substr(system_path.rfind(FileSystem::separator) + 1)) },
						{ "path", new JsonString(system_path) }
					})}
				}));
			}
			send_event("module", new JsonObject({
				{ "reason", new JsonString("new") },
				{ "module", new JsonObject({
					{ "id", new JsonNumber(to_client_id(module_id)) },
					{ "name", new JsonString(module_name) },
				})}
			}));
		}
		++m_module_count;
	}
	
	if (m_pause_on_next_step) {
		m_pause_on_next_step = false;
		debugger->do_pause(cursor);
		send_event("stopped", new JsonObject({
			{ "reason", new JsonString("pause") },
			{ "threadId", new JsonNumber(to_client_id(cursor->get_thread_id())) },
			{ "preserveFocusHint", new JsonBoolean(false) },
			{ "allThreadsStopped", new JsonBoolean(true) }
		}));
		m_variables.clear();
	}
	
	if (m_stdout.can_read()) {
		send_event("output", new JsonObject({
			{ "category", new JsonString("stdout") },
			{ "output", new JsonString(m_stdout.read()) }
		}));
	}
	
	if (m_stderr.can_read()) {
		send_event("output", new JsonObject({
			{ "category", new JsonString("stderr") },
			{ "output", new JsonString(m_stderr.read()) }
		}));
	}

	update_async_commands();

	while (unique_ptr<DapMessage> message = m_reader->nextMessage()) {
		write_log("From client: %s", message->encode().c_str());
		switch (message->get_type()) {
		case DapMessage::request:
			if (!dispatch_request(unique_ptr<DapRequestMessage>(static_cast<DapRequestMessage *>(message.release())), debugger, cursor)) {
				write_log("Unknown request");
			}
			break;
		case DapMessage::response:
		case DapMessage::event:
			break;
		}
	}

	return m_running;
}

bool DapDebugger::check(DebugInterface *debugger, CursorDebugger *cursor) {

	update_async_commands();
	update_pending_breakpoints(debugger);

	while (unique_ptr<DapMessage> message = m_reader->nextMessage()) {
		write_log("From client: %s", message->encode().c_str());
		switch (message->get_type()) {
		case DapMessage::request:
			if (!dispatch_request(unique_ptr<DapRequestMessage>(static_cast<DapRequestMessage *>(message.release())), debugger, cursor)) {
				write_log("Unknown request");
			}
			break;
		case DapMessage::response:
		case DapMessage::event:
			break;
		}
	}

	return m_running;
}

void DapDebugger::cleanup(DebugInterface *debugger, Scheduler *scheduler) {
	send_event("terminated");
}

void DapDebugger::on_thread_started(DebugInterface *debugger, CursorDebugger *cursor) {
	send_event("thread", new JsonObject({
		{ "reason", new JsonString("started") },
		{ "threadId", new JsonNumber(to_client_id(cursor->get_thread_id())) }
	}));
}

void DapDebugger::on_thread_exited(DebugInterface *debugger, CursorDebugger *cursor) {
	send_event("thread", new JsonObject({
		{ "reason", new JsonString("exited") },
		{ "threadId", new JsonNumber(to_client_id(cursor->get_thread_id())) }
	}));
}

bool DapDebugger::on_breakpoint(DebugInterface *debugger, CursorDebugger *cursor, const unordered_set<Breakpoint::Id> &breakpoints) {
	send_event("stopped", new JsonObject({
		{ "reason", new JsonString("breakpoint") },
		{ "threadId", new JsonNumber(to_client_id(cursor->get_thread_id())) },
		{ "preserveFocusHint", new JsonBoolean(false) },
		{ "allThreadsStopped", new JsonBoolean(true) },
		{ "hitBreakpointIds", std::invoke([&breakpoints] {
				JsonArray *array = new JsonArray;
				for (Breakpoint::Id id : breakpoints) {
					array->emplace_back(new JsonNumber(id));
				}
				return array;
			})}
	}));
	m_variables.clear();
	return true;
}

bool DapDebugger::on_exception(DebugInterface *debugger, CursorDebugger *cursor) {
	send_event("stopped", new JsonObject({
		{ "reason", new JsonString("exception") },
		{ "threadId", new JsonNumber(to_client_id(cursor->get_thread_id())) },
		{ "preserveFocusHint", new JsonBoolean(false) },
		{ "allThreadsStopped", new JsonBoolean(true) },
		{ "hitBreakpointIds", new JsonArray }
	}));
	m_variables.clear();
	return true;
}

bool DapDebugger::on_step(DebugInterface *debugger, CursorDebugger *cursor) {
	send_event("stopped", new JsonObject({
		{ "reason", new JsonString("step") },
		{ "threadId", new JsonNumber(to_client_id(cursor->get_thread_id())) },
		{ "preserveFocusHint", new JsonBoolean(false) },
		{ "allThreadsStopped", new JsonBoolean(true) },
		{ "hitBreakpointIds", new JsonArray }
	}));
	m_variables.clear();
	return true;
}

void DapDebugger::shutdown() {
	m_running = false;
}

bool DapDebugger::dispatch_request(std::unique_ptr<DapRequestMessage> message, DebugInterface *debugger, Scheduler *scheduler) {
	if (auto it = g_commands.find(message->get_command()); it != g_commands.end()) {
		const JsonObject *arguments = message->get_arguments();
		call_command(it->second, std::move(message), arguments, debugger);
		return true;
	}
	if (auto it = g_setup_commands.find(message->get_command()); it != g_setup_commands.end()) {
		const JsonObject *arguments = message->get_arguments();
		call_command(it->second, std::move(message), arguments, debugger, scheduler);
		return true;
	}
	return false;
}

bool DapDebugger::dispatch_request(std::unique_ptr<DapRequestMessage> message, DebugInterface *debugger, CursorDebugger *cursor) {
	if (auto it = g_commands.find(message->get_command()); it != g_commands.end()) {
		const JsonObject *arguments = message->get_arguments();
		call_command(it->second, std::move(message), arguments, debugger);
		return true;
	}
	if (auto it = g_runtime_commands.find(message->get_command()); it != g_runtime_commands.end()) {
		const JsonObject *arguments = message->get_arguments();
		call_command(it->second, std::move(message), arguments, debugger, cursor);
		return true;
	}
	return false;
}

void DapDebugger::send_event(const string &event, JsonObject *body) {

	unique_ptr<DapMessage> message(new DapEventMessage(event, body));
	write_log("To client: %s", message->encode().c_str());

	unique_lock<mutex> lock(m_writeMutex);
	m_writer->appendMessage(std::move(message));
}

void DapDebugger::send_response(const DapRequestMessage *request, JsonObject *body) {

	unique_ptr<DapMessage> message(new DapResponseMessage(request, body));
	write_log("To client: %s", message->encode().c_str());

	unique_lock<mutex> lock(m_writeMutex);
	m_writer->appendMessage(std::move(message));
}

static string formatPII(string format, const JsonObject *variables) {

	static const regex g_format_pii_regexp("{([^}]+)}");

	smatch match;
	while (regex_search(format, match, g_format_pii_regexp)) {
		if (const JsonString *value = variables->get_string(match.str(1))) {
			format.replace(match.position(), match.length(), *value);
		}
	}
	return format;
}

void DapDebugger::send_error(const DapRequestMessage *request, int code, const string &format, JsonObject *variables, ErrorDestination destination) {

	JsonObject *error = new JsonObject({
		{ "id", new JsonNumber(code) },
		{ "format", new JsonString(format) }
	});
	if (variables) {
		error->emplace("variables", variables);
	}
	if (destination & User) {
		error->emplace("showUser", new JsonBoolean(true));
	}
	if (destination & Telemetry) {
		error->emplace("sendTelemetry", new JsonBoolean(true));
	}

	unique_ptr<DapMessage> message(new DapResponseMessage(request, formatPII(format, variables), error));
	write_log("To client: %s", message->encode().c_str());

	unique_lock<mutex> lock(m_writeMutex);
	m_writer->appendMessage(std::move(message));
}

size_t DapDebugger::from_client_column_number(size_t number) const {
	return m_client_columns_start_at_1 ? number : number + 1;
}

size_t DapDebugger::to_client_column_number(size_t number) const {
	return m_client_columns_start_at_1 ? number : number - 1;
}

size_t DapDebugger::from_client_line_number(size_t number) const {
	return m_client_lines_start_at_1 ? number : number + 1;
}

size_t DapDebugger::to_client_line_number(size_t number) const {
	return m_client_lines_start_at_1 ? number : number - 1;
}

size_t DapDebugger::from_client_id(size_t id) const {
	return id - 1;
}

size_t DapDebugger::to_client_id(size_t id) const {
	return id + 1;
}

void DapDebugger::set_breakpoints(unique_ptr<DapRequestMessage> request, const JsonObject *arguments, DebugInterface *debugger) {
	const string file_path = *arguments->get_object("source")->get_string("path");
	const string module = to_module_path(file_path);
	{
		const BreakpointList &breakpoints = debugger->get_breakpoints();
		for (const Breakpoint &breakpoint : breakpoints) {
			if (module == breakpoint.info.module_name()) {
				debugger->remove_breakpoint(breakpoint.id);
			}
		}
	}
	Module::Info info = Scheduler::instance()->ast()->module_info(module);
	if (const JsonArray *breakpoints = arguments->get_array("breakpoints")) {
		for (const Json *breakpoint : *breakpoints) {
			if (DebugInfo *debug_info = info.debug_info; debug_info && info.state != Module::not_compiled) {
				size_t line_number = debug_info->to_executable_line_number(from_client_line_number(*breakpoint->toObject()->get_number("line")));
				debugger->create_breakpoint({info.id, module, line_number});
			}
			else {
				m_pending_breakpoints.emplace_back(file_path, from_client_line_number(*breakpoint->toObject()->get_number("line")));
				write_log("New pending breakpoint %s:%zu", m_pending_breakpoints.back().first.c_str(), m_pending_breakpoints.back().second);
			}
		}
	}
	else if (const JsonArray *lines = arguments->get_array("lines")) {
		for (const Json *line : *lines) {
			if (DebugInfo *debug_info = info.debug_info; debug_info && info.state != Module::not_compiled) {
				size_t line_number = debug_info->to_executable_line_number(from_client_line_number(*line->to_number()));
				debugger->create_breakpoint({info.id, module, line_number});
			}
			else {
				m_pending_breakpoints.emplace_back(file_path, from_client_line_number(*line->to_number()));
				write_log("New pending breakpoint %s:%zu", m_pending_breakpoints.back().first.c_str(), m_pending_breakpoints.back().second);
			}
		}
	}
	JsonArray *actual_breakpoints = new JsonArray;
	{
		const BreakpointList breakpoints = debugger->get_breakpoints();
		for (const Breakpoint &breakpoint : breakpoints) {
			if (module == breakpoint.info.module_name()) {
				actual_breakpoints->push_back(new JsonObject {
					{ "verified", new JsonBoolean(true) },
					{ "id", new JsonNumber(to_client_id(breakpoint.id)) },
					{ "line", new JsonNumber(to_client_line_number(breakpoint.info.line_number())) }
				});
			}
		}
	}
	send_response(request.get(), new JsonObject({
		{ "breakpoints", actual_breakpoints }
	}));
}

void DapDebugger::threads(unique_ptr<DapRequestMessage> request, const JsonObject *arguments, DebugInterface *debugger) {
	JsonArray *threads = new JsonArray;
	for (CursorDebugger *thread : debugger->get_threads()) {
		threads->push_back(new JsonObject {
			{ "id", new JsonNumber(to_client_id(thread->get_thread_id())) },
			{ "name", new JsonString("Thread " + to_string(thread->get_thread_id())) }
		});
	}
	send_response(request.get(), new JsonObject {
		{ "threads", threads }
	});
}

void DapDebugger::stack_trace(std::unique_ptr<DapRequestMessage> request, const JsonObject *arguments, DebugInterface *debugger) {
	if (CursorDebugger *cursor = debugger->get_thread(from_client_id(*arguments->get_number("threadId")))) {
		const LineInfoList &call_stack = cursor->cursor()->dump();
		size_t i = 0;
		if (const JsonNumber *startFrame = arguments->get_number("startFrame")) {
			i = *startFrame;
		}
		size_t count = call_stack.size();
		if (const JsonNumber *levels = arguments->get_number("levels")) {
			if (size_t value = static_cast<size_t>(*levels)) {
				count = std::min(i + value, count);
			}
		}
		JsonArray *stack_frames = new JsonArray;
		if (count && i == 0) {
			const string &system_path = to_system_path(cursor->module_name());
			auto stack_frame = new JsonObject {
				{ "id", new JsonNumber(to_client_id(to_stack_frame_id(cursor->get_thread_id(), i))) },
				{ "name", new JsonString("Stack frame " + to_string(i)) },
				{ "moduleId", new JsonNumber(to_client_id(cursor->module_id())) },
			};
			if (!system_path.empty()) {
				stack_frame->emplace("source", new JsonObject {
					{ "name", new JsonString(cursor->system_file_name()) },
					{ "path", new JsonString(cursor->system_path()) }
				});
				stack_frame->emplace("line", new JsonNumber(to_client_line_number(cursor->line_number())));
				stack_frame->emplace("column", new JsonNumber(to_client_column_number(1)));
			}
			stack_frames->push_back(stack_frame);
			++i;
		}
		for (; i < count; ++i) {
			const string &system_path = to_system_path(call_stack[i].module_name());
			auto stack_frame = new JsonObject {
				{ "id", new JsonNumber(to_client_id(to_stack_frame_id(cursor->get_thread_id(), i))) },
				{ "name", new JsonString("Stack frame " + to_string(i)) },
				{ "moduleId", new JsonNumber(to_client_id(call_stack[i].module_id())) },
			};
			if (!system_path.empty()) {
				stack_frame->emplace("source", new JsonObject {
					{ "name", new JsonString(call_stack[i].system_file_name()) },
					{ "path", new JsonString(call_stack[i].system_path()) }
				});
				stack_frame->emplace("line", new JsonNumber(to_client_line_number(call_stack[i].line_number())));
				stack_frame->emplace("column", new JsonNumber(to_client_column_number(1)));
			}
			stack_frames->push_back(stack_frame);
		}
		send_response(request.get(), new JsonObject {
			{ "stackFrames", stack_frames },
			{ "totalFrames", new JsonNumber(call_stack.size()) }
		});
	}
}

void DapDebugger::breakpoint_locations(std::unique_ptr<DapRequestMessage> request, const JsonObject *arguments, DebugInterface *debugger) {
	JsonArray *breakpoints = new JsonArray;
	if (Scheduler *scheduler = Scheduler::instance()) {
		size_t from_line = from_client_line_number(*arguments->get_number("line"));
		size_t to_line = attribute_or_default(arguments->get_number("endLine"), from_line);
		const string module = to_module_path(*arguments->get_object("source")->get_string("path"));
		if (DebugInfo *info = scheduler->ast()->module_info(module).debug_info) {
			for (size_t line = info->to_executable_line_number(from_line); line >= from_line && line <= to_line; line = info->to_executable_line_number(line + 1)) {
				breakpoints->push_back(new JsonObject({
					{ "line", new JsonNumber(to_client_line_number(line)) }
				}));
			}
		}
	}
	send_response(request.get(), new JsonObject({
		{ "breakpoints", breakpoints }
	}));
}

void DapDebugger::scopes(std::unique_ptr<DapRequestMessage> request, const JsonObject *arguments, DebugInterface *debugger) {
	size_t frame_id = from_client_id(*arguments->get_number("frameId"));
	auto [thread_id, frame_index] = from_stack_frame_id(frame_id);
	if (CursorDebugger *thread = debugger->get_thread(thread_id)) {
		JsonArray *scopes = new JsonArray;
		if (const SymbolTable *symbols = thread->symbols(frame_index)) {
			const LineInfo &state = thread->line_info(frame_index);
			scopes->push_back(new JsonObject({
				{ "name", new JsonString("Locals") },
				{ "presentationHint", new JsonString("locals") },
				{ "variablesReference", new JsonNumber(to_client_id(register_frame_variables_reference(frame_id))) },
				{ "namedVariables", new JsonNumber(symbols->size()) },
				{ "expensive", new JsonBoolean(false) },
				{ "source", new JsonObject({
					{ "name", new JsonString(state.system_file_name()) },
					{ "path", new JsonString(state.system_path()) }
				})}
			}));
		}
		send_response(request.get(), new JsonObject({
			{ "scopes", scopes }
		}));
	}
}

void DapDebugger::variables(std::unique_ptr<DapRequestMessage> request, const JsonObject *arguments, DebugInterface *debugger) {
	variables_reference_t &variables_reference = m_variables[from_client_id(*arguments->get_number("variablesReference"))];
	auto [thread_id, frame_index] = from_stack_frame_id(variables_reference.frame_id);
	if (CursorDebugger *thread = debugger->get_thread(thread_id)) {
		JsonArray *variables = new JsonArray;
		if (const SymbolTable *symbols = thread->symbols(frame_index)) {
			if (Object *object = variables_reference.object) {
				for (auto &[symbol, member] : object->metadata->members()) {
					if (member->offset == Class::MemberInfo::invalid_offset) {
						continue;
					}
					auto &reference = Class::MemberInfo::get(member, object);
					if (reference.data()->format == Data::fmt_object && !reference.data<Object>()->metadata->slots().empty()) {
						variables->push_back(new JsonObject({
							{ "name", new JsonString(symbol.str()) },
							{ "value", new JsonString(reference_value(reference)) },
							{ "type", new JsonString(type_name(reference)) },
							{ "variablesReference", new JsonNumber(to_client_id(register_frame_variables_reference(variables_reference.frame_id, reference.data<Object>()))) }
						}));
					}
					else {
						variables->push_back(new JsonObject({
							{ "name", new JsonString(symbol.str()) },
							{ "value", new JsonString(reference_value(reference)) },
							{ "type", new JsonString(type_name(reference)) },
							{ "variablesReference", new JsonNumber(0) }
						}));
					}
				}
			}
			else {
				for (auto &[symbol, reference] : *symbols) {
					if (reference.data()->format == Data::fmt_object && !reference.data<Object>()->metadata->slots().empty()) {
						variables->push_back(new JsonObject({
							{ "name", new JsonString(symbol.str()) },
							{ "value", new JsonString(reference_value(reference)) },
							{ "type", new JsonString(type_name(reference)) },
							{ "variablesReference", new JsonNumber(to_client_id(register_frame_variables_reference(variables_reference.frame_id, reference.data<Object>()))) }
						}));
					}
					else {
						variables->push_back(new JsonObject({
							{ "name", new JsonString(symbol.str()) },
							{ "value", new JsonString(reference_value(reference)) },
							{ "type", new JsonString(type_name(reference)) },
							{ "variablesReference", new JsonNumber(0) }
						}));
					}
				}
			}
		}
		send_response(request.get(), new JsonObject({
			{ "variables", variables }
		}));
	}
}

void DapDebugger::_continue(std::unique_ptr<DapRequestMessage> request, const JsonObject *arguments, mint::DebugInterface *debugger) {
	if (CursorDebugger *cursor = debugger->get_thread(from_client_id(*arguments->get_number("threadId")))) {
		debugger->do_run(cursor);
		send_response(request.get());
	}
}

void DapDebugger::_next(std::unique_ptr<DapRequestMessage> request, const JsonObject *arguments, mint::DebugInterface *debugger) {
	if (CursorDebugger *cursor = debugger->get_thread(from_client_id(*arguments->get_number("threadId")))) {
		debugger->do_next(cursor);
		send_response(request.get());
	}
}

void DapDebugger::_stepIn(std::unique_ptr<DapRequestMessage> request, const JsonObject *arguments, mint::DebugInterface *debugger) {
	if (CursorDebugger *cursor = debugger->get_thread(from_client_id(*arguments->get_number("threadId")))) {
		debugger->do_enter(cursor);
		send_response(request.get());
	}
}

void DapDebugger::_stepOut(std::unique_ptr<DapRequestMessage> request, const JsonObject *arguments, mint::DebugInterface *debugger) {
	if (CursorDebugger *cursor = debugger->get_thread(from_client_id(*arguments->get_number("threadId")))) {
		debugger->do_return(cursor);
		send_response(request.get());
	}
}

void DapDebugger::_pause(std::unique_ptr<DapRequestMessage> request, const JsonObject *arguments, mint::DebugInterface *debugger) {
	if (CursorDebugger *cursor = debugger->get_thread(from_client_id(*arguments->get_number("threadId")))) {
		debugger->do_pause(cursor);
		send_response(request.get());
		send_event("stopped", new JsonObject({
			{ "reason", new JsonString("pause") },
			{ "threadId", new JsonNumber(to_client_id(cursor->get_thread_id())) },
			{ "preserveFocusHint", new JsonBoolean(false) },
			{ "allThreadsStopped", new JsonBoolean(true) }
		}));
		m_variables.clear();
	}
}

void DapDebugger::disconnect(unique_ptr<DapRequestMessage> request, const JsonObject *arguments, DebugInterface *debugger) {
	if (const JsonBoolean *restart = arguments->get_boolean("restart")) {
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
	send_response(request.get());
}

void DapDebugger::terminate(unique_ptr<DapRequestMessage> request, const JsonObject *arguments, DebugInterface *debugger) {
	if (const JsonBoolean *restart = arguments->get_boolean("restart")) {
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
	send_response(request.get());
}

void DapDebugger::initialize(unique_ptr<DapRequestMessage> request, const JsonObject *arguments, DebugInterface *debugger, Scheduler *scheduler) {
	if (const JsonBoolean *columnsStartAt1 = arguments->get_boolean("columnsStartAt1")) {
		m_client_columns_start_at_1 = *columnsStartAt1;
	}
	if (const JsonBoolean *linesStartAt1 = arguments->get_boolean("linesStartAt1")) {
		m_client_lines_start_at_1 = *linesStartAt1;
	}
	if (const JsonString *pathFormat = arguments->get_string("pathFormat")) {
		if (*pathFormat != "path") {
			send_error(request.get(), 2018, "debug adapter only supports native paths", nullptr, Telemetry);
			return;
		}
	}
	send_response(request.get(), new JsonObject({
		{ "supportsConfigurationDoneRequest", new JsonBoolean(true) },
		{ "supportsFunctionBreakpoints", new JsonBoolean(false) },
		{ "supportsConditionalBreakpoints", new JsonBoolean(false) },
		{ "supportsHitConditionalBreakpoints", new JsonBoolean(false) },
		{ "supportsEvaluateForHovers", new JsonBoolean(false) },
		{ "supportsStepBack", new JsonBoolean(false) },
		{ "supportsSetVariable", new JsonBoolean(false) },
		{ "supportsRestartFrame", new JsonBoolean(false) },
		{ "supportsStepInTargetsRequest", new JsonBoolean(true) },
		{ "supportsGotoTargetsRequest", new JsonBoolean(false) },
		{ "supportsCompletionsRequest", new JsonBoolean(false) },
		{ "supportsRestartRequest", new JsonBoolean(false) },
		{ "supportsExceptionOptions", new JsonBoolean(false) },
		{ "supportsValueFormattingOptions", new JsonBoolean(false) },
		{ "supportsExceptionInfoRequest", new JsonBoolean(false) },
		{ "supportTerminateDebuggee", new JsonBoolean(false) },
		{ "supportsDelayedStackTraceLoading", new JsonBoolean(false) },
		{ "supportsLoadedSourcesRequest", new JsonBoolean(false) },
		{ "supportsLogPoints", new JsonBoolean(false) },
		{ "supportsTerminateThreadsRequest", new JsonBoolean(false) },
		{ "supportsSetExpression", new JsonBoolean(false) },
		{ "supportsTerminateRequest", new JsonBoolean(true) },
		{ "supportsDataBreakpoints", new JsonBoolean(false) },
		{ "supportsReadMemoryRequest", new JsonBoolean(false) },
		{ "supportsDisassembleRequest", new JsonBoolean(false) },
		{ "supportsCancelRequest", new JsonBoolean(false) },
		{ "supportsBreakpointLocationsRequest", new JsonBoolean(true) },
		{ "supportsClipboardContext", new JsonBoolean(false) },
		{ "supportsSteppingGranularity", new JsonBoolean(false) },
		{ "supportsInstructionBreakpoints", new JsonBoolean(false) },
		{ "supportsExceptionFilterOptions", new JsonBoolean(false) }
	}));
	send_event("initialized");
}

void DapDebugger::launch(unique_ptr<DapRequestMessage> request, const JsonObject *arguments, DebugInterface *debugger, Scheduler *scheduler) {

	mutex configurationDoneMutex;
	unique_lock<mutex> lock(configurationDoneMutex);
	m_configuration_done.wait_for(lock, 1000ms);

	if (const JsonString *program = arguments->get_string("program")) {
		Process *process = Process::from_main_file(scheduler->ast(), *program);
		if (!process) {
			send_error(request.get(), 1001, "compile error.", nullptr, User);
			return;
		}
		if (const JsonArray *args = arguments->get_array("args")) {
			for (Json *argv : *args) {
				process->parse_argument(*argv->to_string());
			}
		}
		if (const JsonBoolean *stop_on_entry = arguments->get_boolean("stopOnEntry")) {
			m_pause_on_next_step = *stop_on_entry;
		}
		scheduler->push_waiting_process(process);
		send_response(request.get());
		m_configuring = false;
	}
}

void DapDebugger::configuration_done(unique_ptr<DapRequestMessage> request, const JsonObject *arguments, DebugInterface *debugger, Scheduler *scheduler) {
	send_response(request.get());
	m_configuration_done.notify_one();
}

void DapDebugger::write_log(const char *format, ...) {

	va_list args;
	va_start(args, format);
	vfprintf(m_logger, format, args);
	va_end(args);

	fprintf(m_logger, "\n");
	fflush(m_logger);
}

void DapDebugger::update_async_commands() {
	for (auto it = m_async_commands.begin(); it != m_async_commands.end();) {
		if (it->wait_for(0ms) != std::future_status::timeout) {
			it = m_async_commands.erase(it);
		}
		else {
			++it;
		}
	}
}

DapDebugger::StdStreamPipe::StdStreamPipe(StdStreamFileNo number) {
#ifdef OS_WINDOWS
	if (CreatePipe(&m_handles[read_index], &m_handles[write_index], NULL, BUFSIZ)) {
		switch (number) {
		case stdin_fileno:
			SetStdHandle(STD_INPUT_HANDLE, m_handles[write_index]);
			break;
		case stdout_fileno:
			SetStdHandle(STD_OUTPUT_HANDLE, m_handles[write_index]);
			break;
		case stderr_fileno:
			SetStdHandle(STD_ERROR_HANDLE, m_handles[write_index]);
			break;
		}
	}
#else
	if (pipe(m_handles)) {
		dup2(number, m_handles[write_index]);
	}
#endif
}

bool DapDebugger::StdStreamPipe::can_read() const {
#ifdef OS_WINDOWS
	DWORD dwCount = 0;

	return PeekNamedPipe(m_handles[read_index], NULL, 0, NULL, &dwCount, NULL) && dwCount;
#else
	fd_set rfds;
	FD_ZERO(&rfds);
	FD_SET(m_handles[read_index], &rfds);

	timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 0;

	return ::select(1, &rfds, nullptr, nullptr, &tv) == 1;
#endif
}

string DapDebugger::StdStreamPipe::read() {
	char buf[BUFSIZ];
#ifdef OS_WINDOWS
	DWORD dwCount = 0;

	if (ReadFile(m_handles[read_index], buf, BUFSIZ, &dwCount, NULL)) {
		return buf;
	}
#else
	if (::read(m_handles[read_index], buf, BUFSIZ)) {
		return buf;
	}
#endif

	return {};
}

void DapDebugger::update_pending_breakpoints(DebugInterface *debugger) {
	for (auto it = m_pending_breakpoints.begin(); it != m_pending_breakpoints.end();) {
		auto &[file_path, line_number] = *it;
		const string module = to_module_path(file_path);
		Module::Info info = Scheduler::instance()->ast()->module_info(module);
		if (DebugInfo *debug_info = info.debug_info; debug_info && info.state != Module::not_compiled) {
			write_log("Resolved pending breakpoint %s:%zu", file_path.c_str(), line_number);
			Breakpoint::Id id = debugger->create_breakpoint({info.id, module, debug_info->to_executable_line_number(line_number)});
			send_event("breakpoint", new JsonObject {
				{ "reason", new JsonString("new") },
				{ "breakpoint", new JsonObject {
					{ "verified", new JsonBoolean(true) },
					{ "id", new JsonNumber(to_client_id(id)) },
					{ "line", new JsonNumber(to_client_line_number(debugger->get_breakpoint(id).info.line_number())) }
				}}
			});
			it = m_pending_breakpoints.erase(it);
		}
		else {
			++it;
		}
	}
}

size_t DapDebugger::register_frame_variables_reference(size_t frame_id, Object *object) {
	size_t variables_reference_id = m_variables.size();
	m_variables.push_back(variables_reference_t { frame_id, object });
	return variables_reference_id;
}
