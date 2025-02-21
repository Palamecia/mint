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

#include "parser.h"
#include "dictionary.h"

#include <mint/memory/casttool.h>
#include <mint/system/string.h>
#include <mint/system/error.h>
#include <unordered_set>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>

namespace {

static const std::unordered_set<std::string> UNPADDED_PREFIXES = {"(", "[", "{", "."};
static const std::unordered_set<std::string> UNPADDED_POSTFIXES = {")", "]", "}", ",", "."};

bool contains(const std::unordered_set<std::string> &set, const std::string &value) {
	return set.find(value) != end(set);
}

void value_add_token(Constant *constant, const std::string &token) {

	if (token != "\n") {

		if (!constant->value.empty() && !contains(UNPADDED_PREFIXES, std::string(1, constant->value.back()))
			&& !contains(UNPADDED_POSTFIXES, token)) {
			constant->value += " ";
		}

		constant->value += token;
	}
}

void signature_add_token(Function::Signature *signature, const std::string &token) {

	if (!signature->format.empty() && !contains(UNPADDED_PREFIXES, std::string(1, signature->format.back()))
		&& !contains(UNPADDED_POSTFIXES, token)) {
		signature->format += " ";
	}

	signature->format += token;
}

}

Parser::Parser(std::filesystem::path path) :
	m_path(std::move(path)) {}

Parser::~Parser() {
	while (m_context) {
		close_block();
	}
}

void Parser::parse(Dictionary *dictionary) {

	std::ifstream file(m_path);

	m_dictionary = dictionary;
	m_signature = nullptr;
	m_definition = nullptr;

	LexicalHandler::parse(file);
}

bool Parser::on_token(mint::token::Type type, const std::string &token, std::string::size_type offset) {

	switch (get_state()) {
	case EXPECT_FUNCTION:
		break;

	case EXPECT_VALUE:
	case EXPECT_VALUE_SUBEXPRESSION:
		if (auto *instance = static_cast<Constant *>(m_definition)) {
			value_add_token(instance, token);
		}
		break;

	case EXPECT_SIGNATURE:
	case EXPECT_SIGNATURE_SUBEXPRESSION:
		signature_add_token(m_signature, token);
		break;

	default:
		break;
	}

	switch (type) {
	case mint::token::CLASS_TOKEN:
		set_state(EXPECT_CLASS);
		break;
	case mint::token::DEF_TOKEN:
		if (m_definition) {
			if (Function *instance = m_dictionary->get_or_create_function(m_definition->name)) {
				instance->flags = m_definition->flags;
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				delete m_definition;
				m_definition = instance;
			}
			start_modifiers(mint::Reference::DEFAULT);
			set_state(EXPECT_SIGNATURE_BEGIN);
			m_comment.clear();
		}
		else {
			set_state(EXPECT_FUNCTION);
		}
		break;
	case mint::token::ENUM_TOKEN:
		set_state(EXPECT_ENUM);
		m_next_enum_constant = 0;
		break;
	case mint::token::PACKAGE_TOKEN:
		set_state(EXPECT_PACKAGE);
		break;

	case mint::token::SYMBOL_TOKEN:
		if (m_definition) {
			switch (get_state()) {
			case EXPECT_BASE:
				m_base += token;
				break;

			case EXPECT_VALUE:
			case EXPECT_SIGNATURE:
				break;

			default:
				set_state(EXPECT_START);
				break;
			}
		}
		else {
			switch (get_state()) {
			case EXPECT_PACKAGE:
				if (Package *instance = m_dictionary->get_or_create_package(definition_name(token))) {
					push_context(token, instance);
					if (instance->doc.empty()) {
						instance->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
					}
					instance->flags = retrieve_modifiers();
					m_definition = instance;
				}

				set_state(EXPECT_START);
				break;

			case EXPECT_CLASS:
				if (auto *instance = new Class(definition_name(token))) {
					push_context(token, instance);
					if (instance->doc.empty()) {
						instance->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
					}
					instance->flags = retrieve_modifiers();
					m_definition = instance;
				}

				set_state(EXPECT_START);
				break;

			case EXPECT_ENUM:
				if (auto *instance = new Enum(definition_name(token))) {
					push_context(token, instance);
					if (instance->doc.empty()) {
						instance->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
					}
					m_next_enum_constant = 0;
					instance->flags = retrieve_modifiers();
					m_definition = instance;
				}

				set_state(EXPECT_START);
				break;

			case EXPECT_FUNCTION:
				if (Function *instance = m_dictionary->get_or_create_function(definition_name(token))) {
					m_signature = new Function::Signature;
					m_signature->format = "def";
					if (m_signature->doc.empty()) {
						m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
					}
					instance->flags = retrieve_modifiers();
					m_definition = instance;
				}

				set_state(EXPECT_SIGNATURE_BEGIN);
				break;

			case EXPECT_START:
				if (m_modifiers & mint::Reference::GLOBAL) {
					if (auto *instance = new Constant(definition_name(token))) {
						if (instance->doc.empty()) {
							instance->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
						}
						instance->flags = retrieve_modifiers();
						m_definition = instance;
					}
				}
				else if (const Context *context = current_context()) {
					if (context->block == 1) {
						switch (context->definition->type) {
						case Definition::CLASS_DEFINITION:
							if (auto *instance = new Constant(definition_name(token))) {
								if (instance->doc.empty()) {
									instance->doc = cleanup_doc(m_comment, m_comment_line_number,
																m_comment_column_number);
								}
								instance->flags = retrieve_modifiers();
								m_definition = instance;
							}
							break;

						case Definition::ENUM_DEFINITION:
							if (auto *instance = new Constant(definition_name(token))) {
								if (instance->doc.empty()) {
									instance->doc = cleanup_doc(m_comment, m_comment_line_number,
																m_comment_column_number);
								}
								instance->flags = retrieve_modifiers();
								m_definition = instance;
							}
							break;

						default:
							break;
						}
					}
				}

				set_state(EXPECT_START);
				break;

			case EXPECT_CAPTURE:
				return true;

			case EXPECT_SIGNATURE:
				break;

			default:
				set_state(EXPECT_START);
				break;
			}
		}
		start_modifiers(mint::Reference::DEFAULT);
		m_comment.clear();
		break;

	case mint::token::OPEN_PARENTHESIS_TOKEN:
		switch (get_state()) {
		case EXPECT_FUNCTION:
			set_state(EXPECT_PARENTHESIS_OPERATOR);
			break;

		case EXPECT_SIGNATURE:
		case EXPECT_SIGNATURE_SUBEXPRESSION:
			push_state(EXPECT_SIGNATURE_SUBEXPRESSION);
			start_modifiers(mint::Reference::DEFAULT);
			break;

		case EXPECT_VALUE:
		case EXPECT_VALUE_SUBEXPRESSION:
			push_state(EXPECT_VALUE_SUBEXPRESSION);
			start_modifiers(mint::Reference::DEFAULT);
			break;

		case EXPECT_SIGNATURE_BEGIN:
			m_signature->format += " " + token;
			start_modifiers(mint::Reference::DEFAULT);
			set_state(EXPECT_SIGNATURE);
			break;

		default:
			start_modifiers(mint::Reference::DEFAULT);
			break;
		}
		break;

	case mint::token::CLOSE_PARENTHESIS_TOKEN:
		switch (get_state()) {
		case EXPECT_PARENTHESIS_OPERATOR:
			if (Function *instance = m_dictionary->get_or_create_function(definition_name("()"))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(EXPECT_SIGNATURE_BEGIN);
			break;

		case EXPECT_SIGNATURE_SUBEXPRESSION:
		case EXPECT_VALUE_SUBEXPRESSION:
			pop_state();
			break;

		case EXPECT_SIGNATURE:
			pop_state();
			break;

		default:
			break;
		}
		start_modifiers(mint::Reference::DEFAULT);
		break;

	case mint::token::OPEN_BRACKET_TOKEN:
		switch (get_state()) {
		case EXPECT_FUNCTION:
			if (const Context *context = current_context()) {
				if (context->definition->type == Definition::CLASS_DEFINITION) {
					set_state(EXPECT_BRACKET_OPERATOR);
				}
				else {
					start_modifiers(mint::Reference::DEFAULT);
					push_state(EXPECT_CAPTURE);
				}
			}
			else {
				start_modifiers(mint::Reference::DEFAULT);
				push_state(EXPECT_CAPTURE);
			}
			break;

		case EXPECT_SIGNATURE:
		case EXPECT_SIGNATURE_SUBEXPRESSION:
			push_state(EXPECT_SIGNATURE_SUBEXPRESSION);
			start_modifiers(mint::Reference::DEFAULT);
			break;

		case EXPECT_VALUE:
		case EXPECT_VALUE_SUBEXPRESSION:
			push_state(EXPECT_VALUE_SUBEXPRESSION);
			start_modifiers(mint::Reference::DEFAULT);
			break;

		default:
			start_modifiers(mint::Reference::DEFAULT);
			break;
		}
		break;

	case mint::token::CLOSE_BRACKET_TOKEN:
		switch (get_state()) {
		case EXPECT_BRACKET_OPERATOR:
			if (Function *instance = m_dictionary->get_or_create_function(definition_name("[]"))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(EXPECT_SIGNATURE_BEGIN);
			break;

		case EXPECT_CAPTURE:
			pop_state();
			break;

		case EXPECT_SIGNATURE_SUBEXPRESSION:
		case EXPECT_VALUE_SUBEXPRESSION:
			pop_state();
			break;

		default:
			break;
		}
		start_modifiers(mint::Reference::DEFAULT);
		break;

	case mint::token::CLOSE_BRACKET_EQUAL_TOKEN:
		switch (get_state()) {
		case EXPECT_BRACKET_OPERATOR:
			if (Function *instance = m_dictionary->get_or_create_function(definition_name("[]="))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(EXPECT_SIGNATURE_BEGIN);
			break;

		case EXPECT_SIGNATURE_SUBEXPRESSION:
		case EXPECT_VALUE_SUBEXPRESSION:
			pop_state();
			break;

		default:
			break;
		}
		break;

	case mint::token::OPEN_BRACE_TOKEN:
		switch (get_state()) {
		case EXPECT_BASE:
			if (auto *instance = static_cast<Class *>(m_definition)) {
				instance->bases.push_back(m_base);
				m_base.clear();
			}
			break;

		case EXPECT_SIGNATURE:
		case EXPECT_SIGNATURE_SUBEXPRESSION:
			push_state(EXPECT_SIGNATURE_SUBEXPRESSION);
			break;

		case EXPECT_VALUE:
		case EXPECT_VALUE_SUBEXPRESSION:
			push_state(EXPECT_VALUE_SUBEXPRESSION);
			break;

		case EXPECT_FUNCTION:
			pop_state();
			break;

		default:
			break;
		}
		start_modifiers(mint::Reference::DEFAULT);
		open_block();
		break;

	case mint::token::CLOSE_BRACE_TOKEN:
		switch (get_state()) {
		case EXPECT_SIGNATURE_SUBEXPRESSION:
		case EXPECT_VALUE_SUBEXPRESSION:
			pop_state();
			break;

		default:
			break;
		}
		start_modifiers(mint::Reference::DEFAULT);
		m_comment.clear();
		close_block();
		break;

	case mint::token::LINE_END_TOKEN:
		switch (get_state()) {
		case EXPECT_SIGNATURE_SUBEXPRESSION:
		case EXPECT_VALUE_SUBEXPRESSION:
			break;

		case EXPECT_VALUE:
			pop_state();
			[[fallthrough]];

		default:
			if (m_definition) {
				switch (m_definition->type) {
				case Definition::CONSTANT_DEFINITION:
					if (const Context *context = current_context()) {
						if (context->definition->type == Definition::ENUM_DEFINITION) {
							if (auto *instance = static_cast<Constant *>(m_definition)) {
								if (instance->value.empty()) {
									instance->value = std::to_string(m_next_enum_constant++);
								}
								else {
									m_next_enum_constant = mint::to_integer(mint::to_signed_number(instance->value));
									m_next_enum_constant++;
								}
							}
						}
					}
					break;

				case Definition::FUNCTION_DEFINITION:
					if (m_signature) {
						if (auto *instance = static_cast<Function *>(m_definition)) {
							instance->signatures.push_back(m_signature);
						}
						m_signature = nullptr;
					}
					break;

				default:
					break;
				}
				bind_definition_to_context(m_definition);
				m_dictionary->insert_definition(m_definition);
				m_definition = nullptr;
			}
			break;
		}
		start_modifiers(mint::Reference::DEFAULT);
		break;

	case mint::token::CONSTANT_TOKEN:
		start_modifiers(mint::Reference::DEFAULT);
		break;

	case mint::token::NUMBER_TOKEN:
		start_modifiers(mint::Reference::DEFAULT);
		break;

	case mint::token::STRING_TOKEN:
		start_modifiers(mint::Reference::DEFAULT);
		break;

	case mint::token::COLON_TOKEN:
		start_modifiers(mint::Reference::DEFAULT);
		if (m_definition) {
			switch (m_definition->type) {
			case Definition::CLASS_DEFINITION:
				set_state(EXPECT_BASE);
				m_base.clear();
				break;

			default:
				break;
			}
		}
		break;

	case mint::token::EQUAL_TOKEN:
		if (m_definition && m_definition->type == Definition::CONSTANT_DEFINITION) {
			push_state(EXPECT_VALUE);
		}
		break;

	case mint::token::DOT_TOKEN:
		switch (get_state()) {
		case EXPECT_BASE:
			m_base += token;
			break;

		default:
			break;
		}
		break;

	case mint::token::COMMA_TOKEN:
		switch (get_state()) {
		case EXPECT_BASE:
			if (auto *instance = static_cast<Class *>(m_definition)) {
				instance->bases.push_back(m_base);
				m_base.clear();
			}
			break;

		default:
			break;
		}
		break;

	case mint::token::IN_TOKEN:
		switch (get_state()) {
		case EXPECT_FUNCTION:
			if (Function *instance = m_dictionary->get_or_create_function(definition_name(token))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(EXPECT_SIGNATURE_BEGIN);
			break;

		default:
			break;
		}
		break;

	case mint::token::COLON_EQUAL_TOKEN:
		switch (get_state()) {
		case EXPECT_FUNCTION:
			if (Function *instance = m_dictionary->get_or_create_function(definition_name(token))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(EXPECT_SIGNATURE_BEGIN);
			break;

		default:
			break;
		}
		break;

	case mint::token::DBL_PIPE_TOKEN:
		switch (get_state()) {
		case EXPECT_FUNCTION:
			if (Function *instance = m_dictionary->get_or_create_function(definition_name(token))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(EXPECT_SIGNATURE_BEGIN);
			break;

		default:
			break;
		}
		break;

	case mint::token::DBL_AMP_TOKEN:
		switch (get_state()) {
		case EXPECT_FUNCTION:
			if (Function *instance = m_dictionary->get_or_create_function(definition_name(token))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(EXPECT_SIGNATURE_BEGIN);
			break;

		default:
			break;
		}
		break;

	case mint::token::PIPE_TOKEN:
		switch (get_state()) {
		case EXPECT_FUNCTION:
			if (Function *instance = m_dictionary->get_or_create_function(definition_name(token))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(EXPECT_SIGNATURE_BEGIN);
			break;

		default:
			break;
		}
		break;

	case mint::token::CARET_TOKEN:
		switch (get_state()) {
		case EXPECT_FUNCTION:
			if (Function *instance = m_dictionary->get_or_create_function(definition_name(token))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(EXPECT_SIGNATURE_BEGIN);
			break;

		default:
			break;
		}
		break;

	case mint::token::AMP_TOKEN:
		switch (get_state()) {
		case EXPECT_FUNCTION:
			if (Function *instance = m_dictionary->get_or_create_function(definition_name(token))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(EXPECT_SIGNATURE_BEGIN);
			break;

		default:
			break;
		}
		break;

	case mint::token::DBL_EQUAL_TOKEN:
		switch (get_state()) {
		case EXPECT_FUNCTION:
			if (Function *instance = m_dictionary->get_or_create_function(definition_name(token))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(EXPECT_SIGNATURE_BEGIN);
			break;

		default:
			break;
		}
		break;

	case mint::token::EXCLAMATION_EQUAL_TOKEN:
		switch (get_state()) {
		case EXPECT_FUNCTION:
			if (Function *instance = m_dictionary->get_or_create_function(definition_name(token))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(EXPECT_SIGNATURE_BEGIN);
			break;

		default:
			break;
		}
		break;

	case mint::token::LEFT_ANGLED_TOKEN:
		switch (get_state()) {
		case EXPECT_FUNCTION:
			if (Function *instance = m_dictionary->get_or_create_function(definition_name(token))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(EXPECT_SIGNATURE_BEGIN);
			break;

		default:
			break;
		}
		break;

	case mint::token::RIGHT_ANGLED_TOKEN:
		switch (get_state()) {
		case EXPECT_FUNCTION:
			if (Function *instance = m_dictionary->get_or_create_function(definition_name(token))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(EXPECT_SIGNATURE_BEGIN);
			break;

		default:
			break;
		}
		break;

	case mint::token::LEFT_ANGLED_EQUAL_TOKEN:
		switch (get_state()) {
		case EXPECT_FUNCTION:
			if (Function *instance = m_dictionary->get_or_create_function(definition_name(token))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(EXPECT_SIGNATURE_BEGIN);
			break;

		default:
			break;
		}
		break;

	case mint::token::RIGHT_ANGLED_EQUAL_TOKEN:
		switch (get_state()) {
		case EXPECT_FUNCTION:
			if (Function *instance = m_dictionary->get_or_create_function(definition_name(token))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(EXPECT_SIGNATURE_BEGIN);
			break;

		default:
			break;
		}
		break;

	case mint::token::DBL_LEFT_ANGLED_TOKEN:
		switch (get_state()) {
		case EXPECT_FUNCTION:
			if (Function *instance = m_dictionary->get_or_create_function(definition_name(token))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(EXPECT_SIGNATURE_BEGIN);
			break;

		default:
			break;
		}
		break;

	case mint::token::DBL_RIGHT_ANGLED_TOKEN:
		switch (get_state()) {
		case EXPECT_FUNCTION:
			if (Function *instance = m_dictionary->get_or_create_function(definition_name(token))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(EXPECT_SIGNATURE_BEGIN);
			break;

		default:
			break;
		}
		break;

	case mint::token::PLUS_TOKEN:
		switch (get_state()) {
		case EXPECT_FUNCTION:
			if (Function *instance = m_dictionary->get_or_create_function(definition_name(token))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(EXPECT_SIGNATURE_BEGIN);
			break;

		default:
			break;
		}
		break;

	case mint::token::MINUS_TOKEN:
		switch (get_state()) {
		case EXPECT_FUNCTION:
			if (Function *instance = m_dictionary->get_or_create_function(definition_name(token))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(EXPECT_SIGNATURE_BEGIN);
			break;

		default:
			add_modifiers(mint::Reference::PRIVATE_VISIBILITY);
			break;
		}
		break;

	case mint::token::ASTERISK_TOKEN:
		switch (get_state()) {
		case EXPECT_FUNCTION:
			if (Function *instance = m_dictionary->get_or_create_function(definition_name(token))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(EXPECT_SIGNATURE_BEGIN);
			break;

		default:
			break;
		}
		break;

	case mint::token::SLASH_TOKEN:
		switch (get_state()) {
		case EXPECT_FUNCTION:
			if (Function *instance = m_dictionary->get_or_create_function(definition_name(token))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(EXPECT_SIGNATURE_BEGIN);
			break;

		default:
			break;
		}
		break;

	case mint::token::PERCENT_TOKEN:
		switch (get_state()) {
		case EXPECT_FUNCTION:
			if (Function *instance = m_dictionary->get_or_create_function(definition_name(token))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(EXPECT_SIGNATURE_BEGIN);
			break;

		default:
			add_modifiers(mint::Reference::CONST_VALUE);
			break;
		}
		break;

	case mint::token::EXCLAMATION_TOKEN:
		switch (get_state()) {
		case EXPECT_FUNCTION:
			if (Function *instance = m_dictionary->get_or_create_function(definition_name(token))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(EXPECT_SIGNATURE_BEGIN);
			break;

		default:
			break;
		}
		break;

	case mint::token::TILDE_TOKEN:
		switch (get_state()) {
		case EXPECT_FUNCTION:
			if (Function *instance = m_dictionary->get_or_create_function(definition_name(token))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(EXPECT_SIGNATURE_BEGIN);
			break;

		default:
			add_modifiers(mint::Reference::PACKAGE_VISIBILITY);
			break;
		}
		break;

	case mint::token::DBL_PLUS_TOKEN:
		switch (get_state()) {
		case EXPECT_FUNCTION:
			if (Function *instance = m_dictionary->get_or_create_function(definition_name(token))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(EXPECT_SIGNATURE_BEGIN);
			break;

		default:
			break;
		}
		break;

	case mint::token::DBL_MINUS_TOKEN:
		switch (get_state()) {
		case EXPECT_FUNCTION:
			if (Function *instance = m_dictionary->get_or_create_function(definition_name(token))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(EXPECT_SIGNATURE_BEGIN);
			break;

		default:
			break;
		}
		break;

	case mint::token::DBL_ASTERISK_TOKEN:
		switch (get_state()) {
		case EXPECT_FUNCTION:
			if (Function *instance = m_dictionary->get_or_create_function(definition_name(token))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(EXPECT_SIGNATURE_BEGIN);
			break;

		default:
			break;
		}
		break;
	case mint::token::DBL_DOT_TOKEN:
		switch (get_state()) {
		case EXPECT_FUNCTION:
			if (Function *instance = m_dictionary->get_or_create_function(definition_name(token))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(EXPECT_SIGNATURE_BEGIN);
			break;

		default:
			break;
		}
		break;
	case mint::token::TPL_DOT_TOKEN:
		switch (get_state()) {
		case EXPECT_FUNCTION:
			if (Function *instance = m_dictionary->get_or_create_function(definition_name(token))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(EXPECT_SIGNATURE_BEGIN);
			break;

		default:
			break;
		}
		break;
	case mint::token::SHARP_TOKEN:
		add_modifiers(mint::Reference::PROTECTED_VISIBILITY);
		break;

	case mint::token::AT_TOKEN:
		add_modifiers(mint::Reference::GLOBAL);
		break;

	case mint::token::DOLLAR_TOKEN:
		add_modifiers(mint::Reference::CONST_ADDRESS);
		break;

	case mint::token::CONST_TOKEN:
		add_modifiers(mint::Reference::CONST_ADDRESS | mint::Reference::CONST_VALUE);
		break;

	case mint::token::FINAL_TOKEN:
		add_modifiers(mint::Reference::FINAL_MEMBER);
		break;

	case mint::token::OVERRIDE_TOKEN:
		add_modifiers(mint::Reference::OVERRIDE_MEMBER);
		break;

	case mint::token::ASSERT_TOKEN:
	case mint::token::BREAK_TOKEN:
	case mint::token::CASE_TOKEN:
	case mint::token::CATCH_TOKEN:
	case mint::token::CONTINUE_TOKEN:
	case mint::token::DEFAULT_TOKEN:
	case mint::token::ELIF_TOKEN:
	case mint::token::ELSE_TOKEN:
	case mint::token::EXIT_TOKEN:
	case mint::token::FOR_TOKEN:
	case mint::token::IF_TOKEN:
	case mint::token::LIB_TOKEN:
	case mint::token::PRINT_TOKEN:
	case mint::token::RAISE_TOKEN:
	case mint::token::RETURN_TOKEN:
	case mint::token::SWITCH_TOKEN:
	case mint::token::TRY_TOKEN:
	case mint::token::WHILE_TOKEN:
	case mint::token::YIELD_TOKEN:
	case mint::token::IS_TOKEN:
	case mint::token::TYPEOF_TOKEN:
	case mint::token::MEMBERSOF_TOKEN:
	case mint::token::DEFINED_TOKEN:
		start_modifiers(mint::Reference::DEFAULT);
		break;

	case mint::token::COMMENT_TOKEN:
		m_comment = token;
		if (offset == 0) {
			m_dictionary->set_module_doc(cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number));
		}
		break;

	default:
		start_modifiers(mint::Reference::DEFAULT);
		break;
	}
	return true;
}

bool Parser::on_new_line(size_t line_number, std::string::size_type offset) {
	m_line_number = line_number;
	m_line_offset = offset;
	return true;
}

bool Parser::on_comment_begin(std::string::size_type offset) {
	m_comment_line_number = m_line_number;
	m_comment_column_number = offset - m_line_offset;
	return true;
}

void Parser::parse_error(const char *message, size_t column, size_t begin_line, size_t end_line) {

	static constexpr const char *TAB_PLACEHOLDER = "\033[1;30m\xC2\xBB\t\033[0m";
	static constexpr const char *SPACE_PLACEHOLDER = "\033[1;30m\xC2\xB7\033[0m";

	std::string message_line;
	std::ifstream stream(m_path);
	std::string line_content = "\033[0m";
	std::string message_pos = "\033[1;30m";

	for (size_t i = 1; i <= end_line; ++i) {
		getline(stream, line_content, '\n');
		if (i >= begin_line) {
			for (char c : line_content) {
				switch (c) {
				case '\t':
					message_line += TAB_PLACEHOLDER;
					break;
				case ' ':
					message_line += SPACE_PLACEHOLDER;
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
		if (i < column - 1) {
			switch (byte_t c = line_content[i]) {
			case '\t':
				message_line += TAB_PLACEHOLDER;
				message_pos += '\t';
				break;
			case ' ':
				message_line += SPACE_PLACEHOLDER;
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
	}

	message_pos += '^';

	mint::error("%s:%d: %s\n%s\n%s\n", m_path.c_str(), m_line_number, message, message_line.c_str(),
				message_pos.c_str());
}

Parser::State Parser::get_state() const {
	return m_state;
}

void Parser::set_state(State state) {
	m_state = state;
}

void Parser::push_state(State state) {
	m_states.push_back(m_state);
	m_state = state;
}

void Parser::pop_state() {
	if (m_states.empty()) {
		m_state = EXPECT_START;
	}
	else {
		m_state = m_states.back();
		m_states.pop_back();
	}
}

Parser::Context *Parser::current_context() const {
	return m_context;
}

std::string Parser::definition_name(const std::string &token) const {

	std::string name;

	for (const Context *scope : m_contexts) {
		name += scope->name + ".";
	}

	if (m_context) {
		name += m_context->name + ".";
	}

	return name + token;
}

void Parser::push_context(const std::string &name, Definition *definition) {

	if (m_context) {
		m_contexts.push_back(m_context);
	}

	m_context = new Context {name, definition, 0};
}

void Parser::bind_definition_to_context(Definition *definition) {

	/*for (Context* context : m_contexts) {
		bind_definition_to_context(context, definition);
	}*/

	if (m_context) {
		if (m_context->definition == definition) {
			if (!m_contexts.empty()) {
				bind_definition_to_context(m_contexts.back(), definition);
			}
		}
		else {
			bind_definition_to_context(m_context, definition);
		}
	}
}

void Parser::bind_definition_to_context(Context *context, Definition *definition) {

	switch (context->definition->type) {
	case Definition::PACKAGE_DEFINITION:
		if (auto *instance = static_cast<Package *>(context->definition)) {
			instance->members.insert(definition->name);
		}
		break;

	case Definition::ENUM_DEFINITION:
		if (auto *instance = static_cast<Enum *>(context->definition)) {
			instance->members.insert(definition->name);
		}
		break;

	case Definition::CLASS_DEFINITION:
		if (auto *instance = static_cast<Class *>(context->definition)) {
			instance->members.insert(definition->name);
		}
		break;

	default:
		break;
	}
}

void Parser::open_block() {
	if (m_context) {
		m_context->block++;
	}
}

void Parser::close_block() {
	if (m_context && !--m_context->block) {
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

void Parser::start_modifiers(mint::Reference::Flags flags) {
	m_modifiers = flags;
}

void Parser::add_modifiers(mint::Reference::Flags flags) {
	m_modifiers |= flags;
}

mint::Reference::Flags Parser::retrieve_modifiers() {
	mint::Reference::Flags flags = m_modifiers;
	m_modifiers = mint::Reference::DEFAULT;
	return flags;
}

std::string Parser::cleanup_doc(const std::string &comment, size_t line, size_t column) {

	if (mint::starts_with(comment, "/**")) {
		std::stringstream stream(comment);
		stream.seekg(3, std::stringstream::beg);
		return cleanup_multi_line_doc(stream, line, column);
	}

	if (mint::starts_with(comment, "///")) {
		std::stringstream stream(comment);
		stream.seekg(3, std::stringstream::beg);
		return cleanup_single_line_doc(stream, line, column);
	}

	return {};
}

std::string Parser::cleanup_single_line_doc(std::stringstream &stream, size_t line, size_t column) {

	bool finished = false;

	std::string documentation;
	size_t current_line = line;

	if (stream.eof() || stream.get() != ' ') {
		parse_error("expected ' ' character before documentation string", column);
	}

	while (!finished && !stream.eof()) {
		switch (int c = stream.get()) {
		case EOF:
			finished = true;
			break;

		case '\n':
			current_line++;
			documentation += static_cast<char>(c);
			finished = true;
			break;

		case '`':
			documentation += static_cast<char>(c);
			cleanup_script(stream, documentation, line, column + 1, current_line);
			break;

		default:
			documentation += static_cast<char>(c);
			break;
		}
	}

	return documentation;
}

std::string Parser::cleanup_multi_line_doc(std::stringstream &stream, size_t line, size_t column) {

	bool finished = false;
	bool suspect_end = false;

	std::string documentation;
	size_t current_line = line;

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
			current_line++;
			documentation += static_cast<char>(c);
			stream.seekg(static_cast<std::stringstream::off_type>(column + 1), std::stringstream::cur);
			if (stream.eof() || stream.get() != '*') {
				parse_error("expected '*' character for documentation continuation", column + 1, line, current_line);
			}
			if (!stream.eof()) {
				switch (stream.get()) {
				case ' ':
					break;
				case '\n':
					stream.unget();
					break;
				case '/':
					finished = true;
					break;
				default:
					parse_error("expected ' ' character before documentation string", column, line, current_line);
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
			cleanup_script(stream, documentation, line, column, current_line);
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

void Parser::cleanup_script(std::stringstream &stream, std::string &documentation, size_t line, size_t column,
							size_t &current_line) {

	if (!stream.eof()) {

		int c = stream.get();
		documentation += static_cast<char>(c);

		if (c == '`') {
			do {
				cleanup_script(stream, documentation, line, column, current_line);
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
					current_line++;
					documentation += static_cast<char>(c);
					stream.seekg(static_cast<std::stringstream::off_type>(column + 1), std::stringstream::cur);
					if (stream.eof() || (c = stream.get()) != '*') {
						parse_error("expected '*' character for documentation continuation", column + 1, line,
									current_line);
					}
					if (!stream.eof()) {
						switch (stream.get()) {
						case ' ':
							break;
						case '\n':
							stream.unget();
							break;
						case '/':
							finished = true;
							break;
						default:
							parse_error("expected ' ' character before documentation string", column, line,
										current_line);
							break;
						}
					}
					break;

				default:
					documentation += static_cast<char>(c);
					break;
				}
			}
		}
	}
}
