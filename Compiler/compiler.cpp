#include "compiler.h"
#include "AbstractSyntaxTree/module.h"
#include "Memory/object.h"

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

double atob(const char *str) {

	long result = 0;

	for (const char *cptr = str; *cptr; ++cptr) {

		switch (*cptr) {
		case '0':
		case '1':
			result *= 2;
			result += *cptr - '0';
			break;

		default:
			return result;
		}
	}

	return result;
}


double atoo(const char *str) {

	long result = 0;

	for (const char *cptr = str; *cptr; ++cptr) {

		switch (*cptr) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
			result *= 8;
			result += *cptr - '0';
			break;

		default:
			return result;
		}
	}

	return result;
}

double atox(const char *str) {

	long result = 0;

	for (const char *cptr = str; *cptr; ++cptr) {

		switch (*cptr) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			result *= 16;
			result += *cptr - '0';
			break;

		case 'a':
		case 'A':
			result *= 16;
			result += 10;
			break;

		case 'b':
		case 'B':
			result *= 16;
			result += 11;
			break;

		case 'c':
		case 'C':
			result *= 16;
			result += 12;
			break;

		case 'd':
		case 'D':
			result *= 16;
			result += 13;
			break;

		case 'e':
		case 'E':
			result *= 16;
			result += 14;
			break;

		case 'f':
		case 'F':
			result *= 16;
			result += 15;
			break;

		default:
			return result;
		}
	}

	return result;
}

Data *Compiler::makeData(const std::string &token) {

	if (isdigit(token.front())) {
		Number *number = Reference::alloc<Number>();
		const char *value = token.c_str();

		if (value[0] == '0') {
			switch (value[1]) {
			case 'b':
			case 'B':
				number->value = atob(value + 2);
				return number;

			case 'o':
			case 'O':
				number->value = atoo(value + 2);
				return number;

			case 'x':
			case 'X':
				number->value = atox(value + 2);
				return number;

			default:
				break;
			}
		}

		number->value = atof(value);
		return number;
	}

	if (token.front() == '\'' || token.front() == '"') {
		String *string = Reference::alloc<String>();
		string->construct();
		string->str = tokenToString(token);
		return string;
	}

	if (token == "true") {
		Number *number = Reference::alloc<Number>();
		number->value = 1;
		return number;
	}

	if (token == "false") {
		Number *number = Reference::alloc<Number>();
		number->value = 0;
		return number;
	}

	if (token == "null") {
		return Reference::alloc<Null>();
	}

	if (token == "none") {
		return Reference::alloc<None>();
	}

	return nullptr;
}
