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

#include "mint/scheduler/inputstream.h"
#include "mint/scheduler/scheduler.h"
#include "mint/system/assert.h"
#include "mint/system/terminal.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <format>
#include <utility>

using namespace mint;

namespace {

int amount_of_digits(std::size_t value) {

	int amount = 1;

	while (value /= 10) {
		amount++;
	}

	return amount;
}

}

InputStream::InputStream() {
	_terminal.set_auto_braces("{}[]()''\"\"");
	_terminal.set_prompt([this](std::size_t row_number) {
		const std::size_t number = row_number + line_number();
		const std::size_t number_digits = static_cast<std::size_t>(amount_of_digits(number) / 4) + 3;
		if (row_number == 0) {
			return std::format("{: {}} \033[1;32m❯❯❯\033[0m ", number, number_digits);
		}
		return std::format("{: {}} \033[1;32m...\033[0m ", number, number_digits);
	});
}

InputStream& InputStream::instance() {

	static InputStream g_instance;

	return g_instance;
}

bool InputStream::at_end() const {
	return _status == Status::over;
}

bool InputStream::is_valid() const {
	return is_term(stdin_file_no);
}

std::filesystem::path InputStream::path() const {
	return "stdin";
}

void InputStream::next() {
	_level = 0;
	_status = Status::ready;
}

void InputStream::set_highlighter(Terminal::HighlighterFunction highlight) {
	_terminal.set_highlighter(std::move(highlight));
}

void InputStream::set_completion_generator(Terminal::CompletionGeneratorFunction generator) {
	_terminal.set_completion_generator(std::move(generator));
}

void InputStream::set_brace_matcher(Terminal::BraceMatcherFunction matcher) {
	_terminal.set_brace_matcher(std::move(matcher));
}

void InputStream::update_buffer() {

	if (Terminal::get_cursor_column()) {
		Terminal::print(stdout, "\n");
	}

	auto buffer = _terminal.read_line();
	if (buffer.has_value()) {
		_buffer = *buffer;
	}
	else {
		auto* scheduler = Scheduler::instance();
		assert_x(scheduler, __func__, "execution should be done using a scheduler");
		scheduler->exit(EXIT_SUCCESS);
		_buffer.clear();
		_status = Status::over;
	}

	_cptr = _buffer.data();
}

int InputStream::read_char() {

	if (_must_fetch_more) {
		_must_fetch_more = false;
		update_buffer();
	}
	else if ((_status == Status::ready) && (*_cptr == '\0')) {
		update_buffer();
	}

	switch (_status) {
	case Status::ready:
		switch (*_cptr) {
		case '\n':
			if (_level) {
				_must_fetch_more = *(_cptr + 1) == '\0';
			}
			else {
				_status = Status::breaking;
			}
			break;
		case '{':
		case '[':
		case '(':
			_level++;
			break;
		case '}':
		case ']':
		case ')':
			_level--;
			break;
		case '/':
			_status = Status::could_start_comment;
			break;
		case '\'':
			_status = Status::single_quote_string;
			break;
		case '"':
			_status = Status::double_quote_string;
			break;
		default:
			break;
		}

		return next_buffered_char();

	case Status::could_start_comment:
		switch (*_cptr) {
		case '\n':
			if (_level) {
				_must_fetch_more = *(_cptr + 1) == '\0';
				_status = Status::ready;
			}
			else {
				_status = Status::breaking;
			}
			break;
		case '{':
		case '[':
		case '(':
			_status = Status::ready;
			_level++;
			break;
		case '}':
		case ']':
		case ')':
			_status = Status::ready;
			_level--;
			break;
		case '/':
			_status = Status::single_line_comment;
			break;
		case '*':
			_status = Status::multi_line_comment;
			break;
		case '\'':
			_status = Status::single_quote_string;
			break;
		case '"':
			_status = Status::double_quote_string;
			break;
		default:
			_status = Status::ready;
			break;
		}

		return next_buffered_char();

	case Status::single_line_comment:
		switch (*_cptr) {
		case '\n':
			if (_level) {
				_must_fetch_more = *(_cptr + 1) == '\0';
				_status = Status::ready;
			}
			else {
				_status = Status::breaking;
			}
			break;
		default:
			break;
		}

		return next_buffered_char();

	case Status::multi_line_comment:
		switch (*_cptr) {
		case '\n':
			_must_fetch_more = *(_cptr + 1) == '\0';
			break;
		case '*':
			_status = Status::could_end_comment;
			break;
		default:
			break;
		}

		return next_buffered_char();

	case Status::could_end_comment:
		switch (*_cptr) {
		case '\n':
			_must_fetch_more = *(_cptr + 1) == '\0';
			_status = Status::multi_line_comment;
			break;
		case '/':
			_status = Status::ready;
			break;
		default:
			_status = Status::multi_line_comment;
			break;
		}

		return next_buffered_char();

	case Status::single_quote_string:
		switch (*_cptr) {
		case '\n':
			_must_fetch_more = *(_cptr + 1) == '\0';
			break;
		case '\\':
			_status = Status::single_quote_string_escape_next;
			break;
		case '\'':
			_status = Status::ready;
			break;
		default:
			break;
		}

		return next_buffered_char();

	case Status::single_quote_string_escape_next:
		switch (*_cptr) {
		case '\n':
			_must_fetch_more = *(_cptr + 1) == '\0';
			_status = Status::single_quote_string;
			break;
		default:
			_status = Status::single_quote_string;
			break;
		}

		return next_buffered_char();

	case Status::double_quote_string:
		switch (*_cptr) {
		case '\n':
			_must_fetch_more = *(_cptr + 1) == '\0';
			break;
		case '\\':
			_status = Status::single_quote_string_escape_next;
			break;
		case '"':
			_status = Status::ready;
			break;
		default:
			break;
		}

		return next_buffered_char();

	case Status::double_quote_string_escape_next:
		switch (*_cptr) {
		case '\n':
			_must_fetch_more = *(_cptr + 1) == '\0';
			_status = Status::double_quote_string;
			break;
		default:
			_status = Status::double_quote_string;
			break;
		}

		return next_buffered_char();

	case Status::breaking:
		_status = Status::over;
		break;

	case Status::over:
		_status = Status::ready;
		break;
	}

	return EOF;
}

int InputStream::next_buffered_char() {
	return *_cptr++;
}
