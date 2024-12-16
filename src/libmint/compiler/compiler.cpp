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

#include "mint/compiler/compiler.h"
#include "mint/memory/builtin/library.h"
#include "mint/memory/builtin/string.h"
#include "mint/memory/builtin/regex.h"
#include "mint/memory/builtin/array.h"
#include "mint/memory/builtin/hash.h"
#include "mint/memory/casttool.h"
#include "mint/system/plugin.h"

using namespace mint;

static double token_to_number(const std::string &token, bool *error) {
	return to_unsigned_number(token, error);
}

static std::string token_to_string(const std::string &token, bool *error) {

	std::string str;
	bool shift = false;

	for (size_t i = 1; i < token.size() - 1; ++i) {

		char cptr = token[i];

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
						code = (code * 16) + (token[i++] - '0');
					}
					str += static_cast<char>(code);
				}
				else {
					if (error) {
						*error = true;
					}
					return str;
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
							code = (code * 10) + (token[i++] - '0');
						}
						str += static_cast<char>(code);
					}
					else {
						str += '\\';
						str += cptr;
					}
				}
				else {
					if (error) {
						*error = true;
					}
					return str;
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

	if (error) {
		*error = false;
	}
	return str;
}

static std::regex token_to_regex(const std::string &token, bool *error) {

	std::string str;
	std::regex::flag_type flag = std::regex::ECMAScript;
	auto pos = token.find_last_of('/');
	std::string indicators = token.substr(pos + 1, token.size());

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
			if (error) {
				*error = true;
			}
			return std::regex();
		}
	}

	if (error) {
		*error = false;
	}

	try {
		return std::regex(str, flag);
	}
	catch (const std::regex_error &) {
		if (error) {
			*error = true;
		}
	}

	return std::regex();
}

Compiler::Compiler() :
	m_printing(false) {

}

bool Compiler::is_printing() const {
	return m_printing;
}

void Compiler::set_printing(bool enabled) {
	m_printing = enabled;
}

static Compiler::DataHint data_hint_from_token(const std::string &token) {

	if (isdigit(token.front())) {
		return Compiler::data_number_hint;
	}

	if (token.front() == '\'' || token.front() == '"') {
		return Compiler::data_string_hint;
	}

	if (token.front() == '/') {
		return Compiler::data_regex_hint;
	}

	if (token == "true") {
		return Compiler::data_true_hint;
	}

	if (token == "false") {
		return Compiler::data_false_hint;
	}

	if (token == "null") {
		return Compiler::data_null_hint;
	}

	if (token == "none") {
		return Compiler::data_none_hint;
	}

	return Compiler::data_unknown_hint;
}

Data *Compiler::make_data(const std::string &token, DataHint hint) {

	if (hint == data_unknown_hint) {
		hint = data_hint_from_token(token);
	}

	switch (hint) {
	case data_unknown_hint:
		break;
	case data_number_hint:
	{
		bool error = false;
		Number *number = GarbageCollector::instance().alloc<Number>(token_to_number(token, &error));
		if (error) {
			return nullptr;
		}
		return number;
	}
	case data_string_hint:
	{
		bool error = false;
		String *string = GarbageCollector::instance().alloc<String>(token_to_string(token, &error));
		string->construct();
		if (error) {
			return nullptr;
		}
		return string;
	}
	case data_regex_hint:
	{
		bool error = false;
		Regex *regex = GarbageCollector::instance().alloc<Regex>();
		regex->expr = token_to_regex(token, &error);
		regex->initializer = token;
		regex->construct();
		if (error) {
			return nullptr;
		}
		return regex;
	}
	case data_true_hint:
		return GarbageCollector::instance().alloc<Boolean>(true);
	case data_false_hint:
		return GarbageCollector::instance().alloc<Boolean>(false);
	case data_null_hint:
		return GarbageCollector::instance().alloc<Null>();
	case data_none_hint:
		return GarbageCollector::instance().alloc<None>();
	}

	return nullptr;
}

Data *Compiler::make_library(const std::string &token) {

	Library *library = GarbageCollector::instance().alloc<Library>();
	bool error = false;

	std::string plugin = token_to_string(token, &error);
	if (error) {
		return nullptr;
	}

	library->construct();
	if ((library->plugin = Plugin::load(plugin))) {
		return library;
	}

	return nullptr;
}

Data *Compiler::make_array() {

	Array *array = GarbageCollector::instance().alloc<Array>();
	array->construct();
	return array;
}

Data *Compiler::make_hash() {

	Hash *hash = GarbageCollector::instance().alloc<Hash>();
	hash->construct();
	return hash;
}

Data *Compiler::make_none() {
	return GarbageCollector::instance().alloc<None>();
}
