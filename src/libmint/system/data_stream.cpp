/**
 * Copyright (c) 2026 Gauvain CHÉRY.
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

#include "mint/system/data_stream.h"
#include <cstddef>
#include <cstdio>
#include <functional>

using namespace mint;

int DataStream::get_char() {

	const int c = read_char();

	switch (_state) {
	case State::state_new_line:
		begin_line();
		[[fallthrough]];
	case State::state_reading:
		switch (c) {
		case EOF:
		case '\0':
			break;
		case '\n':
			end_line();
			break;
		default:
			++_line_offset;
			break;
		}
	}

	return c;
}

void DataStream::set_new_line_callback(const std::function<void(std::size_t)>& callback) {
	_new_line_callback = callback;
}

std::size_t DataStream::line_number() const {
	return _line_number;
}

DataStream::Position DataStream::position() const {
	if (_state == State::state_new_line && _line_number > 1) {
		return {
		    .line = _line_number - 1,
		    .offset = _last_line_offset,
		};
	}
	return {
	    .line = _line_number,
	    .offset = _line_offset,
	};
}

void DataStream::begin_line() {
	_new_line_callback(_line_number);
	_state = State::state_reading;
	_line_offset = 0;
}

void DataStream::end_line() {
	_last_line_offset = _line_offset;
	_state = State::state_new_line;
	_line_number++;
}
