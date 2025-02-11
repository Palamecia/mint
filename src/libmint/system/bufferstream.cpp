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

#include "mint/system/bufferstream.h"

#include <cstring>

using namespace mint;

BufferStream::BufferStream(const std::string &buffer) :
	m_buffer(strdup(buffer.c_str())),
	m_cptr(m_buffer),
	m_status(READY) {}

BufferStream::~BufferStream() {
	free(const_cast<char *>(m_buffer));
}

bool BufferStream::at_end() const {
	return m_status == OVER;
}

bool BufferStream::is_valid() const {
	return true;
}

std::filesystem::path BufferStream::path() const {
	return "buffer";
}

int BufferStream::read_char() {
	switch (m_status) {
	case READY:
		if (*m_cptr == '\0') {
			m_status = FLUSH;
			return '\n';
		}
		break;
	case FLUSH:
		m_status = OVER;
		return EOF;
	case OVER:
		return EOF;
	}
	return next_buffered_char();
}

int BufferStream::next_buffered_char() {
	return *m_cptr++;
}
