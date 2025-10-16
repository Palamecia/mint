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

#include "dapstream.h"
#include <algorithm>
#include <cstddef>
#include <iterator>
#include <string>

#ifdef MINT_OS_WINDOWS
#include <memory>
#include <Windows.h>
#include <fileapi.h>
#include <minwindef.h>
#include <namedpipeapi.h>
#include <processenv.h>
#include <winbase.h>
#else
#include "mint/system/terminal.h"
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <poll.h>
#include <sys/file.h>
#include <sys/poll.h>
#include <unistd.h>
#endif

DapStreamReader::DapStreamReader() :
#ifdef MINT_OS_WINDOWS
    _handle(GetStdHandle(STD_INPUT_HANDLE)) {
	/// \todo SetStdHandle(STD_INPUT_HANDLE, internal pipe);
#else
    _fd(dup(mint::stdin_file_no)) {
	/// \todo dup2(mint::STDIN_FILE_NO, internal pipe);
#endif
}

DapStreamReader::~DapStreamReader() {
#ifdef MINT_OS_WINDOWS

#else
	close(_fd);
#endif
}

std::size_t DapStreamReader::read(std::string& data) {

	std::size_t size = 0;

#ifdef MINT_OS_WINDOWS
	DWORD count = 0;

	while (PeekNamedPipe(_handle, nullptr, 0, nullptr, &count, nullptr) && count) {
		auto buf = std::make_unique<char[]>(count);
		if (ReadFile(_handle, buf.get(), count, &count, nullptr)) {
			copy_n(buf.get(), count, std::back_inserter(data));
			size += static_cast<std::size_t>(count);
		}
	}
#else
	pollfd rfds {
	    .fd = _fd,
	    .events = POLLIN,
	};

	const int flags = fcntl(rfds.fd, F_GETFL);
	fcntl(rfds.fd, F_SETFL, flags | O_NONBLOCK);

	while (::poll(&rfds, 1, 0) == 1) {
		auto read_buffer = std::array<std::uint8_t, BUFSIZ>();
		if (const auto count = ::read(rfds.fd, read_buffer.data(), read_buffer.size())) {
			std::copy_n(read_buffer.data(), count, std::back_inserter(data));
			size += static_cast<std::size_t>(count);
		}
	}

	fcntl(rfds.fd, F_SETFL, flags);
#endif

	return size;
}

DapStreamWriter::DapStreamWriter() :
#ifdef MINT_OS_WINDOWS
    _handle(GetStdHandle(STD_OUTPUT_HANDLE)) {
#else
    _fd(dup(mint::stdout_file_no)) {
#endif
}

DapStreamWriter::~DapStreamWriter() {
#ifdef MINT_OS_WINDOWS

#else
	close(_fd);
#endif
}

std::size_t DapStreamWriter::write(const std::string& data) {

#ifdef MINT_OS_WINDOWS
	if (DWORD count = 0; WriteFile(_handle, data.data(), static_cast<DWORD>(data.size()), &count, nullptr)) {
		// FlushFileBuffers(_handle);
		return static_cast<std::size_t>(count);
	}
#else
	if (const auto result = ::write(_fd, data.data(), data.size()); result > 0) {
		return static_cast<std::size_t>(result);
	}
#endif

	return invalid_length;
}
