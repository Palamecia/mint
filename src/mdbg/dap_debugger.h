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

#ifndef MDBG_DAP_DEBUGGER_H
#define MDBG_DAP_DEBUGGER_H

#include "debugger_backend.h"
#include "dap_message.h"
#include "json.h"
#include "mint/ast/module.h"
#include "mint/debug/cursor_debugger.h"
#include "mint/debug/debug_interface.h"
#include "mint/memory/object.h"
#include "mint/scheduler/process.h"
#include "mint/scheduler/scheduler.h"
#include "std_stream_pipe.h"

#include <atomic>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

class DapDebugger : public DebuggerBackend {
public:
	DapDebugger(std::unique_ptr<DapMessageReader>&& reader, std::unique_ptr<DapMessageWriter>&& writer);
	DapDebugger(const DapDebugger&) = delete;
	DapDebugger(DapDebugger&&) = delete;
	~DapDebugger();

	DapDebugger& operator=(const DapDebugger&) = delete;
	DapDebugger& operator=(DapDebugger&&) = delete;

	bool setup(Debugger& debugger, mint::Scheduler& scheduler) override;
	bool process_events(Debugger& debugger, mint::CursorDebugger& cursor) override;
	bool check(Debugger& debugger, mint::CursorDebugger& cursor) override;
	void cleanup(Debugger& debugger, mint::Scheduler& scheduler) override;

	void on_thread_started(Debugger& debugger, mint::CursorDebugger& cursor) override;
	void on_thread_exited(Debugger& debugger, mint::CursorDebugger& cursor) override;

	void on_breakpoint_created(Debugger& debugger, const mint::Breakpoint& breakpoint) override;
	void on_breakpoint_deleted(Debugger& debugger, const mint::Breakpoint& breakpoint) override;

	void on_module_loaded(Debugger& debugger, mint::CursorDebugger& cursor, mint::Module& module) override;

	bool on_breakpoint(Debugger& debugger, mint::CursorDebugger& cursor,
	    const std::unordered_set<mint::Breakpoint::Id>& breakpoints) override;
	bool on_exception(Debugger& debugger, mint::CursorDebugger& cursor) override;
	bool on_pause(Debugger& debugger, mint::CursorDebugger& cursor) override;
	bool on_step(Debugger& debugger, mint::CursorDebugger& cursor) override;

	void on_exit(Debugger& debugger, int code) override;
	void on_error(Debugger& debugger) override;

	void shutdown();

protected:
	bool dispatch_request(const DapRequestMessage& message, Debugger& debugger, mint::Scheduler& scheduler);
	bool dispatch_request(const DapRequestMessage& message, Debugger& debugger, mint::CursorDebugger& cursor);

	void send_event(const std::string& event, std::optional<JsonObject> body = std::nullopt);
	void send_response(const DapRequestMessage& request, std::optional<JsonObject> body = std::nullopt);
	void send_error(const DapRequestMessage& request, int code, const std::string& format,
	    std::optional<JsonObject> variables, ErrorDestination destination);

	std::size_t to_column_number(std::size_t number) const;
	std::size_t to_client_column_number(std::size_t number) const;
	std::size_t to_line_number(std::size_t number) const;
	std::size_t to_client_line_number(std::size_t number) const;
	static std::size_t to_frame_id(std::size_t id);
	static std::size_t to_client_id(std::size_t id);
	static mint::Process::ThreadId to_thread_id(std::size_t id);
	static std::size_t to_client_id(mint::Process::ThreadId id);

	void on_set_breakpoints(const DapRequestMessage& request, const JsonObject& arguments, Debugger& debugger);
	void on_threads(const DapRequestMessage& request, const JsonObject& arguments, Debugger& debugger);
	void on_stack_trace(const DapRequestMessage& request, const JsonObject& arguments, Debugger& debugger);
	void on_breakpoint_locations(const DapRequestMessage& request, const JsonObject& arguments, Debugger& debugger);
	void on_scopes(const DapRequestMessage& request, const JsonObject& arguments, Debugger& debugger);
	void on_variables(const DapRequestMessage& request, const JsonObject& arguments, Debugger& debugger);
	void on_continue(const DapRequestMessage& request, const JsonObject& arguments, Debugger& debugger);
	void on_next(const DapRequestMessage& request, const JsonObject& arguments, Debugger& debugger);
	void on_step_in(const DapRequestMessage& request, const JsonObject& arguments, Debugger& debugger);
	void on_step_out(const DapRequestMessage& request, const JsonObject& arguments, Debugger& debugger);
	void on_pause(const DapRequestMessage& request, const JsonObject& arguments, Debugger& debugger);
	void on_disconnect(const DapRequestMessage& request, const JsonObject& arguments, Debugger& debugger);
	void on_terminate(const DapRequestMessage& request, const JsonObject& arguments, Debugger& debugger);

	void on_initialize(const DapRequestMessage& request, const JsonObject& arguments, Debugger& debugger,
	    mint::Scheduler& scheduler);
	void on_launch(const DapRequestMessage& request, const JsonObject& arguments, Debugger& debugger,
	    mint::Scheduler& scheduler);
	void on_configuration_done(const DapRequestMessage& request, const JsonObject& arguments, Debugger& debugger,
	    mint::Scheduler& scheduler);

private:
	std::unique_ptr<DapMessageReader> _reader;
	std::unique_ptr<DapMessageWriter> _writer;
	std::mutex _write_mutex;

	template<typename... Args>
	struct Command {
		using Callback = void (DapDebugger::*)(Args...);
		Callback func = {};
	};

	using GlobalCommand = Command<const DapRequestMessage&, const JsonObject&, Debugger&>;
	using SetupCommand = Command<const DapRequestMessage&, const JsonObject&, Debugger&, mint::Scheduler&>;
	using RuntimeCommand = Command<const DapRequestMessage&, const JsonObject&, Debugger&, mint::CursorDebugger&>;

	static std::unordered_map<std::string, GlobalCommand> g_commands;
	static std::unordered_map<std::string, SetupCommand> g_setup_commands;
	static std::unordered_map<std::string, RuntimeCommand> g_runtime_commands;

	template<class Command, typename... Args>
	void call_command(Command& command, const DapRequestMessage& message, Args&&... args) {
		std::invoke(command.func, this, message, message.get_arguments(), std::forward<Args>(args)...);
	}

	std::atomic_bool _running = {true};
	std::atomic_bool _configuring = {true};
	bool _client_lines_start_at_1 = true;
	bool _client_columns_start_at_1 = true;

	StdStreamPipe _stdin;
	StdStreamPipe _stdout;
	StdStreamPipe _stderr;

	struct VariablesReference {
		std::size_t frame_id = 0;
		mint::Object* object = nullptr;
	};

	std::vector<VariablesReference> _variables;

	std::size_t register_frame_variables_reference(std::size_t frame_id, mint::Object* object = nullptr);
};

#endif // MDBG_DAP_DEBUGGER_H
