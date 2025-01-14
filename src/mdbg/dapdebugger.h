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

#ifndef MDBG_DAPDEBUGGER_H
#define MDBG_DAPDEBUGGER_H

#include "debuggerbackend.h"
#include "dapmessage.h"
#include "stdstreampipe.h"

#include <future>

class DapDebugger : public DebuggerBackend {
public:
	DapDebugger(DapMessageReader *reader, DapMessageWriter *writer);
	~DapDebugger();

	DapDebugger(const DapDebugger &other) = delete;
	DapDebugger &operator=(const DapDebugger &other) = delete;

	bool setup(Debugger *debugger, mint::Scheduler *scheduler) override;
	bool handle_events(Debugger *debugger, mint::CursorDebugger *cursor) override;
	bool check(Debugger *debugger, mint::CursorDebugger *cursor) override;
	void cleanup(Debugger *debugger, mint::Scheduler *scheduler) override;

	void on_thread_started(Debugger *debugger, mint::CursorDebugger *cursor) override;
	void on_thread_exited(Debugger *debugger, mint::CursorDebugger *cursor) override;

	void on_breakpoint_created(Debugger *debugger, const mint::Breakpoint &breakpoint) override;
	void on_breakpoint_deleted(Debugger *debugger, const mint::Breakpoint &breakpoint) override;

	void on_module_loaded(Debugger *debugger, mint::CursorDebugger *cursor, mint::Module *module) override;

	bool on_breakpoint(Debugger *debugger, mint::CursorDebugger *cursor,
					   const std::unordered_set<mint::Breakpoint::Id> &breakpoints) override;
	bool on_exception(Debugger *debugger, mint::CursorDebugger *cursor) override;
	bool on_pause(Debugger *debugger, mint::CursorDebugger *cursor) override;
	bool on_step(Debugger *debugger, mint::CursorDebugger *cursor) override;

	void on_exit(Debugger *debugger, int code) override;
	void on_error(Debugger *debugger) override;

	void shutdown();

protected:
	bool dispatch_request(std::unique_ptr<DapRequestMessage> message, Debugger *debugger, mint::Scheduler *scheduler);
	bool dispatch_request(std::unique_ptr<DapRequestMessage> message, Debugger *debugger, mint::CursorDebugger *cursor);

	void send_event(const std::string &event, JsonObject *body = nullptr);
	void send_response(const DapRequestMessage *request, JsonObject *body = nullptr);
	void send_error(const DapRequestMessage *request, int code, const std::string &format, JsonObject *variables,
					ErrorDestination destination);

	size_t from_client_column_number(size_t number) const;
	size_t to_client_column_number(size_t number) const;
	size_t from_client_line_number(size_t number) const;
	size_t to_client_line_number(size_t number) const;
	size_t from_client_id(size_t id) const;
	size_t to_client_id(size_t id) const;

	void on_set_breakpoints(std::unique_ptr<DapRequestMessage> request, const JsonObject *arguments, Debugger *debugger);
	void on_threads(std::unique_ptr<DapRequestMessage> request, const JsonObject *arguments, Debugger *debugger);
	void on_stack_trace(std::unique_ptr<DapRequestMessage> request, const JsonObject *arguments, Debugger *debugger);
	void on_breakpoint_locations(std::unique_ptr<DapRequestMessage> request, const JsonObject *arguments,
								 Debugger *debugger);
	void on_scopes(std::unique_ptr<DapRequestMessage> request, const JsonObject *arguments, Debugger *debugger);
	void on_variables(std::unique_ptr<DapRequestMessage> request, const JsonObject *arguments, Debugger *debugger);
	void on_continue(std::unique_ptr<DapRequestMessage> request, const JsonObject *arguments, Debugger *debugger);
	void on_next(std::unique_ptr<DapRequestMessage> request, const JsonObject *arguments, Debugger *debugger);
	void on_step_in(std::unique_ptr<DapRequestMessage> request, const JsonObject *arguments, Debugger *debugger);
	void on_step_out(std::unique_ptr<DapRequestMessage> request, const JsonObject *arguments, Debugger *debugger);
	void on_pause(std::unique_ptr<DapRequestMessage> request, const JsonObject *arguments, Debugger *debugger);
	void on_disconnect(std::unique_ptr<DapRequestMessage> request, const JsonObject *arguments, Debugger *debugger);
	void on_terminate(std::unique_ptr<DapRequestMessage> request, const JsonObject *arguments, Debugger *debugger);

	void on_initialize(std::unique_ptr<DapRequestMessage> request, const JsonObject *arguments, Debugger *debugger,
					   mint::Scheduler *scheduler);
	void on_launch(std::unique_ptr<DapRequestMessage> request, const JsonObject *arguments, Debugger *debugger,
				   mint::Scheduler *scheduler);
	void on_configuration_done(std::unique_ptr<DapRequestMessage> request, const JsonObject *arguments,
							   Debugger *debugger, mint::Scheduler *scheduler);

	void write_log(const char *format, ...) __attribute__((format(printf, 2, 3)));

private:
	std::unique_ptr<DapMessageReader> m_reader;
	std::unique_ptr<DapMessageWriter> m_writer;
	std::mutex m_write_mutex;

	enum CommandFlag {
		NO_FLAG = 0x00,
		ASYNC = 0x01
	};

	using CommandFlags = std::underlying_type_t<CommandFlag>;

	struct Command {
		void (DapDebugger::*func)(std::unique_ptr<DapRequestMessage>, const JsonObject *, Debugger *);
		CommandFlags flags = NO_FLAG;
	};

	struct SetupCommand {
		void (DapDebugger::*func)(std::unique_ptr<DapRequestMessage>, const JsonObject *, Debugger *, mint::Scheduler *);
		CommandFlags flags = NO_FLAG;
	};

	struct RuntimeCommand {
		void (DapDebugger::*func)(std::unique_ptr<DapRequestMessage>, const JsonObject *, Debugger *,
								  mint::CursorDebugger *);
		CommandFlags flags = NO_FLAG;
	};

	static std::unordered_map<std::string, Command> g_commands;
	static std::unordered_map<std::string, SetupCommand> g_setup_commands;
	static std::unordered_map<std::string, RuntimeCommand> g_runtime_commands;

	template<class Command, class... Types>
	void call_command(const Command &command, Types... args) {
		if (command.flags & ASYNC) {
			m_async_commands.emplace_back(std::async(command.func, this, std::forward<Types>(args)...));
		}
		else {
			std::invoke(command.func, this, std::forward<Types>(args)...);
		}
	}

	void update_async_commands();

	std::list<std::future<void>> m_async_commands;
	std::condition_variable m_configuration_done;

	std::atomic_bool m_running = {true};
	std::atomic_bool m_configuring = {true};
	bool m_client_lines_start_at_1 = true;
	bool m_client_columns_start_at_1 = true;

	FILE *m_logger;

	StdStreamPipe m_stdin;
	StdStreamPipe m_stdout;
	StdStreamPipe m_stderr;

	struct variables_reference_t {
		size_t frame_id;
		mint::Object *object;
	};

	std::vector<variables_reference_t> m_variables;

	size_t register_frame_variables_reference(size_t frame_id, mint::Object *object = nullptr);
};

#endif // MDBG_DAPDEBUGGER_H
