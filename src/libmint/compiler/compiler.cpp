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

#include "mint/compiler/compiler.h"
#include "mint/memory/builtin/library.h"
#include "mint/memory/builtin/string.h"
#include "mint/memory/builtin/regex.h"
#include "mint/memory/builtin/array.h"
#include "mint/memory/builtin/hash.h"
#include "mint/memory/cast_tools.h"
#include "mint/memory/data.h"
#include "mint/memory/garbage_collector.h"
#include "mint/memory/object.h"
#include "mint/system/plugin.h"
#include "mint/system/string.h"
#include "mint/system/error.h"
#include <cctype>
#include <cstddef>
#include <exception>
#include <regex>
#include <stdexcept>
#include <string>

using namespace mint;

namespace {

double token_to_number(const std::string& token) {
	return to_unsigned_number(token);
}

std::string token_to_string(const std::string& token) {

	std::string str;
	bool shift = false;

	for (std::size_t i = 1; i < token.size() - 1; ++i) {

		const char cptr = token[i];

		if (shift) {
			switch (cptr) {
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
				if (isdigit(token[++i])) {
					int code = 0;
					while (isdigit(token[i])) {
						code = (code * hexadecimal_base) + (token[i++] - '0');
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
				if (cptr) {
					if (isdigit(cptr)) {
						int code = 0;
						while (isdigit(token[i])) {
							code = (code * decimal_base) + (token[i++] - '0');
						}
						str += static_cast<char>(code);
					}
					else {
						str += '\\';
						str += cptr;
					}
				}
				else {
					throw std::invalid_argument(__func__);
				}
			}

			shift = false;
		}
		else if (cptr == '\\') {
			shift = true;
		}
		else {
			str += cptr;
		}
	}

	return str;
}

std::regex token_to_regex(const std::string& token) {

	std::string str;
	std::regex::flag_type flag = std::regex::ECMAScript;
	auto pos = token.find_last_of('/');
	const auto indicators = token.substr(pos + 1, token.size());

	str = token.substr(1, pos - 1);

	for (auto indicator : indicators) {
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

Compiler::Compiler(AbstractSyntaxTree& ast) :
    _ast(ast) {}

bool Compiler::is_printing() const {
	return _printing;
}

void Compiler::set_printing(bool enabled) {
	_printing = enabled;
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
			return GarbageCollector::instance().alloc<Number>(token_to_number(token));
		}
		catch (std::exception&) {
			return nullptr;
		}
	case DataHint::data_string_hint:
		try {
			auto* string = GarbageCollector::instance().alloc<String>(_ast, token_to_string(token));
			string->construct();
			return string;
		}
		catch (std::exception&) {
			return nullptr;
		}
	case DataHint::data_regex_hint:
		try {
			auto* regex = GarbageCollector::instance().alloc<Regex>(_ast);
			regex->expr = token_to_regex(token);
			regex->initializer = token;
			regex->construct();
			return regex;
		}
		catch (std::exception&) {
			return nullptr;
		}
	case DataHint::data_true_hint:
		return GarbageCollector::instance().alloc<Boolean>(true);
	case DataHint::data_false_hint:
		return GarbageCollector::instance().alloc<Boolean>(false);
	case DataHint::data_null_hint:
		return GarbageCollector::instance().alloc<Null>();
	case DataHint::data_none_hint:
		return GarbageCollector::instance().alloc<None>();
	}

	return nullptr;
}

Data& Compiler::make_library(const std::string& token) {
	try {
		auto* library = GarbageCollector::instance().alloc<Library>(_ast);
		library->plugin = Plugin::load(token_to_string(token));
		library->construct();
		return *library;
	}
	catch (const std::exception& error) {
		mint::error("failed to load plugin {}: {}", token, error.what());
	}
}

Data& Compiler::make_package(PackageData& package) {
	return *GarbageCollector::instance().alloc<Package>(package);
}

Data& Compiler::make_number(double value) {
	return *GarbageCollector::instance().alloc<Number>(value);
}

Data& Compiler::make_boolean(bool value) {
	return *GarbageCollector::instance().alloc<Boolean>(value);
}

Data& Compiler::make_array() {
	auto* array = GarbageCollector::instance().alloc<Array>(_ast);
	array->construct();
	return *array;
}

Data& Compiler::make_hash() {
	auto* hash = GarbageCollector::instance().alloc<Hash>(_ast);
	hash->construct();
	return *hash;
}

Data& Compiler::make_none() {
	return *GarbageCollector::instance().alloc<None>();
}

AbstractSyntaxTree& Compiler::ast() {
	return _ast;
}
