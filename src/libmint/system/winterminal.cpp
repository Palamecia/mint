#include "winterminal.h"
#include "system/errno.h"

#include <unordered_map>
#include <string>
#include <list>

namespace ntdef {
#include <ntdef.h>
}

#define BUFFER_SIZE (32 + 17)

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

using namespace std;
using namespace mint;

enum {
	/* Formatting flags */
	FLAG_ALIGN_LEFT =    0x01,
	FLAG_FORCE_SIGN =    0x02,
	FLAG_FORCE_SIGNSP =  0x04,
	FLAG_PAD_ZERO =      0x08,
	FLAG_SPECIAL =       0x10,

	/* Data format flags */
	FLAG_SHORT =         0x100,
	FLAG_LONG =          0x200,
	FLAG_INT64 =         0x400,
#ifdef _WIN64
	FLAG_INTPTR =        FLAG_INT64,
#else
	FLAG_INTPTR =        0,
#endif
	FLAG_LONGDOUBLE =    0x800,
};

static const char digits_l[] = "0123456789abcdef0x";
static const char digits_u[] = "0123456789ABCDEF0X";
static const char *_nullstring = "(null)";
static const char _infinity[] = "#INF";
static const char _nan[] = "#QNAN";

static void set_console_attributes(HANDLE hTerminal, const list<int> &attrs) {

	static WORD defaultAttributes = 0;
	static const unordered_map<int, pair<int, int>> Attributes = {
		{ 00, { -1, -1 } },
		{ 30, { 0, -1 } },
		{ 31, { FOREGROUND_RED, -1 } },
		{ 32, { FOREGROUND_GREEN, -1 } },
		{ 33, { FOREGROUND_GREEN | FOREGROUND_RED, -1 } },
		{ 34, { FOREGROUND_BLUE, -1 } },
		{ 35, { FOREGROUND_BLUE | FOREGROUND_RED, -1 } },
		{ 36, { FOREGROUND_BLUE | FOREGROUND_GREEN, -1 } },
		{ 37, { FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED, -1 } },
		{ 40, { -1, 0 } },
		{ 41, { -1, BACKGROUND_RED } },
		{ 42, { -1, BACKGROUND_GREEN } },
		{ 43, { -1, BACKGROUND_GREEN | BACKGROUND_RED } },
		{ 44, { -1, BACKGROUND_BLUE } },
		{ 45, { -1, BACKGROUND_BLUE | BACKGROUND_RED } },
		{ 46, { -1, BACKGROUND_BLUE | BACKGROUND_GREEN } },
		{ 47, { -1, BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED } }
	};

	if (!defaultAttributes) {
		CONSOLE_SCREEN_BUFFER_INFO info;
		if (!GetConsoleScreenBufferInfo(hTerminal, &info)) {
			return;
		}
		defaultAttributes = info.wAttributes;
	}

	for (int attr : attrs) {

		auto i = Attributes.find(attr);

		if (i != Attributes.end()) {

			int foreground = i->second.first;
			int background = i->second.second;

			if (foreground == -1 && background == -1) {
				SetConsoleTextAttribute(hTerminal, defaultAttributes);
				return;
			}

			CONSOLE_SCREEN_BUFFER_INFO info;
			if (!GetConsoleScreenBufferInfo(hTerminal, &info)) {
				return;
			}

			if (foreground != -1) {
				info.wAttributes &= ~(info.wAttributes & 0x0F);
				info.wAttributes |= static_cast<WORD>(foreground);
			}

			if (background != -1) {
				info.wAttributes &= ~(info.wAttributes & 0xF0);
				info.wAttributes |= static_cast<WORD>(background);
			}

			SetConsoleTextAttribute(hTerminal, info.wAttributes);
		}
	}
}

int mint::WriteMultiByteToConsoleW(HANDLE hConsoleOutput, const char *str, int cbMultiByte) {

	DWORD length = MultiByteToWideChar(CP_UTF8, 0, str, cbMultiByte, nullptr, 0);
	wchar_t *buffer = static_cast<wchar_t *>(alloca(length * sizeof(wchar_t)));
	DWORD numberOfCharsWritten = 0;

	if (MultiByteToWideChar(CP_UTF8, 0, str, cbMultiByte, buffer, length)) {
		if (WriteConsoleW(hConsoleOutput, buffer, length, &numberOfCharsWritten, nullptr)) {
			return static_cast<int>(numberOfCharsWritten);
		}
	}

	errno = errno_from_windows_last_error();
	return EOF;
}

int mint::WriteCharsToConsoleW(HANDLE hConsoleOutput, wchar_t wc, int cbRepeat) {

	wstring buffer(cbRepeat, wc);
	DWORD numberOfCharsWritten = 0;

	if (WriteConsoleW(hConsoleOutput, buffer.c_str(), buffer.length(), &numberOfCharsWritten, nullptr)) {
		return static_cast<int>(numberOfCharsWritten);
	}

	return EOF;
}

bool mint::vt100_enabled_for_console(HANDLE hTerminal) {

	DWORD dwMode = 0;

	if (GetConsoleMode(hTerminal, &dwMode)) {
		return SetConsoleMode(hTerminal, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
	}

	return false;
}

const char *mint::handle_vt100_sequence(HANDLE hTerminal, const char *cptr) {

	int attr = 0;
	list<int> attrs;

	while (*cptr) {
		if (isdigit(*cptr)) {
			attr = (attr * 10) + (*cptr - '0');
		}
		else
			if (*cptr == ';') {
				attrs.push_back(attr);
				attr = 0;
			}
			else
				if (isalpha(*cptr)) {
					attrs.push_back(attr);
					if (*cptr == 'm') {
						set_console_attributes(stdout, attrs);
					}
					return cptr + 1;
				}
		cptr++;
	}

	return cptr;
}

static int streamout(HANDLE hTerminal, const char *prefix, const char *string, size_t len, int fieldwidth, int precision, unsigned int flags) {

	int written_all = 0;

	/* Calculate padding */
	size_t prefixlen = prefix ? strlen(prefix) : 0;
	if (precision < 0) {
		precision = 0;
	}
	int padding = (int)(fieldwidth - len - prefixlen - precision);
	if (padding < 0) {
		padding = 0;
	}

	/* Optional left space padding */
	if (((flags & (FLAG_ALIGN_LEFT | FLAG_PAD_ZERO)) == 0) && padding) {
		int written = WriteCharsToConsoleW(hTerminal, L' ', padding);
		if (written == EOF) {
			return EOF;
		}
		written_all += written;
		padding = 0;
	}

	/* Optional prefix */
	if (prefix) {
		int written = WriteMultiByteToConsoleW(hTerminal, prefix, prefixlen);
		if (written == EOF) {
			return EOF;
		}
		written_all += written;
	}

	/* Optional left '0' padding */
	if ((flags & FLAG_ALIGN_LEFT) == 0) {
		precision += padding;
	}
	if (precision) {
		int written = WriteCharsToConsoleW(hTerminal, L'0', precision);
		if (written == EOF) {
			return EOF;
		}
		written_all += written;
	}

	/* Output the string */
	int written = WriteMultiByteToConsoleW(hTerminal, string, len);
	if (written == EOF) {
		return EOF;
	}
	written_all += written;

	/* Optional right padding */
	if ((flags & FLAG_ALIGN_LEFT) && (padding)) {
		int written = WriteCharsToConsoleW(hTerminal, L' ', padding);
		if (written == EOF) {
			return EOF;
		}
		written_all += written;
	}

	return written_all;
}

static int streamout(HANDLE hTerminal, const char *prefix, const wchar_t *string, size_t len, int fieldwidth, int precision, unsigned int flags) {

	int written_all = 0;

	/* Calculate padding */
	size_t prefixlen = prefix ? strlen(prefix) : 0;
	if (precision < 0) {
		precision = 0;
	}
	int padding = (int)(fieldwidth - len - prefixlen - precision);
	if (padding < 0) {
		padding = 0;
	}

	/* Optional left space padding */
	if (((flags & (FLAG_ALIGN_LEFT | FLAG_PAD_ZERO)) == 0) && padding) {
		int written = WriteCharsToConsoleW(hTerminal, L' ', padding);
		if (written == EOF) {
			return EOF;
		}
		written_all += written;
		padding = 0;
	}

	/* Optional prefix */
	if (prefix) {
		int written = WriteMultiByteToConsoleW(hTerminal, prefix, prefixlen);
		if (written == EOF) {
			return EOF;
		}
		written_all += written;
	}

	/* Optional left '0' padding */
	if ((flags & FLAG_ALIGN_LEFT) == 0) {
		precision += padding;
	}
	if (precision) {
		int written = WriteCharsToConsoleW(hTerminal, L'0', precision);
		if (written == EOF) {
			return EOF;
		}
		written_all += written;
	}

	/* Output the string */
	DWORD numberOfCharsWritten = 0;
	if (!WriteConsoleW(hTerminal, string, len, &numberOfCharsWritten, nullptr)) {
		return EOF;
	}
	written_all += numberOfCharsWritten;

	/* Optional right padding */
	if ((flags & FLAG_ALIGN_LEFT) && (padding)) {
		int written = WriteCharsToConsoleW(hTerminal, L' ', padding);
		if (written == EOF) {
			return EOF;
		}
		written_all += written;
	}

	return written_all;
}

static void format_float(char chr, unsigned int flags, int precision, char **string, const char **prefix, va_list *argptr) {

	const char *digits = digits_l;
	int exponent = 0, sign;
	long double fpval, fpval2;
	int padding = 0, num_digits, val32, base = 10;

	bool is_e = false;

	/* Normalize the precision */
	if (precision < 0) {
		precision = 6;
	}
	else if (precision > 17) {
		padding = precision - 17;
		precision = 17;
	}

	/* Get the float value and calculate the exponent */
	if (flags & FLAG_LONGDOUBLE) {
		fpval = va_arg(*argptr, long double);
	}
	else {
		fpval = va_arg(*argptr, double);
	}

	exponent = (int)floor(fpval == 0 ? 0 : (fpval >= 0 ? log10(fpval) : log10(-fpval)));
	sign = fpval < 0 ? -1 : 1;

	switch (chr) {
	case 'G':
		digits = digits_u;
	case 'g':
		if (precision > 0) {
			precision--;
		}
		if (exponent < -4 || exponent >= precision) {
			is_e = true;
			break;
		}

		/* Shift the decimal point and round */
		fpval2 = round(sign * fpval * pow(10., precision));

		/* Skip trailing zeroes */
		while (precision && (uint64_t)fpval2 % 10 == 0) {
			precision--;
			fpval2 /= 10;
		}
		break;

	case 'E':
		digits = digits_u;
	case 'e':
		is_e = true;
		break;

	case 'A':
		digits = digits_u;
	case 'a':
		base = 16;

	case 'f':
	default:
		/* Shift the decimal point and round */
		fpval2 = round(sign * fpval * pow(10., precision));
		break;
	}

	if (is_e) {
		/* Shift the decimal point and round */
		fpval2 = round(sign * fpval * pow(10., precision - exponent));

		/* Compensate for changed exponent through rounding */
		if (fpval2 >= (uint64_t)pow(10., precision + 1)) {
			exponent++;
			fpval2 = round(sign * fpval * pow(10., precision - exponent));
		}

		val32 = exponent >= 0 ? exponent : -exponent;

		// FIXME: handle length of exponent field:
		// http://msdn.microsoft.com/de-de/library/0fatw238%28VS.80%29.aspx
		num_digits = 3;
		while (num_digits--) {
			*--(*string) = digits[val32 % 10];
			val32 /= 10;
		}

		/* Sign for the exponent */
		*--(*string) = exponent >= 0 ? '+' : '-';

		/* Add 'e' or 'E' separator */
		*--(*string) = digits[0xe];
	}

	/* Handle sign */
	if (fpval < 0) {
		*prefix = "-";
	}
	else if (flags & FLAG_FORCE_SIGN) {
		*prefix = "+";
	}
	else if (flags & FLAG_FORCE_SIGNSP) {
		*prefix = " ";
	}

	/* Handle special cases first */
	if (_isnan(fpval)) {
		(*string) -= sizeof(_nan) / sizeof(TCHAR) - 1;
		strcpy((*string), _nan);
		fpval2 = 1;
	}
	else if (!_finite(fpval)) {
		(*string) -= sizeof(_infinity) / sizeof(TCHAR) - 1;
		strcpy((*string), _infinity);
		fpval2 = 1;
	}
	else {
		/* Zero padding */
		while (padding-- > 0) {
			*--(*string) = '0';
		}

		/* Digits after the decimal point */
		num_digits = precision;
		while (num_digits-- > 0) {
			*--(*string) = digits[(uint64_t)fpval2 % 10];
			fpval2 /= base;
		}
	}

	if (precision > 0 || flags & FLAG_SPECIAL) {
		*--(*string) = '.';
	}

	/* Digits before the decimal point */
	do {
		*--(*string) = digits[(uint64_t)fpval2 % base];
		fpval2 /= base;
	}
	while ((uint64_t)fpval2);
}

static void format_int(char chr, unsigned int flags, int *precision, char **string, const char **prefix, va_list *argptr) {

	const char *digits = digits_l;
	uint64_t val64;
	int base = 10;

	bool is_unsigned = false;

	switch (chr) {
	case 'd':
	case 'i':
		if (flags & FLAG_INT64) {
			val64 = (int64_t)va_arg(*argptr, int64_t);
		}
		else if (flags & FLAG_SHORT) {
			val64 = (int64_t)va_arg(*argptr, short);
		}
		else {
			val64 = (int64_t)va_arg(*argptr, int);
		}

		if ((int64_t)val64 < 0) {
			val64 = -(int64_t)val64;
			*prefix = "-";
		}
		else if (flags & FLAG_FORCE_SIGN) {
			*prefix = "+";
		}
		else if (flags & FLAG_FORCE_SIGNSP) {
			*prefix = " ";
		}
		break;

	case 'o':
		base = 8;
		if (flags & FLAG_SPECIAL) {
			*prefix = "0";
			if (*precision > 0) {
				(*precision)--;
			}
		}
		is_unsigned = true;
		break;

	case 'p':
		*precision = 2 * sizeof(void*);
		flags &= ~FLAG_PAD_ZERO;
		flags |= FLAG_INTPTR;
		/* Fall through */

	case 'X':
		digits = digits_u;
		/* Fall through */

	case 'x':
		base = 16;
		if (flags & FLAG_SPECIAL) {
			*prefix = &digits[16];
		}

	case 'u':
		is_unsigned = true;
		break;
	default:
		break;
	}

	if (is_unsigned) {
		if (flags & FLAG_INT64) {
			val64 = (uint64_t)va_arg(*argptr, uint64_t);
		}
		else if (flags & FLAG_SHORT) {
			val64 = (uint64_t)va_arg(*argptr, unsigned short);
		}
		else {
			val64 = (uint64_t)va_arg(*argptr, unsigned int);
		}
	}

	if (*precision < 0) {
		*precision = 1;
	}

	/* Gather digits in reverse order */
	while (val64 || (*precision > 0)) {
		*--(*string) = digits[val64 % base];
		val64 /= base;
		(*precision)--;
	}
}

int mint::handle_format_flags(HANDLE hTerminal, const char **format, va_list *argptr) {

	char buffer[BUFFER_SIZE + 1];
	int fieldwidth, precision;
	char chr = *(*format)++;

	/* Check for end of format string */
	if (chr == '\0') {
		return 0;
	}

	/* Handle flags */
	unsigned int flags = 0;
	do {
		if (chr == '-') {
			flags |= FLAG_ALIGN_LEFT;
		}
		else if (chr == '+') {
			flags |= FLAG_FORCE_SIGN;
		}
		else if (chr == ' ') {
			flags |= FLAG_FORCE_SIGNSP;
		}
		else if (chr == '0') {
			flags |= FLAG_PAD_ZERO;
		}
		else if (chr == '#') {
			flags |= FLAG_SPECIAL;
		}
		else {
			break;
		}
	}
	while ((chr = *(*format)++));

	/* Handle field width modifier */
	if (chr == '*') {
		fieldwidth = va_arg(*argptr, int);
		if (fieldwidth < 0) {
			flags |= FLAG_ALIGN_LEFT;
			fieldwidth = -fieldwidth;
		}
		chr = *(*format)++;
	}
	else {
		fieldwidth = 0;
		while (chr >= '0' && chr <= '9') {
			fieldwidth = fieldwidth * 10 + (chr - '0');
			chr = *(*format)++;
		}
	}

	/* Handle precision modifier */
	if (chr == '.') {
		chr = *(*format)++;

		if (chr == '*') {
			precision = va_arg(*argptr, int);
			chr = *(*format)++;
		}
		else {
			precision = 0;
			while (chr >= '0' && chr <= '9') {
				precision = precision * 10 + (chr - '0');
				chr = *(*format)++;
			}
		}
	}
	else {
		precision = -1;
	}

	/* Handle argument size prefix */
	switch (chr) {
	case 'h':
		flags |= FLAG_SHORT;
		chr = *(*format)++;
		break;
	case 'w':
		flags |= FLAG_LONG;
		chr = *(*format)++;
		break;
	case 'L':
		flags |= 0;    // FIXME: long double
		chr = *(*format)++;
		break;
	case 'F':
		flags |= 0;    // FIXME: what is that?
		chr = *(*format)++;
		break;
	case 'l':
		/* Check if there is a 2nd 'l' */
		if (*format[1] == 'l') {
			flags |= FLAG_INT64;
			chr = *(*format += 2);
		}
		else {
			flags |= FLAG_LONG;
			chr = *(*format)++;
		}
		break;
	case 'I':
		if (*format[1] == '3' && *format[2] == '2') {
			chr = *(*format += 3);
		}
		else if (*format[1] == '6' && *format[2] == '4') {
			flags |= FLAG_INT64;
			chr = *(*format += 3);
		}
		else if (*format[1] == 'x' || *format[1] == 'X' ||
				 *format[1] == 'd' || *format[1] == 'i' ||
				 *format[1] == 'u' || *format[1] == 'o') {
			flags |= FLAG_INTPTR;
			chr = *(*format += 2);
		}
		break;
	case 'z':
		flags |= FLAG_INTPTR;
		chr = *(*format)++;
		break;

	default:
		break;
	}

	/* Handle the format specifier */
	const char *prefix = 0;

	switch (chr) {
	case 'n':
		if (flags & FLAG_INT64) {
			*va_arg(*argptr, int64_t *) = 0;
		}
		else if (flags & FLAG_SHORT) {
			*va_arg(*argptr, short *) = 0;
		}
		else {
			*va_arg(argptr, int *) = 0;
		}
		break;

	case 'C':
		flags |= FLAG_LONG;
		((wchar_t *)buffer)[0] = va_arg(*argptr, int);
		((wchar_t *)buffer)[1] = L'\0';
		return streamout(hTerminal, prefix, (wchar_t *)buffer, 1, fieldwidth, precision, flags);

	case 'c':
		buffer[0] = va_arg(*argptr, int);
		buffer[1] = '\0';
		return streamout(hTerminal, prefix, buffer, 1, fieldwidth, precision, flags);

	case 'Z':
		if (flags & FLAG_LONG) {
			if (ntdef::UNICODE_STRING *nt_string = va_arg(*argptr, ntdef::UNICODE_STRING *)) {
				if (nt_string->Buffer) {
					return streamout(hTerminal, prefix, nt_string->Buffer, nt_string->Length, fieldwidth, 0, flags);
				}
			}
		}
		else {
			if (ntdef::ANSI_STRING *nt_string = va_arg(*argptr, ntdef::ANSI_STRING *)) {
				if (nt_string->Buffer) {
					return streamout(hTerminal, prefix, nt_string->Buffer, nt_string->Length, fieldwidth, 0, flags);
				}
			}
		}

		return streamout(hTerminal, prefix, _nullstring, strnlen(_nullstring, (unsigned)precision), fieldwidth, 0, flags);

	case 'S':
		flags |= FLAG_LONG;
		if (wchar_t *string = va_arg(*argptr, wchar_t *)) {
			return streamout(hTerminal, prefix, string, wcsnlen(string, (unsigned)precision), fieldwidth, 0, flags);
		}

		return streamout(hTerminal, prefix, _nullstring, strnlen(_nullstring, (unsigned)precision), fieldwidth, 0, flags);

	case 's':
		if (char *string = va_arg(*argptr, char *)) {
			return streamout(hTerminal, prefix, string, strnlen(string, (unsigned)precision), fieldwidth, 0, flags);
		}

		return streamout(hTerminal, prefix, _nullstring, strnlen(_nullstring, (unsigned)precision), fieldwidth, 0, flags);

	case 'G':
	case 'E':
	case 'A':
	case 'g':
	case 'e':
	case 'a':
	case 'f':
	{
		char *string = &buffer[BUFFER_SIZE];
		*--(string) = '\0';
		format_float(chr, flags, precision, &string, &prefix, argptr);
		return streamout(hTerminal, prefix, string, strlen(string), fieldwidth, 0, flags);
	}

	case 'd':
	case 'i':
	case 'o':
	case 'p':
	case 'X':
	case 'x':
	case 'u':
	{
		char *string = &buffer[BUFFER_SIZE];
		*--(string) = '\0';
		format_int(chr, flags, &precision, &string, &prefix, argptr);
		return streamout(hTerminal, prefix, string, strlen(string), fieldwidth, precision, flags);
	}

	default:
		/* Treat anything else as a new character */
		(*format)--;
	}

	return 0;
}
