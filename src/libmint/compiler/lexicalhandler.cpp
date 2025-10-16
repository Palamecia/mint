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

#include "mint/compiler/lexicalhandler.h"
#include "mint/compiler/lexer.h"
#include "mint/compiler/token.h"

#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <algorithm>
#include <istream>
#include <string>
#include <tuple>
#include <vector>

using namespace mint;

#define IS_OPERATOR_ALIAS(_token) \
	(((_token) == "and") || ((_token) == "or") || ((_token) == "xor") || ((_token) == "not"))

#define IS_COMMENT(_token) \
	(((_token).find("/*", pos) != std::string::npos) || ((_token).find("//", pos) != std::string::npos) \
	    || ((_token).find("#!", pos) != std::string::npos))

enum class State : std::uint8_t {
	expect_start,
	expect_comment,
	expect_module,
	expect_definition,
	expect_value,
	expect_operator
};

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

char AbstractLexicalHandlerStream::operator[](std::string::size_type offset) const {
	return _script[offset];
}

std::size_t AbstractLexicalHandlerStream::pos() const {
	return _script.size();
}

int AbstractLexicalHandlerStream::read_char() {
	int c = get();
	if (c != EOF) {
		_script += static_cast<char>(c);
	}
	return c;
}

int AbstractLexicalHandlerStream::next_buffered_char() {
	int c = get();
	if (c != EOF) {
		_script += static_cast<char>(c);
	}
	return c;
}

namespace {

class LexicalHandlerStream : public AbstractLexicalHandlerStream {
public:
	LexicalHandlerStream(const LexicalHandlerStream&) = delete;
	LexicalHandlerStream(LexicalHandlerStream&&) = delete;

	LexicalHandlerStream(std::istream& stream) :
	    _stream(stream) {}

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

std::tuple<std::string::size_type, std::string> find_next_comment(AbstractLexicalHandlerStream& stream,
    std::string::size_type offset) {
	auto pos = std::min({stream.find("/*", offset), stream.find("//", offset), stream.find("#!", offset)});
	if (pos != std::string::npos) {
		return {pos, stream.substr(pos, 2)};
	}
	return {std::string::npos, {}};
}

}

bool LexicalHandler::parse(AbstractLexicalHandlerStream& stream) {

	std::vector<State> state = {State::expect_start};
	std::vector<std::string> context;

	std::string::size_type comment_offset = 0;
	std::string comment;

	auto lexer = Lexer(stream);
	std::size_t pos = 0;

	bool failed_on_new_line = false;
	stream.set_new_line_callback([&](std::size_t line_number) {
		if (failed_on_new_line) {
			return;
		}
		const auto new_line_pos = stream.find("\n", pos);
		while (pos && pos < new_line_pos) {
			switch (state.back()) {
			case State::expect_comment:
				if (auto comment_end = stream.find("*/", pos);
				    comment_end != std::string::npos && comment_end < new_line_pos) {
					comment_end += 2;
					comment += stream.substr(pos, comment_end - pos);
					if (!on_comment(stream.substr(pos, comment_end - pos), pos)) {
						failed_on_new_line = true;
						return;
					}
					if (!on_comment_end(comment_end)) {
						failed_on_new_line = true;
						return;
					}
					if (!on_token(Token::comment_token, comment, comment_offset)) {
						failed_on_new_line = true;
						return;
					}
					pos = comment_end;
					state.pop_back();
				}
				else if (auto comment_end = new_line_pos + 1; comment_end >= pos) {
					comment += stream.substr(pos, comment_end - pos);
					if (!on_comment(stream.substr(pos, comment_end - pos), pos)) {
						failed_on_new_line = true;
						return;
					}
					pos = comment_end;
				}
				break;
			default:
				if (auto [comment_pos, comment_token] = find_next_comment(stream, pos);
				    comment_pos != std::string::npos && comment_pos < new_line_pos) {
					if (pos != comment_pos) {
						if (!on_white_space(stream.substr(pos, comment_pos - pos), pos)) {
							failed_on_new_line = true;
							return;
						}
						pos = comment_pos;
					}
					auto start = new_line_pos;
					if (comment_token == "/*") {
						auto comment_end = stream.find("*/", comment_pos);
						if (comment_end != std::string::npos) {
							comment_end += 2;
							comment_offset = comment_pos;
							comment = stream.substr(comment_pos, comment_end - comment_pos);
							if (!on_comment_begin(comment_pos)) {
								failed_on_new_line = true;
								return;
							}
							if (!on_comment(stream.substr(comment_pos, comment_end - comment_pos), comment_pos)) {
								failed_on_new_line = true;
								return;
							}
							if (!on_comment_end(comment_end)) {
								failed_on_new_line = true;
								return;
							}
							if (!on_token(Token::comment_token, comment, comment_offset)) {
								failed_on_new_line = true;
								return;
							}
							start = comment_end;
						}
						else {
							comment_end = new_line_pos;
							comment_end += 1;
							comment_offset = comment_pos;
							comment = stream.substr(pos, comment_end - pos);
							if (!on_comment_begin(comment_pos)) {
								failed_on_new_line = true;
								return;
							}
							if (!on_comment(stream.substr(pos, comment_end - pos), comment_pos)) {
								failed_on_new_line = true;
								return;
							}
							state.emplace_back(State::expect_comment);
							start = comment_end;
						}
						pos = start;
					}
					else if ((comment_token == "//") || (comment_token == "#!")) {
						start = new_line_pos;
						comment_offset = comment_pos;
						comment = stream.substr(pos, start - pos);
						if (!on_comment_begin(comment_pos)) {
							failed_on_new_line = true;
							return;
						}
						if (!on_comment(stream.substr(pos, start - pos), comment_pos)) {
							failed_on_new_line = true;
							return;
						}
						if (!on_comment_end(start)) {
							failed_on_new_line = true;
							return;
						}
						if (!on_token(Token::comment_token, comment, comment_offset)) {
							failed_on_new_line = true;
							return;
						}
						pos = start;
					}
				}
				else if (pos != new_line_pos) {
					if (!on_white_space(stream.substr(pos, new_line_pos - pos), pos)) {
						failed_on_new_line = true;
						return;
					}
					pos = new_line_pos;
				}
				break;
			}
		}
		if (!on_new_line(line_number, pos ? new_line_pos + 1 : 0)) {
			failed_on_new_line = true;
			return;
		}
	});

	if (!on_script_begin()) {
		return false;
	}

	while (!stream.at_end()) {

		std::string token = lexer.next_token();
		auto token_type = token_from_local_id(Lexer::token_type(token));
		auto start = stream.find(token, pos);
		auto length = token.length();

		if (failed_on_new_line) {
			return false;
		}

		if (start == std::string::npos && token_type == Token::close_bracket_equal_token) {
			std::size_t match_length = 0;
			auto token_match = [&]() {
				match_length = 1;
				for (std::size_t i = start + 1; i < stream.pos(); ++i) {
					++match_length;
					if (stream[i] == '=') {
						return true;
					}
					if (!Lexer::is_white_space(stream[i])) {
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
			do {
				switch (state.back()) {
				case State::expect_comment:
					if (auto comment_end = stream.find("*/", pos);
					    comment_end != std::string::npos && comment_end < start) {
						comment_end += 2;
						comment += stream.substr(pos, comment_end - pos);
						if (!on_comment(stream.substr(pos, comment_end - pos), pos)) {
							return false;
						}
						if (!on_comment_end(comment_end)) {
							return false;
						}
						if (!on_token(Token::comment_token, comment, comment_offset)) {
							failed_on_new_line = true;
							return false;
						}
						state.pop_back();
						pos = comment_end;
					}
					else if (auto comment_end = stream.find('\n', pos); comment_end >= pos) {
						if (comment_end != std::string::npos) {
							comment_end += 1;
							comment += stream.substr(pos, comment_end - pos);
							if (!on_comment(stream.substr(pos, comment_end - pos), pos)) {
								return false;
							}
							pos = comment_end;
						}
						else {
							comment_end = stream.pos();
							comment += stream.substr(pos);
							if (!on_comment(stream.substr(pos), pos)) {
								return false;
							}
							pos = comment_end;
						}
					}
					else if (start != pos) {
						if (!on_white_space(stream.substr(pos, (start - pos)), pos)) {
							return false;
						}
						pos = start;
					}
					break;
				default:
					if (auto [comment_pos, comment_token] = find_next_comment(stream, pos);
					    (comment_pos >= pos) && (comment_pos <= start)) {
						if (pos != comment_pos) {
							if (!on_white_space(stream.substr(pos, comment_pos - pos), pos)) {
								return false;
							}
							pos = comment_pos;
						}
						if (comment_token == "/*") {
							auto comment_end = stream.find("*/", comment_pos);
							if (comment_end != std::string::npos) {
								comment_end += 2;
								comment_offset = comment_pos;
								comment = stream.substr(comment_pos, comment_end - comment_pos);
								if (!on_comment_begin(comment_pos)) {
									return false;
								}
								if (!on_comment(stream.substr(comment_pos, comment_end - comment_pos), comment_pos)) {
									return false;
								}
								if (!on_comment_end(comment_end)) {
									return false;
								}
								if (!on_token(Token::comment_token, comment, comment_offset)) {
									failed_on_new_line = true;
									return false;
								}
								pos = comment_end;
							}
							else {
								comment_offset = comment_pos;
								comment = stream.substr(pos);
								if (!on_comment_begin(comment_pos)) {
									return false;
								}
								if (!on_comment(stream.substr(pos), comment_pos)) {
									return false;
								}
								state.emplace_back(State::expect_comment);
								pos = stream.pos();
							}
						}
						else if ((comment_token == "//") || (comment_token == "#!")) {
							auto comment_end = std::min(start, stream.pos());
							comment_offset = comment_pos;
							comment = stream.substr(pos, comment_end - pos);
							if (!on_comment_begin(comment_pos)) {
								return false;
							}
							if (!on_comment(stream.substr(pos, comment_end - pos), comment_pos)) {
								return false;
							}
							if (!on_comment_end(comment_end)) {
								return false;
							}
							if (!on_token(Token::comment_token, comment, comment_offset)) {
								failed_on_new_line = true;
								return false;
							}
							pos = comment_end;
						}
						start = stream.find(token, pos);
					}
					else if (start != pos) {
						if (!on_white_space(stream.substr(pos, (start - pos)), pos)) {
							return false;
						}
						pos = start;
					}
					break;
				}
			}
			while (pos < start);

			switch (token_type) {
			case Token::line_end_token:
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
					    !regex.empty() && stream[start + regex.length() + 1] == '/') {
						token += regex + lexer.next_token();
						length = token.length();

						if (isalpha(stream[start + length])) {
							token += lexer.next_token();
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
				if ((IS_OPERATOR_ALIAS(token)) || (Lexer::is_operator(token))) {
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
		else {
			token = stream.substr(pos);
			if (IS_COMMENT(token)) {
				if (!on_comment(token, pos)) {
					return false;
				}
			}
			else {
				if (!on_token(Token::symbol_token, token, start)) {
					return false;
				}
			}
		}

		pos = start + length;
	}

	if (!context.empty() && !state.empty() && state.back() == State::expect_value && !on_symbol_token(context, pos)) {
		return false;
	}

	if (pos != stream.pos()) {
		if (!on_white_space(stream.substr(pos), pos)) {
			return false;
		}
	}

	return on_script_end();
}

bool LexicalHandler::parse(std::istream& script) {
	LexicalHandlerStream stream(script);
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
