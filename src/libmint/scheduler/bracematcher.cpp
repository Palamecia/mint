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

#include "bracematcher.h"

using namespace mint;

BraceMatcher::BraceMatcher(std::pair<std::string_view::size_type, bool> &match, std::string_view::size_type offset) :
	m_match(match),
	m_offset(offset) {
	m_match = {std::string_view::npos, true};
}

bool BraceMatcher::on_token(mint::token::Type type, const std::string &token, std::string::size_type offset) {
	switch (type) {
	case token::STRING_TOKEN:
	case token::REGEX_TOKEN:
		if (token.size() < 2 || token.front() != token.back()) {
			m_match.second = false;
		}
		else if (m_offset == offset) {
			m_match.first = offset + token.size() - 1;
		}
		else if (m_offset == offset + token.size() - 1) {
			m_match.first = offset;
		}
		break;
	case token::OPEN_BRACE_TOKEN:
		if (m_offset == offset) {
			m_brace_open = m_brace_depth.size();
		}
		m_brace_depth.push_back(offset);
		break;
	case token::CLOSE_BRACE_TOKEN:
		if (m_offset == offset) {
			m_match.first = m_brace_depth.back();
		}
		m_brace_depth.pop_back();
		if (m_brace_open && *m_brace_open == m_brace_depth.size()) {
			m_match.first = offset;
			m_brace_open = std::nullopt;
		}
		break;
	case token::OPEN_BRACKET_TOKEN:
		if (m_offset == offset) {
			m_bracket_open = m_bracket_depth.size();
		}
		m_bracket_depth.push_back(offset);
		break;
	case token::CLOSE_BRACKET_TOKEN:
	case token::CLOSE_BRACKET_EQUAL_TOKEN:
		if (m_offset == offset) {
			m_match.first = m_bracket_depth.back();
		}
		m_bracket_depth.pop_back();
		if (m_bracket_open && *m_bracket_open == m_bracket_depth.size()) {
			m_match.first = offset;
			m_bracket_open = std::nullopt;
		}
		break;
	case token::OPEN_PARENTHESIS_TOKEN:
		if (m_offset == offset) {
			m_parenthesis_open = m_parenthesis_depth.size();
		}
		m_parenthesis_depth.push_back(offset);
		break;
	case token::CLOSE_PARENTHESIS_TOKEN:
		if (m_offset == offset) {
			m_match.first = m_parenthesis_depth.back();
		}
		m_parenthesis_depth.pop_back();
		if (m_parenthesis_open && *m_parenthesis_open == m_parenthesis_depth.size()) {
			m_match.first = offset;
			m_parenthesis_open = std::nullopt;
		}
		break;
	default:
		break;
	}
	return true;
}

bool BraceMatcher::on_comment_begin([[maybe_unused]] std::string::size_type offset) {
	m_comment = true;
	return true;
}

bool BraceMatcher::on_comment_end([[maybe_unused]] std::string::size_type offset) {
	m_comment = false;
	return true;
}

bool BraceMatcher::on_script_end() {
	if (m_comment || !m_brace_depth.empty() || !m_bracket_depth.empty() || !m_parenthesis_depth.empty()) {
		m_match.second = false;
	}
	return true;
}
