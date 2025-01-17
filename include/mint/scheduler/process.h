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

#ifndef MINT_PROCESS_H
#define MINT_PROCESS_H

#include "mint/ast/cursor.h"

#include <thread>

namespace mint {

class DebugInterface;

class MINT_EXPORT Process {
public:
	using ThreadId = int;

	Process(Cursor *cursor);
	Process(Process &&) = delete;
	Process(const Process &) = default;
	virtual ~Process();

	Process &operator=(Process &&) = delete;
	Process &operator=(const Process &) = default;

	static Process *from_main_file(AbstractSyntaxTree *ast, const std::string &file);
	static Process *from_file(AbstractSyntaxTree *ast, const std::string &file);
	static Process *from_buffer(AbstractSyntaxTree *ast, const std::string &buffer);
	static Process *from_standard_input(AbstractSyntaxTree *ast);

	void parse_argument(const std::string &arg);

	virtual void setup();
	virtual void cleanup();

	bool exec();
	bool debug(DebugInterface *debug_interface);
	bool resume();

	[[nodiscard]] ThreadId get_thread_id() const;
	void set_thread_id(ThreadId id);

	[[nodiscard]] std::thread *get_thread_handle() const;
	void set_thread_handle(std::thread *handle);

	[[nodiscard]] bool is_endless() const;
	[[nodiscard]] Cursor *cursor() const;

protected:
	void set_endless(bool endless);
	void dump();

private:
	Cursor *m_cursor;
	bool m_endless;

	std::thread *m_thread_handle = nullptr;
	ThreadId m_thread_id;
	int m_error_handler;
};

}

#endif // MINT_PROCESS_H
