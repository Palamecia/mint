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

#include "mint/compiler/lexical_handler.h"
#include "mint/compiler/lexer.h"
#include "mint/compiler/token.h"

#include <cassert>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <istream>
#include <queue>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

using namespace mint;

namespace {

constexpr bool is_operator_alias(std::string_view token) {
	return ((token == "and") || (token == "or") || (token == "xor") || (token == "not"));
}

enum class State : std::uint8_t {
	expect_start,
	expect_module,
	expect_definition,
	expect_value,
	expect_operator,
};

}

std::filesystem::path AbstractLexicalHandlerStream::path() const {
	return {};
}

std::string::size_type AbstractLexicalHandlerStream::find(const std::string& substr,
    std::string::size_type offset) const noexcept {
	return substr.empty() ? _script.length() : _script.find(substr, offset);
}

std::string::size_type AbstractLexicalHandlerStream::find(const std::string::value_type ch,
    std::string::size_type offset) const noexcept {
	return _script.find(ch, offset);
}

std::string AbstractLexicalHandlerStream::substr(std::string::size_type offset,
    std::string::size_type count) const noexcept {
	return _script.substr(offset, count);
}

char AbstractLexicalHandlerStream::at(std::string::size_type offset) const {
	return _script.at(offset);
}

std::size_t AbstractLexicalHandlerStream::pos() const {
	return _script.size();
}

int AbstractLexicalHandlerStream::read_char() {
	const auto c = get();
	if (c != EOF) {
		_script += static_cast<char>(c);
	}
	return c;
}

int AbstractLexicalHandlerStream::next_buffered_char() {
	const auto c = get();
	if (c != EOF) {
		_script += static_cast<char>(c);
	}
	return c;
}

namespace {

class LexicalHandlerStream : public AbstractLexicalHandlerStream {
public:
	explicit LexicalHandlerStream(std::istream& stream) :
	    _stream(stream) {}

	LexicalHandlerStream(const LexicalHandlerStream&) = delete;
	LexicalHandlerStream(LexicalHandlerStream&&) = delete;

	~LexicalHandlerStream() {}

	LexicalHandlerStream& operator=(const LexicalHandlerStream&) = delete;
	LexicalHandlerStream& operator=(LexicalHandlerStream&&) = delete;

	[[nodiscard]] bool at_end() const override {
		return _stream.eof();
	}

	[[nodiscard]] bool is_valid() const override {
		return _stream.good();
	}

protected:
	int get() override {
		return _stream.get();
	}

private:
	std::istream& _stream;
};

}

bool LexicalHandler::parse(AbstractLexicalHandlerStream& stream) {

	auto pending_lines = std::queue<std::pair<std::size_t, std::string::size_type>>();
	auto state = std::vector<State> {State::expect_start};
	auto context = std::vector<std::string>();

	auto lexer = Lexer(stream);
	std::size_t pos = 0;

	stream.set_new_line_callback([&](std::size_t line_number) {
		if (pending_lines.empty()) {
			if (pos != 0) {
				pending_lines.emplace(line_number, stream.find("\n", pos));
			}
			else {
				pending_lines.emplace(line_number, 0);
			}
		}
		else {
			pending_lines.emplace(line_number, stream.find("\n", pending_lines.back().second + 1));
		}
	});

	if (!on_script_begin()) {
		return false;
	}

	while (!stream.at_end()) {

		auto [token, token_id] = lexer.next_token();
		const auto token_type = mint::token_from_local_id(token_id);
		auto start = stream.find(token, pos);
		auto length = token.length();

		if (pos == 0 && !pending_lines.empty()) {
			const auto [line_number, new_line_pos] = pending_lines.front();
			pending_lines.pop();
			if (!on_new_line(line_number, new_line_pos)) {
				return false;
			}
		}

		if (start == std::string::npos && token_type == Token::close_bracket_equal_token) {
			std::size_t match_length = 0;
			const auto token_match = [&] {
				match_length = 1;
				for (std::size_t i = start + 1; i < stream.pos(); ++i) {
					++match_length;
					if (stream.at(i) == '=') {
						return true;
					}
					if (!Lexer::is_white_space(stream.at(i))) {
						return false;
					}
				}
				return false;
			};
			start = stream.find(']', pos);
			while (start != std::string::npos && !token_match()) {
				start = stream.find(']', start + 1);
			}
			if (start != std::string::npos) {
				token = stream.substr(start, match_length);
				length = match_length;
			}
		}

		if (start != std::string::npos) {
			if (start != pos && !on_white_space(stream.substr(pos, start - pos), pos)) {
				return false;
			}
			pos = start;
			switch (token_type) {
			case Token::comment_token:
				if (token.starts_with("/*")) {
					if (!on_comment_begin(start)) {
						return false;
					}
					std::string::size_type from = 0;
					for (std::string::size_type pos = token.find('\n'); pos != std::string::npos;
					    from = pos + 1, pos = token.find('\n', from)) {
						if (!on_comment(token.substr(from, pos - from + 1), start + from)) {
							return false;
						}
						const auto [line_number, new_line_pos] = pending_lines.front();
						pending_lines.pop();
						if (!on_new_line(line_number, new_line_pos)) {
							return false;
						}
					}
					if (!on_comment(token.substr(from, length - from), start + from)) {
						return false;
					}
					if (token.ends_with("*/") && !on_comment_end(start + length)) {
						return false;
					}
					if (!on_token(token_type, token, start)) {
						return false;
					}
				}
				else {
					if (!on_comment_begin(start) || !on_comment(token, start) || !on_comment_end(start + length)
					    || !on_token(token_type, token, start)) {
						return false;
					}
				}
				pos = start + length;
				continue;
			case Token::no_line_end_token:
				if (!on_token(token_type, token, start)) {
					return false;
				}
				{
					const auto [line_number, new_line_pos] = pending_lines.front();
					pending_lines.pop();
					if (!on_new_line(line_number, new_line_pos)) {
						return false;
					}
				}
				pos = start + length;
				break;
			case Token::line_end_token:
				if (token == "\n") {
					const auto [line_number, new_line_pos] = pending_lines.front();
					pending_lines.pop();
					if (!on_new_line(line_number, new_line_pos)) {
						return false;
					}
				}
				[[fallthrough]];
			case Token::file_end_token:
				switch (state.back()) {
				case State::expect_module:
					state.pop_back();
					context.clear();
					break;
				default:
					break;
				}
				context.clear();
				if (!on_token(token_type, token, start)) {
					return false;
				}
				pos = start + length;
				continue;
			case Token::assert_token:
			case Token::break_token:
			case Token::case_token:
			case Token::catch_token:
			case Token::class_token:
			case Token::const_token:
			case Token::continue_token:
			case Token::default_token:
			case Token::elif_token:
			case Token::else_token:
			case Token::enum_token:
			case Token::exit_token:
			case Token::final_token:
			case Token::for_token:
			case Token::if_token:
			case Token::in_token:
			case Token::let_token:
			case Token::lib_token:
			case Token::override_token:
			case Token::package_token:
			case Token::print_token:
			case Token::raise_token:
			case Token::return_token:
			case Token::switch_token:
			case Token::try_token:
			case Token::while_token:
			case Token::yield_token:
			case Token::var_token:
			case Token::constant_token:
			case Token::is_token:
			case Token::typeof_token:
			case Token::membersof_token:
			case Token::defined_token:
				switch (state.back()) {
				case State::expect_module:
					if (!on_module_path_token(context, token, start)) {
						return false;
					}
					context.push_back(token);
					if (!on_token(Token::module_path_token, token, start)) {
						return false;
					}
					break;
				default:
					if (!context.empty() && !state.empty() && state.back() == State::expect_value
					    && !on_symbol_token(context, pos)) {
						return false;
					}
					context.clear();
					state.back() = State::expect_start;
					if (!on_token(token_type, token, start)) {
						return false;
					}
				}
				break;

			case Token::def_token:
				switch (state.back()) {
				case State::expect_module:
					if (!on_module_path_token(context, token, start)) {
						return false;
					}
					context.push_back(token);
					if (!on_token(Token::module_path_token, token, start)) {
						return false;
					}
					break;
				default:
					if (!context.empty() && !state.empty() && state.back() == State::expect_value
					    && !on_symbol_token(context, pos)) {
						return false;
					}
					context.clear();
					state.back() = State::expect_definition;
					if (!on_token(token_type, token, start)) {
						return false;
					}
				}
				break;

			case Token::load_token:
				switch (state.back()) {
				case State::expect_module:
					if (!on_module_path_token(context, token, start)) {
						return false;
					}
					context.push_back(token);
					if (!on_token(Token::module_path_token, token, start)) {
						return false;
					}
					break;
				default:
					if (!context.empty() && !state.empty() && state.back() == State::expect_value
					    && !on_symbol_token(context, pos)) {
						return false;
					}
					context.clear();
					state.emplace_back(State::expect_module);
					if (!on_token(token_type, token, start)) {
						return false;
					}
				}
				break;

			case Token::number_token:
			case Token::string_token:
				if (!context.empty() && !state.empty() && state.back() == State::expect_value
				    && !on_symbol_token(context, pos)) {
					return false;
				}
				context.clear();
				state.back() = State::expect_operator;
				if (!on_token(token_type, token, start)) {
					return false;
				}
				break;

			case Token::slash_token:
				if (!context.empty() && !state.empty() && state.back() == State::expect_value
				    && !on_symbol_token(context, pos)) {
					return false;
				}
				context.clear();
				switch (state.back()) {
				case State::expect_operator:
				case State::expect_definition:
					state.back() = State::expect_value;
					if (!on_token(token_type, token, start)) {
						return false;
					}
					break;
				default:
					if (const std::string regex = lexer.read_regex();
					    !regex.empty() && stream.at(start + regex.length() + 1) == '/') {
						token += regex + lexer.next_token().first;
						length = token.length();

						if (isalpha(stream.at(start + length))) {
							token += lexer.next_token().first;
							length = token.length();
						}

						state.back() = State::expect_operator;
						if (!on_token(Token::regex_token, token, start)) {
							return false;
						}
					}
					else {
						if (!on_token(token_type, token, start)) {
							return false;
						}
					}
				}
				break;

			case Token::symbol_token:
				switch (state.back()) {
				case State::expect_module:
					if (!on_module_path_token(context, token, start)) {
						return false;
					}
					context.push_back(token);
					if (!on_token(Token::module_path_token, token, start)) {
						return false;
					}
					break;
				default:
					if (!on_symbol_token(context, token, start)) {
						return false;
					}
					context.push_back(token);
					state.back() = State::expect_operator;
					if (!on_token(token_type, token, start)) {
						return false;
					}
				}
				break;

			case Token::dot_token:
				switch (state.back()) {
				case State::expect_module:
					if (!on_module_path_token(context, token, start)) {
						return false;
					}
					context.push_back(token);
					if (!on_token(Token::module_path_token, token, start)) {
						return false;
					}
					break;
				default:
					state.back() = State::expect_value;
					if (!on_token(token_type, token, start)) {
						return false;
					}
				}
				break;

			case Token::close_brace_token:
			case Token::close_parenthesis_token:
			case Token::close_bracket_equal_token:
				if (!context.empty() && !state.empty() && state.back() == State::expect_value
				    && !on_symbol_token(context, pos)) {
					return false;
				}
				context.clear();
				state.back() = State::expect_operator;
				if (!on_token(token_type, token, start)) {
					return false;
				}
				break;

			default:
				if (!context.empty() && !state.empty() && state.back() == State::expect_value
				    && !on_symbol_token(context, pos)) {
					return false;
				}
				context.clear();
				if (is_operator_alias(token) || Lexer::is_operator(token)) {
					state.back() = State::expect_value;
				}
				else {
					state.back() = State::expect_operator;
				}
				if (!on_token(token_type, token, start)) {
					return false;
				}
				break;
			}
		}
		else if (!token.empty() && !on_token(Token::symbol_token, token, pos)) {
			return false;
		}

		pos = start + length;
	}

	if (!context.empty() && !state.empty() && state.back() == State::expect_value && !on_symbol_token(context, pos)) {
		return false;
	}

	if ((pos != stream.pos()) && (!on_white_space(stream.substr(pos), pos))) {
		return false;
	}

	return on_script_end();
}

bool LexicalHandler::parse(std::istream& script) {
	auto stream = LexicalHandlerStream(script);
	return parse(stream);
}

bool LexicalHandler::on_script_begin() {
	return true;
}

bool LexicalHandler::on_script_end() {
	return true;
}

bool LexicalHandler::on_comment_begin(std::string::size_type /*offset*/) {
	return true;
}

bool LexicalHandler::on_comment_end(std::string::size_type /*offset*/) {
	return true;
}

bool LexicalHandler::on_module_path_token(const std::vector<std::string>& /*context*/, const std::string& /*token*/,
    std::string::size_type /*offset*/) {
	return true;
}

bool LexicalHandler::on_symbol_token(const std::vector<std::string>& /*context*/, const std::string& /*token*/,
    std::string::size_type /*offset*/) {
	return true;
}

bool LexicalHandler::on_symbol_token(const std::vector<std::string>& /*context*/, std::string::size_type /*offset*/) {
	return true;
}

bool LexicalHandler::on_token(Token /*type*/, const std::string& /*token*/, std::string::size_type /*offset*/) {
	return true;
}

bool LexicalHandler::on_white_space(const std::string& /*token*/, std::string::size_type /*offset*/) {
	return true;
}

bool LexicalHandler::on_comment(const std::string& /*token*/, std::string::size_type /*offset*/) {
	return true;
}

bool LexicalHandler::on_new_line(std::size_t /*line_number*/, std::string::size_type /*offset*/) {
	return true;
}
