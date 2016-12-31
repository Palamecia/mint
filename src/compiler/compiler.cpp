#include "compiler/compiler.h"
#include "memory/builtin/library.h"
#include "memory/builtin/string.h"
#include "ast/module.h"

using namespace std;

string tokenToString(const string &token) {

	string str;
	bool shift = false;

	for (size_t i = 1; i < token.size() - 1; ++i) {

		char cptr = token[i];

		if (shift) {
			switch (cptr) {
			case 'n':
				str += '\n';
				break;
			case 'r':
				str += '\r';
				break;
			case 't':
				str += '\t';
				break;
			case '0':
				str += '\0';
				break;
			case '\\':
				str += '\\';
				break;
			default:
				str += cptr;
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

BuildContext *Compiler::g_ctx = nullptr;

Compiler::Compiler() {}

BuildContext *Compiler::context() {
	return g_ctx;
}

Data *Compiler::makeData(const std::string &token) {

	if (isdigit(token.front())) {
		Number *number = Reference::alloc<Number>();
		const char *value = token.c_str();
		char *error = nullptr;

		if (value[0] == '0') {
			switch (value[1]) {
			case 'b':
			case 'B':
				number->value = strtol(value + 2, &error, 2);
				if (*error) {
					return nullptr;
				}
				return number;

			case 'o':
			case 'O':
				number->value = strtol(value + 2, &error, 8);
				if (*error) {
					return nullptr;
				}
				return number;

			case 'x':
			case 'X':
				number->value = strtol(value + 2, &error, 16);
				if (*error) {
					return nullptr;
				}
				return number;

			default:
				break;
			}
		}

		number->value = strtod(value, &error);
		if (*error) {
			return nullptr;
		}
		return number;
	}

	if (token.front() == '\'' || token.front() == '"') {
		String *string = Reference::alloc<String>();
		string->construct();
		string->str = tokenToString(token);
		return string;
	}

	if (token == "true") {
		Boolean *boolean = Reference::alloc<Boolean>();
		boolean->value = true;
		return boolean;
	}

	if (token == "false") {
		Boolean *boolean = Reference::alloc<Boolean>();
		boolean->value = false;
		return boolean;
	}

	if (token == "null") {
		return Reference::alloc<Null>();
	}

	if (token == "none") {
		return Reference::alloc<None>();
	}

	return nullptr;
}
