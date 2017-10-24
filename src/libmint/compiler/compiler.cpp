#include "compiler/compiler.h"
#include "memory/builtin/library.h"
#include "memory/builtin/string.h"
#include "memory/builtin/array.h"
#include "memory/builtin/hash.h"
#include "system/plugin.h"
#include "ast/module.h"

using namespace std;

string tokenToString(const string &token, bool *error) {

	string str;
	bool shift = false;
	if (error) {
		*error = false;
	}

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
				str += '\e';
				break;
			case 'x':
				if (isdigit(token[++i])) {
					int code = 0;
					while (isdigit(token[i])) {
						code = (code * 16) + (token[i++] - '0');
					}
					str += code;
				}
				else {
					if (error) {
						*error = true;
					}
					return str;
				}
				break;
			case '\\':
				str += '\\';
				break;
			default:
				if (cptr) {
					int code = 0;
					while (isdigit(token[i])) {
						code = (code * 10) + (token[i++] - '0');
					}
					str += code;
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

	return str;
}

BuildContext *Compiler::g_ctx = nullptr;

Compiler::Compiler() {}

BuildContext *Compiler::context() {
	return g_ctx;
}

Data *Compiler::makeLibrary(const std::string &token) {

	Data *library = Reference::alloc<Library>();
	bool error = false;

	string plugin = tokenToString(token, &error);
	if (error) {
		return nullptr;
	}

	((Library *)library)->construct();
	if ((((Library *)library)->plugin = Plugin::load(plugin))) {
		return library;
	}

	return nullptr;
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
		bool error = false;

		string->construct();
		string->str = tokenToString(token, &error);

		if (error) {
			return nullptr;
		}

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

Data *Compiler::makeArray() {

	Data *array = Reference::alloc<Array>();
	((Array *)array)->construct();
	return array;
}

Data *Compiler::makeHash() {

	Data *hash = Reference::alloc<Hash>();
	((Hash *)hash)->construct();
	return hash;
}
