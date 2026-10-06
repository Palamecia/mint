/**
 * Copyright (c) 2026 Gauvain CHÉRY.
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

#include "mint/compiler/compiler.h"
#include "mint/compiler/descriptions.h"
#include "mint/memory/builtin/library.h"
#include "mint/memory/builtin/string.h"
#include "mint/memory/builtin/regex.h"
#include "mint/memory/builtin/array.h"
#include "mint/memory/builtin/hash.h"
#include "mint/memory/cast_tools.h"
#include "mint/memory/class.h"
#include "mint/memory/data.h"
#include "mint/memory/garbage_collector.h"
#include "mint/memory/object.h"
#include "mint/memory/reference.h"
#include "mint/program/module.h"
#include "mint/program/node.h"
#include "mint/program/program.h"
#include "mint/system/plugin.h"
#include "mint/system/string.h"
#include "mint/system/error.h"
#include <cctype>
#include <exception>
#include <functional>
#include <iterator>
#include <memory>
#include <regex>
#include <stdexcept>
#include <string>
#include <vector>

using namespace mint;

namespace {

double token_to_number(const std::string& token) {
	return to_unsigned_number(token);
}

std::string token_to_string(const std::string& token) {

	auto str = std::string();
	bool shift = false;

	for (auto it = std::next(token.begin()); it != std::prev(token.end()); ++it) {

		if (shift) {
			switch (*it) {
			case '0':
				str += '\0';
				break;
			case 'a':
				str += '\a';
				break;
			case 'b':
				str += '\b';
				break;
			case 't':
				str += '\t';
				break;
			case 'n':
				str += '\n';
				break;
			case 'v':
				str += '\v';
				break;
			case 'f':
				str += '\f';
				break;
			case 'r':
				str += '\r';
				break;
			case 'e':
				str += '\x1B';
				break;
			case 'x':
				if (isdigit(*++it)) {
					int code = 0;
					while (isdigit(*it)) {
						code = (code * hexadecimal_base) + (*it++ - '0');
					}
					str += static_cast<char>(code);
				}
				else {
					throw std::invalid_argument(__func__);
				}
				break;
			case '"':
				str += '"';
				break;
			case '\'':
				str += '\'';
				break;
			case '\\':
				str += '\\';
				break;
			default:
				if (*it) {
					if (isdigit(*it)) {
						int code = 0;
						while (isdigit(*it)) {
							code = (code * decimal_base) + (*it++ - '0');
						}
						str += static_cast<char>(code);
					}
					else {
						str += '\\';
						str += *it;
					}
				}
				else {
					throw std::invalid_argument(__func__);
				}
			}

			shift = false;
		}
		else if (*it == '\\') {
			shift = true;
		}
		else {
			str += *it;
		}
	}

	return str;
}

std::regex token_to_regex(const std::string& token) {

	std::string str;
	std::regex::flag_type flag = std::regex::ECMAScript;
	const auto pos = token.find_last_of('/');
	const auto indicators = token.substr(pos + 1, token.size());

	str = token.substr(1, pos - 1);

	for (const auto indicator : indicators) {
		switch (indicator) {
		case 'c':
			flag |= std::regex::collate;
			break;
		case 'i':
			flag |= std::regex::icase;
			break;
		default:
			throw std::invalid_argument(__func__);
		}
	}

	return std::regex(str, flag);
}

Compiler::DataHint data_hint_from_token(const std::string& token) {
	if (isdigit(token.front())) {
		return Compiler::DataHint::data_number_hint;
	}
	if (token.front() == '\'' || token.front() == '"') {
		return Compiler::DataHint::data_string_hint;
	}
	if (token.front() == '/') {
		return Compiler::DataHint::data_regex_hint;
	}
	if (token == "true") {
		return Compiler::DataHint::data_true_hint;
	}
	if (token == "false") {
		return Compiler::DataHint::data_false_hint;
	}
	if (token == "null") {
		return Compiler::DataHint::data_null_hint;
	}
	if (token == "none") {
		return Compiler::DataHint::data_none_hint;
	}
	return Compiler::DataHint::data_unknown_hint;
}

}

Compiler::Compiler(Program& program, ModuleInfo& data) :
    _program(program),
    _data(data) {}

bool Compiler::is_printing() const {
	return _printing;
}

void Compiler::set_printing(bool enabled) {
	_printing = enabled;
}

Reference* mint::Compiler::make_constant(const std::string& token, DataHint hint) {
	if (auto* data = make_data(token, hint)) {
		return _data.get().bytecode.make_constant(data);
	}
	return nullptr;
}

Data* Compiler::make_data(const std::string& token, DataHint hint) {

	if (hint == DataHint::data_unknown_hint) {
		hint = data_hint_from_token(token);
	}

	switch (hint) {
	case DataHint::data_unknown_hint:
		break;
	case DataHint::data_number_hint:
		try {
			return make_data<Number>(token_to_number(token));
		}
		catch (...) {
			mint::error("token '{}' is not a valid constant", token);
		}
	case DataHint::data_string_hint:
		try {
			auto* string = GarbageCollector::instance().alloc<String>(_program, token_to_string(token));
			string->construct();
			return string;
		}
		catch (...) {
			mint::error("token '{}' is not a valid constant", token);
		}
	case DataHint::data_regex_hint:
		try {
			auto* regex = GarbageCollector::instance().alloc<Regex>(_program);
			regex->expr = token_to_regex(token);
			regex->pattern = token;
			regex->construct();
			return regex;
		}
		catch (...) {
			mint::error("token '{}' is not a valid constant", token);
		}
	case DataHint::data_true_hint:
		return make_data<Boolean>(true);
	case DataHint::data_false_hint:
		return make_data<Boolean>(false);
	case DataHint::data_null_hint:
		return make_data<Null>();
	case DataHint::data_none_hint:
		return make_data<None>();
	}

	mint::error("token '{}' is not a valid constant", token);
}

template<>
Library* Compiler::make_data<Library>(const std::string& token) {
	try {
		auto* library = GarbageCollector::instance().alloc<Library>(_program);
		library->plugin = Plugin::load(token_to_string(token));
		library->construct();
		return library;
	}
	catch (const std::exception& error) {
		mint::error("failed to load plugin {}: {}", token, error.what());
	}
}

void mint::Compiler::push_node(const Node& node) {
	_data.get().bytecode.push_node(node);
}

void mint::Compiler::push_nodes(const std::vector<Node>& nodes) {
	_data.get().bytecode.push_nodes(nodes);
}

Program& mint::Compiler::program() {
	return _program;
}

ModuleInfo& mint::Compiler::data() {
	return _data;
}
