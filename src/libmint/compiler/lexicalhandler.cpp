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
using namespace std;

#define is_operator_alias(_token) \
	((_token == "and") || (_token == "or") || (_token == "xor") || (_token == "not"))

#define is_comment(_token) \
	((_token.find("/*", pos) != string::npos) || (_token.find("//", pos) != string::npos) || (_token.find("#!", pos) != string::npos))

enum State {
	expect_start,
	expect_comment,
	expect_module,
	expect_value,
	expect_operator
};

string AbstractLexicalHandlerStream::path() const {
	return {};
}

string::size_type AbstractLexicalHandlerStream::find(const string &substr, string::size_type offset) const noexcept {
	return m_script.find(substr, offset);
}

string::size_type AbstractLexicalHandlerStream::find(const string::value_type ch, string::size_type offset) const noexcept {
	return m_script.find(ch, offset);
}

string AbstractLexicalHandlerStream::substr(string::size_type offset, string::size_type count) const noexcept {
	return m_script.substr(offset, count);
}

char AbstractLexicalHandlerStream::operator [](string::size_type offset) const {
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
	LexicalHandlerStream(istream &stream) :
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
	istream &m_stream;
	string m_script;
};

bool LexicalHandler::parse(AbstractLexicalHandlerStream &stream) {

	vector<State> state = {expect_start};
	vector<string> context;

	string::size_type comment_offset;
	string comment;

	Lexer lexer(&stream);
	size_t pos = 0;

	bool failed_on_new_line = false;
	stream.set_new_line_callback([&](size_t line_number) {
		if (auto start = stream.find("\n", pos); start != string::npos) {
			switch (state.back()) {
			case expect_comment:
				{
					auto comment_end = stream.find("*/", pos);
					if (comment_end != string::npos) {
						comment_end += 2;
						comment += stream.substr(pos, comment_end - pos);
						if (!on_comment(stream.substr(pos, comment_end - pos), pos)) {
							failed_on_new_line = true;
							return;
						}
						if (!on_comment_end()) {
							failed_on_new_line = true;
							return;
						}
						if (!on_token(token::comment_token, comment, comment_offset)) {
							failed_on_new_line = true;
							return;
						}
						pos = comment_end + 2;
						state.pop_back();
					}
					else {
						comment_end = stream.find('\n', pos);
						start = comment_end + 1;
						comment += stream.substr(pos, start - pos);
						if (!on_comment(stream.substr(pos, start - pos), pos)) {
							failed_on_new_line = true;
							return;
						}
						pos = start;
					}
				}
				break;

			default:
				{
					auto comment_pos = stream.find("/*", pos);
					if (comment_pos != string::npos) {
						if (pos != comment_pos) {
							if (!on_white_space(stream.substr(pos, comment_pos - pos), pos)) {
								failed_on_new_line = true;
								return;
							}
							pos = comment_pos;
						}
						auto comment_end = stream.find("*/", comment_pos);
						if (comment_end != string::npos) {
							comment_end += 2;
							comment_offset = comment_pos;
							comment = stream.substr(comment_pos, comment_end - comment_pos);
							if (!on_comment_begin()) {
								failed_on_new_line = true;
								return;
							}
							if (!on_comment(stream.substr(comment_pos, comment_end - comment_pos), comment_pos)) {
								failed_on_new_line = true;
								return;
							}
							if (!on_comment_end()) {
								failed_on_new_line = true;
								return;
							}
							if (!on_token(token::comment_token, comment, comment_offset)) {
								failed_on_new_line = true;
								return;
							}
							start = comment_end;
						}
						else {
							comment_end = stream.find('\n', comment_pos);
							comment_end += 1;
							comment_offset = comment_pos;
							comment = stream.substr(pos, comment_end - pos);
							if (!on_comment_begin()) {
								failed_on_new_line = true;
								return;
							}
							if (!on_comment(stream.substr(pos, comment_end - pos), comment_pos)) {
								failed_on_new_line = true;
								return;
							}
							state.emplace_back(expect_comment);
							start = comment_end;
						}
						pos = start;
					}
					else {
						comment_pos = stream.find("//", pos);
						if (comment_pos != string::npos) {
							if (pos != comment_pos) {
								if (!on_white_space(stream.substr(pos, comment_pos - pos), pos)) {
									failed_on_new_line = true;
									return;
								}
								pos = comment_pos;
							}
							comment_offset = comment_pos;
							comment = stream.substr(pos, start - pos);
							if (!on_comment_begin()) {
								failed_on_new_line = true;
								return;
							}
							if (!on_comment(stream.substr(pos, start - pos), comment_pos)) {
								failed_on_new_line = true;
								return;
							}
							if (!on_comment_end()) {
								failed_on_new_line = true;
								return;
							}
							if (!on_token(token::comment_token, comment, comment_offset)) {
								failed_on_new_line = true;
								return;
							}
							pos = start;
						}
					}
				}
				break;
			}
		}
		if (!on_new_line(line_number)) {
			failed_on_new_line = true;
			return;
		}
	});

	if (!on_script_begin()) {
		return false;
	}

	while (!stream.at_end()) {

		string token = lexer.next_token();
		auto token_type = token::from_local_id(lexer.token_type(token));
		auto start = stream.find(token, pos);
		auto lenght = token.length();

		if (failed_on_new_line) {
			return false;
		}

		if (start == string::npos && token_type == token::close_bracket_equal_token) {
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
			while (start != string::npos && !token_match()) {
				start = stream.find(']', start + 1);
			}
			if (start != string::npos) {
				token = stream.substr(start, match_length);
				lenght = match_length;
			}
		}

		if (start != string::npos) {

			switch (state.back()) {
			case expect_comment:
				{
					auto comment_end = stream.find("*/", pos);
					if (comment_end != string::npos) {
						comment_end += 2;
						comment += stream.substr(pos, comment_end - pos);
						if (!on_comment(stream.substr(pos, comment_end - pos), pos)) {
							return false;
						}
						if (!on_comment_end()) {
							return false;
						}
						if (!on_token(token::comment_token, comment, comment_offset)) {
							failed_on_new_line = true;
							return false;
						}
						start = stream.find(token, comment_end);
						if (start != comment_end) {
							if (!on_white_space(stream.substr(comment_end, start - comment_end), comment_end)) {
								return false;
							}
						}
						state.pop_back();
						pos = start;
					}
					else {
						comment_end = stream.find('\n', pos);
						if (comment_end != string::npos) {
							start = comment_end + 1;
							comment += stream.substr(pos, start - pos);
							if (!on_comment(stream.substr(pos, start - pos), pos)) {
								return false;
							}
							pos = start;
						}
						else {
							start = stream.pos();
							comment += stream.substr(pos);
							if (!on_comment(stream.substr(pos), pos)) {
								return false;
							}
							pos = start;
						}
					}
				}
				break;
			default:
				{
					auto comment_pos = stream.find("/*", pos);
					if (((comment_pos >= pos) && (comment_pos <= start))) {
						if (pos != comment_pos) {
							if (!on_white_space(stream.substr(pos, comment_pos - pos), pos)) {
								return false;
							}
							pos = comment_pos;
						}
						auto comment_end = stream.find("*/", comment_pos);
						if (comment_end != string::npos) {
							comment_end += 2;
							comment_offset = comment_pos;
							comment = stream.substr(comment_pos, comment_end - comment_pos);
							if (!on_comment_begin()) {
								return false;
							}
							if (!on_comment(stream.substr(comment_pos, comment_end - comment_pos), comment_pos)) {
								return false;
							}
							if (!on_comment_end()) {
								return false;
							}
							if (!on_token(token::comment_token, comment, comment_offset)) {
								failed_on_new_line = true;
								return false;
							}
							start = stream.find(token, comment_end);
							if (start != comment_end) {
								if (!on_white_space(stream.substr(comment_end, start - comment_end), comment_end)) {
									return false;
								}
							}
						}
						else {
							comment_offset = comment_pos;
							comment = stream.substr(pos);
							if (!on_comment_begin()) {
								return false;
							}
							if (!on_comment(stream.substr(pos), comment_pos)) {
								return false;
							}
							state.emplace_back(expect_comment);
							start = stream.pos();
						}
						pos = start;
					}
					else {
						comment_pos = stream.find("//", pos);
						if (((comment_pos >= pos) && (comment_pos <= start))) {
							if (pos != comment_pos) {
								if (!on_white_space(stream.substr(pos, comment_pos - pos), pos)) {
									return false;
								}
								pos = comment_pos;
							}
							if (start == pos) {
								start = stream.pos();
							}
							comment_offset = comment_pos;
							comment = stream.substr(pos, start - pos);
							if (!on_comment_begin()) {
								return false;
							}
							if (!on_comment(stream.substr(pos, start - pos), comment_pos)) {
								return false;
							}
							if (!on_comment_end()) {
								return false;
							}
							if (!on_token(token::comment_token, comment, comment_offset)) {
								failed_on_new_line = true;
								return false;
							}
							pos = start;
						}
					}
				}
				break;
			}

			if (start != pos) {
				if (!on_white_space(stream.substr(pos, (start - pos)), pos)) {
					return false;
				}
			}

			switch (token_type) {
			case token::line_end_token:
			case token::file_end_token:
				switch (state.back()) {
				case expect_module:
					state.pop_back();
					context.clear();
					break;
				default:
					break;
				}
				if (!on_token(token_type, token, start)) {
					return false;
				}
				pos = start + lenght;
				continue;
			case token::assert_token:
			case token::break_token:
			case token::case_token:
			case token::catch_token:
			case token::class_token:
			case token::const_token:
			case token::continue_token:
			case token::def_token:
			case token::default_token:
			case token::elif_token:
			case token::else_token:
			case token::enum_token:
			case token::exit_token:
			case token::for_token:
			case token::if_token:
			case token::in_token:
			case token::let_token:
			case token::lib_token:
			case token::package_token:
			case token::print_token:
			case token::raise_token:
			case token::return_token:
			case token::switch_token:
			case token::try_token:
			case token::while_token:
			case token::yield_token:
			case token::var_token:
			case token::constant_token:
			case token::is_token:
			case token::typeof_token:
			case token::membersof_token:
			case token::defined_token:
				switch (state.back()) {
				case expect_module:
					if (!on_module_path_token(context, token, start)) {
						return false;
					}
					context.push_back(token);
					if (!on_token(token::module_path_token, token, start)) {
						return false;
					}
					break;
				default:
					if (!context.empty() && !state.empty() && state.back() == expect_value && !on_symbol_token(context, pos)) {
						return false;
					}
					context.clear();
					state.back() = expect_start;
					if (!on_token(token_type, token, start)) {
						return false;
					}
				}
				break;

			case token::load_token:
				switch (state.back()) {
				case expect_module:
					if (!on_module_path_token(context, token, start)) {
						return false;
					}
					context.push_back(token);
					if (!on_token(token::module_path_token, token, start)) {
						return false;
					}
					break;
				default:
					if (!context.empty() && !state.empty() && state.back() == expect_value && !on_symbol_token(context, pos)) {
						return false;
					}
					context.clear();
					state.emplace_back(expect_module);
					if (!on_token(token_type, token, start)) {
						return false;
					}
				}
				break;

			case token::number_token:
				if (!context.empty() && !state.empty() && state.back() == expect_value && !on_symbol_token(context, pos)) {
					return false;
				}
				context.clear();
				state.back() = expect_operator;
				if (!on_token(token_type, token, start)) {
					return false;
				}
				break;

			case token::string_token:
				if (!context.empty() && !state.empty() && state.back() == expect_value && !on_symbol_token(context, pos)) {
					return false;
				}
				context.clear();
				state.back() = expect_operator;
				if (!on_token(token_type, token, start)) {
					return false;
				}
				break;

			case token::slash_token:
				if (!context.empty() && !state.empty() && state.back() == expect_value && !on_symbol_token(context, pos)) {
					return false;
				}
				context.clear();
				switch (state.back()) {
				case expect_operator:
					state.back() = expect_value;
					if (!on_token(token_type, token, start)) {
						return false;
					}
					break;
				default:
					token += lexer.read_regex();
					token += lexer.next_token();
					lenght = token.length();

					if (isalpha(stream[start + lenght])) {
						token += lexer.next_token();
						lenght = token.length();
					}

					state.back() = expect_operator;
					if (!on_token(token::regex_token, token, start)) {
						return false;
					}
				}
				break;

			case token::symbol_token:
				switch (state.back()) {
				case expect_module:
					if (!on_module_path_token(context, token, start)) {
						return false;
					}
					context.push_back(token);
					if (!on_token(token::module_path_token, token, start)) {
						return false;
					}
					break;
				default:
					if (!on_symbol_token(context, token, start)) {
						return false;
					}
					context.push_back(token);
					state.back() = expect_operator;
					if (!on_token(token_type, token, start)) {
						return false;
					}
				}
				break;

			case token::dot_token:
				switch (state.back()) {
				case expect_module:
					if (!on_module_path_token(context, token, start)) {
						return false;
					}
					context.push_back(token);
					if (!on_token(token::module_path_token, token, start)) {
						return false;
					}
					break;
				default:
					state.back() = expect_value;
					if (!on_token(token_type, token, start)) {
						return false;
					}
				}
				break;

			case token::close_brace_token:
			case token::close_parenthesis_token:
			case token::close_bracket_equal_token:
				if (!context.empty() && !state.empty() && state.back() == expect_value && !on_symbol_token(context, pos)) {
					return false;
				}
				context.clear();
				state.back() = expect_operator;
				if (!on_token(token_type, token, start)) {
					return false;
				}
				break;

			default:
				if (!context.empty() && !state.empty() && state.back() == expect_value && !on_symbol_token(context, pos)) {
					return false;
				}
				context.clear();
				if (is_operator_alias(token)) {
					state.back() = expect_value;
				}
				else if (Lexer::is_operator(token)) {
					state.back() = expect_value;
				}
				else {
					state.back() = expect_operator;
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
				if (!on_token(token::symbol_token, token, start)) {
					return false;
				}
			}
		}

		pos = start + lenght;
	}

	if (!context.empty() && !state.empty() && state.back() == expect_value && !on_symbol_token(context, pos)) {
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

bool LexicalHandler::on_comment_begin() {
	return true;
}

bool LexicalHandler::on_comment_end() {
	return true;
}

bool LexicalHandler::on_module_path_token(const vector<string> &context, const string &token, string::size_type offset) {
	return true;
}

bool LexicalHandler::on_symbol_token(const vector<string> &context, const string &token, string::size_type offset) {
	return true;
}

bool LexicalHandler::on_symbol_token(const std::vector<std::string> &context, std::string::size_type offset) {
	return true;
}

bool LexicalHandler::on_token(token::Type type, const string &token, string::size_type offset) {
	return true;
}

bool LexicalHandler::on_white_space(const string &token, string::size_type offset) {
	return true;
}

bool LexicalHandler::on_comment(const string &token, string::size_type offset) {
	return true;
}

bool LexicalHandler::on_new_line(size_t line_number) {
	return true;
}
