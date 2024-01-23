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

#ifdef OS_WINDOWS
#include <Windows.h>
#endif

#include <mint/system/terminal.h>
#include <future>

class DapDebugger : public DebuggerBackend {
public:
	DapDebugger(DapMessageReader *reader, DapMessageWriter *writer);
	~DapDebugger();

	bool setup(mint::DebugInterface *debugger, mint::Scheduler *scheduler) override;
	bool handle_events(mint::DebugInterface *debugger, mint::CursorDebugger *cursor) override;
	bool check(mint::DebugInterface *debugger, mint::CursorDebugger *cursor) override;
	void cleanup(mint::DebugInterface *debugger, mint::Scheduler *scheduler) override;

	void on_thread_started(mint::DebugInterface *debugger, mint::CursorDebugger *cursor) override;
	void on_thread_exited(mint::DebugInterface *debugger, mint::CursorDebugger *cursor) override;

	bool on_breakpoint(mint::DebugInterface *debugger, mint::CursorDebugger *cursor, const std::unordered_set<mint::Breakpoint::Id> &breakpoints) override;
	bool on_exception(mint::DebugInterface *debugger, mint::CursorDebugger *cursor) override;
	bool on_step(mint::DebugInterface *debugger, mint::CursorDebugger *cursor) override;

	void shutdown();

protected:
	bool dispatch_request(std::unique_ptr<DapRequestMessage> message, mint::DebugInterface *debugger, mint::Scheduler *scheduler);
	bool dispatch_request(std::unique_ptr<DapRequestMessage> message, mint::DebugInterface *debugger, mint::CursorDebugger *cursor);

	void send_event(const std::string &event, JsonObject *body = nullptr);
	void send_response(const DapRequestMessage *request, JsonObject *body = nullptr);
	void send_error(const DapRequestMessage *request, int code, const std::string &format, JsonObject *variables, ErrorDestination destination);

	size_t from_client_column_number(size_t number) const;
	size_t to_client_column_number(size_t number) const;
	size_t from_client_line_number(size_t number) const;
	size_t to_client_line_number(size_t number) const;
	size_t from_client_id(size_t id) const;
	size_t to_client_id(size_t id) const;

	void set_breakpoints(std::unique_ptr<DapRequestMessage> request, const JsonObject *arguments, mint::DebugInterface *debugger);
	void threads(std::unique_ptr<DapRequestMessage> request, const JsonObject *arguments, mint::DebugInterface *debugger);
	void stack_trace(std::unique_ptr<DapRequestMessage> request, const JsonObject *arguments, mint::DebugInterface *debugger);
	void breakpoint_locations(std::unique_ptr<DapRequestMessage> request, const JsonObject *arguments, mint::DebugInterface *debugger);
	void scopes(std::unique_ptr<DapRequestMessage> request, const JsonObject *arguments, mint::DebugInterface *debugger);
	void variables(std::unique_ptr<DapRequestMessage> request, const JsonObject *arguments, mint::DebugInterface *debugger);
	void _continue(std::unique_ptr<DapRequestMessage> request, const JsonObject *arguments, mint::DebugInterface *debugger);
	void _next(std::unique_ptr<DapRequestMessage> request, const JsonObject *arguments, mint::DebugInterface *debugger);
	void _stepIn(std::unique_ptr<DapRequestMessage> request, const JsonObject *arguments, mint::DebugInterface *debugger);
	void _stepOut(std::unique_ptr<DapRequestMessage> request, const JsonObject *arguments, mint::DebugInterface *debugger);
	void _pause(std::unique_ptr<DapRequestMessage> request, const JsonObject *arguments, mint::DebugInterface *debugger);
	void disconnect(std::unique_ptr<DapRequestMessage> request, const JsonObject *arguments, mint::DebugInterface *debugger);
	void terminate(std::unique_ptr<DapRequestMessage> request, const JsonObject *arguments, mint::DebugInterface *debugger);

	void initialize(std::unique_ptr<DapRequestMessage> request, const JsonObject *arguments, mint::DebugInterface *debugger, mint::Scheduler *scheduler);
	void launch(std::unique_ptr<DapRequestMessage> request, const JsonObject *arguments, mint::DebugInterface *debugger, mint::Scheduler *scheduler);
	void configuration_done(std::unique_ptr<DapRequestMessage> request, const JsonObject *arguments, mint::DebugInterface *debugger, mint::Scheduler *scheduler);

	void write_log(const char *format, ...) __attribute__((format(printf, 2, 3)));

private:
	std::unique_ptr<DapMessageReader> m_reader;
	std::unique_ptr<DapMessageWriter> m_writer;
	std::mutex m_writeMutex;

	enum CommandFlag {
		NoFlag = 0x00,
		Async = 0x01
	};
	using CommandFlags = int;

	struct Command {
		void(DapDebugger::*func)(std::unique_ptr<DapRequestMessage>, const JsonObject *, mint::DebugInterface *);
		CommandFlags flags = NoFlag;
	};

	struct SetupCommand {
		void(DapDebugger::*func)(std::unique_ptr<DapRequestMessage>, const JsonObject *, mint::DebugInterface *, mint::Scheduler *);
		CommandFlags flags = NoFlag;
	};

	struct RuntimeCommand {
		void(DapDebugger::*func)(std::unique_ptr<DapRequestMessage>, const JsonObject *, mint::DebugInterface *, mint::CursorDebugger *);
		CommandFlags flags = NoFlag;
	};

	static std::unordered_map<std::string, Command> g_commands;
	static std::unordered_map<std::string, SetupCommand> g_setup_commands;
	static std::unordered_map<std::string, RuntimeCommand> g_runtime_commands;

	template<class Command, class... Types>
	void call_command(const Command &command, Types... args) {
		if (command.flags & Async) {
			m_async_commands.emplace_back(std::async(command.func, this, std::forward<Types>(args)...));
		}
		else {
			std::invoke(command.func, this, std::forward<Types>(args)...);
		}
	}

	void update_async_commands();

	size_t m_module_count = 0;
	std::list<std::future<void>> m_async_commands;
	std::condition_variable m_configuration_done;

	std::atomic_bool m_running = { true };
	std::atomic_bool m_configuring = { true };
	bool m_pause_on_next_step = false;
	bool m_client_lines_start_at_1 = true;
	bool m_client_columns_start_at_1 = true;

	FILE *m_logger;

	class StdStreamPipe {
	public:
#ifdef OS_WINDOWS
		using handle_t = HANDLE;
#else
		using handle_t = int;
#endif
		StdStreamPipe(mint::StdStreamFileNo number);

		bool can_read() const;
		std::string read();

	private:
		static constexpr const int read_index = 0;
		static constexpr const int write_index = 1;
		handle_t m_handles[2];
	};

	StdStreamPipe m_stdin;
	StdStreamPipe m_stdout;
	StdStreamPipe m_stderr;

	struct variables_reference_t {
		size_t frame_id;
		mint::Object *object;
	};

	std::vector<std::pair<std::string, size_t>> m_pending_breakpoints;

	void update_pending_breakpoints(mint::DebugInterface *debugger);

	std::vector<variables_reference_t> m_variables;

	size_t register_frame_variables_reference(size_t frame_id, mint::Object *object = nullptr);
};

#endif // MDBG_DAPDEBUGGER_H
