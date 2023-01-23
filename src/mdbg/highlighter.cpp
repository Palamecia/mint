#include "highlighter.h"

#include <memory/globaldata.h>
#include <memory/class.h>
#include <system/datastream.h>
#include <system/terminal.h>
#include <system/assert.h>
#include <compiler/lexer.h>
#include <compiler/token.h>

using namespace std;
using namespace mint;

#define is_standard_symbol(_token) \
	((_token == "self") || (_token == "va_args"))

#define is_operator_alias(_token) \
	((_token == "and") || (_token == "or") || (_token == "xor") || (_token == "not"))

#define is_comment(_token) \
	((_token.find("/*", pos) != string::npos) || (_token.find("//", pos) != string::npos) || (_token.find("#!", pos) != string::npos))

bool resolve_path(const vector<Symbol> &context, PackageData *&pack, ClassDescription *&desc) {

	for (const Symbol &symbol : context) {
		if (desc) {
			desc = desc->findClassDescription(symbol);
			if (desc == nullptr) {
				return false;
			}
		}
		else if (pack) {
			desc = pack->findClassDescription(symbol);
			if (desc == nullptr) {
				pack = pack->findPackage(symbol);
				if (pack == nullptr) {
					return false;
				}
			}
		}
		else {
			GlobalData *globalData = GlobalData::instance();
			desc = globalData->findClassDescription(symbol);
			if (desc == nullptr) {
				pack = globalData->findPackage(symbol);
				if (pack == nullptr) {
					desc = globalData->findClassDescription(symbol);
					if (desc == nullptr) {
						pack = globalData->findPackage(symbol);
						if (pack == nullptr) {
							return false;
						}
					}
				}
			}
		}
	}

	return true;
}

static bool is_defined_class(const vector<Symbol> &context, const Symbol &token) {

	PackageData *pack = nullptr;
	ClassDescription *desc = nullptr;

	if (resolve_path(context, pack, desc)) {

		if (desc) {
			return desc->findClassDescription(token) != nullptr;
		}

		if (pack) {
			return pack->getClass(token) != nullptr;
		}

		GlobalData *globalData = GlobalData::instance();
		return globalData->getClass(token) != nullptr;
	}

	return false;
}

static bool is_defined_symbol(const vector<Symbol> &context, const Symbol &token) {

	PackageData *pack = nullptr;
	ClassDescription *desc = nullptr;

	if (resolve_path(context, pack, desc)) {

		if (desc) {
			Class *prototype = desc->generate();
			return prototype->globals().find(token) != prototype->globals().end();
		}

		if (pack) {
			return pack->symbols().find(token) != pack->symbols().end();
		}

		GlobalData *globalData = GlobalData::instance();
		return globalData->symbols().find(token) != globalData->symbols().end();
	}

	return false;
}

enum Style {
	text,
	comment,
	keyword,
	constant,
	user_type,
	number_literal,
	string_literal,
	regex_literal,
	standard_symbol
};

void set_style(Style style) {
	switch (style) {
	case text:
		term_print(stdout, "\033[0m");
		break;

	case comment:
		term_print(stdout, "\033[1;30m");
		break;

	case keyword:
		term_print(stdout, "\033[0m");
		term_print(stdout, "\033[3;34m");
		break;

	case constant:
		term_print(stdout, "\033[0;33m");
		break;

	case user_type:
		term_print(stdout, "\033[0m");
		term_print(stdout, "\033[1;31m");
		break;

	case number_literal:
		term_print(stdout, "\033[0;31m");
		break;

	case string_literal:
		term_print(stdout, "\033[0;32m");
		break;

	case regex_literal:
		term_print(stdout, "\033[0;35m");
		break;

	case standard_symbol:
		term_print(stdout, "\033[0m");
		term_print(stdout, "\033[3;33m");
		break;
	}
}

enum State {
	expect_start,
	expect_comment,
	expect_module,
	expect_value,
	expect_operator
};

class HighlighterStream : public DataStream {
public:
	HighlighterStream(ifstream &&stream) :
		m_stream(move(stream)) {

	}

	~HighlighterStream() {

	}

	bool atEnd() const override {
		return m_stream.eof();
	}

	bool isValid() const override {
		return m_stream.good();
	}

	string path() const override {
		return string();
	}

	string::size_type find(const string &substr, string::size_type offset = 0) const noexcept {
		return m_script.find(substr, offset);
	}

	string substr(string::size_type offset = 0, string::size_type count = string::npos) const noexcept {
		return m_script.substr(offset, count);
	}

	char operator [](string::size_type offset) const {
		return m_script[offset];
	}

	size_t pos() const {
		return m_script.size();
	}

protected:
	int readChar() override {
		int c = m_stream.get();
		m_script += c;
		return c;
	}

	int nextBufferedChar() override {
		int c = m_stream.get();
		m_script += c;
		return c;
	}

private:
	ifstream m_stream;
	string m_script;
};

void print_line_number(size_t line_number, int digits, bool current) {
	if (current) {
		term_printf(stdout, "\033[1;31m %*zd >| \033[0m", digits, line_number);
	}
	else {
		term_printf(stdout, "\033[1;30m %*zd  | \033[0m", digits, line_number);
	}
}

static void print_highlighted(const string &str) {
	term_print(stdout, str.c_str());
}

void print_highlighted(size_t from_line, size_t to_line, size_t current_line, ifstream &&script) {

	vector<State> state = {expect_start};
	vector<Symbol> context;
	bool print = false;

	HighlighterStream stream(move(script));
	Lexer lexer(&stream);
	size_t pos = 0;

	stream.setNewLineCallback([&](size_t line_number) {

		if (line_number == from_line) {
			print = true;
		}

		if (line_number <= to_line) {

			auto start = stream.find("\n", pos);

			if (start != string::npos) {

				int recheck = 0;

				do {
					switch (state.back()) {
					case expect_comment:
					{
						auto comment_end = stream.find("*/", pos);
						set_style(comment);

						if (comment_end != string::npos) {

							if (print) {
								print_highlighted(stream.substr(pos, (comment_end + 2) - pos));
							}

							pos = comment_end + 2;
							state.pop_back();
						}
						else {

							if (print) {
								print_highlighted(stream.substr(pos, start - pos));
							}

							pos = start + 1;
						}
					}
						break;

					default:
					{
						auto comment_pos = stream.find("/*", pos);

						if (comment_pos != string::npos) {

							if (print) {
								print_highlighted(stream.substr(pos, comment_pos - pos));
							}

							state.emplace_back(expect_comment);
							pos = comment_pos;
							++recheck;
						}
						else {

							comment_pos = stream.find("//", pos);

							if (comment_pos != string::npos) {

								set_style(comment);

								if (print) {
									print_highlighted(stream.substr(pos, start - pos));
								}

								pos = start;
							}
						}
					}
						break;
					}
				}
				while (recheck--);
			}

			if (print) {

				if (line_number != from_line) {
					term_print(stdout, "\n");
				}

				auto amount_of_digits = [] (size_t value) -> int {
					int amount = 1;
					while (value /= 10) {
						amount++;
					}
					return amount;
				};

				const int digits = (amount_of_digits(line_number + static_cast<size_t>(to_line)) / 4) + 3;
				print_line_number(line_number, digits, line_number == current_line);
			}
		}
	});

	while (!stream.atEnd()) {

		string token = lexer.nextToken();
		auto start = stream.find(token, pos);
		auto lenght = token.length();

		if (to_line < stream.lineNumber()) {
			break;
		}

		if (start != string::npos) {

			switch (state.back()) {
			case expect_comment:
			{
				auto comment_end = stream.find("*/", pos);
				start = stream.find(token, comment_end + 2);
				set_style(comment);

				if (print) {
					print_highlighted(stream.substr(pos, start - pos));
				}

				state.pop_back();
				pos = start;
			}
				break;
			default:
			{
				auto comment_pos = stream.find("/*", pos);

				if (((comment_pos >= pos) && (comment_pos <= start))) {

					start = stream.find(token, stream.find("*/", comment_pos) + 2);
					set_style(comment);

					if (print) {
						print_highlighted(stream.substr(pos, start - pos));
					}

					pos = start;
				}
				else {

					comment_pos = stream.find("//", pos);

					if (((comment_pos >= pos) && (comment_pos <= start))) {

						set_style(comment);

						if (print) {
							print_highlighted(stream.substr(pos, start - pos));
						}

						pos = start;
					}
				}
			}
				break;
			}

			switch (token::fromLocalId(lexer.tokenType(token))) {
			case token::line_end_token:
			case token::file_end_token:
				switch (state.back()) {
				case expect_module:
					state.pop_back();
					break;
				default:
					break;
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
					set_style(standard_symbol);
					break;
				default:
					state.back() = expect_start;
					set_style(keyword);
					context.clear();
				}
				break;

			case token::load_token:
				switch (state.back()) {
				case expect_module:
					set_style(standard_symbol);
					break;
				default:
					state.emplace_back(expect_module);
					set_style(keyword);
					context.clear();
				}
				break;

			case token::number_token:
				state.back() = expect_operator;
				set_style(number_literal);
				context.clear();
				break;

			case token::string_token:
				state.back() = expect_operator;
				set_style(string_literal);
				context.clear();
				break;

			case token::slash_token:
				switch (state.back()) {
				case expect_operator:
					state.back() = expect_value;
					set_style(text);
					break;
				default:
					token += lexer.readRegex();
					token += lexer.nextToken();
					lenght = token.length();

					if (isalpha(stream[start + lenght])) {
						token += lexer.nextToken();
						lenght = token.length();
					}

					state.back() = expect_operator;
					set_style(regex_literal);
				}
				context.clear();
				break;

			case token::symbol_token:
				switch (state.back()) {
				case expect_module:
					set_style(standard_symbol);
					break;
				default:
					if (is_defined_class(context, Symbol(token))) {
						set_style(user_type);
					}
					else if (is_defined_symbol(context, Symbol(token))) {
						set_style(constant);
					}
					else if (is_standard_symbol(token)) {
						set_style(standard_symbol);
					}
					else {
						set_style(text);
					}
					context.push_back(Symbol(token));
					state.back() = expect_operator;
				}
				break;

			case token::dot_token:
				switch (state.back()) {
				case expect_module:
					set_style(text);
					break;
				default:
					state.back() = expect_value;
					set_style(text);
				}
				break;

			case token::close_brace_token:
			case token::close_parenthesis_token:
			case token::close_bracket_equal_token:
				state.back() = expect_operator;
				set_style(text);
				break;

			default:
				if (is_operator_alias(token)) {
					state.back() = expect_value;
					set_style(keyword);
				}
				else {
					set_style(text);
					if (Lexer::isOperator(token)) {
						state.back() = expect_value;
					}
					else {
						state.back() = expect_operator;
					}
				}
				context.clear();
				break;
			}

			if (print) {
				print_highlighted(stream.substr(pos, (start - pos) + lenght));
			}
		}
		else {

			token = stream.substr(pos);

			if (is_comment(token)) {
				set_style(comment);
			}
			else {
				set_style(text);
			}

			if (print) {
				print_highlighted(token);
			}
		}

		pos = start + lenght;
	}

	term_print(stdout, "\033[0m\n");
}
