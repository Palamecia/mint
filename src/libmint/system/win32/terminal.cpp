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

#include "terminal.h"
#include "mint/system/terminal.h"
#include "mint/system/errno.h"

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

static void set_console_attributes(HANDLE hTerminal, const std::list<int> &attrs) {

	static WORD defaultAttributes = 0;
	static const std::unordered_map<int, std::pair<int, int>> Attributes = {
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

DWORD mint::term_setup_mode() {
	DWORD mode;
	HANDLE hTty = GetStdHandle(STD_INPUT_HANDLE);
	GetConsoleMode(hTty, &mode);
	SetConsoleMode(hTty, ENABLE_QUICK_EDIT_MODE | ENABLE_WINDOW_INPUT);
	return mode;
}

void mint::term_reset_mode(DWORD mode) {
	SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), mode);
}

char *mint::term_readline(const char *prompt) {

	size_t buffer_pos = 0;
	size_t buffer_length = 128;
	char *buffer = static_cast<char *>(malloc(buffer_length));

	Terminal::print(stdout, prompt);

	if (buffer) {
		for (int c = getchar(); (c != '\n') && (c != EOF); c = getchar()) {
			buffer[buffer_pos++] = static_cast<char>(c);
			if (buffer_pos == buffer_length) {
				buffer_length += 128;
				buffer = static_cast<char *>(realloc(buffer, buffer_length));
			}
		}

		buffer[buffer_pos] = '\0';
	}

	return buffer;
}

//-------------------------------------------------------------
// Push escape codes (used on Windows to insert keys)
//-------------------------------------------------------------

static void tty_push_bytes(tty_t *tty, const std::string &bytes) {
	for (char ch : bytes) {
		tty->byte_buffer.push(static_cast<byte_t>(ch));
	}
}

static unsigned csi_mods(uint32_t mods) {
	unsigned m = 1;
	if (mods & event_key_mod_shift) m += 1;
	if (mods & event_key_mod_alt)   m += 2;
	if (mods & event_key_mod_ctrl)  m += 4;
	return m;
}

// Push ESC [ <vtcode> ; <mods> ~
static void tty_cpush_csi_vt(tty_t* tty, uint32_t mods, uint32_t vtcode ) {
	tty_push_bytes(tty,"\033[" + std::to_string(vtcode) + ";" + std::to_string(csi_mods(mods)) + "~");
}

// push ESC [ 1 ; <mods> <xcmd>
static void tty_cpush_csi_xterm( tty_t* tty, uint32_t mods, char xcode) {
	tty_push_bytes(tty,"\033[1;" + std::to_string(csi_mods(mods)) + std::string(1, xcode));
}

// push ESC [ <unicode> ; <mods> u
static void tty_cpush_csi_unicode(tty_t* tty, uint32_t mods, uint32_t unicode) {
	if ((unicode < 0x80 && mods == 0)
		|| (mods == event_key_mod_ctrl && unicode < ' ' && unicode != event_key_tab && unicode != event_key_enter && unicode != event_key_linefeed && unicode != event_key_backsp)
		|| (mods == event_key_mod_shift && unicode >= ' ' && unicode <= event_key_rubout)) {
		tty->byte_buffer.push(static_cast<byte_t>(unicode));
	}
	else if (mods == 0) {
		if (unicode < 0x0800) {
			tty->byte_buffer.push(static_cast<byte_t>((unicode >> 6  & 0x1F) | 0xC0));
			tty->byte_buffer.push(static_cast<byte_t>((unicode >> 0  & 0x3F) | 0x80));
		}
		else if (unicode < 0x010000) {
			tty->byte_buffer.push(static_cast<byte_t>((unicode >> 12 & 0x0F) | 0xE0));
			tty->byte_buffer.push(static_cast<byte_t>((unicode >> 6  & 0x3F) | 0x80));
			tty->byte_buffer.push(static_cast<byte_t>((unicode >> 0  & 0x3F) | 0x80));
		}
		else if (unicode < 0x110000) {
			tty->byte_buffer.push(static_cast<byte_t>((unicode >> 18 & 0x07) | 0xF0));
			tty->byte_buffer.push(static_cast<byte_t>((unicode >> 12 & 0x3F) | 0x80));
			tty->byte_buffer.push(static_cast<byte_t>((unicode >> 6  & 0x3F) | 0x80));
			tty->byte_buffer.push(static_cast<byte_t>((unicode >> 0  & 0x3F) | 0x80));
		}
	}
	else {
		tty_push_bytes(tty,"\033[" + std::to_string(unicode) + ";" + std::to_string(csi_mods(mods)) + "u");
	}
}

// Read from the console input events and push escape codes into the tty cbuffer.
void mint::term_read_input(tty_t *tty, std::optional<std::chrono::milliseconds> timeout) {

	HANDLE hConsole = GetStdHandle(STD_INPUT_HANDLE);

	//  wait for a key down event
	INPUT_RECORD inp;
	DWORD count;
	uint32_t surrogate_hi = 0;

	for (;;) {
		// check if there are events if in non-blocking timeout mode
		if (timeout.has_value()) {
			// first peek ahead
			if (!GetNumberOfConsoleInputEvents(hConsole, &count)) {
				return;
			}
			if (count == 0) {
				if (auto timeout_ms = timeout->count()) {
					// wait for input events for at most timeout milli seconds
					ULONGLONG start_ms = GetTickCount64();
					DWORD res = WaitForSingleObject(hConsole, static_cast<DWORD>(timeout_ms));
					switch (res) {
					case WAIT_OBJECT_0:
						// input is available, decrease our timeout
						timeout = std::chrono::milliseconds(timeout_ms - std::max(0ull, GetTickCount64() - start_ms));
						break;
					case WAIT_TIMEOUT:
					case WAIT_ABANDONED:
					case WAIT_FAILED:
					default:
						return;
					}
				}
				else {
					// out of time
					return;
				}
			}
		}

		// (blocking) Read from the input
		if (!ReadConsoleInputW(hConsole, &inp, 1, &count)) {
			return;
		}

		if (count != 1) {
			return;
		}

		// resize event?
		if (inp.EventType == WINDOW_BUFFER_SIZE_EVENT) {
			tty->event_buffer.push(event_resize);
			continue;
		}

		// wait for key down events
		if (inp.EventType != KEY_EVENT) {
			continue;
		}

		// the modifier state
		DWORD modstate = inp.Event.KeyEvent.dwControlKeyState;

		// we need to handle shift up events separately
		if (!inp.Event.KeyEvent.bKeyDown && inp.Event.KeyEvent.wVirtualKeyCode == VK_SHIFT) {
			modstate &= (DWORD)~SHIFT_PRESSED;
		}

		// ignore AltGr
		DWORD altgr = LEFT_CTRL_PRESSED | RIGHT_ALT_PRESSED;
		if ((modstate & altgr) == altgr) {
			modstate &= ~altgr;
		}

		// get modifiers
		uint32_t mods = 0;
		if ((modstate & (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED)) != 0) {
			mods |= event_key_mod_ctrl;
		}
		if ((modstate & (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED)) != 0) {
			mods |= event_key_mod_alt;
		}
		if ((modstate & SHIFT_PRESSED) != 0) {
			mods |= event_key_mod_shift;
		}

		// virtual keys
		WCHAR chr = inp.Event.KeyEvent.uChar.UnicodeChar;
		WORD virt = inp.Event.KeyEvent.wVirtualKeyCode;

		// only process keydown events (except for Alt-up which is used for unicode pasting...)
		if (!inp.Event.KeyEvent.bKeyDown && virt != VK_MENU) {
			continue;
		}

		if (chr == 0) {
			switch (virt) {
			case VK_UP:
				tty_cpush_csi_xterm(tty, mods, 'A');
				return;
			case VK_DOWN:
				tty_cpush_csi_xterm(tty, mods, 'B');
				return;
			case VK_RIGHT:
				tty_cpush_csi_xterm(tty, mods, 'C');
				return;
			case VK_LEFT:
				tty_cpush_csi_xterm(tty, mods, 'D');
				return;
			case VK_END:
				tty_cpush_csi_xterm(tty, mods, 'F');
				return;
			case VK_HOME:
				tty_cpush_csi_xterm(tty, mods, 'H');
				return;
			case VK_DELETE:
				tty_cpush_csi_vt(tty,mods,3);
				return;
			case VK_PRIOR:
				tty_cpush_csi_vt(tty,mods,5);
				return;   //page up
			case VK_NEXT:
				tty_cpush_csi_vt(tty,mods,6);
				return;   //page down
			case VK_TAB:
				tty_cpush_csi_unicode(tty,mods,9);
				return;
			case VK_RETURN:
				tty_cpush_csi_unicode(tty,mods,13);
				return;
			default: {
				uint32_t vtcode = 0;
				if (virt >= VK_F1 && virt <= VK_F5) {
					vtcode = 10 + (virt - VK_F1);
				}
				else if (virt >= VK_F6 && virt <= VK_F10) {
					vtcode = 17 + (virt - VK_F6);
				}
				else if (virt >= VK_F11 && virt <= VK_F12) {
					vtcode = 13 + (virt - VK_F11);
				}
				if (vtcode > 0) {
					tty_cpush_csi_vt(tty, mods, vtcode);
					return;
				}
			}
			}
			// ignore other control keys (shift etc).
		}
		// high surrogate pair
		else if (chr >= 0xD800 && chr <= 0xDBFF) {
			surrogate_hi = (chr - 0xD800);
		}
		// low surrogate pair
		else if (chr >= 0xDC00 && chr <= 0xDFFF) {
			chr = ((surrogate_hi << 10) + (chr - 0xDC00) + 0x10000);
			tty_cpush_csi_unicode(tty,mods,chr);
			surrogate_hi = 0;
			return;
		}
		// regular character
		else {
			tty_cpush_csi_unicode(tty,mods,chr);
			return;
		}
	}
}

bool mint::term_update_dim(term_t *term) {

	ssize_t rows = 0;
	ssize_t cols = 0;
	CONSOLE_SCREEN_BUFFER_INFO sbinfo;
	if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &sbinfo)) {
		cols = (ssize_t)sbinfo.srWindow.Right - (ssize_t)sbinfo.srWindow.Left + 1;
		rows = (ssize_t)sbinfo.srWindow.Bottom - (ssize_t)sbinfo.srWindow.Top + 1;
	}

	bool changed = (term->width != cols || term->height != rows);
	term->width = cols;
	term->height = rows;
	return changed;
}

bool mint::term_get_cursor_pos(cursor_pos_t *pos) {
	CONSOLE_SCREEN_BUFFER_INFO sbinfo;
	if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &sbinfo)) {
		pos->column = sbinfo.dwCursorPosition.X;
		pos->row = sbinfo.dwCursorPosition.Y;
		return true;
	}
	return false;
}

bool mint::term_set_cursor_pos(const cursor_pos_t &pos) {
	COORD dwCursorPosition = { static_cast<SHORT>(pos.column), static_cast<SHORT>(pos.row) };
	return SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), dwCursorPosition);
}

size_t mint::term_get_tab_width(size_t column) {
	const size_t tab_width = 8; /// \todo get console info
	return tab_width - column % tab_width;
}

int mint::WriteMultiByteToConsoleW(HANDLE hConsoleOutput, const char *str, int cbMultiByte) {

	std::wstring buffer(MultiByteToWideChar(CP_UTF8, 0, str, cbMultiByte, nullptr, 0), L'\0');
	DWORD numberOfCharsWritten = 0;

	if (MultiByteToWideChar(CP_UTF8, 0, str, cbMultiByte, buffer.data(), buffer.length())) {
		if (WriteConsoleW(hConsoleOutput, buffer.data(), buffer.length(), &numberOfCharsWritten, nullptr)) {
			return static_cast<int>(numberOfCharsWritten);
		}
	}

	errno = errno_from_windows_last_error();
	return EOF;
}

int mint::WriteCharsToConsoleW(HANDLE hConsoleOutput, wchar_t wc, int cbRepeat) {

	std::wstring buffer(cbRepeat, wc);
	DWORD numberOfCharsWritten = 0;

	if (WriteConsoleW(hConsoleOutput, buffer.c_str(), buffer.length(), &numberOfCharsWritten, nullptr)) {
		return static_cast<int>(numberOfCharsWritten);
	}

	return EOF;
}

bool mint::term_vt100_enabled_for_console(HANDLE hTerminal) {

	DWORD dwMode = 0;

	if (GetConsoleMode(hTerminal, &dwMode)) {
		return SetConsoleMode(hTerminal, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
	}

	return false;
}

const char *mint::term_handle_vt100_sequence(HANDLE hTerminal, const char *cptr) {

	int attr = 0;
	std::list<int> attrs;

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

int mint::term_handle_format_flags(HANDLE hTerminal, const char **format, va_list *argptr) {

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
