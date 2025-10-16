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

#include "parser.h"
#include "definition.h"
#include "dictionary.h"
#include "docnode.h"

#include "mint/compiler/lexicalhandler.h"
#include "mint/compiler/token.h"
#include "mint/config.h"
#include "mint/memory/casttool.h"
#include "mint/memory/reference.h"
#include "mint/system/error.h"
#include <cstddef>
#include <cstdio>
#include <memory>
#include <string>
#include <unordered_set>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <utility>
#include <vector>

namespace {

const std::unordered_set<std::string> unpadded_prefixes = {"(", "[", "{", "."};
const std::unordered_set<std::string> unpadded_postfixes = {")", "]", "}", ",", "."};

void value_add_token(Constant& constant, const std::string& token) {
	if (token != "\n") {
		if (!constant.value.empty() && !unpadded_prefixes.contains(std::string(1, constant.value.back()))
		    && !unpadded_postfixes.contains(token)) {
			constant.value += " ";
		}
		constant.value += token;
	}
}

void signature_add_token(Function::Signature& signature, const std::string& token) {
	if (!signature.format.empty() && !unpadded_prefixes.contains(std::string(1, signature.format.back()))
	    && !unpadded_postfixes.contains(token)) {
		signature.format += " ";
	}
	signature.format += token;
}

}

Parser::Parser(std::filesystem::path path) :
    _path(std::move(path)) {}

void Parser::parse(Dictionary& dictionary) {

	_context = std::make_unique<Context>(Context {
	    .dictionary = dictionary,
	});

	auto file = std::ifstream(_path);
	LexicalHandler::parse(file);
}

bool Parser::on_token(mint::Token type, const std::string& token, std::string::size_type offset) {

	switch (get_state()) {
	case State::expect_function:
		break;

	case State::expect_value:
	case State::expect_value_subexpression:
		if (auto instance = std::static_pointer_cast<Constant>(_context->definition)) {
			value_add_token(*instance, token);
		}
		break;

	case State::expect_signature:
	case State::expect_signature_subexpression:
		signature_add_token(*_context->signature, token);
		break;

	default:
		break;
	}

	switch (type) {
	case mint::Token::class_token:
		set_state(State::expect_class);
		break;
	case mint::Token::def_token:
		if (_context->definition) {
			if (auto instance = _context->dictionary.get().get_or_create_function(_context->definition->name)) {
				instance->flags = _context->definition->flags;
				_context->signature = std::make_shared<Function::Signature>(Function::Signature {
				    .format = "def",
				    .doc = parse_doc(
				        cleanup_doc(_context->comment, _context->comment_line_number, _context->comment_column_number)),
				});
				_context->definition = instance;
			}
			start_modifiers(mint::Reference::default_flags);
			set_state(State::expect_signature_begin);
			_context->comment.clear();
		}
		else {
			set_state(State::expect_function);
		}
		break;
	case mint::Token::enum_token:
		set_state(State::expect_enum);
		_context->next_enum_constant = 0;
		break;
	case mint::Token::package_token:
		set_state(State::expect_package);
		break;

	case mint::Token::symbol_token:
		if (_context->definition) {
			switch (get_state()) {
			case State::expect_base:
				_context->base += token;
				break;

			case State::expect_value:
			case State::expect_signature:
				break;

			default:
				set_state(State::expect_start);
				break;
			}
		}
		else {
			switch (get_state()) {
			case State::expect_package:
				if (auto instance = _context->dictionary.get().get_or_create_package(definition_name(token))) {
					push_context(token, instance);
					if (!instance->doc) {
						instance->doc = parse_doc(cleanup_doc(_context->comment, _context->comment_line_number,
						    _context->comment_column_number));
					}
					instance->flags = retrieve_modifiers();
					_context->definition = instance;
				}

				set_state(State::expect_start);
				break;

			case State::expect_class:
				if (auto instance = std::make_shared<Class>(definition_name(token))) {
					push_context(token, instance);
					if (!instance->doc) {
						instance->doc = parse_doc(cleanup_doc(_context->comment, _context->comment_line_number,
						    _context->comment_column_number));
					}
					instance->flags = retrieve_modifiers();
					_context->definition = instance;
				}

				set_state(State::expect_start);
				break;

			case State::expect_enum:
				if (auto instance = std::make_shared<Enum>(definition_name(token))) {
					push_context(token, instance);
					if (!instance->doc) {
						instance->doc = parse_doc(cleanup_doc(_context->comment, _context->comment_line_number,
						    _context->comment_column_number));
					}
					_context->next_enum_constant = 0;
					instance->flags = retrieve_modifiers();
					_context->definition = instance;
				}

				set_state(State::expect_start);
				break;

			case State::expect_function:
				if (auto instance = _context->dictionary.get().get_or_create_function(definition_name(token))) {
					_context->signature = std::make_shared<Function::Signature>(Function::Signature {
					    .format = "def",
					    .doc = parse_doc(cleanup_doc(_context->comment, _context->comment_line_number,
					        _context->comment_column_number)),
					});
					instance->flags = retrieve_modifiers();
					_context->definition = instance;
				}

				set_state(State::expect_signature_begin);
				break;

			case State::expect_start:
				if (_context->modifiers & mint::Reference::global) {
					if (auto instance = std::make_shared<Constant>(definition_name(token))) {
						if (!instance->doc) {
							instance->doc = parse_doc(cleanup_doc(_context->comment, _context->comment_line_number,
							    _context->comment_column_number));
						}
						instance->flags = retrieve_modifiers();
						_context->definition = instance;
					}
				}
				else if (const auto* context = current_context()) {
					if (context->depth == 1) {
						switch (context->definition->type) {
						case Definition::class_definition:
							if (auto instance = std::make_shared<Constant>(definition_name(token))) {
								if (!instance->doc) {
									instance->doc = parse_doc(cleanup_doc(_context->comment,
									    _context->comment_line_number, _context->comment_column_number));
								}
								instance->flags = retrieve_modifiers();
								_context->definition = instance;
							}
							break;

						case Definition::enum_definition:
							if (auto instance = std::make_shared<Constant>(definition_name(token))) {
								if (!instance->doc) {
									instance->doc = parse_doc(cleanup_doc(_context->comment,
									    _context->comment_line_number, _context->comment_column_number));
								}
								instance->flags = retrieve_modifiers();
								_context->definition = instance;
							}
							break;

						default:
							break;
						}
					}
				}

				set_state(State::expect_start);
				break;

			case State::expect_capture:
				return true;

			case State::expect_signature:
				break;

			default:
				set_state(State::expect_start);
				break;
			}
		}
		start_modifiers(mint::Reference::default_flags);
		_context->comment.clear();
		break;

	case mint::Token::open_parenthesis_token:
		switch (get_state()) {
		case State::expect_function:
			set_state(State::expect_parenthesis_operator);
			break;

		case State::expect_signature:
		case State::expect_signature_subexpression:
			push_state(State::expect_signature_subexpression);
			start_modifiers(mint::Reference::default_flags);
			break;

		case State::expect_value:
		case State::expect_value_subexpression:
			push_state(State::expect_value_subexpression);
			start_modifiers(mint::Reference::default_flags);
			break;

		case State::expect_signature_begin:
			_context->signature->format += " " + token;
			start_modifiers(mint::Reference::default_flags);
			set_state(State::expect_signature);
			break;

		default:
			start_modifiers(mint::Reference::default_flags);
			break;
		}
		break;

	case mint::Token::close_parenthesis_token:
		switch (get_state()) {
		case State::expect_parenthesis_operator:
			if (auto instance = _context->dictionary.get().get_or_create_function(definition_name("()"))) {
				_context->signature = std::make_shared<Function::Signature>(Function::Signature {
				    .format = "def",
				    .doc = parse_doc(
				        cleanup_doc(_context->comment, _context->comment_line_number, _context->comment_column_number)),
				});
				instance->flags = retrieve_modifiers();
				_context->definition = instance;
			}
			set_state(State::expect_signature_begin);
			break;

		case State::expect_signature_subexpression:
		case State::expect_value_subexpression:
		case State::expect_signature:
			pop_state();
			break;

		default:
			break;
		}
		start_modifiers(mint::Reference::default_flags);
		break;

	case mint::Token::open_bracket_token:
		switch (get_state()) {
		case State::expect_function:
			if (const auto* context = current_context()) {
				if (context->definition->type == Definition::class_definition) {
					set_state(State::expect_bracket_operator);
				}
				else {
					start_modifiers(mint::Reference::default_flags);
					push_state(State::expect_capture);
				}
			}
			else {
				start_modifiers(mint::Reference::default_flags);
				push_state(State::expect_capture);
			}
			break;

		case State::expect_signature:
		case State::expect_signature_subexpression:
			push_state(State::expect_signature_subexpression);
			start_modifiers(mint::Reference::default_flags);
			break;

		case State::expect_value:
		case State::expect_value_subexpression:
			push_state(State::expect_value_subexpression);
			start_modifiers(mint::Reference::default_flags);
			break;

		default:
			start_modifiers(mint::Reference::default_flags);
			break;
		}
		break;

	case mint::Token::close_bracket_token:
		switch (get_state()) {
		case State::expect_bracket_operator:
			if (auto instance = _context->dictionary.get().get_or_create_function(definition_name("[]"))) {
				_context->signature = std::make_shared<Function::Signature>(Function::Signature {.format = "def",
				    .doc = parse_doc(cleanup_doc(_context->comment, _context->comment_line_number,
				        _context->comment_column_number))});
				instance->flags = retrieve_modifiers();
				_context->definition = instance;
			}
			set_state(State::expect_signature_begin);
			break;

		case State::expect_signature_subexpression:
		case State::expect_value_subexpression:
		case State::expect_capture:
			pop_state();
			break;

		default:
			break;
		}
		start_modifiers(mint::Reference::default_flags);
		break;

	case mint::Token::close_bracket_equal_token:
		switch (get_state()) {
		case State::expect_bracket_operator:
			if (auto instance = _context->dictionary.get().get_or_create_function(definition_name("[]="))) {
				_context->signature = std::make_shared<Function::Signature>(Function::Signature {
				    .format = "def",
				    .doc = parse_doc(
				        cleanup_doc(_context->comment, _context->comment_line_number, _context->comment_column_number)),
				});
				instance->flags = retrieve_modifiers();
				_context->definition = instance;
			}
			set_state(State::expect_signature_begin);
			break;

		case State::expect_signature_subexpression:
		case State::expect_value_subexpression:
			pop_state();
			break;

		default:
			break;
		}
		break;

	case mint::Token::open_brace_token:
		switch (get_state()) {
		case State::expect_base:
			if (auto instance = std::static_pointer_cast<Class>(_context->definition)) {
				instance->bases.push_back(_context->base);
				_context->base.clear();
			}
			break;

		case State::expect_signature:
		case State::expect_signature_subexpression:
			push_state(State::expect_signature_subexpression);
			break;

		case State::expect_value:
		case State::expect_value_subexpression:
			push_state(State::expect_value_subexpression);
			break;

		case State::expect_function:
			pop_state();
			break;

		default:
			break;
		}
		start_modifiers(mint::Reference::default_flags);
		open_block();
		break;

	case mint::Token::close_brace_token:
		switch (get_state()) {
		case State::expect_signature_subexpression:
		case State::expect_value_subexpression:
			pop_state();
			break;

		default:
			break;
		}
		start_modifiers(mint::Reference::default_flags);
		_context->comment.clear();
		close_block();
		break;

	case mint::Token::line_end_token:
		switch (get_state()) {
		case State::expect_signature_subexpression:
		case State::expect_value_subexpression:
			break;

		case State::expect_value:
			pop_state();
			[[fallthrough]];

		default:
			if (_context->definition) {
				switch (_context->definition->type) {
				case Definition::constant_definition:
					if (const auto* context = current_context()) {
						if (context->definition->type == Definition::enum_definition) {
							if (auto instance = std::static_pointer_cast<Constant>(_context->definition)) {
								if (instance->value.empty()) {
									instance->value = std::to_string(_context->next_enum_constant++);
								}
								else {
									_context->next_enum_constant = mint::to_signed_integer(instance->value);
									_context->next_enum_constant++;
								}
							}
						}
					}
					break;

				case Definition::function_definition:
					if (_context->signature) {
						if (auto instance = std::static_pointer_cast<Function>(_context->definition)) {
							instance->signatures.emplace_back(_context->signature);
						}
						_context->signature.reset();
					}
					break;

				default:
					break;
				}
				bind_definition_to_context(*_context->definition);
				_context->dictionary.get().insert_definition(_context->definition);
				_context->definition = nullptr;
			}
			break;
		}
		start_modifiers(mint::Reference::default_flags);
		break;

	case mint::Token::constant_token:
	case mint::Token::number_token:
	case mint::Token::string_token:
		start_modifiers(mint::Reference::default_flags);
		break;

	case mint::Token::colon_token:
		start_modifiers(mint::Reference::default_flags);
		if (_context->definition) {
			switch (_context->definition->type) {
			case Definition::class_definition:
				set_state(State::expect_base);
				_context->base.clear();
				break;

			default:
				break;
			}
		}
		break;

	case mint::Token::equal_token:
		if (_context->definition && _context->definition->type == Definition::constant_definition) {
			push_state(State::expect_value);
		}
		break;

	case mint::Token::dot_token:
		switch (get_state()) {
		case State::expect_base:
			_context->base += token;
			break;

		default:
			break;
		}
		break;

	case mint::Token::comma_token:
		switch (get_state()) {
		case State::expect_base:
			if (auto instance = std::static_pointer_cast<Class>(_context->definition)) {
				instance->bases.push_back(_context->base);
				_context->base.clear();
			}
			break;

		default:
			break;
		}
		break;

	case mint::Token::in_token:
		switch (get_state()) {
		case State::expect_function:
			if (auto instance = _context->dictionary.get().get_or_create_function(definition_name(token))) {
				_context->signature = std::make_shared<Function::Signature>(Function::Signature {
				    .format = "def",
				    .doc = parse_doc(
				        cleanup_doc(_context->comment, _context->comment_line_number, _context->comment_column_number)),
				});
				instance->flags = retrieve_modifiers();
				_context->definition = instance;
			}
			set_state(State::expect_signature_begin);
			break;

		default:
			break;
		}
		break;

	case mint::Token::colon_equal_token:
		switch (get_state()) {
		case State::expect_function:
			if (auto instance = _context->dictionary.get().get_or_create_function(definition_name(token))) {
				_context->signature = std::make_shared<Function::Signature>(Function::Signature {
				    .format = "def",
				    .doc = parse_doc(
				        cleanup_doc(_context->comment, _context->comment_line_number, _context->comment_column_number)),
				});
				instance->flags = retrieve_modifiers();
				_context->definition = instance;
			}
			set_state(State::expect_signature_begin);
			break;

		default:
			break;
		}
		break;

	case mint::Token::dbl_pipe_token:
		switch (get_state()) {
		case State::expect_function:
			if (auto instance = _context->dictionary.get().get_or_create_function(definition_name(token))) {
				_context->signature = std::make_shared<Function::Signature>(Function::Signature {
				    .format = "def",
				    .doc = parse_doc(
				        cleanup_doc(_context->comment, _context->comment_line_number, _context->comment_column_number)),
				});
				instance->flags = retrieve_modifiers();
				_context->definition = instance;
			}
			set_state(State::expect_signature_begin);
			break;

		default:
			break;
		}
		break;

	case mint::Token::dbl_amp_token:
		switch (get_state()) {
		case State::expect_function:
			if (auto instance = _context->dictionary.get().get_or_create_function(definition_name(token))) {
				_context->signature = std::make_shared<Function::Signature>(Function::Signature {
				    .format = "def",
				    .doc = parse_doc(
				        cleanup_doc(_context->comment, _context->comment_line_number, _context->comment_column_number)),
				});
				instance->flags = retrieve_modifiers();
				_context->definition = instance;
			}
			set_state(State::expect_signature_begin);
			break;

		default:
			break;
		}
		break;

	case mint::Token::pipe_token:
		switch (get_state()) {
		case State::expect_function:
			if (auto instance = _context->dictionary.get().get_or_create_function(definition_name(token))) {
				_context->signature = std::make_shared<Function::Signature>(Function::Signature {
				    .format = "def",
				    .doc = parse_doc(
				        cleanup_doc(_context->comment, _context->comment_line_number, _context->comment_column_number)),
				});
				instance->flags = retrieve_modifiers();
				_context->definition = instance;
			}
			set_state(State::expect_signature_begin);
			break;

		default:
			break;
		}
		break;

	case mint::Token::caret_token:
		switch (get_state()) {
		case State::expect_function:
			if (auto instance = _context->dictionary.get().get_or_create_function(definition_name(token))) {
				_context->signature = std::make_shared<Function::Signature>(Function::Signature {
				    .format = "def",
				    .doc = parse_doc(
				        cleanup_doc(_context->comment, _context->comment_line_number, _context->comment_column_number)),
				});
				instance->flags = retrieve_modifiers();
				_context->definition = instance;
			}
			set_state(State::expect_signature_begin);
			break;

		default:
			break;
		}
		break;

	case mint::Token::amp_token:
		switch (get_state()) {
		case State::expect_function:
			if (auto instance = _context->dictionary.get().get_or_create_function(definition_name(token))) {
				_context->signature = std::make_shared<Function::Signature>(Function::Signature {
				    .format = "def",
				    .doc = parse_doc(
				        cleanup_doc(_context->comment, _context->comment_line_number, _context->comment_column_number)),
				});
				instance->flags = retrieve_modifiers();
				_context->definition = instance;
			}
			set_state(State::expect_signature_begin);
			break;

		default:
			break;
		}
		break;

	case mint::Token::dbl_equal_token:
		switch (get_state()) {
		case State::expect_function:
			if (auto instance = _context->dictionary.get().get_or_create_function(definition_name(token))) {
				_context->signature = std::make_shared<Function::Signature>(Function::Signature {
				    .format = "def",
				    .doc = parse_doc(
				        cleanup_doc(_context->comment, _context->comment_line_number, _context->comment_column_number)),
				});
				instance->flags = retrieve_modifiers();
				_context->definition = instance;
			}
			set_state(State::expect_signature_begin);
			break;

		default:
			break;
		}
		break;

	case mint::Token::exclamation_equal_token:
		switch (get_state()) {
		case State::expect_function:
			if (auto instance = _context->dictionary.get().get_or_create_function(definition_name(token))) {
				_context->signature = std::make_shared<Function::Signature>(Function::Signature {
				    .format = "def",
				    .doc = parse_doc(
				        cleanup_doc(_context->comment, _context->comment_line_number, _context->comment_column_number)),
				});
				instance->flags = retrieve_modifiers();
				_context->definition = instance;
			}
			set_state(State::expect_signature_begin);
			break;

		default:
			break;
		}
		break;

	case mint::Token::left_angled_token:
		switch (get_state()) {
		case State::expect_function:
			if (auto instance = _context->dictionary.get().get_or_create_function(definition_name(token))) {
				_context->signature = std::make_shared<Function::Signature>(Function::Signature {
				    .format = "def",
				    .doc = parse_doc(
				        cleanup_doc(_context->comment, _context->comment_line_number, _context->comment_column_number)),
				});
				instance->flags = retrieve_modifiers();
				_context->definition = instance;
			}
			set_state(State::expect_signature_begin);
			break;

		default:
			break;
		}
		break;

	case mint::Token::right_angled_token:
		switch (get_state()) {
		case State::expect_function:
			if (auto instance = _context->dictionary.get().get_or_create_function(definition_name(token))) {
				_context->signature = std::make_shared<Function::Signature>(Function::Signature {
				    .format = "def",
				    .doc = parse_doc(
				        cleanup_doc(_context->comment, _context->comment_line_number, _context->comment_column_number)),
				});
				instance->flags = retrieve_modifiers();
				_context->definition = instance;
			}
			set_state(State::expect_signature_begin);
			break;

		default:
			break;
		}
		break;

	case mint::Token::left_angled_equal_token:
		switch (get_state()) {
		case State::expect_function:
			if (auto instance = _context->dictionary.get().get_or_create_function(definition_name(token))) {
				_context->signature = std::make_shared<Function::Signature>(Function::Signature {
				    .format = "def",
				    .doc = parse_doc(
				        cleanup_doc(_context->comment, _context->comment_line_number, _context->comment_column_number)),
				});
				instance->flags = retrieve_modifiers();
				_context->definition = instance;
			}
			set_state(State::expect_signature_begin);
			break;

		default:
			break;
		}
		break;

	case mint::Token::right_angled_equal_token:
		switch (get_state()) {
		case State::expect_function:
			if (auto instance = _context->dictionary.get().get_or_create_function(definition_name(token))) {
				_context->signature = std::make_shared<Function::Signature>(Function::Signature {
				    .format = "def",
				    .doc = parse_doc(
				        cleanup_doc(_context->comment, _context->comment_line_number, _context->comment_column_number)),
				});
				instance->flags = retrieve_modifiers();
				_context->definition = instance;
			}
			set_state(State::expect_signature_begin);
			break;

		default:
			break;
		}
		break;

	case mint::Token::dbl_left_angled_token:
		switch (get_state()) {
		case State::expect_function:
			if (auto instance = _context->dictionary.get().get_or_create_function(definition_name(token))) {
				_context->signature = std::make_shared<Function::Signature>(Function::Signature {
				    .format = "def",
				    .doc = parse_doc(
				        cleanup_doc(_context->comment, _context->comment_line_number, _context->comment_column_number)),
				});
				instance->flags = retrieve_modifiers();
				_context->definition = instance;
			}
			set_state(State::expect_signature_begin);
			break;

		default:
			break;
		}
		break;

	case mint::Token::dbl_right_angled_token:
		switch (get_state()) {
		case State::expect_function:
			if (auto instance = _context->dictionary.get().get_or_create_function(definition_name(token))) {
				_context->signature = std::make_shared<Function::Signature>(Function::Signature {
				    .format = "def",
				    .doc = parse_doc(
				        cleanup_doc(_context->comment, _context->comment_line_number, _context->comment_column_number)),
				});
				instance->flags = retrieve_modifiers();
				_context->definition = instance;
			}
			set_state(State::expect_signature_begin);
			break;

		default:
			break;
		}
		break;

	case mint::Token::plus_token:
		switch (get_state()) {
		case State::expect_function:
			if (auto instance = _context->dictionary.get().get_or_create_function(definition_name(token))) {
				_context->signature = std::make_shared<Function::Signature>(Function::Signature {
				    .format = "def",
				    .doc = parse_doc(
				        cleanup_doc(_context->comment, _context->comment_line_number, _context->comment_column_number)),
				});
				instance->flags = retrieve_modifiers();
				_context->definition = instance;
			}
			set_state(State::expect_signature_begin);
			break;

		default:
			break;
		}
		break;

	case mint::Token::minus_token:
		switch (get_state()) {
		case State::expect_function:
			if (auto instance = _context->dictionary.get().get_or_create_function(definition_name(token))) {
				_context->signature = std::make_shared<Function::Signature>(Function::Signature {
				    .format = "def",
				    .doc = parse_doc(
				        cleanup_doc(_context->comment, _context->comment_line_number, _context->comment_column_number)),
				});
				instance->flags = retrieve_modifiers();
				_context->definition = instance;
			}
			set_state(State::expect_signature_begin);
			break;

		default:
			add_modifiers(mint::Reference::private_visibility);
			break;
		}
		break;

	case mint::Token::asterisk_token:
		switch (get_state()) {
		case State::expect_function:
			if (auto instance = _context->dictionary.get().get_or_create_function(definition_name(token))) {
				_context->signature = std::make_shared<Function::Signature>(Function::Signature {
				    .format = "def",
				    .doc = parse_doc(
				        cleanup_doc(_context->comment, _context->comment_line_number, _context->comment_column_number)),
				});
				instance->flags = retrieve_modifiers();
				_context->definition = instance;
			}
			set_state(State::expect_signature_begin);
			break;

		default:
			break;
		}
		break;

	case mint::Token::slash_token:
		switch (get_state()) {
		case State::expect_function:
			if (auto instance = _context->dictionary.get().get_or_create_function(definition_name(token))) {
				_context->signature = std::make_shared<Function::Signature>(Function::Signature {
				    .format = "def",
				    .doc = parse_doc(
				        cleanup_doc(_context->comment, _context->comment_line_number, _context->comment_column_number)),
				});
				instance->flags = retrieve_modifiers();
				_context->definition = instance;
			}
			set_state(State::expect_signature_begin);
			break;

		default:
			break;
		}
		break;

	case mint::Token::percent_token:
		switch (get_state()) {
		case State::expect_function:
			if (auto instance = _context->dictionary.get().get_or_create_function(definition_name(token))) {
				_context->signature = std::make_shared<Function::Signature>(Function::Signature {
				    .format = "def",
				    .doc = parse_doc(
				        cleanup_doc(_context->comment, _context->comment_line_number, _context->comment_column_number)),
				});
				instance->flags = retrieve_modifiers();
				_context->definition = instance;
			}
			set_state(State::expect_signature_begin);
			break;

		default:
			add_modifiers(mint::Reference::const_value);
			break;
		}
		break;

	case mint::Token::exclamation_token:
		switch (get_state()) {
		case State::expect_function:
			if (auto instance = _context->dictionary.get().get_or_create_function(definition_name(token))) {
				_context->signature = std::make_shared<Function::Signature>(Function::Signature {
				    .format = "def",
				    .doc = parse_doc(
				        cleanup_doc(_context->comment, _context->comment_line_number, _context->comment_column_number)),
				});
				instance->flags = retrieve_modifiers();
				_context->definition = instance;
			}
			set_state(State::expect_signature_begin);
			break;

		default:
			break;
		}
		break;

	case mint::Token::tilde_token:
		switch (get_state()) {
		case State::expect_function:
			if (auto instance = _context->dictionary.get().get_or_create_function(definition_name(token))) {
				_context->signature = std::make_shared<Function::Signature>(Function::Signature {
				    .format = "def",
				    .doc = parse_doc(
				        cleanup_doc(_context->comment, _context->comment_line_number, _context->comment_column_number)),
				});
				instance->flags = retrieve_modifiers();
				_context->definition = instance;
			}
			set_state(State::expect_signature_begin);
			break;

		default:
			add_modifiers(mint::Reference::package_visibility);
			break;
		}
		break;

	case mint::Token::dbl_plus_token:
		switch (get_state()) {
		case State::expect_function:
			if (auto instance = _context->dictionary.get().get_or_create_function(definition_name(token))) {
				_context->signature = std::make_shared<Function::Signature>(Function::Signature {
				    .format = "def",
				    .doc = parse_doc(
				        cleanup_doc(_context->comment, _context->comment_line_number, _context->comment_column_number)),
				});
				instance->flags = retrieve_modifiers();
				_context->definition = instance;
			}
			set_state(State::expect_signature_begin);
			break;

		default:
			break;
		}
		break;

	case mint::Token::dbl_minus_token:
		switch (get_state()) {
		case State::expect_function:
			if (auto instance = _context->dictionary.get().get_or_create_function(definition_name(token))) {
				_context->signature = std::make_shared<Function::Signature>(Function::Signature {
				    .format = "def",
				    .doc = parse_doc(
				        cleanup_doc(_context->comment, _context->comment_line_number, _context->comment_column_number)),
				});
				instance->flags = retrieve_modifiers();
				_context->definition = instance;
			}
			set_state(State::expect_signature_begin);
			break;

		default:
			break;
		}
		break;

	case mint::Token::dbl_asterisk_token:
		switch (get_state()) {
		case State::expect_function:
			if (auto instance = _context->dictionary.get().get_or_create_function(definition_name(token))) {
				_context->signature = std::make_shared<Function::Signature>(Function::Signature {
				    .format = "def",
				    .doc = parse_doc(
				        cleanup_doc(_context->comment, _context->comment_line_number, _context->comment_column_number)),
				});
				instance->flags = retrieve_modifiers();
				_context->definition = instance;
			}
			set_state(State::expect_signature_begin);
			break;

		default:
			break;
		}
		break;
	case mint::Token::dbl_dot_token:
		switch (get_state()) {
		case State::expect_function:
			if (auto instance = _context->dictionary.get().get_or_create_function(definition_name(token))) {
				_context->signature = std::make_shared<Function::Signature>(Function::Signature {
				    .format = "def",
				    .doc = parse_doc(
				        cleanup_doc(_context->comment, _context->comment_line_number, _context->comment_column_number)),
				});
				instance->flags = retrieve_modifiers();
				_context->definition = instance;
			}
			set_state(State::expect_signature_begin);
			break;

		default:
			break;
		}
		break;
	case mint::Token::tpl_dot_token:
		switch (get_state()) {
		case State::expect_function:
			if (auto instance = _context->dictionary.get().get_or_create_function(definition_name(token))) {
				_context->signature = std::make_shared<Function::Signature>(Function::Signature {
				    .format = "def",
				    .doc = parse_doc(
				        cleanup_doc(_context->comment, _context->comment_line_number, _context->comment_column_number)),
				});
				instance->flags = retrieve_modifiers();
				_context->definition = instance;
			}
			set_state(State::expect_signature_begin);
			break;

		default:
			break;
		}
		break;
	case mint::Token::sharp_token:
		add_modifiers(mint::Reference::protected_visibility);
		break;

	case mint::Token::at_token:
		add_modifiers(mint::Reference::global);
		break;

	case mint::Token::dollar_token:
		add_modifiers(mint::Reference::const_address);
		break;

	case mint::Token::const_token:
		add_modifiers(mint::Reference::const_address | mint::Reference::const_value);
		break;

	case mint::Token::final_token:
		add_modifiers(mint::Reference::final_member);
		break;

	case mint::Token::override_token:
		add_modifiers(mint::Reference::override_member);
		break;

	case mint::Token::assert_token:
	case mint::Token::break_token:
	case mint::Token::case_token:
	case mint::Token::catch_token:
	case mint::Token::continue_token:
	case mint::Token::default_token:
	case mint::Token::elif_token:
	case mint::Token::else_token:
	case mint::Token::exit_token:
	case mint::Token::for_token:
	case mint::Token::if_token:
	case mint::Token::lib_token:
	case mint::Token::print_token:
	case mint::Token::raise_token:
	case mint::Token::return_token:
	case mint::Token::switch_token:
	case mint::Token::try_token:
	case mint::Token::while_token:
	case mint::Token::yield_token:
	case mint::Token::is_token:
	case mint::Token::typeof_token:
	case mint::Token::membersof_token:
	case mint::Token::defined_token:
		start_modifiers(mint::Reference::default_flags);
		break;

	case mint::Token::comment_token:
		_context->comment = token;
		if (offset == 0) {
			_context->dictionary.get().set_module_doc(
			    cleanup_doc(_context->comment, _context->comment_line_number, _context->comment_column_number));
		}
		break;

	default:
		start_modifiers(mint::Reference::default_flags);
		break;
	}
	return true;
}

bool Parser::on_new_line(std::size_t line_number, std::string::size_type offset) {
	_context->line_number = line_number;
	_context->line_offset = offset;
	return true;
}

bool Parser::on_comment_begin(std::string::size_type offset) {
	_context->comment_line_number = _context->line_number;
	_context->comment_column_number = offset - _context->line_offset;
	return true;
}

void Parser::parse_error(const std::string& message, std::size_t column, std::size_t begin_line, std::size_t end_line) {

	static constexpr const char* tab_placeholder = "\033[1;30m\xC2\xBB\t\033[0m";
	static constexpr const char* space_placeholder = "\033[1;30m\xC2\xB7\033[0m";

	std::string message_line;
	std::ifstream stream(_path);
	std::string line_content = "\033[0m";
	std::string message_pos = "\033[1;30m";

	for (std::size_t i = 1; i <= end_line; ++i) {
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

	for (std::size_t i = 0; i < line_content.size(); ++i) {
		if (i < column - 1) {
			switch (const auto c = line_content[i]) {
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

					std::size_t size = 2;

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

	mint::error("{}:{}: {}\n{}\n{}\n", _path.generic_string(), _context->line_number, message, message_line,
	    message_pos);
}

Parser::State Parser::get_state() const {
	return _context->state;
}

void Parser::set_state(State state) {
	_context->state = state;
}

void Parser::push_state(State state) {
	_context->states.push_back(_context->state);
	_context->state = state;
}

void Parser::pop_state() {
	if (_context->states.empty()) {
		_context->state = State::expect_start;
	}
	else {
		_context->state = _context->states.back();
		_context->states.pop_back();
	}
}

Parser::ScriptContext* Parser::current_context() const {
	return _context->context.get();
}

std::string Parser::definition_name(const std::string& token) const {

	std::string name;

	for (const auto& scope : _context->contexts) {
		name += scope->name + ".";
	}

	if (_context->context) {
		name += _context->context->name + ".";
	}

	return name + token;
}

void Parser::push_context(const std::string& name, const std::shared_ptr<Definition>& definition) {

	if (_context->context) {
		_context->contexts.emplace_back(std::move(_context->context));
	}

	_context->context = std::make_unique<ScriptContext>(ScriptContext {
	    .name = name,
	    .definition = definition,
	    .depth = 0,
	});
}

void Parser::bind_definition_to_context(Definition& definition) {

	/*for (Context* context : _contexts) {
		bind_context->definition_to_context(context, definition);
	}*/

	if (_context->context) {
		if (&definition == _context->context->definition.get()) {
			if (!_context->contexts.empty()) {
				bind_definition_to_context(*_context->contexts.back(), definition);
			}
		}
		else {
			bind_definition_to_context(*_context->context, definition);
		}
	}
}

void Parser::bind_definition_to_context(ScriptContext& context, Definition& definition) {

	switch (context.definition->type) {
	case Definition::package_definition:
		if (auto instance = std::static_pointer_cast<Package>(context.definition)) {
			instance->members.insert(definition.name);
		}
		break;

	case Definition::enum_definition:
		if (auto instance = std::static_pointer_cast<Enum>(context.definition)) {
			instance->members.insert(definition.name);
		}
		break;

	case Definition::class_definition:
		if (auto instance = std::static_pointer_cast<Class>(context.definition)) {
			instance->members.insert(definition.name);
		}
		break;

	default:
		break;
	}
}

void Parser::open_block() {
	if (_context->context) {
		_context->context->depth++;
	}
}

void Parser::close_block() {
	if (_context->context && !--_context->context->depth) {
		if (_context->contexts.empty()) {
			_context->context.reset();
		}
		else {
			_context->context = std::move(_context->contexts.back());
			_context->contexts.pop_back();
		}
	}
}

void Parser::start_modifiers(mint::Reference::Flags flags) {
	_context->modifiers = flags;
}

void Parser::add_modifiers(mint::Reference::Flags flags) {
	_context->modifiers |= flags;
}

mint::Reference::Flags Parser::retrieve_modifiers() {
	const auto flags = _context->modifiers;
	_context->modifiers = mint::Reference::default_flags;
	return flags;
}

std::string Parser::cleanup_doc(const std::string& comment, std::size_t line, std::size_t column) {

	if (comment.starts_with("/**")) {
		std::stringstream stream(comment);
		stream.seekg(3, std::stringstream::beg);
		return cleanup_multi_line_doc(stream, line, column);
	}

	if (comment.starts_with("///")) {
		std::stringstream stream(comment);
		stream.seekg(3, std::stringstream::beg);
		return cleanup_single_line_doc(stream, line, column);
	}

	return {};
}

std::string Parser::cleanup_single_line_doc(std::stringstream& stream, std::size_t line, std::size_t column) {

	bool finished = false;

	std::string documentation;
	std::size_t current_line = line;

	if (stream.eof() || stream.get() != ' ') {
		parse_error("expected ' ' character before documentation string", column);
	}

	while (!finished && !stream.eof()) {
		switch (const auto c = stream.get()) {
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

std::string Parser::cleanup_multi_line_doc(std::stringstream& stream, std::size_t line, std::size_t column) {

	bool finished = false;
	bool suspect_end = false;

	std::string documentation;
	std::size_t current_line = line;

	while (!finished && !stream.eof()) {
		switch (const auto c = stream.get()) {
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

void Parser::cleanup_script(std::stringstream& stream, std::string& documentation, std::size_t line, std::size_t column,
    std::size_t& current_line) {

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
