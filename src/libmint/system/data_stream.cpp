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

#include "mint/system/data_stream.h"
#include "mint/config.h"
#include <cstddef>
#include <cstdio>
#include <functional>
#include <string>

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
			_cached_line += static_cast<char>(c);
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

std::string DataStream::line_error() {

	auto line = _cached_line;
	const auto err_pos = line.empty() ? 0 : line.size() - 1;

	if (line.empty() || line.back() != '\n') {
		int c = next_buffered_char();
		while ((c != '\n') && (c != '\0') && (c != EOF)) {
			line += static_cast<char>(c);
			c = next_buffered_char();
		}
		line += '\n';
	}

	if (err_pos > 1) {
		for (std::size_t i = 0; i < err_pos - 1; ++i) {

			auto c = static_cast<byte_t>(_cached_line[i]);

			if (c == '\t') {
				line += '\t';
			}
			else if (c & 0x80) {

				std::size_t size = 2;

				if (c & 0x04) {
					size++;
					if (c & 0x02) {
						size++;
					}
				}

				if (i + size < err_pos - 1) {
					line += ' ';
				}

				i += size - 1;
			}
			else {
				line += ' ';
			}
		}
	}
	line += '^';

	if (_state != State::state_new_line) {
		end_line();
	}

	return line;
}

void DataStream::begin_line() {
	_new_line_callback(_line_number);
	_state = State::state_reading;
	_cached_line.clear();
}

void DataStream::end_line() {
	_state = State::state_new_line;
	_line_number++;
}
