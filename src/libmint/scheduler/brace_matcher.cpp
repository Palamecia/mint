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

#include "brace_matcher.h"
#include "mint/compiler/token.h"
#include <optional>
#include <string>
#include <string_view>
#include <utility>

using namespace mint;

BraceMatcher::BraceMatcher(std::string_view::size_type offset) :
    _offset(offset) {}

std::pair<std::string_view::size_type, bool> BraceMatcher::match() const {
	return _match;
}

bool BraceMatcher::on_token(mint::Token type, const std::string& token, std::string::size_type offset) {
	switch (type) {
	case Token::string_token:
	case Token::regex_token:
		if (token.size() < 2 || token.front() != token.back()) {
			_match.second = false;
		}
		else if (_offset == offset) {
			_match.first = offset + token.size() - 1;
		}
		else if (_offset == offset + token.size() - 1) {
			_match.first = offset;
		}
		break;
	case Token::open_brace_token:
		if (_offset == offset) {
			_brace_open = _brace_depth.size();
		}
		_brace_depth.push_back(offset);
		break;
	case Token::close_brace_token:
		if (_offset == offset) {
			_match.first = _brace_depth.back();
		}
		_brace_depth.pop_back();
		if (_brace_open && *_brace_open == _brace_depth.size()) {
			_match.first = offset;
			_brace_open = std::nullopt;
		}
		break;
	case Token::open_bracket_token:
		if (_offset == offset) {
			_bracket_open = _bracket_depth.size();
		}
		_bracket_depth.push_back(offset);
		break;
	case Token::close_bracket_token:
	case Token::close_bracket_equal_token:
		if (_offset == offset) {
			_match.first = _bracket_depth.back();
		}
		_bracket_depth.pop_back();
		if (_bracket_open && *_bracket_open == _bracket_depth.size()) {
			_match.first = offset;
			_bracket_open = std::nullopt;
		}
		break;
	case Token::open_parenthesis_token:
		if (_offset == offset) {
			_parenthesis_open = _parenthesis_depth.size();
		}
		_parenthesis_depth.push_back(offset);
		break;
	case Token::close_parenthesis_token:
		if (_offset == offset) {
			_match.first = _parenthesis_depth.back();
		}
		_parenthesis_depth.pop_back();
		if (_parenthesis_open && *_parenthesis_open == _parenthesis_depth.size()) {
			_match.first = offset;
			_parenthesis_open = std::nullopt;
		}
		break;
	default:
		break;
	}
	return true;
}

bool BraceMatcher::on_comment_begin([[maybe_unused]] std::string::size_type offset) {
	_comment = true;
	return true;
}

bool BraceMatcher::on_comment_end([[maybe_unused]] std::string::size_type offset) {
	_comment = false;
	return true;
}

bool BraceMatcher::on_script_end() {
	if (_comment || !_brace_depth.empty() || !_bracket_depth.empty() || !_parenthesis_depth.empty()) {
		_match.second = false;
	}
	return true;
}
