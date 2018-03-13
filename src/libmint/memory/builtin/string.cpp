#include "memory/builtin/string.h"
#include "memory/casttool.h"
#include "memory/memorytool.h"
#include "memory/functiontool.h"
#include "ast/abstractsyntaxtree.h"
#include "ast/cursor.h"
#include "system/utf8iterator.h"
#include "system/error.h"

#include <cstring>

using namespace std;
using namespace mint;

StringClass *StringClass::instance() {

	static StringClass g_instance;
	return &g_instance;
}

String::String() : Object(StringClass::instance()) {}

enum Flag {
	string_left = 0x01,
	string_plus = 0x02,
	string_space = 0x04,
	string_special = 0x08,
	string_zeropad = 0x10,
	string_large = 0x20,
	string_sign = 0x40
};

void string_format(Cursor *cursor, string &dest, const string &format, const Array::values_type &args);
template<typename numtype>
string string_integer(numtype number, int base, int size, int precision, int flags);
template<typename numtype>
string string_real(numtype number, int fmt, int size, int precision, int flags);
template<typename numtype>
string string_hex_real(numtype number, int size, int precision, int flags);

StringClass::StringClass() : Class("string", Class::string) {

	createBuiltinMember(":=", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							Reference &rvalue = *cursor->stack().at(base);
							Reference &self = *cursor->stack().at(base - 1);

							self.data<String>()->str = to_string(rvalue);

							cursor->stack().pop_back();
						}));

	createBuiltinMember("+", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							Reference &rvalue = *cursor->stack().at(base);
							Reference &self = *cursor->stack().at(base - 1);

							Reference *result = Reference::create<String>();
							result->data<String>()->construct();
							result->data<String>()->str = self.data<String>()->str + to_string(rvalue);

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().push_back(SharedReference::unique(result));
						}));

	createBuiltinMember("%", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							Reference &rvalue = *cursor->stack().at(base);
							Reference &self = *cursor->stack().at(base - 1);

							Reference *result = Reference::create<String>();
							result->data<String>()->construct();
							string_format(cursor, result->data<String>()->str, self.data<String>()->str, to_array(rvalue));

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().push_back(SharedReference::unique(result));
						}));

	createBuiltinMember("==", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							Reference &rvalue = *cursor->stack().at(base);
							Reference &self = *cursor->stack().at(base - 1);

							Reference *result = Reference::create<Number>();
							result->data<Number>()->value = self.data<String>()->str == to_string(rvalue);

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().push_back(SharedReference::unique(result));
						}));

	createBuiltinMember("!=", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							Reference &rvalue = *cursor->stack().at(base);
							Reference &self = *cursor->stack().at(base - 1);

							Reference *result = Reference::create<Number>();
							result->data<Number>()->value = self.data<String>()->str != to_string(rvalue);

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().push_back(SharedReference::unique(result));
						}));

	createBuiltinMember("<", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							Reference &rvalue = *cursor->stack().at(base);
							Reference &self = *cursor->stack().at(base - 1);

							Reference *result = Reference::create<Number>();
							result->data<Number>()->value = self.data<String>()->str < to_string(rvalue);

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().push_back(SharedReference::unique(result));
						}));

	createBuiltinMember(">", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							Reference &rvalue = *cursor->stack().at(base);
							Reference &self = *cursor->stack().at(base - 1);

							Reference *result = Reference::create<Number>();
							result->data<Number>()->value = self.data<String>()->str > to_string(rvalue);

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().push_back(SharedReference::unique(result));

						}));

	createBuiltinMember("<=", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							Reference &rvalue = *cursor->stack().at(base);
							Reference &self = *cursor->stack().at(base - 1);

							Reference *result = Reference::create<Number>();
							result->data<Number>()->value = self.data<String>()->str <= to_string(rvalue);

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().push_back(SharedReference::unique(result));
						}));

	createBuiltinMember(">=", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							Reference &rvalue = *cursor->stack().at(base);
							Reference &self = *cursor->stack().at(base - 1);

							Reference *result = Reference::create<Number>();
							result->data<Number>()->value = self.data<String>()->str >= to_string(rvalue);

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().push_back(SharedReference::unique(result));
						}));

	createBuiltinMember("&&", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							Reference &rvalue = *cursor->stack().at(base);
							Reference &self = *cursor->stack().at(base - 1);

							Reference *result = Reference::create<Number>();
							result->data<Number>()->value = self.data<String>()->str.size() && to_number(cursor, rvalue);

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().push_back(SharedReference::unique(result));
						}));

	createBuiltinMember("||", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							Reference &rvalue = *cursor->stack().at(base);
							Reference &self = *cursor->stack().at(base - 1);

							Reference *result = Reference::create<Number>();
							result->data<Number>()->value = self.data<String>()->str.size() || to_number(cursor, rvalue);

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().push_back(SharedReference::unique(result));
						}));

	createBuiltinMember("^", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							Reference &rvalue = *cursor->stack().at(base);
							Reference &self = *cursor->stack().at(base - 1);

							Reference *result = Reference::create<Number>();
							result->data<Number>()->value = self.data<String>()->str.size() ^ (size_t)to_number(cursor, rvalue);

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().push_back(SharedReference::unique(result));
						}));

	createBuiltinMember("!", 1, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							Reference &self = *cursor->stack().back();

							Reference *result = Reference::create<Number>();
							result->data<Number>()->value = self.data<String>()->str.empty();

							cursor->stack().pop_back();
							cursor->stack().push_back(SharedReference::unique(result));
						}));

	createBuiltinMember("[]", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							Reference &rvalue = *cursor->stack().at(base);
							Reference &self = *cursor->stack().at(base - 1);

							Reference *result = Reference::create<String>();
							result->data<String>()->construct();
							if ((rvalue.data()->format == Data::fmt_object) && (rvalue.data<Object>()->metadata->metatype() == Class::iterator)) {
								for (SharedReference item(nullptr); iterator_next(rvalue.data<Iterator>(), item);) {
									result->data<String>()->str += *(utf8iterator(self.data<String>()->str.begin()) + (size_t)to_number(cursor, *item));
								}
							}
							else {
								result->data<String>()->str = *(utf8iterator(self.data<String>()->str.begin()) + (size_t)to_number(cursor, rvalue));
							}

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().push_back(SharedReference::unique(result));
						}));

	/// \todo register operator overloads

	createBuiltinMember("size", 1, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							Reference &self = *cursor->stack().back();

							Reference *result = Reference::create<Number>();
							result->data<Number>()->value = utf8length(self.data<String>()->str);

							cursor->stack().pop_back();
							cursor->stack().push_back(SharedReference::unique(result));
						}));

	createBuiltinMember("empty", 1, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							Reference &self = *cursor->stack().back();

							Reference *result = Reference::create<Number>();
							result->data<Number>()->value = self.data<String>()->str.empty();

							cursor->stack().pop_back();
							cursor->stack().push_back(SharedReference::unique(result));
						}));

	createBuiltinMember("clear", 1, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							SharedReference self = cursor->stack().back();

							self->data<String>()->str.clear();

							cursor->stack().pop_back();
							cursor->stack().push_back(self);
						}));

	createBuiltinMember("replace", 3, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							Reference &str = *cursor->stack().at(base);
							Reference &pattern = *cursor->stack().at(base - 1);
							SharedReference self = cursor->stack().at(base - 2);

							std::string before = to_string(pattern);
							std::string after = to_string(str);

							size_t pos = 0;
							while ((pos = self->data<String>()->str.find(before, pos)) != string::npos) {
								self->data<String>()->str.replace(pos, before.size(), after);
								pos += after.size();
							}

							cursor->stack().pop_back();
							cursor->stack().push_back(self);
						}));

	createBuiltinMember("contains", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							Reference &other = *cursor->stack().at(base);
							Reference &self = *cursor->stack().at(base - 1);

							Reference *result = Reference::create<Boolean>();
							result->data<Boolean>()->value = self.data<String>()->str.find(to_string(other)) != string::npos;

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().push_back(SharedReference::unique(result));
						}));

	createBuiltinMember("indexOf", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							Reference &other = *cursor->stack().at(base);
							Reference &self = *cursor->stack().at(base - 1);

							auto pos = self.data<String>()->str.find(to_string(other));

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							if (pos != string::npos) {
								cursor->stack().push_back(create_number(pos));
							}
							else {
								cursor->stack().push_back(SharedReference::unique(Reference::create<None>()));
							}
						}));

	createBuiltinMember("indexOf", 3, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							Reference &from = *cursor->stack().at(base);
							Reference &other = *cursor->stack().at(base - 1);
							Reference &self = *cursor->stack().at(base - 2);

							auto pos = self.data<String>()->str.find(to_string(other), static_cast<size_t>(to_number(cursor, from)));

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().pop_back();
							if (pos != string::npos) {
								cursor->stack().push_back(create_number(pos));
							}
							else {
								cursor->stack().push_back(SharedReference::unique(Reference::create<None>()));
							}
						}));

	createBuiltinMember("lastIndexOf", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							Reference &other = *cursor->stack().at(base);
							Reference &self = *cursor->stack().at(base - 1);

							auto pos = self.data<String>()->str.rfind(to_string(other));

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							if (pos != string::npos) {
								cursor->stack().push_back(create_number(pos));
							}
							else {
								cursor->stack().push_back(SharedReference::unique(Reference::create<None>()));
							}
						}));

	createBuiltinMember("lastIndexOf", 3, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							Reference &from = *cursor->stack().at(base);
							Reference &other = *cursor->stack().at(base - 1);
							Reference &self = *cursor->stack().at(base - 2);

							auto pos = self.data<String>()->str.rfind(to_string(other), static_cast<size_t>(to_number(cursor, from)));

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().pop_back();
							if (pos != string::npos) {
								cursor->stack().push_back(create_number(pos));
							}
							else {
								cursor->stack().push_back(SharedReference::unique(Reference::create<None>()));
							}
						}));

	createBuiltinMember("startsWith", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							Reference &other = *cursor->stack().at(base);
							Reference &self = *cursor->stack().at(base - 1);

							Reference *result = Reference::create<Boolean>();
							result->data<Boolean>()->value = self.data<String>()->str.find(to_string(other)) == 0;

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().push_back(SharedReference::unique(result));
						}));

	createBuiltinMember("endsWith", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							Reference &other = *cursor->stack().at(base);
							Reference &self = *cursor->stack().at(base - 1);

							std::string str = to_string(other);

							Reference *result = Reference::create<Boolean>();
							result->data<Boolean>()->value = (self.data<String>()->str.size() >= str.size()) &&
							(self.data<String>()->str.rfind(str) == self.data<String>()->str.size() - str.size());

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().push_back(SharedReference::unique(result));
						}));

	createBuiltinMember("split", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							std::string sep = to_string(*cursor->stack().at(base));
							std::string self = cursor->stack().at(base - 1)->data<String>()->str;

							Reference *result = Reference::create<Array>();
							if (sep.empty()) {
								for (utf8iterator i = self.begin(); i != self.end(); ++i) {
									array_append(result->data<Array>(), create_string(*i));
								}
							}
							else {
								size_t from = 0;
								size_t pos = self.find(sep);
								while (pos != std::string::npos) {
									array_append(result->data<Array>(), create_string(self.substr(from, pos - from)));
									pos = self.find(sep, from = pos + sep.size());
								}
								if (from < self.size()) {
									array_append(result->data<Array>(), create_string(self.substr(from, pos - from)));
								}
							}
							result->data<Array>()->construct();

							cursor->stack().pop_back();
							cursor->stack().pop_back();
							cursor->stack().push_back(SharedReference::unique(result));
						}));

}

void string_format(Cursor *cursor, string &dest, const string &format, const Array::values_type &args) {

	int flags = 0;

	int fieldWidth;
	int precision;

	size_t argn = 0;
	for (string::const_iterator cptr = format.begin(); cptr != format.end(); cptr++) {

		if ((*cptr == '%') && (argn < args.size())) {

			Reference *argv = args[argn++].get();
			bool handled = false;

			while (!handled && cptr != format.end()) {
				if (++cptr == format.end()) {
					error("incomplete format '%s'", format.c_str());
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
							error("incomplete format '%s'", format.c_str());
						}
					}
					fieldWidth = atoi(num.c_str());
				}
				else if (*cptr == '*') {
					if (++cptr == format.end()) {
						error("incomplete format '%s'", format.c_str());
					}
					fieldWidth = to_number(cursor, *argv);
					argv = args[argn++].get();
					if (fieldWidth < 0) {
						fieldWidth = -fieldWidth;
						flags |= string_left;
					}
				}

				precision = -1;
				if (*cptr == '.') {
					if (++cptr == format.end()) {
						error("incomplete format '%s'", format.c_str());
					}
					if (isdigit(*cptr)) {
						string num;
						while (isdigit(*cptr)) {
							num += *cptr;
							if (++cptr == format.end()) {
								error("incomplete format '%s'", format.c_str());
							}
						}
						precision = atoi(num.c_str());
					}
					else if (*cptr == '*') {
						if (++cptr == format.end()) {
							error("incomplete format '%s'", format.c_str());
						}
						precision = to_number(cursor, *argv);
						argv = args[argn++].get();
					}
					if (precision < 0) {
						precision = 0;
					}
				}

				string s;
				int len;
				int base = 10;

				switch (*cptr) {
				case 'c':
					if (!(flags & string_left)) while (--fieldWidth > 0) dest += ' ';
					dest += to_char(*argv);
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
#ifdef OS_WINDOWS
				{
					void *ptr = argv->data();
					dest += string_integer(*reinterpret_cast<unsigned long *>(&ptr), 16, fieldWidth, precision, flags);
				}
#else
					dest += string_integer(reinterpret_cast<unsigned long>(argv->data()), 16, fieldWidth, precision, flags);
#endif
					continue;
				case 'A':
					flags |= string_large;
				case 'a':
					dest += string_hex_real(to_number(cursor, *argv), fieldWidth, precision, flags);
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
					dest += string_real(to_number(cursor, *argv), *cptr, fieldWidth, precision, flags | string_sign);
					continue;
				default:
					dest += *cptr;
					continue;
				}

				if (flags & string_sign) {
					dest += string_integer((long)to_number(cursor, *argv), base, fieldWidth, precision, flags);
				}
				else {
					dest += string_integer((unsigned long)to_number(cursor, *argv), base, fieldWidth, precision, flags);
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

	/// \todo convert real number to string

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
string string_hex_real(numtype number, int size, int precision, int flags) {

	string result;
	const char *digits = (flags & string_large) ? "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ" : "0123456789abcdefghijklmnopqrstuvwxyz";

	/// \todo convert real number to hex string

	return result;
}
