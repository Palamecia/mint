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

#include "mint/compiler/lexicalhandler.h"
#include "mint/compiler/lexer.h"

using namespace mint;

#define is_operator_alias(_token) \
	((_token == "and") || (_token == "or") || (_token == "xor") || (_token == "not"))

#define is_comment(_token) \
	((_token.find("/*", pos) != std::string::npos) \
	|| (_token.find("//", pos) != std::string::npos) \
	|| (_token.find("#!", pos) != std::string::npos))

enum State {
	EXPECT_START,
	EXPECT_COMMENT,
	EXPECT_MODULE,
	EXPECT_DEFINITION,
	EXPECT_VALUE,
	EXPECT_OPERATOR
};

std::string AbstractLexicalHandlerStream::path() const {
	return {};
}

std::string::size_type AbstractLexicalHandlerStream::find(const std::string &substr, std::string::size_type offset) const noexcept {
	return substr.empty() ? m_script.length() : m_script.find(substr, offset);
}

std::string::size_type AbstractLexicalHandlerStream::find(const std::string::value_type ch, std::string::size_type offset) const noexcept {
	return m_script.find(ch, offset);
}

std::string AbstractLexicalHandlerStream::substr(std::string::size_type offset, std::string::size_type count) const noexcept {
	return m_script.substr(offset, count);
}

char AbstractLexicalHandlerStream::operator [](std::string::size_type offset) const {
	return m_script[offset];
}

size_t AbstractLexicalHandlerStream::pos() const {
	return m_script.size();
}

int AbstractLexicalHandlerStream::read_char() {
	int c = get();
	if (c != EOF) {
		m_script += c;
	}
	return c;
}

int AbstractLexicalHandlerStream::next_buffered_char() {
	int c = get();
	if (c != EOF) {
		m_script += c;
	}
	return c;
}

class LexicalHandlerStream : public AbstractLexicalHandlerStream {
public:
	LexicalHandlerStream(std::istream &stream) :
		m_stream(stream) {

	}

	~LexicalHandlerStream() {

	}

	bool at_end() const override {
		return m_stream.eof();
	}

	bool is_valid() const override {
		return m_stream.good();
	}

protected:
	int get() override {
		return m_stream.get();
	}

private:
	std::istream &m_stream;
};

static std::tuple<std::string::size_type, std::string> find_next_comment(AbstractLexicalHandlerStream &stream, std::string::size_type offset) {
	auto pos = std::min(stream.find("/*", offset), std::min(stream.find("//", offset), stream.find("#!", offset)));
	if (pos != std::string::npos) {
		return { pos, stream.substr(pos, 2) };
	}
	return { std::string::npos, {} };
}

bool LexicalHandler::parse(AbstractLexicalHandlerStream &stream) {

	std::vector<State> state = {EXPECT_START};
	std::vector<std::string> context;

	std::string::size_type comment_offset;
	std::string comment;

	Lexer lexer(&stream);
	size_t pos = 0;

	bool failed_on_new_line = false;
	stream.set_new_line_callback([&](size_t line_number) {
		if (failed_on_new_line) {
			return;
		}
		const auto new_line_pos = stream.find("\n", pos);
		while (pos && pos < new_line_pos) {
			switch (state.back()) {
			case EXPECT_COMMENT:
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
					if (!on_token(token::COMMENT_TOKEN, comment, comment_offset)) {
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
							if (!on_token(token::COMMENT_TOKEN, comment, comment_offset)) {
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
							state.emplace_back(EXPECT_COMMENT);
							start = comment_end;
						}
						pos = start;
					}
					else if (comment_token == "//") {
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
						if (!on_token(token::COMMENT_TOKEN, comment, comment_offset)) {
							failed_on_new_line = true;
							return;
						}
						pos = start;
					}
					else if (comment_token == "#!") {
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
						if (!on_token(token::COMMENT_TOKEN, comment, comment_offset)) {
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
		auto token_type = token::from_local_id(lexer.token_type(token));
		auto start = stream.find(token, pos);
		auto length = token.length();

		if (failed_on_new_line) {
			return false;
		}

		if (start == std::string::npos && token_type == token::CLOSE_BRACKET_EQUAL_TOKEN) {
			size_t match_length = 0;
			auto token_match = [&]() {
				match_length = 1;
				for (size_t i = start + 1; i < stream.pos(); ++i) {
					++match_length;
					if (stream[i] == '=') {
						return true;
					}
					if (!lexer.is_white_space(stream[i])) {
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
				case EXPECT_COMMENT:
					if (auto comment_end = stream.find("*/", pos); comment_end != std::string::npos && comment_end < start) {
						comment_end += 2;
						comment += stream.substr(pos, comment_end - pos);
						if (!on_comment(stream.substr(pos, comment_end - pos), pos)) {
							return false;
						}
						if (!on_comment_end(comment_end)) {
							return false;
						}
						if (!on_token(token::COMMENT_TOKEN, comment, comment_offset)) {
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
								if (!on_token(token::COMMENT_TOKEN, comment, comment_offset)) {
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
								state.emplace_back(EXPECT_COMMENT);
								pos = stream.pos();
							}
						}
						else if (comment_token == "//") {
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
							if (!on_token(token::COMMENT_TOKEN, comment, comment_offset)) {
								failed_on_new_line = true;
								return false;
							}
							pos = comment_end;
						}
						else if (comment_token == "#!") {
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
							if (!on_token(token::COMMENT_TOKEN, comment, comment_offset)) {
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
			case token::LINE_END_TOKEN:
			case token::FILE_END_TOKEN:
				switch (state.back()) {
				case EXPECT_MODULE:
					state.pop_back();
					context.clear();
					break;
				default:
					break;
				}
				if (!on_token(token_type, token, start)) {
					return false;
				}
				pos = start + length;
				continue;
			case token::ASSERT_TOKEN:
			case token::BREAK_TOKEN:
			case token::CASE_TOKEN:
			case token::CATCH_TOKEN:
			case token::CLASS_TOKEN:
			case token::CONST_TOKEN:
			case token::CONTINUE_TOKEN:
			case token::DEFAULT_TOKEN:
			case token::ELIF_TOKEN:
			case token::ELSE_TOKEN:
			case token::ENUM_TOKEN:
			case token::EXIT_TOKEN:
			case token::FINAL_TOKEN:
			case token::FOR_TOKEN:
			case token::IF_TOKEN:
			case token::IN_TOKEN:
			case token::LET_TOKEN:
			case token::LIB_TOKEN:
			case token::OVERRIDE_TOKEN:
			case token::PACKAGE_TOKEN:
			case token::PRINT_TOKEN:
			case token::RAISE_TOKEN:
			case token::RETURN_TOKEN:
			case token::SWITCH_TOKEN:
			case token::TRY_TOKEN:
			case token::WHILE_TOKEN:
			case token::YIELD_TOKEN:
			case token::VAR_TOKEN:
			case token::CONSTANT_TOKEN:
			case token::IS_TOKEN:
			case token::TYPEOF_TOKEN:
			case token::MEMBERSOF_TOKEN:
			case token::DEFINED_TOKEN:
				switch (state.back()) {
				case EXPECT_MODULE:
					if (!on_module_path_token(context, token, start)) {
						return false;
					}
					context.push_back(token);
					if (!on_token(token::MODULE_PATH_TOKEN, token, start)) {
						return false;
					}
					break;
				default:
					if (!context.empty() && !state.empty() && state.back() == EXPECT_VALUE && !on_symbol_token(context, pos)) {
						return false;
					}
					context.clear();
					state.back() = EXPECT_START;
					if (!on_token(token_type, token, start)) {
						return false;
					}
				}
				break;

			case token::DEF_TOKEN:
				switch (state.back()) {
				case EXPECT_MODULE:
					if (!on_module_path_token(context, token, start)) {
						return false;
					}
					context.push_back(token);
					if (!on_token(token::MODULE_PATH_TOKEN, token, start)) {
						return false;
					}
					break;
				default:
					if (!context.empty() && !state.empty() && state.back() == EXPECT_VALUE && !on_symbol_token(context, pos)) {
						return false;
					}
					context.clear();
					state.back() = EXPECT_DEFINITION;
					if (!on_token(token_type, token, start)) {
						return false;
					}
				}
				break;

			case token::LOAD_TOKEN:
				switch (state.back()) {
				case EXPECT_MODULE:
					if (!on_module_path_token(context, token, start)) {
						return false;
					}
					context.push_back(token);
					if (!on_token(token::MODULE_PATH_TOKEN, token, start)) {
						return false;
					}
					break;
				default:
					if (!context.empty() && !state.empty() && state.back() == EXPECT_VALUE && !on_symbol_token(context, pos)) {
						return false;
					}
					context.clear();
					state.emplace_back(EXPECT_MODULE);
					if (!on_token(token_type, token, start)) {
						return false;
					}
				}
				break;

			case token::NUMBER_TOKEN:
				if (!context.empty() && !state.empty() && state.back() == EXPECT_VALUE && !on_symbol_token(context, pos)) {
					return false;
				}
				context.clear();
				state.back() = EXPECT_OPERATOR;
				if (!on_token(token_type, token, start)) {
					return false;
				}
				break;

			case token::STRING_TOKEN:
				if (!context.empty() && !state.empty() && state.back() == EXPECT_VALUE && !on_symbol_token(context, pos)) {
					return false;
				}
				context.clear();
				state.back() = EXPECT_OPERATOR;
				if (!on_token(token_type, token, start)) {
					return false;
				}
				break;

			case token::SLASH_TOKEN:
				if (!context.empty() && !state.empty() && state.back() == EXPECT_VALUE && !on_symbol_token(context, pos)) {
					return false;
				}
				context.clear();
				switch (state.back()) {
				case EXPECT_OPERATOR:
				case EXPECT_DEFINITION:
					state.back() = EXPECT_VALUE;
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

						state.back() = EXPECT_OPERATOR;
						if (!on_token(token::REGEX_TOKEN, token, start)) {
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

			case token::SYMBOL_TOKEN:
				switch (state.back()) {
				case EXPECT_MODULE:
					if (!on_module_path_token(context, token, start)) {
						return false;
					}
					context.push_back(token);
					if (!on_token(token::MODULE_PATH_TOKEN, token, start)) {
						return false;
					}
					break;
				default:
					if (!on_symbol_token(context, token, start)) {
						return false;
					}
					context.push_back(token);
					state.back() = EXPECT_OPERATOR;
					if (!on_token(token_type, token, start)) {
						return false;
					}
				}
				break;

			case token::DOT_TOKEN:
				switch (state.back()) {
				case EXPECT_MODULE:
					if (!on_module_path_token(context, token, start)) {
						return false;
					}
					context.push_back(token);
					if (!on_token(token::MODULE_PATH_TOKEN, token, start)) {
						return false;
					}
					break;
				default:
					state.back() = EXPECT_VALUE;
					if (!on_token(token_type, token, start)) {
						return false;
					}
				}
				break;

			case token::CLOSE_BRACE_TOKEN:
			case token::CLOSE_PARENTHESIS_TOKEN:
			case token::CLOSE_BRACKET_EQUAL_TOKEN:
				if (!context.empty() && !state.empty() && state.back() == EXPECT_VALUE && !on_symbol_token(context, pos)) {
					return false;
				}
				context.clear();
				state.back() = EXPECT_OPERATOR;
				if (!on_token(token_type, token, start)) {
					return false;
				}
				break;

			default:
				if (!context.empty() && !state.empty() && state.back() == EXPECT_VALUE && !on_symbol_token(context, pos)) {
					return false;
				}
				context.clear();
				if (is_operator_alias(token)) {
					state.back() = EXPECT_VALUE;
				}
				else if (Lexer::is_operator(token)) {
					state.back() = EXPECT_VALUE;
				}
				else {
					state.back() = EXPECT_OPERATOR;
				}
				if (!on_token(token_type, token, start)) {
					return false;
				}
				break;
			}
		}
		else {
			token = stream.substr(pos);
			if (is_comment(token)) {
				if (!on_comment(token, pos)) {
					return false;
				}
			}
			else {
				if (!on_token(token::SYMBOL_TOKEN, token, start)) {
					return false;
				}
			}
		}

		pos = start + length;
	}

	if (!context.empty() && !state.empty() && state.back() == EXPECT_VALUE && !on_symbol_token(context, pos)) {
		return false;
	}

	if (pos != stream.pos()) {
		if (!on_white_space(stream.substr(pos), pos)) {
			return false;
		}
	}

	return on_script_end();
}

bool LexicalHandler::parse(std::istream &script) {
	LexicalHandlerStream stream(script);
	return parse(stream);
}

bool LexicalHandler::on_script_begin() {
	return true;
}

bool LexicalHandler::on_script_end() {
	return true;
}

bool LexicalHandler::on_comment_begin(std::string::size_type offset) {
	return true;
}

bool LexicalHandler::on_comment_end(std::string::size_type offset) {
	return true;
}

bool LexicalHandler::on_module_path_token(const std::vector<std::string> &context, const std::string &token, std::string::size_type offset) {
	return true;
}

bool LexicalHandler::on_symbol_token(const std::vector<std::string> &context, const std::string &token, std::string::size_type offset) {
	return true;
}

bool LexicalHandler::on_symbol_token(const std::vector<std::string> &context, std::string::size_type offset) {
	return true;
}

bool LexicalHandler::on_token(token::Type type, const std::string &token, std::string::size_type offset) {
	return true;
}

bool LexicalHandler::on_white_space(const std::string &token, std::string::size_type offset) {
	return true;
}

bool LexicalHandler::on_comment(const std::string &token, std::string::size_type offset) {
	return true;
}

bool LexicalHandler::on_new_line(size_t line_number, std::string::size_type offset) {
	return true;
}
