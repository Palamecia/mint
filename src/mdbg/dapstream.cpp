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

#include "dapstream.h"

#ifdef OS_UNIX
#include <poll.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/file.h>
#include <sys/poll.h>
#include <mint/system/terminal.h>
#endif

DapStreamReader::DapStreamReader() :
#ifdef OS_WINDOWS
	m_handle(GetStdHandle(STD_INPUT_HANDLE)) {
	/// \todo SetStdHandle(STD_INPUT_HANDLE, internal pipe);
#else
	m_fd(dup(mint::STDIN_FILE_NO)) {
/// \todo dup2(mint::STDIN_FILE_NO, internal pipe);
#endif
}

DapStreamReader::~DapStreamReader() {
#ifdef OS_WINDOWS

#else
	close(m_fd);
#endif
}

size_t DapStreamReader::read(std::string &data) {

	size_t size = 0;

#ifdef OS_WINDOWS
	DWORD dwCount = 0;

	while (PeekNamedPipe(m_handle, NULL, 0, NULL, &dwCount, NULL) && dwCount) {
		char *buf = new char[dwCount];
		if (ReadFile(m_handle, buf, dwCount, &dwCount, NULL)) {
			copy_n(buf, dwCount, back_inserter(data));
			size += static_cast<size_t>(dwCount);
		}
		delete[] buf;
	}
#else
	pollfd rfds;
	rfds.fd = m_fd;
	rfds.events = POLLIN;

	const int flags = fcntl(rfds.fd, F_GETFL);
	fcntl(rfds.fd, F_SETFL, flags | O_NONBLOCK);

	while (::poll(&rfds, 1, 0) == 1) {
		uint8_t read_buffer[BUFSIZ];
		if (size_t count = ::read(rfds.fd, read_buffer, BUFSIZ)) {
			copy_n(read_buffer, count, back_inserter(data));
		}
	}

	fcntl(rfds.fd, F_SETFL, flags);
#endif

	return size;
}

DapStreamWriter::DapStreamWriter() :
#ifdef OS_WINDOWS
	m_handle(GetStdHandle(STD_OUTPUT_HANDLE)) {
#else
	m_fd(dup(mint::STDOUT_FILE_NO)) {
#endif
}

DapStreamWriter::~DapStreamWriter() {
#ifdef OS_WINDOWS

#else
	close(m_fd);
#endif
}

size_t DapStreamWriter::write(const std::string &data) {

#ifdef OS_WINDOWS
	DWORD dwCount = 0;

	if (WriteFile(m_handle, data.data(), static_cast<DWORD>(data.size()), &dwCount, NULL)) {
		// FlushFileBuffers(m_handle);
		return static_cast<size_t>(dwCount);
	}
#else
	auto result = ::write(m_fd, data.data(), data.size());
	if (result > 0) {
		return static_cast<size_t>(result);
	}
#endif

	return INVALID_LENGTH;
}
