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
#include "mint/system/terminal.h"

#include <cstring>

#define MINT_NEW_LINE_PROMPT "\033[1;32m>>>\033[0m "
#define MINT_CONTINUE_PROMPT "\033[1;32m...\033[0m "

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
		std::string prompt(number_digits + strlen(MINT_NEW_LINE_PROMPT) + 2, ' ');

		if (row_number) {
			snprintf(prompt.data(), prompt.length(), "% *zd " MINT_CONTINUE_PROMPT, static_cast<int>(number_digits),
					 number);
		}
		else {
			snprintf(prompt.data(), prompt.length(), "% *zd " MINT_NEW_LINE_PROMPT, static_cast<int>(number_digits),
					 number);
		}

		return prompt;
	});
}

InputStream::~InputStream() {}

InputStream &InputStream::instance() {

	static InputStream g_instance;

	return g_instance;
}

bool InputStream::at_end() const {
	return m_status == OVER;
}

bool InputStream::is_valid() const {
	return is_term(STDIN_FILE_NO);
}

std::string InputStream::path() const {
	return "stdin";
}

void InputStream::next() {
	m_level = 0;
	m_status = READY;
}

void InputStream::set_highlighter(Terminal::HighlighterFunction highlight) {
	m_terminal.set_highlighter(highlight);
}

void InputStream::set_completion_generator(Terminal::CompletionGeneratorFunction generator) {
	m_terminal.set_completion_generator(generator);
}

void InputStream::set_brace_matcher(Terminal::BraceMatcherFunction matcher) {
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
		m_status = OVER;
	}

	m_cptr = m_buffer.data();
}

int InputStream::read_char() {

	if (m_must_fetch_more) {
		m_must_fetch_more = false;
		update_buffer();
	}
	else if ((m_status == READY) && (*m_cptr == '\0')) {
		update_buffer();
	}

	switch (m_status) {
	case READY:
		switch (*m_cptr) {
		case '\n':
			if (m_level) {
				m_must_fetch_more = *(m_cptr + 1) == '\0';
			}
			else {
				m_status = BREAKING;
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
			m_status = COULD_START_COMMENT;
			break;
		case '\'':
			m_status = SINGLE_QUOTE_STRING;
			break;
		case '"':
			m_status = DOUBLE_QUOTE_STRING;
			break;
		default:
			break;
		}

		return next_buffered_char();

	case COULD_START_COMMENT:
		switch (*m_cptr) {
		case '\n':
			if (m_level) {
				m_must_fetch_more = *(m_cptr + 1) == '\0';
				m_status = READY;
			}
			else {
				m_status = BREAKING;
			}
			break;
		case '{':
		case '[':
		case '(':
			m_status = READY;
			m_level++;
			break;
		case '}':
		case ']':
		case ')':
			m_status = READY;
			m_level--;
			break;
		case '/':
			m_status = SINGLE_LINE_COMMENT;
			break;
		case '*':
			m_status = MULTI_LINE_COMMENT;
			break;
		case '\'':
			m_status = SINGLE_QUOTE_STRING;
			break;
		case '"':
			m_status = DOUBLE_QUOTE_STRING;
			break;
		default:
			m_status = READY;
			break;
		}

		return next_buffered_char();

	case SINGLE_LINE_COMMENT:
		switch (*m_cptr) {
		case '\n':
			if (m_level) {
				m_must_fetch_more = *(m_cptr + 1) == '\0';
				m_status = READY;
			}
			else {
				m_status = BREAKING;
			}
			break;
		default:
			break;
		}

		return next_buffered_char();

	case MULTI_LINE_COMMENT:
		switch (*m_cptr) {
		case '\n':
			m_must_fetch_more = *(m_cptr + 1) == '\0';
			break;
		case '*':
			m_status = COULD_END_COMMENT;
			break;
		default:
			break;
		}

		return next_buffered_char();

	case COULD_END_COMMENT:
		switch (*m_cptr) {
		case '\n':
			m_must_fetch_more = *(m_cptr + 1) == '\0';
			m_status = MULTI_LINE_COMMENT;
			break;
		case '/':
			m_status = READY;
			break;
		default:
			m_status = MULTI_LINE_COMMENT;
			break;
		}

		return next_buffered_char();

	case SINGLE_QUOTE_STRING:
		switch (*m_cptr) {
		case '\n':
			m_must_fetch_more = *(m_cptr + 1) == '\0';
			break;
		case '\\':
			m_status = SINGLE_QUOTE_STRING_ESCAPE_NEXT;
			break;
		case '\'':
			m_status = READY;
			break;
		default:
			break;
		}

		return next_buffered_char();

	case SINGLE_QUOTE_STRING_ESCAPE_NEXT:
		switch (*m_cptr) {
		case '\n':
			m_must_fetch_more = *(m_cptr + 1) == '\0';
			m_status = SINGLE_QUOTE_STRING;
			break;
		default:
			m_status = SINGLE_QUOTE_STRING;
			break;
		}

		return next_buffered_char();

	case DOUBLE_QUOTE_STRING:
		switch (*m_cptr) {
		case '\n':
			m_must_fetch_more = *(m_cptr + 1) == '\0';
			break;
		case '\\':
			m_status = SINGLE_QUOTE_STRING_ESCAPE_NEXT;
			break;
		case '"':
			m_status = READY;
			break;
		default:
			break;
		}

		return next_buffered_char();

	case DOUBLE_QUOTE_STRING_ESCAPE_NEXT:
		switch (*m_cptr) {
		case '\n':
			m_must_fetch_more = *(m_cptr + 1) == '\0';
			m_status = DOUBLE_QUOTE_STRING;
			break;
		default:
			m_status = DOUBLE_QUOTE_STRING;
			break;
		}

		return next_buffered_char();

	case BREAKING:
		m_status = OVER;
		break;

	case OVER:
		m_status = READY;
		break;
	}

	return EOF;
}

int InputStream::next_buffered_char() {
	return *m_cptr++;
}
