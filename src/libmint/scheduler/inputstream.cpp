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

#include "mint/scheduler/inputstream.h"
#include "mint/scheduler/scheduler.h"

#include <cstring>

#define MINT_NEW_LINE_PROMPT "\033[1;32m>>>\033[0m "
#define MINT_CONTINUE_PROMPT "\033[1;32m...\033[0m "

using namespace std;
using namespace mint;

int amount_of_digits(size_t value) {

	int amount = 1;

	while (value /= 10) {
		amount++;
	}

	return amount;
}

InputStream::InputStream() {
	m_terminal.set_auto_braces("{}[]()''\"\"");
	m_terminal.set_prompt([this](size_t row_number) {

		size_t number = row_number + line_number();
		size_t number_digits = static_cast<size_t>(amount_of_digits(number) / 4) + 3;
		string prompt(number_digits + strlen(MINT_NEW_LINE_PROMPT) + 2, ' ');

		if (row_number) {
			snprintf(prompt.data(), prompt.length(), "% *zd " MINT_CONTINUE_PROMPT, static_cast<int>(number_digits), number);
		}
		else {
			snprintf(prompt.data(), prompt.length(), "% *zd " MINT_NEW_LINE_PROMPT, static_cast<int>(number_digits), number);
		}

		return prompt;
	});
}

InputStream::~InputStream() {

}

InputStream &InputStream::instance() {

	static InputStream g_instance;

	return g_instance;
}

bool InputStream::at_end() const {
	return m_status == over;
}

bool InputStream::is_valid() const {
	return is_term(stdin_fileno);
}

string InputStream::path() const {
	return "stdin";
}

void InputStream::next() {
	m_level = 0;
	m_status = ready;
}

void InputStream::set_higlighter(function<string(string_view, string_view::size_type)> highlight) {
	m_terminal.set_higlighter(highlight);
}

void InputStream::set_completion_generator(function<bool(string_view, string_view::size_type, vector<completion_t> &)> generator) {
	m_terminal.set_completion_generator(generator);
}

void InputStream::set_brace_matcher(function<pair<string_view::size_type, bool>(string_view, string_view::size_type)> matcher) {
	m_terminal.set_brace_matcher(matcher);
}

void InputStream::update_buffer() {

	if (Terminal::get_cursor_column()) {
		Terminal::print(stdout, "\n");
	}

	auto buffer = m_terminal.read_line();
	if (buffer.has_value()) {
		m_buffer = *buffer;
	}
	else {
		Scheduler::instance()->exit(EXIT_SUCCESS);
		m_buffer.clear();
		m_status = over;
	}

	m_cptr = m_buffer.data();
}

int InputStream::read_char() {

	if (m_must_fetch_more) {
		m_must_fetch_more = false;
		update_buffer();
	}
	else if ((m_status == ready) && (*m_cptr == '\0')) {
		update_buffer();
	}

	switch (m_status) {
	case ready:
		switch (*m_cptr) {
		case '\n':
			if (m_level) {
				m_must_fetch_more = *(m_cptr + 1) == '\0';
			}
			else {
				m_status = breaking;
			}
			break;
		case '{':
		case '[':
		case '(':
			m_level++;
			break;
		case '}':
		case ']':
		case ')':
			m_level--;
			break;
		case '/':
			m_status = could_start_comment;
			break;
		case '\'':
			m_status = single_quote_string;
			break;
		case '"':
			m_status = double_quote_string;
			break;
		default:
			break;
		}

		return next_buffered_char();

	case could_start_comment:
		switch (*m_cptr) {
		case '\n':
			if (m_level) {
				m_must_fetch_more = *(m_cptr + 1) == '\0';
				m_status = ready;
			}
			else {
				m_status = breaking;
			}
			break;
		case '{':
		case '[':
		case '(':
			m_status = ready;
			m_level++;
			break;
		case '}':
		case ']':
		case ')':
			m_status = ready;
			m_level--;
			break;
		case '/':
			m_status = single_line_comment;
			break;
		case '*':
			m_status = multi_line_comment;
			break;
		case '\'':
			m_status = single_quote_string;
			break;
		case '"':
			m_status = double_quote_string;
			break;
		default:
			m_status = ready;
			break;
		}

		return next_buffered_char();

	case single_line_comment:
		switch (*m_cptr) {
		case '\n':
			if (m_level) {
				m_must_fetch_more = *(m_cptr + 1) == '\0';
				m_status = ready;
			}
			else {
				m_status = breaking;
			}
			break;
		default:
			break;
		}

		return next_buffered_char();

	case multi_line_comment:
		switch (*m_cptr) {
		case '\n':
			m_must_fetch_more = *(m_cptr + 1) == '\0';
			break;
		case '*':
			m_status = could_end_comment;
			break;
		default:
			break;
		}

		return next_buffered_char();

	case could_end_comment:
		switch (*m_cptr) {
		case '\n':
			m_must_fetch_more = *(m_cptr + 1) == '\0';
			m_status = multi_line_comment;
			break;
		case '/':
			m_status = ready;
			break;
		default:
			m_status = multi_line_comment;
			break;
		}

		return next_buffered_char();

	case single_quote_string:
		switch (*m_cptr) {
		case '\n':
			m_must_fetch_more = *(m_cptr + 1) == '\0';
			break;
		case '\\':
			m_status = single_quote_string_escape_next;
			break;
		case '\'':
			m_status = ready;
			break;
		default:
			break;
		}

		return next_buffered_char();

	case single_quote_string_escape_next:
		switch (*m_cptr) {
		case '\n':
			m_must_fetch_more = *(m_cptr + 1) == '\0';
			m_status = single_quote_string;
			break;
		default:
			m_status = single_quote_string;
			break;
		}

		return next_buffered_char();

	case double_quote_string:
		switch (*m_cptr) {
		case '\n':
			m_must_fetch_more = *(m_cptr + 1) == '\0';
			break;
		case '\\':
			m_status = single_quote_string_escape_next;
			break;
		case '"':
			m_status = ready;
			break;
		default:
			break;
		}

		return next_buffered_char();

	case double_quote_string_escape_next:
		switch (*m_cptr) {
		case '\n':
			m_must_fetch_more = *(m_cptr + 1) == '\0';
			m_status = double_quote_string;
			break;
		default:
			m_status = double_quote_string;
			break;
		}

		return next_buffered_char();

	case breaking:
		m_status = over;
		break;

	case over:
		m_status = ready;
		break;
	}

	return EOF;
}

int InputStream::next_buffered_char() {
	return *m_cptr++;
}
