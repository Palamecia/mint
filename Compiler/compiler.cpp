#include "compiler.h"
#include "AbstractSyntaxTree/modul.h"
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

Data *Compiler::makeData(const std::string &token) {

	/// \todo check data type
	if (isdigit(token.front())) {
		Number *number = Reference::alloc<Number>();
		number->data = atof(token.c_str());
		return number;
	}

	if (token.front() == '\'' || token.front() == '"') {
		String *string = Reference::alloc<String>();
		string->str = tokenToString(token);
		return string;
	}

	if (token == "null") {
		return Reference::alloc<Null>();
	}

	if (token == "none") {
		return Reference::alloc<None>();
	}
}
