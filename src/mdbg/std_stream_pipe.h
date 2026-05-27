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

#ifndef MDBG_STD_STREAM_PIPE_H
#define MDBG_STD_STREAM_PIPE_H

#include "mint/system/terminal.h"
#include <cstddef>
#include <array>
#include <string>

#ifdef MINT_OS_WINDOWS
#include <Windows.h>
#include <winnt.h>
#endif

class StdStreamPipe {
public:
#ifdef MINT_OS_WINDOWS
	using handle_t = HANDLE;
#else
	using handle_t = int;
#endif
	StdStreamPipe(const StdStreamPipe&) = default;
	StdStreamPipe(StdStreamPipe&&) = delete;
	StdStreamPipe(mint::StdStreamFileNo number);
	~StdStreamPipe();

	StdStreamPipe& operator=(const StdStreamPipe&) = default;
	StdStreamPipe& operator=(StdStreamPipe&&) = delete;

	[[nodiscard]] bool can_read() const;
	[[nodiscard]] std::string read();

private:
	static constexpr std::size_t read_index = 0;
	static constexpr std::size_t write_index = 1;

	std::array<handle_t, 2> _handles;
};

#endif // MDBG_STD_STREAM_PIPE_H
