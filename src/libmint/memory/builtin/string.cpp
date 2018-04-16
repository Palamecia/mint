#include "memory/builtin/string.h"
#include "memory/casttool.h"
#include "memory/memorytool.h"
#include "memory/functiontool.h"
#include "ast/abstractsyntaxtree.h"
#include "ast/cursor.h"
#include "system/utf8iterator.h"
#include "system/error.h"

#include <cstring>
#include <cmath>

using namespace std;
using namespace mint;

static constexpr const char *lower_digits = "0123456789abcdefghijklmnopqrstuvwxyz";
static constexpr const char *upper_digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

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

enum DigitsFormat {
	scientific_format,
	decimal_format,
	shortest_format
};

void string_format(Cursor *cursor, string &dest, const string &format, Iterator *args);
template<typename numtype>
string string_integer(numtype number, int base, int size, int precision, int flags);
template<typename numtype>
string string_real(numtype number, int base, DigitsFormat format, int size, int precision, int flags);

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

							Reference &values = *cursor->stack().at(base);
							Reference &self = *cursor->stack().at(base - 1);

							SharedReference it = Reference::create<Iterator>();
							it->data<Iterator>()->construct();
							iterator_init(it->data<Iterator>(), values);
							Reference *result = Reference::create<String>();
							result->data<String>()->construct();
							string_format(cursor, result->data<String>()->str, self.data<String>()->str, it->data<Iterator>());

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
								while (SharedReference item = iterator_next(rvalue.data<Iterator>())) {
									result->data<String>()->str += *(utf8iterator(self.data<String>()->str.begin()) + (size_t)to_number(cursor, *item));
								}
							}
							else {
								auto offset = to_number(cursor, rvalue);
								if (offset < 0) {
									offset = self.data<String>()->str.size() + offset;
								}
								result->data<String>()->str = *(utf8iterator(self.data<String>()->str.begin()) + static_cast<size_t>(offset));
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

void string_format(Cursor *cursor, string &dest, const string &format, Iterator *args) {

	int flags = 0;

	int field_width;
	int precision;

	for (string::const_iterator cptr = format.begin(); cptr != format.end(); cptr++) {

		if ((*cptr == '%') && !args->ctx.empty()) {

			SharedReference argv = iterator_next(args);
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

				field_width = -1;
				if (isdigit(*cptr)) {
					string num;
					while (isdigit(*cptr)) {
						num += *cptr;
						if (++cptr == format.end()) {
							error("incomplete format '%s'", format.c_str());
						}
					}
					field_width = atoi(num.c_str());
				}
				else if (*cptr == '*') {
					if (++cptr == format.end()) {
						error("incomplete format '%s'", format.c_str());
					}
					field_width = to_number(cursor, *argv);
					argv = iterator_next(args);
					if (field_width < 0) {
						field_width = -field_width;
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
						argv = iterator_next(args);
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
					if (!(flags & string_left)) while (--field_width > 0) dest += ' ';
					dest += to_char(*argv);
					while (--field_width > 0) dest += ' ';
					continue;
				case 's':
					s = to_string(*argv);
					len = (precision < 0) ? s.size() : min((size_t)precision, s.size());
					if (!(flags & string_left)) while (len < field_width--) dest += ' ';
					dest += s.substr(0, len);
					while (len < field_width--) dest += ' ';
					continue;
				case 'P':
					flags |= string_large;
				case 'p':
					if (field_width == -1) {
						field_width = 2 * sizeof(void *);
						flags |= string_zeropad;
					}
#ifdef OS_WINDOWS
				{
					void *ptr = argv->data();
					dest += string_integer(*reinterpret_cast<unsigned long *>(&ptr), 16, fieldWidth, precision, flags);
				}
#else
					dest += string_integer(reinterpret_cast<unsigned long>(argv->data()), 16, field_width, precision, flags);
#endif
					continue;
				case 'A':
					flags |= string_large;
				case 'a':
					dest += string_real(to_number(cursor, *argv), 16, decimal_format, field_width, precision, flags);
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
					flags |= string_large;
				case 'e':
					dest += string_real(to_number(cursor, *argv), 10, scientific_format, field_width, precision, flags | string_sign);
					continue;
				case 'F':
					flags |= string_large;
				case 'f':
					dest += string_real(to_number(cursor, *argv), 10, decimal_format, field_width, precision, flags | string_sign);
					continue;
				case 'G':
					flags |= string_large;
				case 'g':
					dest += string_real(to_number(cursor, *argv), 10, shortest_format, field_width, precision, flags | string_sign);
					continue;
				default:
					dest += *cptr;
					continue;
				}

				if (flags & string_sign) {
					dest += string_integer((long)to_number(cursor, *argv), base, field_width, precision, flags);
				}
				else {
					dest += string_integer((unsigned long)to_number(cursor, *argv), base, field_width, precision, flags);
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
	const char *digits = (flags & string_large) ? upper_digits : lower_digits;

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

	string::iterator stop = buffer.end();
	string::iterator start = buffer.begin();

	while ((start != buffer.end()) && (*start != '.')) {
		start++;
	}
	if (start++ != buffer.end()) {
		while ((start != buffer.end()) && (*start != 'e') && (*start != 'E')) {
			start++;
		}
		stop = start--;
		while (*start == '0') {
			start--;
		}
		if (*start == '.') {
			start--;
		}
		buffer.erase(start + 1, stop);
	}
}

template<typename numtype>
string digits_to_string(numtype number, int base, DigitsFormat format, int precision, bool capexp, int *decpt, bool *sign) {

	string result;
	numtype fi, fj;
	const char *digits = (capexp) ? upper_digits : lower_digits;

	int r2 = 0;
	*sign = false;
	if (number < 0) {
		*sign = true;
		number = -number;
	}
	number = modf(number, &fi);

	if (fi != 0) {
		string buffer;
		while (fi != 0) {
			fj = modf(fi / base, &fi);
			buffer += digits[static_cast<int>((fj + .03) * base)];
			r2++;
		}
		for (auto i = buffer.rbegin(); i != buffer.rend(); ++i) {
			result += *i;
		}
	}
	else if (number > 0) {
		while ((fj = number * base) < 1) {
			number = fj;
			r2--;
		}
	}
	int pos = precision;
	if (format == decimal_format) {
		pos += r2;
	}
	*decpt = r2;
	if (pos < 0) {
		return result;
	}
	while (result.size() <= static_cast<size_t>(pos)) {
		number *= base;
		number = modf(number, &fj);
		result += digits[static_cast<int>(fj)];
	}
	int last = pos;
	result[pos] += base / 2;
	while (result[pos] > digits[base - 1]) {
		result[pos] = '0';
		if (pos > 0) {
			++result[--pos];
		}
		else {
			result[pos] = '1';
			(*decpt)++;
			if (format == decimal_format) {
				if (last > 0) {
					result[last] = '0';
				}
				result.push_back('0');
				last++;
			}
		}
	}
	while(last < static_cast<int>(result.size())) {
		result.pop_back();
	}
	return result;
}

template<typename numtype>
string real_to_string(numtype number, int base, DigitsFormat format, int precision, bool capexp) {

	string result;
	int decpt = 0;
	bool sign = false;
	const char *digits = (capexp) ? upper_digits : lower_digits;

	if (format == shortest_format) {
		digits_to_string(number, base, scientific_format, precision, capexp, &decpt, &sign);
		int magnitude = decpt - 1;
		if ((magnitude < -4) || (magnitude > precision - 1)) {
			format = scientific_format;
			precision -= 1;
		}
		else {
			format = decimal_format;
			precision -= decpt;
		}
	}

	if (format == scientific_format) {
		string num_digits = digits_to_string(number, base, format, precision + 1, capexp, &decpt, &sign);

		if (sign) {
			result += '-';
		}
		result += num_digits.front();
		if (precision > 0) {
			result += '.';
		}
		result += string(num_digits.data() + 1, precision) + (capexp ? 'E' : 'e');

		int exp = 0;

		if (decpt == 0) {
			if (number == 0.0) {
				exp = 0;
			}
			else {
				exp = -1;
			}
		}
		else {
			exp = decpt - 1;
		}

		if (exp < 0) {
			result += '-';
			exp = -exp;
		}
		else {
			result += '+';
		}

		char buffer[4];
		buffer[3] = '\0';
		buffer[2] = digits[(exp % base)];
		exp = exp / base;
		buffer[1] = digits[(exp % base)];
		exp = exp / base;
		buffer[0] = digits[(exp % base)];
		result += buffer;
	}
	else if (format == decimal_format) {
		string num_digits = digits_to_string(number, base, format, precision, capexp, &decpt, &sign);
		if (sign) {
			result += '-';
		}
		if (!num_digits.empty()) {
			if (decpt <= 0) {
				result += '0';
				result += '.';
				for (int pos = 0; pos < -decpt; pos++) {
					result += '0';
				}
				result += num_digits;
			}
			else {
				for (size_t pos = 0; pos < num_digits.size(); ++pos) {
					if (static_cast<int>(pos) == decpt) {
						result += '.';
					}
					result += num_digits[pos];
				}
			}
		}
		else {
			result += '0';
			if (precision > 0) {
				result += '.';
				for (int pos = 0; pos < precision; pos++) {
					result += '0';
				}
			}
		}
	}

	return result;
}

template<typename numtype>
string string_real(numtype number, int base, DigitsFormat format, int size, int precision, int flags) {

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
	else if ((precision == 0) && (format == shortest_format)) {
		precision = 1;
	}

	buffer = real_to_string(number, base, format, precision, flags & string_large);

	if ((flags & string_special) && (precision == 0)) {
		force_decimal_point(buffer);
	}

	if ((format == shortest_format) && !(flags & string_special)) {
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
