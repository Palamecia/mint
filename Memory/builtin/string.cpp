#include "Memory/class.h"
#include "Memory/object.h"
#include "Memory/casttool.h"
#include "Memory/memorytool.h"
#include "AbstractSyntaxTree/abstractsyntaxtree.h"
#include "System/utf8iterator.h"
#include "System/error.h"

#include <cstring>

using namespace std;

enum Flag {
	string_left = 0x01,
	string_plus = 0x02,
	string_space = 0x04,
	string_special = 0x08,
	string_zeropad = 0x10,
	string_large = 0x20,
	string_sign = 0x40
};

void string_format(AbstractSynatxTree *ast, string &dest, const string &format, const vector<unique_ptr<Reference>> &args);
template<typename numtype>
string string_integer(numtype number, int base, int size, int precision, int flags);
template<typename numtype>
string string_real(numtype number, int fmt, int size, int precision, int flags);
template<typename numtype>
string string_hex_real(numtype number, char qualifier, int size, int precision, int flags);

StringClass::StringClass() : Class("string") {

	createBuiltinMember(":=", 2, AbstractSynatxTree::createBuiltinMethode(STRING_TYPE, [] (AbstractSynatxTree *ast) {

							size_t base = get_base(ast);

							Reference &rvalue = ast->stack().at(base).get();
							Reference &lvalue = ast->stack().at(base - 1).get();

							((String *)lvalue.data())->str = to_string(rvalue);

							ast->stack().pop_back();
						}));

	createBuiltinMember("+", 2, AbstractSynatxTree::createBuiltinMethode(STRING_TYPE, [] (AbstractSynatxTree *ast) {

							size_t base = get_base(ast);

							Reference &rvalue = ast->stack().at(base).get();
							Reference &lvalue = ast->stack().at(base - 1).get();

							Reference *result = Reference::create<String>();
							((String *)result->data())->construct();
							((String *)result->data())->str = ((String *)lvalue.data())->str + to_string(rvalue);

							ast->stack().pop_back();
							ast->stack().pop_back();
							ast->stack().push_back(SharedReference::unique(result));
						}));

	createBuiltinMember("%", 2, AbstractSynatxTree::createBuiltinMethode(STRING_TYPE, [] (AbstractSynatxTree *ast) {

							size_t base = get_base(ast);

							Reference &rvalue = ast->stack().at(base).get();
							Reference &lvalue = ast->stack().at(base - 1).get();

							Reference *result = Reference::create<String>();
							((String *)result->data())->construct();
							string_format(ast, ((String *)result->data())->str, ((String *)lvalue.data())->str, to_array(rvalue));

							ast->stack().pop_back();
							ast->stack().pop_back();
							ast->stack().push_back(SharedReference::unique(result));
						}));

	createBuiltinMember("==", 2, AbstractSynatxTree::createBuiltinMethode(STRING_TYPE, [] (AbstractSynatxTree *ast) {

							size_t base = get_base(ast);

							Reference &rvalue = ast->stack().at(base).get();
							Reference &lvalue = ast->stack().at(base - 1).get();

							Reference *result = Reference::create<Number>();
							((Number *)result->data())->value = ((String *)lvalue.data())->str == to_string(rvalue);

							ast->stack().pop_back();
							ast->stack().pop_back();
							ast->stack().push_back(SharedReference::unique(result));
						}));

	createBuiltinMember("!=", 2, AbstractSynatxTree::createBuiltinMethode(STRING_TYPE, [] (AbstractSynatxTree *ast) {

							size_t base = get_base(ast);

							Reference &rvalue = ast->stack().at(base).get();
							Reference &lvalue = ast->stack().at(base - 1).get();

							Reference *result = Reference::create<Number>();
							((Number *)result->data())->value = ((String *)lvalue.data())->str != to_string(rvalue);

							ast->stack().pop_back();
							ast->stack().pop_back();
							ast->stack().push_back(SharedReference::unique(result));
						}));

	createBuiltinMember("<", 2, AbstractSynatxTree::createBuiltinMethode(STRING_TYPE, [] (AbstractSynatxTree *ast) {

							size_t base = get_base(ast);

							Reference &rvalue = ast->stack().at(base).get();
							Reference &lvalue = ast->stack().at(base - 1).get();

							Reference *result = Reference::create<Number>();
							((Number *)result->data())->value = ((String *)lvalue.data())->str < to_string(rvalue);

							ast->stack().pop_back();
							ast->stack().pop_back();
							ast->stack().push_back(SharedReference::unique(result));
						}));

	createBuiltinMember(">", 2, AbstractSynatxTree::createBuiltinMethode(STRING_TYPE, [] (AbstractSynatxTree *ast) {

							size_t base = get_base(ast);

							Reference &rvalue = ast->stack().at(base).get();
							Reference &lvalue = ast->stack().at(base - 1).get();

							Reference *result = Reference::create<Number>();
							((Number *)result->data())->value = ((String *)lvalue.data())->str > to_string(rvalue);

							ast->stack().pop_back();
							ast->stack().pop_back();
							ast->stack().push_back(SharedReference::unique(result));

						}));

	createBuiltinMember("<=", 2, AbstractSynatxTree::createBuiltinMethode(STRING_TYPE, [] (AbstractSynatxTree *ast) {

							size_t base = get_base(ast);

							Reference &rvalue = ast->stack().at(base).get();
							Reference &lvalue = ast->stack().at(base - 1).get();

							Reference *result = Reference::create<Number>();
							((Number *)result->data())->value = ((String *)lvalue.data())->str <= to_string(rvalue);

							ast->stack().pop_back();
							ast->stack().pop_back();
							ast->stack().push_back(SharedReference::unique(result));
						}));

	createBuiltinMember(">=", 2, AbstractSynatxTree::createBuiltinMethode(STRING_TYPE, [] (AbstractSynatxTree *ast) {

							size_t base = get_base(ast);

							Reference &rvalue = ast->stack().at(base).get();
							Reference &lvalue = ast->stack().at(base - 1).get();

							Reference *result = Reference::create<Number>();
							((Number *)result->data())->value = ((String *)lvalue.data())->str >= to_string(rvalue);

							ast->stack().pop_back();
							ast->stack().pop_back();
							ast->stack().push_back(SharedReference::unique(result));
						}));

	createBuiltinMember("&&", 2, AbstractSynatxTree::createBuiltinMethode(STRING_TYPE, [] (AbstractSynatxTree *ast) {

							size_t base = get_base(ast);

							Reference &rvalue = ast->stack().at(base).get();
							Reference &lvalue = ast->stack().at(base - 1).get();

							Reference *result = Reference::create<Number>();
							((Number *)result->data())->value = ((String *)lvalue.data())->str.size() && to_number(ast, rvalue);

							ast->stack().pop_back();
							ast->stack().pop_back();
							ast->stack().push_back(SharedReference::unique(result));
						}));

	createBuiltinMember("||", 2, AbstractSynatxTree::createBuiltinMethode(STRING_TYPE, [] (AbstractSynatxTree *ast) {

							size_t base = get_base(ast);

							Reference &rvalue = ast->stack().at(base).get();
							Reference &lvalue = ast->stack().at(base - 1).get();

							Reference *result = Reference::create<Number>();
							((Number *)result->data())->value = ((String *)lvalue.data())->str.size() || to_number(ast, rvalue);

							ast->stack().pop_back();
							ast->stack().pop_back();
							ast->stack().push_back(SharedReference::unique(result));
						}));

	createBuiltinMember("^", 2, AbstractSynatxTree::createBuiltinMethode(STRING_TYPE, [] (AbstractSynatxTree *ast) {

							size_t base = get_base(ast);

							Reference &rvalue = ast->stack().at(base).get();
							Reference &lvalue = ast->stack().at(base - 1).get();

							Reference *result = Reference::create<Number>();
							((Number *)result->data())->value = ((String *)lvalue.data())->str.size() ^ (size_t)to_number(ast, rvalue);

							ast->stack().pop_back();
							ast->stack().pop_back();
							ast->stack().push_back(SharedReference::unique(result));
						}));

	createBuiltinMember("!", 1, AbstractSynatxTree::createBuiltinMethode(STRING_TYPE, [] (AbstractSynatxTree *ast) {

							Reference &value = ast->stack().back().get();

							Reference *result = Reference::create<Number>();
							((Number *)result->data())->value = ((String *)value.data())->str.empty();

							ast->stack().pop_back();
							ast->stack().push_back(SharedReference::unique(result));
						}));

	createBuiltinMember("[]", 2, AbstractSynatxTree::createBuiltinMethode(STRING_TYPE, [] (AbstractSynatxTree *ast) {

							size_t base = get_base(ast);

							Reference &rvalue = ast->stack().at(base).get();
							Reference &lvalue = ast->stack().at(base - 1).get();

							Reference *result = Reference::create<String>();
							((String *)result->data())->construct();
							((String *)result->data())->str = *(utf8iterator(((String *)lvalue.data())->str.begin()) + (size_t)to_number(ast, rvalue));

							ast->stack().pop_back();
							ast->stack().pop_back();
							ast->stack().push_back(SharedReference::unique(result));
						}));

	/// \todo register operator overloads

	createBuiltinMember("size", 1, AbstractSynatxTree::createBuiltinMethode(STRING_TYPE, [] (AbstractSynatxTree *ast) {

							Reference &value = ast->stack().back().get();

							Reference *result = Reference::create<Number>();
							((Number *)result->data())->value = utf8length(((String *)value.data())->str);

							ast->stack().pop_back();
							ast->stack().push_back(SharedReference::unique(result));
						}));
}

void string_format(AbstractSynatxTree *ast, string &dest, const string &format, const vector<unique_ptr<Reference>> &args) {

	int flags = 0;

	int fieldWidth;
	int precision;
	int qualifier;

	size_t argn = 0;
	for (string::const_iterator cptr = format.begin(); cptr != format.end(); cptr++) {

		if ((*cptr == '%') && (argn < args.size())) {

			Reference *argv = args[argn++].get();
			bool handled = false;

			while (!handled && cptr != format.end()) {
				if (++cptr == format.end()) {
					error(""); /// \todo
				}
				switch (*cptr) {
				case '-':
					flags |= string_left;
					continue;
				case '+':
					flags |= string_plus;
					continue;
				case ' ':
					flags |= string_space;
					continue;
				case '#':
					flags |= string_special;
					continue;
				case '0':
					flags |= string_zeropad;
					continue;
				default:
					handled = true;
					break;
				}

				fieldWidth = -1;
				if (isdigit(*cptr)) {
					string num;
					while (isdigit(*cptr)) {
						num += *cptr;
						if (++cptr == format.end()) {
							error(""); /// \todo
						}
					}
					fieldWidth = atoi(num.c_str());
				}
				else if (*cptr == '*') {
					if (++cptr == format.end()) {
						error(""); /// \todo
					}
					fieldWidth = to_number(ast, *argv);
					argv = args[argn++].get();
					if (fieldWidth < 0) {
						fieldWidth = -fieldWidth;
						flags |= string_left;
					}
				}

				precision = -1;
				if (*cptr == '.') {
					if (++cptr == format.end()) {
						error(""); /// \todo
					}
					if (isdigit(*cptr)) {
						string num;
						while (isdigit(*cptr)) {
							num += *cptr;
							if (++cptr == format.end()) {
								error(""); /// \todo
							}
						}
						precision = atoi(num.c_str());
					}
					else if (*cptr == '*') {
						if (++cptr == format.end()) {
							error(""); /// \todo
						}
						precision = to_number(ast, *argv);
						argv = args[argn++].get();
					}
					if (precision < 0) {
						precision = 0;
					}
				}

				qualifier = -1;
				if (*cptr == 'h' || *cptr == 'l' || *cptr == 'L') {
					qualifier = *cptr;
					if (++cptr == format.end()) {
						error(""); /// \todo
					}
				}

				string s;
				int len;
				int base = 10;

				switch (*cptr) {
				case 'c':
					if (!(flags & string_left)) while (--fieldWidth > 0) dest += ' ';
					dest += to_char(ast, *argv);
					while (--fieldWidth > 0) dest += ' ';
					continue;
				case 's':
					s = to_string(*argv);
					len = (precision < 0) ? s.size() : min((size_t)precision, s.size());
					if (!(flags & string_left)) while (len < fieldWidth--) dest += ' ';
					dest += s.substr(0, len);
					while (len < fieldWidth--) dest += ' ';
					continue;
				case 'P':
					flags |= string_large;
				case 'p':
					if (fieldWidth == -1) {
						fieldWidth = 2 * sizeof(void *);
						flags |= string_zeropad;
					}
					dest += string_integer((unsigned long)argv->data(), 16, fieldWidth, precision, flags);
					continue;
				case 'A':
					flags |= string_large;
				case 'a':
					dest += string_hex_real(to_number(ast, *argv), qualifier, fieldWidth, precision, flags);
					continue;
				case 'B':
					flags |= string_large;
				case 'b':
					base = 2;
					break;
				case 'O':
					flags |= string_large;
				case 'o':
					base = 8;
					break;
				case 'X':
					flags |= string_large;
				case 'x':
					base = 16;
					break;
				case 'd':
				case 'i':
					flags |= string_sign;
					break;
				case 'u':
					break;
				case 'E':
				case 'G':
				case 'e':
				case 'f':
				case 'g':
					dest += string_real(to_number(ast, *argv), *cptr, fieldWidth, precision, flags | string_sign);
					continue;
				default:
				  dest += *cptr;
				  continue;
				}

				if (qualifier == 'l') {
					if (flags & string_sign) {
						dest += string_integer((long)to_number(ast, *argv), base, fieldWidth, precision, flags);
					}
					else {
						dest += string_integer((unsigned long)to_number(ast, *argv), base, fieldWidth, precision, flags);
					}
				}
				else if (qualifier == 'h') {
					if (flags & string_sign) {
						dest += string_integer((short)to_number(ast, *argv), base, fieldWidth, precision, flags);
					}
					else {
						dest += string_integer((unsigned short)to_number(ast, *argv), base, fieldWidth, precision, flags);
					}
				}
				else if (flags & string_sign) {
					dest += string_integer((long)to_number(ast, *argv), base, fieldWidth, precision, flags);
				}
				else {
					dest += string_integer((unsigned long)to_number(ast, *argv), base, fieldWidth, precision, flags);
				}
			}
		}
		else {
			dest += *cptr;
		}
	}
}

template<typename numtype>
string string_integer(numtype number, int base, int size, int precision, int flags) {

	string tmp;
	string result;
	const char *digits = (flags & string_large) ? "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ" : "0123456789abcdefghijklmnopqrstuvwxyz";

	if (flags & string_left) {
		flags &= ~string_zeropad;
	}
	if (base < 2 || base > 36) {
		return result;
	}

	char c = (flags & string_zeropad) ? '0' : ' ';
	char sign = 0;
	if (flags & string_sign) {
		if (number < 0) {
			sign = '-';
			number = -number;
			size--;
		}
		else if (flags & string_plus) {
			sign = '+';
			size--;
		}
		else if (flags & string_space) {
			sign = ' ';
			size--;
		}
	}

	if (flags & string_special) {
		if ((base == 16) || (base == 8) || (base == 2)) {
			size -= 2;
		}
	}

	if (number == 0) {
		tmp = "0";
	}
	else {
		while (number != 0) {
			tmp += digits[number % base];
			number = number / base;
		}
	}

	if ((int)tmp.size() > precision) precision = tmp.size();
	size -= precision;
	if (!(flags & (string_zeropad + string_left))) while (size-- > 0) result += ' ';
	if (sign) result += sign;

	if (flags & string_special) {
		if (base == 16) {
			result += "0";
			result += digits[33];
		}
		else if (base == 8) {
			result += "0";
			result += digits[24];
		}
		else if (base == 2) {
			result += "0";
			result += digits[11];
		}
	}

	if (!(flags & string_left)) while (size -- > 0) result += c;
	while ((int)tmp.size() < precision--) result += '0';
	for (auto cptr = tmp.rbegin(); cptr != tmp.rend(); ++cptr) {
		result += *cptr;
	}
	while (size-- > 0) result += ' ';

	return result;
}

void force_decimal_point(string &buffer) {

	string::iterator cptr = buffer.begin();

	while (cptr != buffer.end()) {
		if (*cptr == '.') {
			return;
		}
		if ((*cptr == 'e') || (*cptr == 'E')) {
			break;
		}
		++cptr;
	}

	if (cptr != buffer.end()) {
		buffer.insert(cptr, '.');
	}
	else {
		buffer += '.';
	}
}

void crop_zeros(string &buffer) {

	string::iterator stop;
	string::iterator cptr = buffer.begin();

	while ((cptr != buffer.end()) && (*cptr != '.')) {
		cptr++;
	}
	if (cptr++ != buffer.end()) {
		stop = cptr--;
		while (*cptr == '0') {
			cptr--;
		}
		if (*cptr == '.') {
			cptr--;
		}
		buffer.erase(cptr, stop);
	}
}

template<typename numtype>
string real_to_string(numtype number, int fmt, int precision) {

	string result;
	bool capexp = false;

	if ((fmt == 'G') || (fmt == 'E')) {
		capexp = true;
		fmt += 'a' - 'A';
	}

	/// \todo

	return result;
}

template<typename numtype>
string string_real(numtype number, int fmt, int size, int precision, int flags) {

	string result;
	string buffer;

	if (flags & string_left) {
		flags &= ~string_zeropad;
	}

	char c = (flags & string_zeropad) ? '0' : ' ';
	char sign = 0;
	if (flags & string_sign) {
		if (number < 0.0) {
			sign = '-';
			number = -number;
			size--;
		}
		else if (flags & string_plus) {
			sign = '+';
			size--;
		}
		else if (flags & string_space) {
			sign = ' ';
			size--;
		}
	}

	if (precision < 0) {
		precision = 6;
	}
	else if ((precision == 0) && (fmt == 'g')) {
		precision = 1;
	}

	buffer = real_to_string(number, fmt, precision);

	if ((flags & string_special) && (precision == 0)) {
		force_decimal_point(buffer);
	}

	if ((fmt == 'g') && !(flags & string_special)) {
		crop_zeros(buffer);
	}

	size -= buffer.size();
	if (!(flags & (string_zeropad | string_left))) while (size-- > 0) result += ' ';
	if (sign) result += sign;
	if (!(flags & string_left)) while (size-- > 0) result += c;
	result += buffer;
	while (size-- > 0) result += ' ';

	return result;
}

template<typename numtype>
string string_hex_real(numtype number, char qualifier, int size, int precision, int flags) {

	string result;
	const char *digits = (flags & string_large) ? "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ" : "0123456789abcdefghijklmnopqrstuvwxyz";

	if (qualifier == 'l') {
		/// \todo
	}
	else {
		/// \todo
	}

	return result;
}
