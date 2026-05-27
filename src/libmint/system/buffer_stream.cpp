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

#include "mint/system/buffer_stream.h"

#include <cstdio>
#include <filesystem>
#include <string>
#include <utility>

using namespace mint;

BufferStream::BufferStream(std::string buffer) :
    _buffer(std::move(buffer)),
    _status(Status::ready) {}

bool BufferStream::at_end() const {
	return _status == Status::over;
}

bool BufferStream::is_valid() const {
	return true;
}

std::filesystem::path BufferStream::path() const {
	return "buffer";
}

int BufferStream::read_char() {
	switch (_status) {
	case Status::ready:
		if (_pos == _buffer.size()) {
			_status = Status::flush;
			return '\n';
		}
		break;
	case Status::flush:
		_status = Status::over;
		return EOF;
	case Status::over:
		return EOF;
	}
	return next_buffered_char();
}

int BufferStream::next_buffered_char() {
	if (_pos < _buffer.size()) {
		return _buffer[_pos++];
	}
	return 0;
}
