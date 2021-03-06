#include "parser.h"
#include "definition.h"
#include "dictionnary.h"

#include <system/bufferstream.h>
#include <memory/reference.h>
#include <compiler/lexer.h>
#include <compiler/token.h>
#include <sstream>
#include <vector>

using namespace mint;
using namespace std;

#define is_comment(_token) \
	((_token.find("/*", pos) != string::npos) || (_token.find("//", pos) != string::npos) || (_token.find("#!", pos) != string::npos))

static void cleanup_script(stringstream &stream, string &documentation, size_t column) {

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

Parser::Parser(const string &script) :
	m_script(script),
	m_state(expect_start),
	m_context(nullptr) {

}

Parser::~Parser() {
	while (m_context) {
		closeBlock();
	}
}

void Parser::parse(Dictionnary *dictionnary) {

	Function::Signature *signature = nullptr;
	Definition *definition = nullptr;
	Reference::Flags flags = Reference::standard;
	intmax_t next_enum_constant = 0;
	string comment;
	string base;

	BufferStream stream(m_script);
	Lexer lexer(&stream);
	size_t pos = 0;

	while (!stream.atEnd()) {

		string token = lexer.nextToken();
		auto start = m_script.find(token, pos);
		auto lenght = token.length();

		if (start != string::npos) {

			auto comment_pos = m_script.find("/*", pos);

			if ((comment_pos >= pos) && (comment_pos <= start)) {
				start = m_script.find(token, m_script.find("*/", comment_pos) + 2);
				comment = m_script.substr(pos, start - pos).c_str();
				if (pos == 0) {
					dictionnary->setModuleDoc(cleanupDoc(comment));
				}
				pos = start;
			}

			switch (getState()) {
			case expect_value:
				if (Constant *instance = static_cast<Constant *>(definition)) {
					if (token != "\n") {
						if (!instance->value.empty()
								&& instance->value.back() != '('
								&& instance->value.back() != '['
								&& token != ")" && token != "]" && token != ",") {
							instance->value += " ";
						}
						instance->value += token;
					}
				}
				break;

			case expect_signature:
				if (signature->format.back() != '('
						&& signature->format.back() != '['
						&& token != ")" && token != "]" && token != ",") {
					signature->format += " ";
				}
				signature->format += token;
				break;

			default:
				break;
			}

			switch (token::fromLocalId(lexer.tokenType(token))) {
			case token::class_token:
				flags = Reference::standard;
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
					flags = Reference::standard;
					if (getState() != expect_signature) {
						setState(expect_start);
					}
					comment.clear();
				}
				else {
					setState(expect_function);
				}
				break;
			case token::enum_token:
				flags = Reference::standard;
				setState(expect_enum);
				break;
			case token::package_token:
				flags = Reference::standard;
				setState(expect_package);
				break;

			case token::symbol_token:
				if (definition) {
					switch (getState()) {
					case expect_base:
						base += token;
						break;

					case expect_signature:
						break;

					default:
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
							instance->flags = flags;
							definition = instance;
						}

						setState(expect_start);
						break;

					case expect_class:
						if (Class *instance = new Class(definitionName(token))) {
							pushContext(token, instance);
							if (instance->doc.empty()) {
								instance->doc = cleanupDoc(comment);
							}
							instance->flags = flags;
							definition = instance;
						}

						setState(expect_start);
						break;

					case expect_enum:
						if (Enum *instance = new Enum(definitionName(token))) {
							pushContext(token, instance);
							if (instance->doc.empty()) {
								instance->doc = cleanupDoc(comment);
							}
							next_enum_constant = 0;
							instance->flags = flags;
							definition = instance;
						}

						setState(expect_start);
						break;

					case expect_function:
						if (Function *instance = dictionnary->getOrCreateFunction(definitionName(token))) {
							signature = new Function::Signature;
							signature->format = "def";
							if (signature->doc.empty()) {
								signature->doc = cleanupDoc(comment);
							}
							instance->flags = flags;
							definition = instance;
						}

						setState(expect_start);
						break;

					case expect_start:
						if (flags & Reference::global) {
							if (Constant *instance = new Constant(definitionName(token))) {
								if (instance->doc.empty()) {
									instance->doc = cleanupDoc(comment);
								}
								instance->flags = flags;
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
										instance->flags = flags;
										definition = instance;
									}
									break;

								case Definition::enum_definition:
									if (Constant *instance = new Constant(definitionName(token))) {
										if (instance->doc.empty()) {
											instance->doc = cleanupDoc(comment);
										}
										instance->flags = flags;
										definition = instance;
									}
									break;

								default:
									break;
								}
							}
						}

						setState(expect_start);
						break;

					case expect_signature:
						break;

					default:
						setState(expect_start);
						break;
					}
				}
				flags = Reference::standard;
				comment.clear();
				break;

			case token::open_parenthesis_token:
				flags = Reference::standard;
				if (signature) {
					signature->format += " " + token;
					setState(expect_signature);
				}
				else {
					switch (getState()) {
					case expect_function:
						setState(expect_parenthesis_operator);
						break;

					default:
						break;
					}
				}
				break;

			case token::close_parenthesis_token:
				flags = Reference::standard;
				if (signature) {
					setState(expect_start);
				}
				else {
					switch (getState()) {
					case expect_parenthesis_operator:
						if (Function *instance = dictionnary->getOrCreateFunction(definitionName("()"))) {
							signature = new Function::Signature;
							signature->format = "def";
							if (signature->doc.empty()) {
								signature->doc = cleanupDoc(comment);
							}
							instance->flags = flags;
							definition = instance;
						}
						break;

					default:
						break;
					}
				}
				break;

			case token::open_bracket_token:
				flags = Reference::standard;
				if (signature) {
					signature->format += " " + token;
					setState(expect_signature);
				}
				else {
					switch (getState()) {
					case expect_function:
						setState(expect_bracket_operator);
						break;

					default:
						break;
					}
				}
				break;

			case token::close_bracket_token:
				flags = Reference::standard;
				if (signature) {
					setState(expect_start);
				}
				else {
					switch (getState()) {
					case expect_bracket_operator:
						if (Function *instance = dictionnary->getOrCreateFunction(definitionName("[]"))) {
							signature = new Function::Signature;
							signature->format = "def";
							if (signature->doc.empty()) {
								signature->doc = cleanupDoc(comment);
							}
							instance->flags = flags;
							definition = instance;
						}
						break;

					default:
						break;
					}
				}
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
						instance->flags = flags;
						definition = instance;
					}
					break;

				default:
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

				default:
					break;
				}
				flags = Reference::standard;
				openBlock();
				break;

			case token::close_brace_token:
				flags = Reference::standard;
				closeBlock();
				break;

			case token::line_end_token:
				flags = Reference::standard;
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
					dictionnary->insertDefinition(definition);
					definition = nullptr;
				}
				break;

			case token::constant_token:
				flags = Reference::standard;
				break;

			case token::number_token:
				flags = Reference::standard;
				break;

			case token::string_token:
				flags = Reference::standard;
				break;

			case token::dbldot_token:
				flags = Reference::standard;
				if (definition) {
					switch (definition->type) {
					case Definition::class_definition:
						setState(expect_base);
						break;

					default:
						break;
					}
				}
				break;

			case token::equal_token:
				if (definition && definition->type == Definition::constant_definition) {
					setState(expect_value);
				}
				break;

			case token::dot_token:
				switch (getState()) {
				case expect_base:
					base += token;
					break;

				default:
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
						instance->flags = flags;
						definition = instance;
					}
					break;

				default:
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
						instance->flags = flags;
						definition = instance;
					}
					break;

				default:
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
						instance->flags = flags;
						definition = instance;
					}
					break;

				default:
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
						instance->flags = flags;
						definition = instance;
					}
					break;

				default:
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
						instance->flags = flags;
						definition = instance;
					}
					break;

				default:
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
						instance->flags = flags;
						definition = instance;
					}
					break;

				default:
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
						instance->flags = flags;
						definition = instance;
					}
					break;

				default:
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
						instance->flags = flags;
						definition = instance;
					}
					break;

				default:
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
						instance->flags = flags;
						definition = instance;
					}
					break;

				default:
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
						instance->flags = flags;
						definition = instance;
					}
					break;

				default:
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
						instance->flags = flags;
						definition = instance;
					}
					break;

				default:
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
						instance->flags = flags;
						definition = instance;
					}
					break;

				default:
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
						instance->flags = flags;
						definition = instance;
					}
					break;

				default:
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
						instance->flags = flags;
						definition = instance;
					}
					break;

				default:
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
						instance->flags = flags;
						definition = instance;
					}
					break;

				default:
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
						instance->flags = flags;
						definition = instance;
					}
					break;

				default:
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
						instance->flags = flags;
						definition = instance;
					}
					break;

				default:
					flags |= Reference::private_visibility;
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
						instance->flags = flags;
						definition = instance;
					}
					break;

				default:
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
						instance->flags = flags;
						definition = instance;
					}
					break;

				default:
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
						instance->flags = flags;
						definition = instance;
					}
					break;

				default:
					flags |= Reference::const_value;
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
						instance->flags = flags;
						definition = instance;
					}
					break;

				default:
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
						instance->flags = flags;
						definition = instance;
					}
					break;

				default:
					flags |= Reference::package_visibility;
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
						instance->flags = flags;
						definition = instance;
					}
					break;

				default:
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
						instance->flags = flags;
						definition = instance;
					}
					break;

				default:
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
						instance->flags = flags;
						definition = instance;
					}
					break;

				default:
					break;
				}
				break;

			case token::sharp_token:
				flags |= Reference::protected_visibility;
				break;

			case token::at_token:
				flags |= Reference::global;
				break;

			case token::dollar_token:
				flags |= Reference::const_address;
				break;

			case token::const_token:
				flags |= Reference::const_address | Reference::const_value;
				break;

			default:
				flags = Reference::standard;
				break;
			}

			pos = start + lenght;
		}
		else {

			token = m_script.substr(pos);

			if (is_comment(token)) {
				start = m_script.find(token, m_script.find("\n", pos) + 2);
				comment = m_script.substr(pos, start - pos).c_str();
				if (pos == 0) {
					dictionnary->setModuleDoc(cleanupDoc(comment));
				}
				pos = start;
			}

			break;
		}
	}
}

Parser::State Parser::getState() const {
	return m_state;
}

void Parser::setState(State state) {
	m_state = state;
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

string Parser::cleanupDoc(const string &comment) const {

	auto pos = string::npos;

	if ((pos = comment.find("/**")) != string::npos) {
		stringstream stream(comment);
		stream.seekg(pos + 3, stream.beg);
		return cleanupMultiLineDoc(stream);
	}

	if ((pos = comment.find("///")) != string::npos) {
		stringstream stream(comment);
		stream.seekg(pos + 3, stream.beg);
		return cleanupSingleLineDoc(stream);
	}

	return string();
}

string Parser::cleanupSingleLineDoc(stringstream &stream) const {

	bool finished = false;

	string documentation;
	size_t column = stream.tellg();

	while (!finished && !stream.eof()) {
		switch (int c = stream.get()) {
		case '\n':
			documentation += static_cast<char>(c);
			finished = true;
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

string Parser::cleanupMultiLineDoc(stringstream &stream) const {

	bool finished = false;
	bool suspect_end = false;

	string documentation;
	size_t column = stream.tellg();

	while (!finished && !stream.eof()) {
		switch (int c = stream.get()) {
		case '\n':
			if (suspect_end) {
				documentation += '*';
				suspect_end = false;
			}
			documentation += static_cast<char>(c);
			stream.seekg(column, stream.cur);
			break;

		case '*':
			suspect_end = true;
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
