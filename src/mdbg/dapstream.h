/**
 * Copyright (c) 2025 Gauvain CHERY.
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

#ifndef MDBG_DAPSTREAM_H
#define MDBG_DAPSTREAM_H

#include "dapmessage.h"

#ifdef OS_WINDOWS
#include <Windows.h>
#endif

class DapStreamReader : public DapMessageReader {
public:
	DapStreamReader();
	DapStreamReader(const DapStreamReader &) = delete;
	DapStreamReader(DapStreamReader &&) = delete;
	~DapStreamReader();

	DapStreamReader &operator=(const DapStreamReader &) = delete;
	DapStreamReader &operator=(DapStreamReader &&) = delete;

protected:
	size_t read(std::string &data) override;

private:
#ifdef OS_WINDOWS
	HANDLE m_handle;
#else
	int m_fd;
#endif
};

class DapStreamWriter : public DapMessageWriter {
public:
	DapStreamWriter();
	DapStreamWriter(const DapStreamWriter &) = delete;
	DapStreamWriter(DapStreamWriter &&) = delete;
	~DapStreamWriter();

	DapStreamWriter &operator=(const DapStreamWriter &) = delete;
	DapStreamWriter &operator=(DapStreamWriter &&) = delete;

protected:
	size_t write(const std::string &data) override;

private:
#ifdef OS_WINDOWS
	HANDLE m_handle;
#else
	int m_fd;
#endif
};

#endif // MDBG_DAPSTREAM_H
