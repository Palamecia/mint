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

#ifndef MINT_SCHEDULER_PROCESS_H
#define MINT_SCHEDULER_PROCESS_H

#include "mint/ast/cursor.h"
#include "mint/config.h"
#include "mint/scheduler/processor.h"

#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <thread>

namespace mint {

class DebugInterface;
class Scheduler;

class MINT_EXPORT Process {
public:
	using ThreadId = int;

	Process(Scheduler& scheduler, std::unique_ptr<Cursor>&& cursor);
	Process(Process&&) = delete;
	Process(const Process&) = delete;
	virtual ~Process() = default;

	Process& operator=(Process&&) = delete;
	Process& operator=(const Process&) = delete;

	static std::unique_ptr<Process> from_main_file(Scheduler& scheduler, const std::filesystem::path& file);
	static std::unique_ptr<Process> from_file(Scheduler& scheduler, const std::filesystem::path& file);
	static std::unique_ptr<Process> from_buffer(Scheduler& scheduler, const std::string& buffer);
	static std::unique_ptr<Process> from_standard_input(Scheduler& scheduler);

	void parse_argument(const std::string& arg);

	virtual void setup();
	virtual bool resume();
	virtual void cleanup();

	[[nodiscard]] ProcessStatus exec();

	[[nodiscard]] ThreadId get_thread_id() const;
	void set_thread_id(ThreadId id);

	[[nodiscard]] std::thread* get_thread_handle() const;
	std::unique_ptr<std::thread> release_thread_handle();
	void set_thread_handle(std::unique_ptr<std::thread>&& handle);

	[[nodiscard]] bool is_thread() const;
	[[nodiscard]] Cursor& cursor() const;
	[[nodiscard]] Scheduler& scheduler() const;

protected:
	void on_error();

private:
	std::reference_wrapper<Scheduler> _scheduler;
	std::unique_ptr<Cursor> _cursor;

	std::unique_ptr<std::thread> _thread_handle;
	ThreadId _thread_id = 0;
	int _error_handler = 0;
};

}

#endif // MINT_SCHEDULER_PROCESS_H
