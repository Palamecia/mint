#include "parser.h"
#include "definition.h"
#include "dictionnary.h"

#include <system/bufferstream.h>
#include <system/error.h>
#include <compiler/lexer.h>
#include <compiler/token.h>
#include <unordered_set>
#include <fstream>
#include <sstream>
#include <vector>

using namespace mint;
using namespace std;

#define is_comment(_token) \
	((_token.find("/*", pos) != string::npos) || (_token.find("//", pos) != string::npos) || (_token.find("#!", pos) != string::npos))

static const unordered_set<string> g_unpadded_prefixes = { "(", "[", "{", "." };
static const unordered_set<string> g_unpadded_postfixes = { ")", "]", "}", ",", "." };
static bool contains(const unordered_set<string> &set, const string &value) {
	return set.find(value) != end(set);
}

static void cleanup_script(stringstream &stream, string &documentation, stringstream::off_type column) {

	if (!stream.eof()) {

		int c = stream.get();
		documentation += static_cast<char>(c);

		if (c == '`') {
			do {
				cleanup_script(stream, documentation, column);
				c = stream.get();
				documentation += static_cast<char>(c);
			}
			while (c != '`');
		}
		else {

			bool finished = false;

			while (!finished && !stream.eof()) {
				switch (c = stream.get()) {
				case '`':
					documentation += static_cast<char>(c);
					finished = true;
					break;

				case '\n':
					documentation += static_cast<char>(c);
					stream.seekg(column, stream.cur);
					break;

				default:
					documentation += static_cast<char>(c);
					break;
				}
			}
		}
	}
}

Parser::Parser(const string &path) :
	m_path(path),
	m_lineNumber(1),
	m_state(expect_start),
	m_parserState(parsing_start),
	m_modifiers(Reference::standard),
	m_context(nullptr) {

}

Parser::~Parser() {
	while (m_context) {
		closeBlock();
	}
}

void value_add_token(Constant *constant, const string& token) {

	if (token != "\n") {

		if (!constant->value.empty()
				&& !contains(g_unpadded_prefixes, string(1, constant->value.back()))
				&& !contains(g_unpadded_postfixes, token)) {
			constant->value += " ";
		}

		constant->value += token;
	}
}

void signature_add_token(Function::Signature *signature, const string& token) {

	if (!signature->format.empty()
			&& !contains(g_unpadded_prefixes, string(1, signature->format.back()))
			&& !contains(g_unpadded_postfixes, token)) {
		signature->format += " ";
	}

	signature->format += token;
}

void Parser::parse(Dictionnary *dictionnary) {

	Function::Signature *signature = nullptr;
	Definition *definition = nullptr;
	intmax_t next_enum_constant = 0;
	string comment;
	string base;

	stringstream sstream;
	ifstream file(m_path);
	sstream << file.rdbuf();
	string m_script = sstream.str();
	BufferStream stream(m_script);
	Lexer lexer(&stream);
	auto start = string::npos;
	auto lenght = string::npos;

	for (size_t pos = 0; !stream.atEnd(); pos = start + lenght) {

		string token = lexer.nextToken();
		start = m_script.find(token, pos);
		lenght = token.length();
		bool ignore = false;

		if ((start == string::npos) && (token == "]=")) {
			start = m_script.find("]", pos);
			lenght = m_script.find("=", start) - pos + 1;
		}

		if (start != string::npos) {

			auto comment_pos = m_script.find("/*", pos);

			if ((comment_pos >= pos) && (comment_pos <= start)) {
				start = m_script.find(token, m_script.find("*/", comment_pos) + 2);
				comment = m_script.substr(pos, start - pos);
				if (pos == 0) {
					dictionnary->setModuleDoc(cleanupDoc(comment));
				}
				pos = start;
			}
			else {

				comment_pos = m_script.find("//", pos);

				if ((comment_pos >= pos) && (comment_pos <= start)) {
					comment = m_script.substr(pos, start - pos);
					if (pos == 0) {
						dictionnary->setModuleDoc(cleanupDoc(comment));
					}
					pos = start;
				}
			}

			switch (getState()) {
			case expect_function:
				break;

			case expect_value:
			case expect_value_subexpression:
				if ((token == "/") && (m_parserState == parsing_value)) {
					m_parserState = parsing_operator;
					token += lexer.readRegex();
					token += lexer.nextToken();
					lenght = token.length();
					ignore = true;
				}
				if (Constant *instance = static_cast<Constant *>(definition)) {
					value_add_token(instance, token);
				}
				break;

			case expect_signature:
			case expect_signature_subexpression:
				if ((token == "/") && (m_parserState == parsing_value)) {
					m_parserState = parsing_operator;
					token += lexer.readRegex();
					token += lexer.nextToken();
					lenght = token.length();
					ignore = true;
				}
				signature_add_token(signature, token);
				break;

			default:
				if ((token == "/") && (m_parserState == parsing_value)) {
					m_parserState = parsing_operator;
					token += lexer.readRegex();
					token += lexer.nextToken();
					lenght = token.length();
					ignore = true;
				}
				break;
			}

			if (ignore) {
				continue;
			}

			switch (token::fromLocalId(lexer.tokenType(token))) {
			case token::class_token:
				m_parserState = parsing_start;
				setState(expect_class);
				break;
			case token::def_token:
				if (definition) {
					if (Function *instance = dictionnary->getOrCreateFunction(definition->name)) {
						instance->flags = definition->flags;
						signature = new Function::Signature;
						signature->format = "def";
						if (signature->doc.empty()) {
							signature->doc = cleanupDoc(comment);
						}
						delete definition;
						definition = instance;
					}
					startModifiers(Reference::standard);
					setState(expect_signature_begin);
					comment.clear();
				}
				else {
					setState(expect_function);
				}
				m_parserState = parsing_start;
				break;
			case token::enum_token:
				m_parserState = parsing_start;
				setState(expect_enum);
				break;
			case token::package_token:
				m_parserState = parsing_start;
				setState(expect_package);
				break;

			case token::symbol_token:
				if (definition) {
					switch (getState()) {
					case expect_base:
						m_parserState = parsing_start;
						base += token;
						break;

					case expect_value:
					case expect_signature:
						m_parserState = parsing_operator;
						break;

					default:
						m_parserState = parsing_operator;
						setState(expect_start);
						break;
					}
				}
				else {
					switch (getState()) {
					case expect_package:
						if (Package *instance = dictionnary->getOrCreatePackage(definitionName(token))) {
							pushContext(token, instance);
							if (instance->doc.empty()) {
								instance->doc = cleanupDoc(comment);
							}
							instance->flags = retrieveModifiers();
							definition = instance;
						}

						m_parserState = parsing_start;
						setState(expect_start);
						break;

					case expect_class:
						if (Class *instance = new Class(definitionName(token))) {
							pushContext(token, instance);
							if (instance->doc.empty()) {
								instance->doc = cleanupDoc(comment);
							}
							instance->flags = retrieveModifiers();
							definition = instance;
						}

						m_parserState = parsing_start;
						setState(expect_start);
						break;

					case expect_enum:
						if (Enum *instance = new Enum(definitionName(token))) {
							pushContext(token, instance);
							if (instance->doc.empty()) {
								instance->doc = cleanupDoc(comment);
							}
							next_enum_constant = 0;
							instance->flags = retrieveModifiers();
							definition = instance;
						}

						m_parserState = parsing_start;
						setState(expect_start);
						break;

					case expect_function:
						if (Function *instance = dictionnary->getOrCreateFunction(definitionName(token))) {
							signature = new Function::Signature;
							signature->format = "def";
							if (signature->doc.empty()) {
								signature->doc = cleanupDoc(comment);
							}
							instance->flags = retrieveModifiers();
							definition = instance;
						}

						m_parserState = parsing_start;
						setState(expect_signature_begin);
						break;

					case expect_start:
						if (m_modifiers & Reference::global) {
							if (Constant *instance = new Constant(definitionName(token))) {
								if (instance->doc.empty()) {
									instance->doc = cleanupDoc(comment);
								}
								instance->flags = retrieveModifiers();
								definition = instance;
							}
						}
						else if (Context* context = currentContext()) {
							if (context->bloc == 1) {
								switch (context->definition->type) {
								case Definition::class_definition:
									if (Constant *instance = new Constant(definitionName(token))) {
										if (instance->doc.empty()) {
											instance->doc = cleanupDoc(comment);
										}
										instance->flags = retrieveModifiers();
										definition = instance;
									}
									break;

								case Definition::enum_definition:
									if (Constant *instance = new Constant(definitionName(token))) {
										if (instance->doc.empty()) {
											instance->doc = cleanupDoc(comment);
										}
										instance->flags = retrieveModifiers();
										definition = instance;
									}
									break;

								default:
									break;
								}
							}
						}

						m_parserState = parsing_operator;
						setState(expect_start);
						break;

					case expect_capture:
						m_parserState = parsing_operator;
						continue;

					case expect_signature:
						m_parserState = parsing_operator;
						break;

					default:
						m_parserState = parsing_operator;
						setState(expect_start);
						break;
					}
				}
				startModifiers(Reference::standard);
				comment.clear();
				break;

			case token::open_parenthesis_token:
				switch (getState()) {
				case expect_function:
					setState(expect_parenthesis_operator);
					break;

				case expect_signature:
				case expect_signature_subexpression:
					pushState(expect_signature_subexpression);
					startModifiers(Reference::standard);
					break;

				case expect_value:
				case expect_value_subexpression:
					pushState(expect_value_subexpression);
					startModifiers(Reference::standard);
					break;

				case expect_signature_begin:
					signature->format += " " + token;
					startModifiers(Reference::standard);
					setState(expect_signature);
					break;

				default:
					startModifiers(Reference::standard);
					break;
				}
				m_parserState = parsing_value;
				break;

			case token::close_parenthesis_token:
				switch (getState()) {
				case expect_parenthesis_operator:
					if (Function *instance = dictionnary->getOrCreateFunction(definitionName("()"))) {
						signature = new Function::Signature;
						signature->format = "def";
						if (signature->doc.empty()) {
							signature->doc = cleanupDoc(comment);
						}
						instance->flags = retrieveModifiers();
						definition = instance;
					}
					m_parserState = parsing_operator;
					setState(expect_signature_begin);
					break;

				case expect_signature_subexpression:
				case expect_value_subexpression:
					m_parserState = parsing_operator;
					popState();
					break;

				case expect_signature:
					m_parserState = parsing_operator;
					popState();
					break;

				default:
					m_parserState = parsing_operator;
					break;
				}
				startModifiers(Reference::standard);
				break;

			case token::open_bracket_token:
				switch (getState()) {
				case expect_function:
					if (Context* context = currentContext()) {
						if (context->definition->type == Definition::class_definition) {
							setState(expect_bracket_operator);
						}
						else {
							startModifiers(Reference::standard);
							pushState(expect_capture);
						}
					}
					else {
						startModifiers(Reference::standard);
						pushState(expect_capture);
					}
					break;

				case expect_signature:
				case expect_signature_subexpression:
					pushState(expect_signature_subexpression);
					startModifiers(Reference::standard);
					break;

				case expect_value:
				case expect_value_subexpression:
					pushState(expect_value_subexpression);
					startModifiers(Reference::standard);
					break;

				default:
					startModifiers(Reference::standard);
					break;
				}
				m_parserState = parsing_value;
				break;

			case token::close_bracket_token:
				switch (getState()) {
				case expect_bracket_operator:
					if (Function *instance = dictionnary->getOrCreateFunction(definitionName("[]"))) {
						signature = new Function::Signature;
						signature->format = "def";
						if (signature->doc.empty()) {
							signature->doc = cleanupDoc(comment);
						}
						instance->flags = retrieveModifiers();
						definition = instance;
					}
					m_parserState = parsing_operator;
					setState(expect_signature_begin);
					break;

				case expect_capture:
					popState();
					break;

				case expect_signature_subexpression:
				case expect_value_subexpression:
					popState();
					break;

				default:
					break;
				}
				startModifiers(Reference::standard);
				m_parserState = parsing_value;
				break;

			case token::close_bracket_equal_token:
				switch (getState()) {
				case expect_bracket_operator:
					if (Function *instance = dictionnary->getOrCreateFunction(definitionName("[]="))) {
						signature = new Function::Signature;
						signature->format = "def";
						if (signature->doc.empty()) {
							signature->doc = cleanupDoc(comment);
						}
						instance->flags = retrieveModifiers();
						definition = instance;
					}
					m_parserState = parsing_value;
					setState(expect_signature_begin);
					break;

				case expect_signature_subexpression:
				case expect_value_subexpression:
					m_parserState = parsing_value;
					popState();
					break;

				default:
					m_parserState = parsing_value;
					break;
				}
				break;

			case token::open_brace_token:
				switch (getState()) {
				case expect_base:
					if (Class *instace = static_cast<Class *>(definition)) {
						instace->bases.push_back(base);
						base.clear();
					}
					break;

				case expect_signature:
				case expect_signature_subexpression:
					pushState(expect_signature_subexpression);
					break;

				case expect_value:
				case expect_value_subexpression:
					pushState(expect_value_subexpression);
					break;

				case expect_function:
					popState();
					break;

				default:
					break;
				}
				startModifiers(Reference::standard);
				m_parserState = parsing_value;
				openBlock();
				break;

			case token::close_brace_token:
				switch (getState()) {
				case expect_signature_subexpression:
				case expect_value_subexpression:
					popState();
					break;

				default:
					break;
				}
				startModifiers(Reference::standard);
				m_parserState = parsing_operator;
				comment.clear();
				closeBlock();
				break;

			case token::line_end_token:
				switch (getState()) {
				case expect_signature_subexpression:
				case expect_value_subexpression:
					break;

				case expect_value:
					popState();
					fall_through;

				default:
					if (definition) {
						switch (definition->type) {
						case Definition::constant_definition:
							if (Context* context = currentContext()) {
								if (context->definition->type == Definition::enum_definition) {
									if (Constant *instance = static_cast<Constant *>(definition)) {
										if (instance->value.empty()) {
											stringstream stream;
											stream << next_enum_constant++;
											instance->value = stream.str();
										}
										else {
											stringstream stream(instance->value);
											stream >> next_enum_constant;
											next_enum_constant++;
										}
									}
								}
							}
							break;

						case Definition::function_definition:
							if (signature) {
								if (Function *instance = static_cast<Function *>(definition)) {
									instance->signatures.push_back(signature);
								}
								signature = nullptr;
							}
							break;

						default:
							break;
						}
						bindDefinitionToContext(definition);
						dictionnary->insertDefinition(definition);
						definition = nullptr;
					}
					break;
				}
				startModifiers(Reference::standard);
				m_parserState = parsing_start;
				m_lineNumber++;
				break;

			case token::constant_token:
				startModifiers(Reference::standard);
				m_parserState = parsing_operator;
				break;

			case token::number_token:
				startModifiers(Reference::standard);
				m_parserState = parsing_operator;
				break;

			case token::string_token:
				startModifiers(Reference::standard);
				m_parserState = parsing_operator;
				break;

			case token::dbldot_token:
				startModifiers(Reference::standard);
				if (definition) {
					switch (definition->type) {
					case Definition::class_definition:
						setState(expect_base);
						break;

					default:
						m_parserState = parsing_value;
						break;
					}
				}
				break;

			case token::equal_token:
				if (definition && definition->type == Definition::constant_definition) {
					pushState(expect_value);
				}
				m_parserState = parsing_value;
				break;

			case token::dot_token:
				switch (getState()) {
				case expect_base:
					base += token;
					break;

				default:
					m_parserState = parsing_value;
					break;
				}
				break;

			case token::comma_token:
				switch (getState()) {
				case expect_base:
					if (Class *instace = static_cast<Class *>(definition)) {
						instace->bases.push_back(base);
						base.clear();
					}
					break;

				default:
					m_parserState = parsing_value;
					break;
				}
				break;

			case token::in_token:
				switch (getState()) {
				case expect_function:
					if (Function *instance = dictionnary->getOrCreateFunction(definitionName(token))) {
						signature = new Function::Signature;
						signature->format = "def";
						if (signature->doc.empty()) {
							signature->doc = cleanupDoc(comment);
						}
						instance->flags = retrieveModifiers();
						definition = instance;
					}
					setState(expect_signature_begin);
					break;

				default:
					m_parserState = parsing_start;
					break;
				}
				break;

			case token::dbldot_equal_token:
				switch (getState()) {
				case expect_function:
					if (Function *instance = dictionnary->getOrCreateFunction(definitionName(token))) {
						signature = new Function::Signature;
						signature->format = "def";
						if (signature->doc.empty()) {
							signature->doc = cleanupDoc(comment);
						}
						instance->flags = retrieveModifiers();
						definition = instance;
					}
					setState(expect_signature_begin);
					break;

				default:
					m_parserState = parsing_value;
					break;
				}
				break;

			case token::dbl_pipe_token:
				switch (getState()) {
				case expect_function:
					if (Function *instance = dictionnary->getOrCreateFunction(definitionName(token))) {
						signature = new Function::Signature;
						signature->format = "def";
						if (signature->doc.empty()) {
							signature->doc = cleanupDoc(comment);
						}
						instance->flags = retrieveModifiers();
						definition = instance;
					}
					setState(expect_signature_begin);
					break;

				default:
					m_parserState = parsing_value;
					break;
				}
				break;

			case token::dbl_amp_token:
				switch (getState()) {
				case expect_function:
					if (Function *instance = dictionnary->getOrCreateFunction(definitionName(token))) {
						signature = new Function::Signature;
						signature->format = "def";
						if (signature->doc.empty()) {
							signature->doc = cleanupDoc(comment);
						}
						instance->flags = retrieveModifiers();
						definition = instance;
					}
					setState(expect_signature_begin);
					break;

				default:
					m_parserState = parsing_value;
					break;
				}
				break;

			case token::pipe_token:
				switch (getState()) {
				case expect_function:
					if (Function *instance = dictionnary->getOrCreateFunction(definitionName(token))) {
						signature = new Function::Signature;
						signature->format = "def";
						if (signature->doc.empty()) {
							signature->doc = cleanupDoc(comment);
						}
						instance->flags = retrieveModifiers();
						definition = instance;
					}
					setState(expect_signature_begin);
					break;

				default:
					m_parserState = parsing_value;
					break;
				}
				break;

			case token::caret_token:
				switch (getState()) {
				case expect_function:
					if (Function *instance = dictionnary->getOrCreateFunction(definitionName(token))) {
						signature = new Function::Signature;
						signature->format = "def";
						if (signature->doc.empty()) {
							signature->doc = cleanupDoc(comment);
						}
						instance->flags = retrieveModifiers();
						definition = instance;
					}
					setState(expect_signature_begin);
					break;

				default:
					m_parserState = parsing_value;
					break;
				}
				break;

			case token::amp_token:
				switch (getState()) {
				case expect_function:
					if (Function *instance = dictionnary->getOrCreateFunction(definitionName(token))) {
						signature = new Function::Signature;
						signature->format = "def";
						if (signature->doc.empty()) {
							signature->doc = cleanupDoc(comment);
						}
						instance->flags = retrieveModifiers();
						definition = instance;
					}
					setState(expect_signature_begin);
					break;

				default:
					m_parserState = parsing_value;
					break;
				}
				break;

			case token::dbl_equal_token:
				switch (getState()) {
				case expect_function:
					if (Function *instance = dictionnary->getOrCreateFunction(definitionName(token))) {
						signature = new Function::Signature;
						signature->format = "def";
						if (signature->doc.empty()) {
							signature->doc = cleanupDoc(comment);
						}
						instance->flags = retrieveModifiers();
						definition = instance;
					}
					setState(expect_signature_begin);
					break;

				default:
					m_parserState = parsing_value;
					break;
				}
				break;

			case token::exclamation_equal_token:
				switch (getState()) {
				case expect_function:
					if (Function *instance = dictionnary->getOrCreateFunction(definitionName(token))) {
						signature = new Function::Signature;
						signature->format = "def";
						if (signature->doc.empty()) {
							signature->doc = cleanupDoc(comment);
						}
						instance->flags = retrieveModifiers();
						definition = instance;
					}
					setState(expect_signature_begin);
					break;

				default:
					m_parserState = parsing_value;
					break;
				}
				break;

			case token::left_angled_token:
				switch (getState()) {
				case expect_function:
					if (Function *instance = dictionnary->getOrCreateFunction(definitionName(token))) {
						signature = new Function::Signature;
						signature->format = "def";
						if (signature->doc.empty()) {
							signature->doc = cleanupDoc(comment);
						}
						instance->flags = retrieveModifiers();
						definition = instance;
					}
					setState(expect_signature_begin);
					break;

				default:
					m_parserState = parsing_value;
					break;
				}
				break;

			case token::right_angled_token:
				switch (getState()) {
				case expect_function:
					if (Function *instance = dictionnary->getOrCreateFunction(definitionName(token))) {
						signature = new Function::Signature;
						signature->format = "def";
						if (signature->doc.empty()) {
							signature->doc = cleanupDoc(comment);
						}
						instance->flags = retrieveModifiers();
						definition = instance;
					}
					setState(expect_signature_begin);
					break;

				default:
					m_parserState = parsing_value;
					break;
				}
				break;

			case token::left_angled_equal_token:
				switch (getState()) {
				case expect_function:
					if (Function *instance = dictionnary->getOrCreateFunction(definitionName(token))) {
						signature = new Function::Signature;
						signature->format = "def";
						if (signature->doc.empty()) {
							signature->doc = cleanupDoc(comment);
						}
						instance->flags = retrieveModifiers();
						definition = instance;
					}
					setState(expect_signature_begin);
					break;

				default:
					m_parserState = parsing_value;
					break;
				}
				break;

			case token::right_angled_equal_token:
				switch (getState()) {
				case expect_function:
					if (Function *instance = dictionnary->getOrCreateFunction(definitionName(token))) {
						signature = new Function::Signature;
						signature->format = "def";
						if (signature->doc.empty()) {
							signature->doc = cleanupDoc(comment);
						}
						instance->flags = retrieveModifiers();
						definition = instance;
					}
					setState(expect_signature_begin);
					break;

				default:
					m_parserState = parsing_value;
					break;
				}
				break;

			case token::dbl_left_angled_token:
				switch (getState()) {
				case expect_function:
					if (Function *instance = dictionnary->getOrCreateFunction(definitionName(token))) {
						signature = new Function::Signature;
						signature->format = "def";
						if (signature->doc.empty()) {
							signature->doc = cleanupDoc(comment);
						}
						instance->flags = retrieveModifiers();
						definition = instance;
					}
					setState(expect_signature_begin);
					break;

				default:
					m_parserState = parsing_value;
					break;
				}
				break;

			case token::dbl_right_angled_token:
				switch (getState()) {
				case expect_function:
					if (Function *instance = dictionnary->getOrCreateFunction(definitionName(token))) {
						signature = new Function::Signature;
						signature->format = "def";
						if (signature->doc.empty()) {
							signature->doc = cleanupDoc(comment);
						}
						instance->flags = retrieveModifiers();
						definition = instance;
					}
					setState(expect_signature_begin);
					break;

				default:
					m_parserState = parsing_value;
					break;
				}
				break;

			case token::plus_token:
				switch (getState()) {
				case expect_function:
					if (Function *instance = dictionnary->getOrCreateFunction(definitionName(token))) {
						signature = new Function::Signature;
						signature->format = "def";
						if (signature->doc.empty()) {
							signature->doc = cleanupDoc(comment);
						}
						instance->flags = retrieveModifiers();
						definition = instance;
					}
					setState(expect_signature_begin);
					break;

				default:
					m_parserState = parsing_value;
					break;
				}
				break;

			case token::minus_token:
				switch (getState()) {
				case expect_function:
					if (Function *instance = dictionnary->getOrCreateFunction(definitionName(token))) {
						signature = new Function::Signature;
						signature->format = "def";
						if (signature->doc.empty()) {
							signature->doc = cleanupDoc(comment);
						}
						instance->flags = retrieveModifiers();
						definition = instance;
					}
					setState(expect_signature_begin);
					break;

				default:
					addModifiers(Reference::private_visibility);
					m_parserState = parsing_value;
					break;
				}
				break;

			case token::asterisk_token:
				switch (getState()) {
				case expect_function:
					if (Function *instance = dictionnary->getOrCreateFunction(definitionName(token))) {
						signature = new Function::Signature;
						signature->format = "def";
						if (signature->doc.empty()) {
							signature->doc = cleanupDoc(comment);
						}
						instance->flags = retrieveModifiers();
						definition = instance;
					}
					setState(expect_signature_begin);
					break;

				default:
					m_parserState = parsing_value;
					break;
				}
				break;

			case token::slash_token:
				switch (getState()) {
				case expect_function:
					if (Function *instance = dictionnary->getOrCreateFunction(definitionName(token))) {
						signature = new Function::Signature;
						signature->format = "def";
						if (signature->doc.empty()) {
							signature->doc = cleanupDoc(comment);
						}
						instance->flags = retrieveModifiers();
						definition = instance;
					}
					setState(expect_signature_begin);
					break;

				default:
					m_parserState = parsing_value;
					break;
				}
				break;

			case token::percent_token:
				switch (getState()) {
				case expect_function:
					if (Function *instance = dictionnary->getOrCreateFunction(definitionName(token))) {
						signature = new Function::Signature;
						signature->format = "def";
						if (signature->doc.empty()) {
							signature->doc = cleanupDoc(comment);
						}
						instance->flags = retrieveModifiers();
						definition = instance;
					}
					setState(expect_signature_begin);
					break;

				default:
					addModifiers(Reference::const_value);
					m_parserState = parsing_value;
					break;
				}
				break;

			case token::exclamation_token:
				switch (getState()) {
				case expect_function:
					if (Function *instance = dictionnary->getOrCreateFunction(definitionName(token))) {
						signature = new Function::Signature;
						signature->format = "def";
						if (signature->doc.empty()) {
							signature->doc = cleanupDoc(comment);
						}
						instance->flags = retrieveModifiers();
						definition = instance;
					}
					setState(expect_signature_begin);
					break;

				default:
					m_parserState = parsing_value;
					break;
				}
				break;

			case token::tilde_token:
				switch (getState()) {
				case expect_function:
					if (Function *instance = dictionnary->getOrCreateFunction(definitionName(token))) {
						signature = new Function::Signature;
						signature->format = "def";
						if (signature->doc.empty()) {
							signature->doc = cleanupDoc(comment);
						}
						instance->flags = retrieveModifiers();
						definition = instance;
					}
					setState(expect_signature_begin);
					break;

				default:
					addModifiers(Reference::package_visibility);
					m_parserState = parsing_value;
					break;
				}
				break;

			case token::dbl_plus_token:
				switch (getState()) {
				case expect_function:
					if (Function *instance = dictionnary->getOrCreateFunction(definitionName(token))) {
						signature = new Function::Signature;
						signature->format = "def";
						if (signature->doc.empty()) {
							signature->doc = cleanupDoc(comment);
						}
						instance->flags = retrieveModifiers();
						definition = instance;
					}
					setState(expect_signature_begin);
					break;

				default:
					m_parserState = parsing_value;
					break;
				}
				break;

			case token::dbl_minus_token:
				switch (getState()) {
				case expect_function:
					if (Function *instance = dictionnary->getOrCreateFunction(definitionName(token))) {
						signature = new Function::Signature;
						signature->format = "def";
						if (signature->doc.empty()) {
							signature->doc = cleanupDoc(comment);
						}
						instance->flags = retrieveModifiers();
						definition = instance;
					}
					setState(expect_signature_begin);
					break;

				default:
					m_parserState = parsing_value;
					break;
				}
				break;

			case token::dbl_asterisk_token:
				switch (getState()) {
				case expect_function:
					if (Function *instance = dictionnary->getOrCreateFunction(definitionName(token))) {
						signature = new Function::Signature;
						signature->format = "def";
						if (signature->doc.empty()) {
							signature->doc = cleanupDoc(comment);
						}
						instance->flags = retrieveModifiers();
						definition = instance;
					}
					setState(expect_signature_begin);
					break;

				default:
					m_parserState = parsing_value;
					break;
				}
				break;
			case token::dot_dot_token:
				switch (getState()) {
				case expect_function:
					if (Function *instance = dictionnary->getOrCreateFunction(definitionName(token))) {
						signature = new Function::Signature;
						signature->format = "def";
						if (signature->doc.empty()) {
							signature->doc = cleanupDoc(comment);
						}
						instance->flags = retrieveModifiers();
						definition = instance;
					}
					setState(expect_signature_begin);
					break;

				default:
					m_parserState = parsing_value;
					break;
				}
				break;
			case token::tpl_dot_token:
				switch (getState()) {
				case expect_function:
					if (Function *instance = dictionnary->getOrCreateFunction(definitionName(token))) {
						signature = new Function::Signature;
						signature->format = "def";
						if (signature->doc.empty()) {
							signature->doc = cleanupDoc(comment);
						}
						instance->flags = retrieveModifiers();
						definition = instance;
					}
					setState(expect_signature_begin);
					break;

				default:
					m_parserState = parsing_value;
					break;
				}
				break;
			case token::sharp_token:
				addModifiers(Reference::protected_visibility);
				m_parserState = parsing_value;
				break;

			case token::at_token:
				addModifiers(Reference::global);
				m_parserState = parsing_value;
				break;

			case token::dollar_token:
				addModifiers(Reference::const_address);
				m_parserState = parsing_value;
				break;

			case token::const_token:
				addModifiers(Reference::const_address | Reference::const_value);
				m_parserState = parsing_value;
				break;

			case token::assert_token:
			case token::break_token:
			case token::case_token:
			case token::catch_token:
			case token::continue_token:
			case token::default_token:
			case token::elif_token:
			case token::else_token:
			case token::exit_token:
			case token::for_token:
			case token::if_token:
			case token::lib_token:
			case token::print_token:
			case token::raise_token:
			case token::return_token:
			case token::switch_token:
			case token::try_token:
			case token::while_token:
			case token::yield_token:
			case token::is_token:
			case token::typeof_token:
			case token::membersof_token:
			case token::defined_token:
				startModifiers(Reference::standard);
				m_parserState = parsing_start;
				break;

			default:
				startModifiers(Reference::standard);
				if (Lexer::isOperator(token)) {
					m_parserState = parsing_value;
				}
				else {
					m_parserState = parsing_operator;
				}
				break;
			}
		}
	}
}

void Parser::parse_error(const char *message, size_t column, size_t start_line) {

	static constexpr const char *tab_placeholder = "\033[1;30m\xC2\xBB\t\033[0m";
	static constexpr const char *space_placeholder = "\033[1;30m\xC2\xB7\033[0m";

	string message_line;
	ifstream stream(m_path);
	string line_content = "\033[0m";
	string message_pos = "\033[1;30m";

	for (size_t i = 0; i < m_lineNumber; ++i) {
		getline(stream, line_content, '\n');
		if (i >= start_line - 1 && i < m_lineNumber) {
			for (char c : line_content) {
				switch (c) {
				case '\t':
					message_line += tab_placeholder;
					break;
				case ' ':
					message_line += space_placeholder;
					break;
				default:
					message_line += c;
					break;
				}
			}
			message_line += '\n';
		}
	}

	for (size_t i = 0; i < line_content.size(); ++i) {

		byte c = line_content[i];

		if (i < column - 1) {
			switch (c) {
			case '\t':
				message_line += tab_placeholder;
				message_pos += '\t';
				break;
			case ' ':
				message_line += space_placeholder;
				message_pos += ' ';
				break;
			default:
				if (c & 0x80) {

					size_t size = 2;

					if (c & 0x04) {
						size++;
						if (c & 0x02) {
							size++;
						}
					}

					if (i + size < column - 1) {
						message_pos += ' ';
					}

					message_line += line_content.substr(i, size);
					i += size - 1;
				}
				else {
					message_line += static_cast<char>(c);
					message_pos += ' ';
				}
			}
		}
		else {
			switch (c) {
			case '\t':
				message_line += tab_placeholder;
				break;
			case ' ':
				message_line += space_placeholder;
				break;
			default:
				message_line += static_cast<char>(c);
				break;
			}
		}
	}

	message_pos += '^';

	error("%s:%d: %s\n%s\n%s\n", m_path.c_str(), m_lineNumber, message, message_line.c_str(), message_pos.c_str());
}

Parser::State Parser::getState() const {
	return m_state;
}

void Parser::setState(State state) {
	m_state = state;
}

void Parser::pushState(State state) {
	m_states.push_back(m_state);
	m_state = state;
}

void Parser::popState() {
	if (m_states.empty()) {
		m_state = expect_start;
	}
	else {
		m_state = m_states.back();
		m_states.pop_back();
	}
}

Parser::Context *Parser::currentContext() const {
	return m_context;
}

string Parser::definitionName(const string &token) const {

	string name;

	for (Context *scope : m_contexts) {
		name += scope->name + ".";
	}

	if (m_context) {
		name += m_context->name + ".";
	}

	return name + token;
}

void Parser::pushContext(const string &name, Definition* definition) {

	if (m_context) {
		m_contexts.push_back(m_context);
	}

	m_context = new Context{name, definition, 0};
}

void Parser::bindDefinitionToContext(Definition* definition) {

	/*for (Context* context : m_contexts) {
		bindDefinitionToContext(context, definition);
	}*/

	if (m_context) {
		if (m_context->definition == definition) {
			if (!m_contexts.empty()) {
				bindDefinitionToContext(m_contexts.back(), definition);
			}
		}
		else {
			bindDefinitionToContext(m_context, definition);
		}
	}
}

void Parser::bindDefinitionToContext(Context* context, Definition* definition) {

	switch (context->definition->type) {
	case Definition::package_definition:
		if (Package *instance = static_cast<Package *>(context->definition)) {
			instance->members.insert(definition->name);
		}
		break;

	case Definition::enum_definition:
		if (Enum *instance = static_cast<Enum *>(context->definition)) {
			instance->members.insert(definition->name);
		}
		break;

	case Definition::class_definition:
		if (Class *instance = static_cast<Class *>(context->definition)) {
			instance->members.insert(definition->name);
		}
		break;

	default:
		break;
	}
}

void Parser::openBlock() {
	if (m_context) {
		m_context->bloc++;
	}
}

void Parser::closeBlock() {
	if (m_context && !--m_context->bloc) {
		delete m_context;
		if (m_contexts.empty()) {
			m_context = nullptr;
		}
		else {
			m_context = m_contexts.back();
			m_contexts.pop_back();
		}
	}
}

void Parser::startModifiers(Reference::Flags flags) {
	m_modifiers = flags;
}

void Parser::addModifiers(Reference::Flags flags) {
	m_modifiers |= flags;
}

Reference::Flags Parser::retrieveModifiers() {
	Reference::Flags flags = m_modifiers;
	m_modifiers = Reference::standard;
	return flags;
}

string Parser::cleanupDoc(const string &comment) {

	auto pos = string::npos;

	if ((pos = comment.find("/**")) != string::npos) {
		stringstream stream(comment);
		stream.seekg(static_cast<stringstream::off_type>(pos + 3), stream.beg);
		return cleanupMultiLineDoc(stream);
	}

	if ((pos = comment.find("///")) != string::npos) {
		stringstream stream(comment);
		stream.seekg(static_cast<stringstream::off_type>(pos + 3), stream.beg);
		return cleanupSingleLineDoc(stream);
	}

	return string();
}

string Parser::cleanupSingleLineDoc(stringstream &stream) {

	bool finished = false;

	string documentation;
	stringstream::off_type column = stream.tellg();

	if (stream.eof() || stream.get() != ' ') {
		parse_error("expected ' ' character before documentation string", column);
	}

	column++;

	while (!finished && !stream.eof()) {
		switch (int c = stream.get()) {
		case EOF:
			finished = true;
			break;

		case '\n':
			documentation += static_cast<char>(c);
			finished = true;
			m_lineNumber++;
			break;

		case '`':
			documentation += static_cast<char>(c);
			cleanup_script(stream, documentation, column);
			break;

		default:
			documentation += static_cast<char>(c);
			break;
		}
	}

	return documentation;
}

string Parser::cleanupMultiLineDoc(stringstream &stream) {

	bool finished = false;
	bool suspect_end = false;

	string documentation;
	size_t start_line = m_lineNumber - 1;
	stringstream::off_type column = stream.tellg();

	while (!finished && !stream.eof()) {
		switch (int c = stream.get()) {
		case EOF:
			finished = true;
			break;

		case '\n':
			if (suspect_end) {
				documentation += '*';
				suspect_end = false;
			}
			documentation += static_cast<char>(c);
			stream.seekg(column - 2, stream.cur);
			m_lineNumber++;
			if (stream.eof() || stream.get() != '*') {
				parse_error("expected '*' character for documentation continuation", column, start_line);
			}
			if (!stream.eof()) {
				switch (stream.get()) {
				case ' ':
					break;
				case '/':
					finished = true;
					break;
				default:
					parse_error("expected ' ' character before documentation string", column, start_line);
					break;
				}
			}
			break;

		case '*':
			if (suspect_end) {
				documentation += '*';
			}
			else {
				suspect_end = true;
			}
			break;

		case '/':
			if (suspect_end) {
				finished = true;
			}
			else {
				documentation += static_cast<char>(c);
			}
			break;

		case '`':
			if (suspect_end) {
				documentation += '*';
				suspect_end = false;
			}
			documentation += static_cast<char>(c);
			cleanup_script(stream, documentation, column);
			break;

		default:
			if (suspect_end) {
				documentation += '*';
				suspect_end = false;
			}
			documentation += static_cast<char>(c);
			break;
		}
	}

	return documentation;
}
