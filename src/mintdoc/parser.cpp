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

#include "parser.h"
#include "dictionnary.h"

#include <mint/memory/casttool.h>
#include <mint/system/string.h>
#include <mint/system/error.h>
#include <unordered_set>
#include <fstream>
#include <sstream>
#include <vector>

static const std::unordered_set<std::string> g_unpadded_prefixes = { "(", "[", "{", "." };
static const std::unordered_set<std::string> g_unpadded_postfixes = { ")", "]", "}", ",", "." };
static bool contains(const std::unordered_set<std::string> &set, const std::string &value) {
	return set.find(value) != end(set);
}

Parser::Parser(const std::string &path) :
	m_path(path) {

}

Parser::~Parser() {
	while (m_context) {
		close_block();
	}
}

void value_add_token(Constant *constant, const std::string& token) {

	if (token != "\n") {

		if (!constant->value.empty()
				&& !contains(g_unpadded_prefixes, std::string(1, constant->value.back()))
				&& !contains(g_unpadded_postfixes, token)) {
			constant->value += " ";
		}

		constant->value += token;
	}
}

void signature_add_token(Function::Signature *signature, const std::string& token) {

	if (!signature->format.empty()
			&& !contains(g_unpadded_prefixes, std::string(1, signature->format.back()))
			&& !contains(g_unpadded_postfixes, token)) {
		signature->format += " ";
	}

	signature->format += token;
}

void Parser::parse(Dictionnary *dictionnary) {

	std::ifstream file(m_path);

	m_dictionnary = dictionnary;
	m_signature = nullptr;
	m_definition = nullptr;

	LexicalHandler::parse(file);
}

bool Parser::on_token(mint::token::Type type, const std::string &token, std::string::size_type offset) {

	switch (get_state()) {
	case expect_function:
		break;

	case expect_value:
	case expect_value_subexpression:
		if (Constant *instance = static_cast<Constant *>(m_definition)) {
			value_add_token(instance, token);
		}
		break;

	case expect_signature:
	case expect_signature_subexpression:
		signature_add_token(m_signature, token);
		break;

	default:
		break;
	}

	switch (type) {
	case mint::token::class_token:
		set_state(expect_class);
		break;
	case mint::token::def_token:
		if (m_definition) {
			if (Function *instance = m_dictionnary->get_or_create_function(m_definition->name)) {
				instance->flags = m_definition->flags;
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				delete m_definition;
				m_definition = instance;
			}
			start_modifiers(mint::Reference::standard);
			set_state(expect_signature_begin);
			m_comment.clear();
		}
		else {
			set_state(expect_function);
		}
		break;
	case mint::token::enum_token:
		set_state(expect_enum);
		m_next_enum_constant = 0;
		break;
	case mint::token::package_token:
		set_state(expect_package);
		break;

	case mint::token::symbol_token:
		if (m_definition) {
			switch (get_state()) {
			case expect_base:
				m_base += token;
				break;

			case expect_value:
			case expect_signature:
				break;

			default:
				set_state(expect_start);
				break;
			}
		}
		else {
			switch (get_state()) {
			case expect_package:
				if (Package *instance = m_dictionnary->get_or_create_package(definition_name(token))) {
					push_context(token, instance);
					if (instance->doc.empty()) {
						instance->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
					}
					instance->flags = retrieve_modifiers();
					m_definition = instance;
				}

				set_state(expect_start);
				break;

			case expect_class:
				if (Class *instance = new Class(definition_name(token))) {
					push_context(token, instance);
					if (instance->doc.empty()) {
						instance->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
					}
					instance->flags = retrieve_modifiers();
					m_definition = instance;
				}

				set_state(expect_start);
				break;

			case expect_enum:
				if (Enum *instance = new Enum(definition_name(token))) {
					push_context(token, instance);
					if (instance->doc.empty()) {
						instance->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
					}
					m_next_enum_constant = 0;
					instance->flags = retrieve_modifiers();
					m_definition = instance;
				}

				set_state(expect_start);
				break;

			case expect_function:
				if (Function *instance = m_dictionnary->get_or_create_function(definition_name(token))) {
					m_signature = new Function::Signature;
					m_signature->format = "def";
					if (m_signature->doc.empty()) {
						m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
					}
					instance->flags = retrieve_modifiers();
					m_definition = instance;
				}

				set_state(expect_signature_begin);
				break;

			case expect_start:
				if (m_modifiers & mint::Reference::global) {
					if (Constant *instance = new Constant(definition_name(token))) {
						if (instance->doc.empty()) {
							instance->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
						}
						instance->flags = retrieve_modifiers();
						m_definition = instance;
					}
				}
				else if (const Context* context = current_context()) {
					if (context->block == 1) {
						switch (context->definition->type) {
						case Definition::class_definition:
							if (Constant *instance = new Constant(definition_name(token))) {
								if (instance->doc.empty()) {
									instance->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
								}
								instance->flags = retrieve_modifiers();
								m_definition = instance;
							}
							break;

						case Definition::enum_definition:
							if (Constant *instance = new Constant(definition_name(token))) {
								if (instance->doc.empty()) {
									instance->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
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

				set_state(expect_start);
				break;

			case expect_capture:
				return true;

			case expect_signature:
				break;

			default:
				set_state(expect_start);
				break;
			}
		}
		start_modifiers(mint::Reference::standard);
		m_comment.clear();
		break;

	case mint::token::open_parenthesis_token:
		switch (get_state()) {
		case expect_function:
			set_state(expect_parenthesis_operator);
			break;

		case expect_signature:
		case expect_signature_subexpression:
			push_state(expect_signature_subexpression);
			start_modifiers(mint::Reference::standard);
			break;

		case expect_value:
		case expect_value_subexpression:
			push_state(expect_value_subexpression);
			start_modifiers(mint::Reference::standard);
			break;

		case expect_signature_begin:
			m_signature->format += " " + token;
			start_modifiers(mint::Reference::standard);
			set_state(expect_signature);
			break;

		default:
			start_modifiers(mint::Reference::standard);
			break;
		}
		break;

	case mint::token::close_parenthesis_token:
		switch (get_state()) {
		case expect_parenthesis_operator:
			if (Function *instance = m_dictionnary->get_or_create_function(definition_name("()"))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(expect_signature_begin);
			break;

		case expect_signature_subexpression:
		case expect_value_subexpression:
			pop_state();
			break;

		case expect_signature:
			pop_state();
			break;

		default:
			break;
		}
		start_modifiers(mint::Reference::standard);
		break;

	case mint::token::open_bracket_token:
		switch (get_state()) {
		case expect_function:
			if (const Context* context = current_context()) {
				if (context->definition->type == Definition::class_definition) {
					set_state(expect_bracket_operator);
				}
				else {
					start_modifiers(mint::Reference::standard);
					push_state(expect_capture);
				}
			}
			else {
				start_modifiers(mint::Reference::standard);
				push_state(expect_capture);
			}
			break;

		case expect_signature:
		case expect_signature_subexpression:
			push_state(expect_signature_subexpression);
			start_modifiers(mint::Reference::standard);
			break;

		case expect_value:
		case expect_value_subexpression:
			push_state(expect_value_subexpression);
			start_modifiers(mint::Reference::standard);
			break;

		default:
			start_modifiers(mint::Reference::standard);
			break;
		}
		break;

	case mint::token::close_bracket_token:
		switch (get_state()) {
		case expect_bracket_operator:
			if (Function *instance = m_dictionnary->get_or_create_function(definition_name("[]"))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(expect_signature_begin);
			break;

		case expect_capture:
			pop_state();
			break;

		case expect_signature_subexpression:
		case expect_value_subexpression:
			pop_state();
			break;

		default:
			break;
		}
		start_modifiers(mint::Reference::standard);
		break;

	case mint::token::close_bracket_equal_token:
		switch (get_state()) {
		case expect_bracket_operator:
			if (Function *instance = m_dictionnary->get_or_create_function(definition_name("[]="))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(expect_signature_begin);
			break;

		case expect_signature_subexpression:
		case expect_value_subexpression:
			pop_state();
			break;

		default:
			break;
		}
		break;

	case mint::token::open_brace_token:
		switch (get_state()) {
		case expect_base:
			if (Class *instace = static_cast<Class *>(m_definition)) {
				instace->bases.push_back(m_base);
				m_base.clear();
			}
			break;

		case expect_signature:
		case expect_signature_subexpression:
			push_state(expect_signature_subexpression);
			break;

		case expect_value:
		case expect_value_subexpression:
			push_state(expect_value_subexpression);
			break;

		case expect_function:
			pop_state();
			break;

		default:
			break;
		}
		start_modifiers(mint::Reference::standard);
		open_block();
		break;

	case mint::token::close_brace_token:
		switch (get_state()) {
		case expect_signature_subexpression:
		case expect_value_subexpression:
			pop_state();
			break;

		default:
			break;
		}
		start_modifiers(mint::Reference::standard);
		m_comment.clear();
		close_block();
		break;

	case mint::token::line_end_token:
		switch (get_state()) {
		case expect_signature_subexpression:
		case expect_value_subexpression:
			break;

		case expect_value:
			pop_state();
			[[fallthrough]];

		default:
			if (m_definition) {
				switch (m_definition->type) {
				case Definition::constant_definition:
					if (const Context* context = current_context()) {
						if (context->definition->type == Definition::enum_definition) {
							if (Constant *instance = static_cast<Constant *>(m_definition)) {
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

				case Definition::function_definition:
					if (m_signature) {
						if (Function *instance = static_cast<Function *>(m_definition)) {
							instance->signatures.push_back(m_signature);
						}
						m_signature = nullptr;
					}
					break;

				default:
					break;
				}
				bind_definition_to_context(m_definition);
				m_dictionnary->insert_definition(m_definition);
				m_definition = nullptr;
			}
			break;
		}
		start_modifiers(mint::Reference::standard);
		break;

	case mint::token::constant_token:
		start_modifiers(mint::Reference::standard);
		break;

	case mint::token::number_token:
		start_modifiers(mint::Reference::standard);
		break;

	case mint::token::string_token:
		start_modifiers(mint::Reference::standard);
		break;

	case mint::token::dbldot_token:
		start_modifiers(mint::Reference::standard);
		if (m_definition) {
			switch (m_definition->type) {
			case Definition::class_definition:
				set_state(expect_base);
				m_base.clear();
				break;

			default:
				break;
			}
		}
		break;

	case mint::token::equal_token:
		if (m_definition && m_definition->type == Definition::constant_definition) {
			push_state(expect_value);
		}
		break;

	case mint::token::dot_token:
		switch (get_state()) {
		case expect_base:
			m_base += token;
			break;

		default:
			break;
		}
		break;

	case mint::token::comma_token:
		switch (get_state()) {
		case expect_base:
			if (Class *instance = static_cast<Class *>(m_definition)) {
				instance->bases.push_back(m_base);
				m_base.clear();
			}
			break;

		default:
			break;
		}
		break;

	case mint::token::in_token:
		switch (get_state()) {
		case expect_function:
			if (Function *instance = m_dictionnary->get_or_create_function(definition_name(token))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(expect_signature_begin);
			break;

		default:
			break;
		}
		break;

	case mint::token::dbldot_equal_token:
		switch (get_state()) {
		case expect_function:
			if (Function *instance = m_dictionnary->get_or_create_function(definition_name(token))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(expect_signature_begin);
			break;

		default:
			break;
		}
		break;

	case mint::token::dbl_pipe_token:
		switch (get_state()) {
		case expect_function:
			if (Function *instance = m_dictionnary->get_or_create_function(definition_name(token))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(expect_signature_begin);
			break;

		default:
			break;
		}
		break;

	case mint::token::dbl_amp_token:
		switch (get_state()) {
		case expect_function:
			if (Function *instance = m_dictionnary->get_or_create_function(definition_name(token))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(expect_signature_begin);
			break;

		default:
			break;
		}
		break;

	case mint::token::pipe_token:
		switch (get_state()) {
		case expect_function:
			if (Function *instance = m_dictionnary->get_or_create_function(definition_name(token))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(expect_signature_begin);
			break;

		default:
			break;
		}
		break;

	case mint::token::caret_token:
		switch (get_state()) {
		case expect_function:
			if (Function *instance = m_dictionnary->get_or_create_function(definition_name(token))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(expect_signature_begin);
			break;

		default:
			break;
		}
		break;

	case mint::token::amp_token:
		switch (get_state()) {
		case expect_function:
			if (Function *instance = m_dictionnary->get_or_create_function(definition_name(token))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(expect_signature_begin);
			break;

		default:
			break;
		}
		break;

	case mint::token::dbl_equal_token:
		switch (get_state()) {
		case expect_function:
			if (Function *instance = m_dictionnary->get_or_create_function(definition_name(token))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(expect_signature_begin);
			break;

		default:
			break;
		}
		break;

	case mint::token::exclamation_equal_token:
		switch (get_state()) {
		case expect_function:
			if (Function *instance = m_dictionnary->get_or_create_function(definition_name(token))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(expect_signature_begin);
			break;

		default:
			break;
		}
		break;

	case mint::token::left_angled_token:
		switch (get_state()) {
		case expect_function:
			if (Function *instance = m_dictionnary->get_or_create_function(definition_name(token))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(expect_signature_begin);
			break;

		default:
			break;
		}
		break;

	case mint::token::right_angled_token:
		switch (get_state()) {
		case expect_function:
			if (Function *instance = m_dictionnary->get_or_create_function(definition_name(token))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(expect_signature_begin);
			break;

		default:
			break;
		}
		break;

	case mint::token::left_angled_equal_token:
		switch (get_state()) {
		case expect_function:
			if (Function *instance = m_dictionnary->get_or_create_function(definition_name(token))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(expect_signature_begin);
			break;

		default:
			break;
		}
		break;

	case mint::token::right_angled_equal_token:
		switch (get_state()) {
		case expect_function:
			if (Function *instance = m_dictionnary->get_or_create_function(definition_name(token))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(expect_signature_begin);
			break;

		default:
			break;
		}
		break;

	case mint::token::dbl_left_angled_token:
		switch (get_state()) {
		case expect_function:
			if (Function *instance = m_dictionnary->get_or_create_function(definition_name(token))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(expect_signature_begin);
			break;

		default:
			break;
		}
		break;

	case mint::token::dbl_right_angled_token:
		switch (get_state()) {
		case expect_function:
			if (Function *instance = m_dictionnary->get_or_create_function(definition_name(token))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(expect_signature_begin);
			break;

		default:
			break;
		}
		break;

	case mint::token::plus_token:
		switch (get_state()) {
		case expect_function:
			if (Function *instance = m_dictionnary->get_or_create_function(definition_name(token))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(expect_signature_begin);
			break;

		default:
			break;
		}
		break;

	case mint::token::minus_token:
		switch (get_state()) {
		case expect_function:
			if (Function *instance = m_dictionnary->get_or_create_function(definition_name(token))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(expect_signature_begin);
			break;

		default:
			add_modifiers(mint::Reference::private_visibility);
			break;
		}
		break;

	case mint::token::asterisk_token:
		switch (get_state()) {
		case expect_function:
			if (Function *instance = m_dictionnary->get_or_create_function(definition_name(token))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(expect_signature_begin);
			break;

		default:
			break;
		}
		break;

	case mint::token::slash_token:
		switch (get_state()) {
		case expect_function:
			if (Function *instance = m_dictionnary->get_or_create_function(definition_name(token))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(expect_signature_begin);
			break;

		default:
			break;
		}
		break;

	case mint::token::percent_token:
		switch (get_state()) {
		case expect_function:
			if (Function *instance = m_dictionnary->get_or_create_function(definition_name(token))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(expect_signature_begin);
			break;

		default:
			add_modifiers(mint::Reference::const_value);
			break;
		}
		break;

	case mint::token::exclamation_token:
		switch (get_state()) {
		case expect_function:
			if (Function *instance = m_dictionnary->get_or_create_function(definition_name(token))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(expect_signature_begin);
			break;

		default:
			break;
		}
		break;

	case mint::token::tilde_token:
		switch (get_state()) {
		case expect_function:
			if (Function *instance = m_dictionnary->get_or_create_function(definition_name(token))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(expect_signature_begin);
			break;

		default:
			add_modifiers(mint::Reference::package_visibility);
			break;
		}
		break;

	case mint::token::dbl_plus_token:
		switch (get_state()) {
		case expect_function:
			if (Function *instance = m_dictionnary->get_or_create_function(definition_name(token))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(expect_signature_begin);
			break;

		default:
			break;
		}
		break;

	case mint::token::dbl_minus_token:
		switch (get_state()) {
		case expect_function:
			if (Function *instance = m_dictionnary->get_or_create_function(definition_name(token))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(expect_signature_begin);
			break;

		default:
			break;
		}
		break;

	case mint::token::dbl_asterisk_token:
		switch (get_state()) {
		case expect_function:
			if (Function *instance = m_dictionnary->get_or_create_function(definition_name(token))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(expect_signature_begin);
			break;

		default:
			break;
		}
		break;
	case mint::token::dot_dot_token:
		switch (get_state()) {
		case expect_function:
			if (Function *instance = m_dictionnary->get_or_create_function(definition_name(token))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(expect_signature_begin);
			break;

		default:
			break;
		}
		break;
	case mint::token::tpl_dot_token:
		switch (get_state()) {
		case expect_function:
			if (Function *instance = m_dictionnary->get_or_create_function(definition_name(token))) {
				m_signature = new Function::Signature;
				m_signature->format = "def";
				if (m_signature->doc.empty()) {
					m_signature->doc = cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number);
				}
				instance->flags = retrieve_modifiers();
				m_definition = instance;
			}
			set_state(expect_signature_begin);
			break;

		default:
			break;
		}
		break;
	case mint::token::sharp_token:
		add_modifiers(mint::Reference::protected_visibility);
		break;

	case mint::token::at_token:
		add_modifiers(mint::Reference::global);
		break;

	case mint::token::dollar_token:
		add_modifiers(mint::Reference::const_address);
		break;

	case mint::token::const_token:
		add_modifiers(mint::Reference::const_address | mint::Reference::const_value);
		break;

	case mint::token::final_token:
		add_modifiers(mint::Reference::final_member);
		break;

	case mint::token::override_token:
		add_modifiers(mint::Reference::override_member);
		break;

	case mint::token::assert_token:
	case mint::token::break_token:
	case mint::token::case_token:
	case mint::token::catch_token:
	case mint::token::continue_token:
	case mint::token::default_token:
	case mint::token::elif_token:
	case mint::token::else_token:
	case mint::token::exit_token:
	case mint::token::for_token:
	case mint::token::if_token:
	case mint::token::lib_token:
	case mint::token::print_token:
	case mint::token::raise_token:
	case mint::token::return_token:
	case mint::token::switch_token:
	case mint::token::try_token:
	case mint::token::while_token:
	case mint::token::yield_token:
	case mint::token::is_token:
	case mint::token::typeof_token:
	case mint::token::membersof_token:
	case mint::token::defined_token:
		start_modifiers(mint::Reference::standard);
		break;

	case mint::token::comment_token:
		m_comment = token;
		if (offset == 0) {
			m_dictionnary->set_module_doc(cleanup_doc(m_comment, m_comment_line_number, m_comment_column_number));
		}
		break;

	default:
		start_modifiers(mint::Reference::standard);
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

	static constexpr const char *tab_placeholder = "\033[1;30m\xC2\xBB\t\033[0m";
	static constexpr const char *space_placeholder = "\033[1;30m\xC2\xB7\033[0m";

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
		if (i < column - 1) {
			switch (byte_t c = line_content[i]) {
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
	}

	message_pos += '^';
	
	mint::error("%s:%d: %s\n%s\n%s\n", m_path.c_str(), m_line_number, message, message_line.c_str(), message_pos.c_str());
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
		m_state = expect_start;
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

void Parser::push_context(const std::string &name, Definition* definition) {

	if (m_context) {
		m_contexts.push_back(m_context);
	}

	m_context = new Context{name, definition, 0};
}

void Parser::bind_definition_to_context(Definition* definition) {

	/*for (Context* context : m_contexts) {
		bindDefinitionToContext(context, definition);
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

void Parser::bind_definition_to_context(Context* context, Definition* definition) {

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
	m_modifiers = mint::Reference::standard;
	return flags;
}

std::string Parser::cleanup_doc(const std::string &comment, size_t line, size_t column) {

	if (mint::starts_with(comment, "/**")) {
		std::stringstream stream(comment);
		stream.seekg(3, stream.beg);
		return cleanup_multi_line_doc(stream, line, column);
	}

	if (mint::starts_with(comment, "///")) {
		std::stringstream stream(comment);
		stream.seekg(3, stream.beg);
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
			stream.seekg(column + 1, stream.cur);
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

void Parser::cleanup_script(std::stringstream &stream, std::string &documentation, size_t line, size_t column, size_t &current_line) {

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
					stream.seekg(column + 1, stream.cur);
					if (stream.eof() || (c = stream.get()) != '*') {
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

				default:
					documentation += static_cast<char>(c);
					break;
				}
			}
		}
	}
}
