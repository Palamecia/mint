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

#include "mint/system/terminal.h"
#include "mint/system/utf8.h"
#include "mint/system/assert.h"
#include "mint/system/errno.h"

#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef OS_WINDOWS
#include "win32/terminal.h"
#include <io.h>
#else
#include "unix/terminal.h"
#include <unistd.h>
#include <stdarg.h>
#endif

using namespace mint;
using std::chrono::operator ""ms;

term_t Terminal::g_term;
tty_t Terminal::g_tty;

size_t Terminal::get_width() {
	term_t term;
	term_update_dim(&term);
	return term.width;
}

size_t Terminal::get_height() {
	term_t term;
	term_update_dim(&term);
	return term.height;
}

size_t Terminal::get_cursor_row() {
	cursor_pos_t pos = { 0, 0 };
	term_get_cursor_pos(&pos);
	return pos.row;
}

size_t Terminal::get_cursor_column() {
	cursor_pos_t pos = { 0, 0 };
	term_get_cursor_pos(&pos);
	return pos.column;
}

cursor_pos_t Terminal::get_cursor_pos() {
	cursor_pos_t pos = { 0, 0 };
	term_get_cursor_pos(&pos);
	return pos;
}

void Terminal::set_cursor_pos(const cursor_pos_t &pos) {
	term_set_cursor_pos(pos);
}

void Terminal::set_cursor_pos(size_t row, size_t column) {
	term_set_cursor_pos({ row, column });
}

void Terminal::move_cursor_left(size_t count) {
	if (count) {
		printf(stdout, "\033[%zdD", count);
	}
}

void Terminal::move_cursor_right(size_t count) {
	if (count) {
		printf(stdout, "\033[%zdC", count);
	}
}

void Terminal::move_cursor_up(size_t count) {
	if (count) {
		printf(stdout, "\033[%zdA", count);
	}
}

void Terminal::move_cursor_down(size_t count) {
	if (count) {
		printf(stdout, "\033[%zdB", count);
	}
}

void Terminal::move_cursor_to_start_of_line() {
	print(stdout, "\r");
}

void Terminal::set_prompt(std::function<std::string(size_t)> prompt) {
	m_prompt = prompt;
}

void Terminal::set_auto_braces(const std::string &auto_braces) {
	m_auto_braces = reinterpret_cast<const byte_t *>(auto_braces.data());
}

void Terminal::set_highlighter(std::function<std::string(std::string_view, std::string_view::size_type)> highlight) {
	m_highlight = highlight;
}

void Terminal::set_completion_generator(std::function<bool(std::string_view, std::string_view::size_type, std::vector<completion_t> &)> generator) {
	m_generate_completions = generator;
}

void Terminal::set_brace_matcher(std::function<std::pair<std::string_view::size_type, bool>(std::string_view, std::string_view::size_type)> matcher) {
	m_braces_match = matcher;
}

void Terminal::add_history(const std::string &line) {
	auto it = m_history.begin();
	while (it != m_history.end()) {
		if (*it == line) {
			it = m_history.erase(it);
		}
		else {
			++it;
		}
	}
	m_history.push_back(line);
}

std::optional<std::string> Terminal::read_line() {
	auto mode = term_setup_mode();
	auto buffer = edit();
	term_reset_mode(mode);
	return buffer;
}

int Terminal::print(FILE *stream, const char *str) {

#ifdef OS_UNIX
	return fputs(str, stream);
#else
	HANDLE hTerminal = INVALID_HANDLE_VALUE;

	if (stream == stdout) {
		hTerminal = GetStdHandle(STD_OUTPUT_HANDLE);
	}
	else if (stream == stderr) {
		hTerminal = GetStdHandle(STD_ERROR_HANDLE);
	}
	else {
		return fputs(str, stream);
	}

	if (term_vt100_enabled_for_console(hTerminal)) {
		return WriteMultiByteToConsoleW(hTerminal, str);
	}

	int written_all = 0;

	while (const char *cptr = strstr(str, "\033[")) {

		int written = WriteMultiByteToConsoleW(hTerminal, str, static_cast<int>(cptr - str));

		if (written == EOF) {
			errno = errno_from_windows_last_error();
			return written;
		}

		str = term_handle_vt100_sequence(hTerminal, cptr + 2);
		written_all += written;
	}

	if (*str) {
		int written = WriteMultiByteToConsoleW(hTerminal, str);
		if (written == EOF) {
			errno = errno_from_windows_last_error();
			return written;
		}
		written_all += written;
	}

	return written_all;
#endif
}

int Terminal::printf(FILE *stream, const char *format, ...) {
	va_list args;
	va_start(args, format);
	int written = Terminal::vprintf(stream, format, args);
	va_end(args);
	return written;
}

int Terminal::vprintf(FILE *stream, const char *format, va_list args) {

#ifdef OS_UNIX
	return vfprintf(stream, format, args);
#else
	HANDLE hTerminal = INVALID_HANDLE_VALUE;

	switch (int fd = fileno(stream)) {
	case STDOUT_FILE_NO:
		hTerminal = GetStdHandle(STD_OUTPUT_HANDLE);
		break;
	case STDERR_FILE_NO:
		hTerminal = GetStdHandle(STD_ERROR_HANDLE);
		break;
	default:
		return vfprintf(stream, format, args);
	}

	int written = 0;
	int written_all = 0;

	if (term_vt100_enabled_for_console(hTerminal)) {
		while (const char *cptr = strstr(format, "%")) {

			if (int prefix_length = static_cast<int>(cptr - format)) {
				written = WriteMultiByteToConsoleW(hTerminal, format, prefix_length);
				if (written == EOF) {
					errno = errno_from_windows_last_error();
					return written;
				}
				written_all += written;
			}

			format = cptr + 1;
			written = term_handle_format_flags(hTerminal, &format, &args);
			if (written == EOF) {
				errno = errno_from_windows_last_error();
				return written;
			}
			written_all += written;
		}
	}
	else {
		while (const char *cptr = strpbrk(format, "%\033")) {

			if (int prefix_length = static_cast<int>(cptr - format)) {
				written = WriteMultiByteToConsoleW(hTerminal, format, prefix_length);
				if (written == EOF) {
					errno = errno_from_windows_last_error();
					return written;
				}
				written_all += written;
			}

			switch (*cptr) {
			case '%':
				format = cptr + 1;
				written = term_handle_format_flags(hTerminal, &format, &args);
				if (written == EOF) {
					errno = errno_from_windows_last_error();
					return written;
				}
				written_all += written;
				break;
			case '\033':
				if (cptr[1] == '[') {
					format = term_handle_vt100_sequence(hTerminal, cptr + 2);
				}
				else {
					format = cptr + 1;
				}
				break;
			}
		}
	}

	if (*format) {
		int written = WriteMultiByteToConsoleW(hTerminal, format);
		if (written == EOF) {
			errno = errno_from_windows_last_error();
			return written;
		}
		written_all += written;
	}

	return written_all;
#endif
}

void Terminal::clear_to_end_of_line() {
	print(stdout, "\033[K");
}

void Terminal::clear_line() {
	print(stdout, "\r\033[K");
}

TtyEvent Terminal::wait_for_event(std::optional<std::chrono::milliseconds> timeout) {

	TtyEvent event;

	// is there a push_count back code?
	if (!g_tty.event_buffer.empty()) {
		event = g_tty.event_buffer.front();
		g_tty.event_buffer.pop();
		return event;
	}

	// read a single char/byte from a character stream
	byte_t byte = read_byte(timeout);
	if (!byte) {
		return EVENT_KEY_NONE;
	}

	if (byte == EVENT_KEY_ESC) {
		event = event_from_esc(100ms);
	}
	else if (isascii(byte)) {
		event = static_cast<TtyEvent>(byte);
	}
	else {
		// utf8 sequence
		event = static_cast<TtyEvent>(0xEE000U + byte);
	}

	auto key  = event & 0x0FFFFFFFU;
	auto mods = event & 0xF0000000U;

	// treat EVENT_KEY_RUBOUT (0x7F) as EVENT_KEY_BACKSP
	if (key == EVENT_KEY_RUBOUT) {
		event = static_cast<TtyEvent>(EVENT_KEY_BACKSP | mods);
	}
	// ctrl+'_' is translated to '\x1F' on Linux, translate it back
	else if (key == '\x1F' && (mods & EVENT_KEY_MOD_ALT) == 0) {
		key = '_';
		event = static_cast<TtyEvent>(EVENT_KEY_MOD_CTRL | '_');
	}
	// treat ctrl/shift + enter always as EVENT_KEY_LINEFEED for portability
	else if (key == EVENT_KEY_ENTER && (mods == EVENT_KEY_MOD_SHIFT || mods == EVENT_KEY_MOD_ALT || mods == EVENT_KEY_MOD_CTRL)) {
		event = EVENT_KEY_LINEFEED;
	}
	// treat ctrl+tab always as shift+tab for portability
	else if (event == (EVENT_KEY_MOD_CTRL | EVENT_KEY_TAB)) {
		event = static_cast<TtyEvent>(EVENT_KEY_MOD_SHIFT | EVENT_KEY_TAB);
	}
	// treat ctrl+end/alt+>/alt-down and ctrl+home/alt+</alt-up always as pagedown/pageup for portability
	else if (event == (EVENT_KEY_MOD_ALT | EVENT_KEY_DOWN) || event == (EVENT_KEY_MOD_ALT | '>') || event == (EVENT_KEY_MOD_CTRL | EVENT_KEY_END)) {
		event = EVENT_KEY_PAGEDOWN;
	}
	else if (event == (EVENT_KEY_MOD_ALT | EVENT_KEY_UP) || event == (EVENT_KEY_MOD_ALT | '<') || event == (EVENT_KEY_MOD_CTRL | EVENT_KEY_HOME)) {
		event = EVENT_KEY_PAGEUP;
	}

	// treat C0 codes without EVENT_KEY_MOD_CTRL
	if (key < ' ' && (mods & EVENT_KEY_MOD_CTRL) != 0) {
		event = static_cast<TtyEvent>(event & ~EVENT_KEY_MOD_CTRL);
	}

	return event;
}

TtyEvent Terminal::event_from_esc(std::optional<std::chrono::milliseconds> timeout) {

	uint32_t mods = 0;
	byte_t peek = 0;

	// lone ESC?
	if (!(peek = read_byte(timeout))) {
		return EVENT_KEY_ESC;
	}

	// treat ESC ESC as Alt modifier (macOS sends ESC ESC [ [A-D] for alt-<cursor>)
	if (peek == EVENT_KEY_ESC) {
		if (!(peek = read_byte(timeout.has_value() ? std::optional<std::chrono::milliseconds> {timeout.value() / 10} : std::nullopt))) {
			return static_cast<TtyEvent>(EVENT_KEY_ESC | EVENT_KEY_MOD_ALT);  // ESC <anychar>
		}
		mods |= EVENT_KEY_MOD_ALT;
	}

	// CSI ?
	if (peek == '[') {
		if (!(peek = read_byte(timeout.has_value() ? std::optional<std::chrono::milliseconds> {timeout.value() / 10} : std::nullopt))) {
			return static_cast<TtyEvent>('[' | EVENT_KEY_MOD_ALT);  // ESC <anychar>
		}
		return event_from_csi('[', peek, mods, timeout.has_value() ? std::optional<std::chrono::milliseconds> {timeout.value() / 10} : std::nullopt);  // ESC [ ...
	}

	// SS3?
	if (peek == 'O' || peek == 'o' || peek == '?' /*vt52*/) {
		uint8_t c1 = peek;
		if (!(peek = read_byte(timeout.has_value() ? std::optional<std::chrono::milliseconds> {timeout.value() / 10} : std::nullopt))) {
			return static_cast<TtyEvent>(c1 | EVENT_KEY_MOD_ALT);  // ESC <anychar>
		}
		if (c1 == 'o') {
			// ETerm uses this for ctrl+<cursor>
			mods |= EVENT_KEY_MOD_CTRL;
		}
		// treat all as standard SS3 'O'
		return event_from_csi('O', peek, mods, timeout.has_value() ? std::optional<std::chrono::milliseconds> {timeout.value() / 10} : std::nullopt);  // ESC [Oo?] ...
	}

	// OSC: we may get a delayed query response; ensure it is ignored
	if (peek == ']') {
		if (!(peek = read_byte(timeout.has_value() ? std::optional<std::chrono::milliseconds> {timeout.value() / 10} : std::nullopt))) {
			return static_cast<TtyEvent>(']' | EVENT_KEY_MOD_ALT);  // ESC <anychar>
		}
		return event_from_osc(peek, timeout.has_value() ? std::optional<std::chrono::milliseconds> {timeout.value() / 10} : std::nullopt);  // ESC ] ...
	}

	// Alt+<char>
	return static_cast<TtyEvent>(peek | EVENT_KEY_MOD_ALT);  // ESC <anychar>
}

TtyEvent Terminal::event_from_osc(byte_t peek, std::optional<std::chrono::milliseconds> timeout) {

	// keep reading until termination: OSC is terminated by BELL, or ESC \ (ST)  (and STX)
	for (;;) {
		if (peek <= '\x07') {  // BELL and anything below (STX, ^C, ^D)
			if (peek != '\x07') {
				g_tty.byte_buffer.push(peek);
			}
			break;
		}
		else if (peek == '\x1B') {
			if (!(peek = read_byte(timeout))) {
				break;
			}
			byte_t c1 = peek;
			if (c1 == '\\') {
				break;
			}
			g_tty.byte_buffer.push(c1);
		}
		if (!(peek = read_byte(timeout))) {
			break;
		}
	}
	return EVENT_KEY_NONE;
}

//-------------------------------------------------------------
// Decode escape sequences
//-------------------------------------------------------------

static TtyEvent esc_decode_vt(uint32_t vt_code) {
	switch(vt_code) {
	case 1:
		return EVENT_KEY_HOME;
	case 2:
		return EVENT_KEY_INS;
	case 3:
		return EVENT_KEY_DEL;
	case 4:
		return EVENT_KEY_END;
	case 5:
		return EVENT_KEY_PAGEUP;
	case 6:
		return EVENT_KEY_PAGEDOWN;
	case 7:
		return EVENT_KEY_HOME;
	case 8:
		return EVENT_KEY_END;
	case 10:
		return EVENT_KEY_F1;
	case 11:
		return EVENT_KEY_F2;
	case 12:
		return EVENT_KEY_F3;
	case 13:
		return EVENT_KEY_F4;
	case 14:
		return EVENT_KEY_F5;
	case 15:
		return EVENT_KEY_F6;
	case 16:
		return EVENT_KEY_F5; // minicom
	default:
		if (vt_code >= 17 && vt_code <= 21) return static_cast<TtyEvent>(EVENT_KEY_F1 + 5  + (vt_code - 17));
		if (vt_code >= 23 && vt_code <= 26) return static_cast<TtyEvent>(EVENT_KEY_F1 + 10 + (vt_code - 23));
		if (vt_code >= 28 && vt_code <= 29) return static_cast<TtyEvent>(EVENT_KEY_F1 + 14 + (vt_code - 28));
		if (vt_code >= 31 && vt_code <= 34) return static_cast<TtyEvent>(EVENT_KEY_F1 + 16 + (vt_code - 31));
	}
	return EVENT_KEY_NONE;
}

static TtyEvent esc_decode_xterm(uint8_t xcode) {
	// ESC [
	switch(xcode) {
	case 'A':
		return EVENT_KEY_UP;
	case 'B':
		return EVENT_KEY_DOWN;
	case 'C':
		return EVENT_KEY_RIGHT;
	case 'D':
		return EVENT_KEY_LEFT;
	case 'E':
		return static_cast<TtyEvent>('5');          // numpad 5
	case 'F':
		return EVENT_KEY_END;
	case 'H':
		return EVENT_KEY_HOME;
	case 'Z':
		return static_cast<TtyEvent>(EVENT_KEY_TAB | EVENT_KEY_MOD_SHIFT);
	// Freebsd:
	case 'I':
		return EVENT_KEY_PAGEUP;
	case 'L':
		return EVENT_KEY_INS;
	case 'M':
		return EVENT_KEY_F1;
	case 'N':
		return EVENT_KEY_F2;
	case 'O':
		return EVENT_KEY_F3;
	case 'P':
		return EVENT_KEY_F4;       // note: differs from <https://en.wikipedia.org/wiki/ANSI_escape_code#CSI_(Control_Sequence_Introducer)_sequences>
	case 'Q':
		return EVENT_KEY_F5;
	case 'R':
		return EVENT_KEY_F6;
	case 'S':
		return EVENT_KEY_F7;
	case 'T':
		return EVENT_KEY_F8;
	case 'U':
		return EVENT_KEY_PAGEDOWN; // Mach
	case 'V':
		return EVENT_KEY_PAGEUP;   // Mach
	case 'W':
		return EVENT_KEY_F11;
	case 'X':
		return EVENT_KEY_F12;
	case 'Y':
		return EVENT_KEY_END;      // Mach
	}
	return EVENT_KEY_NONE;
}

static TtyEvent esc_decode_ss3(uint8_t ss3_code) {
	// ESC O
	switch(ss3_code) {
	case 'A':
		return EVENT_KEY_UP;
	case 'B':
		return EVENT_KEY_DOWN;
	case 'C':
		return EVENT_KEY_RIGHT;
	case 'D':
		return EVENT_KEY_LEFT;
	case 'E':
		return static_cast<TtyEvent>('5');           // numpad 5
	case 'F':
		return EVENT_KEY_END;
	case 'H':
		return EVENT_KEY_HOME;
	case 'I':
		return EVENT_KEY_TAB;
	case 'Z':
		return static_cast<TtyEvent>(EVENT_KEY_TAB | EVENT_KEY_MOD_SHIFT);
	case 'M':
		return EVENT_KEY_LINEFEED;
	case 'P':
		return EVENT_KEY_F1;
	case 'Q':
		return EVENT_KEY_F2;
	case 'R':
		return EVENT_KEY_F3;
	case 'S':
		return EVENT_KEY_F4;
	// on Mach
	case 'T':
		return EVENT_KEY_F5;
	case 'U':
		return EVENT_KEY_F6;
	case 'V':
		return EVENT_KEY_F7;
	case 'W':
		return EVENT_KEY_F8;
	case 'X':
		return EVENT_KEY_F9;  // '=' on vt220
	case 'Y':
		return EVENT_KEY_F10;
	// numpad
	case 'a':
		return EVENT_KEY_UP;
	case 'b':
		return EVENT_KEY_DOWN;
	case 'c':
		return EVENT_KEY_RIGHT;
	case 'd':
		return EVENT_KEY_LEFT;
	case 'j':
		return static_cast<TtyEvent>('*');
	case 'k':
		return static_cast<TtyEvent>('+');
	case 'l':
		return static_cast<TtyEvent>(',');
	case 'm':
		return static_cast<TtyEvent>('-');
	case 'n':
		return EVENT_KEY_DEL; // '.'
	case 'o':
		return static_cast<TtyEvent>('/');
	case 'p':
		return EVENT_KEY_INS;
	case 'q':
		return EVENT_KEY_END;
	case 'r':
		return EVENT_KEY_DOWN;
	case 's':
		return EVENT_KEY_PAGEDOWN;
	case 't':
		return EVENT_KEY_LEFT;
	case 'u':
		return static_cast<TtyEvent>('5');
	case 'v':
		return EVENT_KEY_RIGHT;
	case 'w':
		return EVENT_KEY_HOME;
	case 'x':
		return EVENT_KEY_UP;
	case 'y':
		return EVENT_KEY_PAGEUP;
	}
	return EVENT_KEY_NONE;
}

TtyEvent Terminal::event_from_csi(byte_t c1, byte_t peek, uint32_t mods0, std::optional<std::chrono::milliseconds> timeout) {

	// CSI starts with 0x9b (c1=='[') | ESC [ (c1=='[') | ESC [Oo?] (c1 == 'O')  /* = SS3 */

	// check for extra starter '[' (Linux sends ESC [ [ 15 ~  for F5 for example)
	if (c1 == '[' && strchr("[Oo", static_cast<char>(peek)) != nullptr) {
		uint8_t cx = peek;
		if (!(peek = read_byte(timeout))) {
			c1 = cx;
		}
	}

	// "special" characters ('?' is used for private sequences)
	uint8_t special = 0;
	if (strchr(":<=>?", static_cast<char>(peek)) != nullptr) {
		special = peek;
		if (!(peek = read_byte(timeout))) {
			g_tty.byte_buffer.push(special);
			return static_cast<TtyEvent>(c1 | EVENT_KEY_MOD_ALT);       // Alt+<anychar>
		}
	}

	static auto read_csi_num = [read_byte = &Terminal::read_byte](uint8_t *ppeek, std::optional<std::chrono::milliseconds> timeout) -> uint32_t {
		uint32_t i = 0;
		size_t count = 0;
		while (isdigit(*ppeek) && count < 16) {
			uint8_t digit = *ppeek - '0';
			if ((*ppeek = read_byte(timeout.has_value() ? std::optional<std::chrono::milliseconds> {timeout.value() / 10} : std::nullopt))) {
				i = 10 * i + digit;
				++count;
			}
		}
		if (count > 0) {
			return i;
		}
		return 1; // default
	};

	// up to 2 parameters that default to 1
	uint32_t num1 = read_csi_num(&peek, timeout), num2 = 1;
	if (peek == ';') {
		if (!(peek = read_byte(timeout))) {
			return EVENT_KEY_NONE;
		}
		num2 = read_csi_num(&peek, timeout);
	}

	// the final character (we do not allow 'intermediate characters')
	uint8_t final = peek;
	uint32_t modifiers = mods0;

	// Adjust special cases into standard ones.
	if ((final == '@' || final == '9') && c1 == '[' && num1 == 1) {
		// ESC [ @, ESC [ 9  : on Mach
		if (final == '@')      num1 = 3; // DEL
		else if (final == '9') num1 = 2; // INS
		final = '~';
	}
	else if (final == '^' || final == '$' || final == '@') {
		// Eterm/rxvt/urxt
		if (final=='^') modifiers |= EVENT_KEY_MOD_CTRL;
		if (final=='$') modifiers |= EVENT_KEY_MOD_SHIFT;
		if (final=='@') modifiers |= EVENT_KEY_MOD_SHIFT | EVENT_KEY_MOD_CTRL;
		final = '~';
	}
	else if (c1 == '[' && final >= 'a' && final <= 'd') {  // note: do not catch ESC [ .. u  (for unicode)
		// ESC [ [a-d]  : on Eterm for shift+ cursor
		modifiers |= EVENT_KEY_MOD_SHIFT;
		final = 'A' + (final - 'a');
	}

	if (((c1 == 'O') || (c1=='[' && final != '~' && final != 'u')) &&
		(num2 == 1 && num1 > 1 && num1 <= 8))
	{
		// on haiku the modifier can be parameter 1, make it parameter 2 instead
		num2 = num1;
		num1 = 1;
	}

	// parameter 2 determines the modifiers
	if (num2 > 1 && num2 <= 9) {
		if (num2 == 9) num2 = 3; // iTerm2 in xterm mode
		num2--;
		if (num2 & 0x1) modifiers |= EVENT_KEY_MOD_SHIFT;
		if (num2 & 0x2) modifiers |= EVENT_KEY_MOD_ALT;
		if (num2 & 0x4) modifiers |= EVENT_KEY_MOD_CTRL;
	}

	// and translate
	TtyEvent event = EVENT_KEY_NONE;
	if (final == '~') {
		// vt codes
		event = esc_decode_vt(num1);
	}
	else if (c1 == '[' && final == 'u') {
		// unicode
		event = static_cast<TtyEvent>(num1);
	}
	else if (c1 == 'O' && ((final >= 'A' && final <= 'Z') || (final >= 'a' && final <= 'z'))) {
		// ss3
		event = esc_decode_ss3(final);
	}
	else if (num1 == 1 && final >= 'A' && final <= 'Z') {
		// xterm
		event = esc_decode_xterm(final);
	}
	else if (c1 == '[' && final == 'R') {
		// cursor position
		event = EVENT_KEY_NONE;
	}

	return (event != EVENT_KEY_NONE ? static_cast<TtyEvent>(event | modifiers) : EVENT_KEY_NONE);
}

byte_t Terminal::read_byte(std::optional<std::chrono::milliseconds> timeout) {

	// any events in the input queue?
	if (g_tty.byte_buffer.empty()) {
		term_read_input(&g_tty, timeout);
	}

	// in our pushback buffer?
	if (!g_tty.byte_buffer.empty()) {
		byte_t byte = g_tty.byte_buffer.front();
		g_tty.byte_buffer.pop();
		return byte;
	}

	return 0;
}

// skip an escape sequence
// <https://www.xfree86.org/current/ctlseqs.html>
static bool skip_esc(std::string_view str, size_t *esclen) {
	if (str.empty() || str.size() <= 1 || str[0] != '\033') {
		return false;
	}
	if (esclen) {
		*esclen = 0;
	}
	if (strchr("[PX^_]", str[1]) != nullptr) {
		// CSI (ESC [), DCS (ESC P), SOS (ESC X), PM (ESC ^), APC (ESC _), and OSC (ESC ]): terminated with a special sequence
		bool final_csi = (str[1] == '[');  // CSI terminates with 0x40-0x7F; otherwise ST (bell or ESC \)
		size_t n = 2;
		while (str.size() > n) {
			byte_t c = str[n++];
			if ((final_csi && c >= 0x40 && c <= 0x7F) // terminating byte: @A–Z[\]^_`a–z{|}~
				|| (!final_csi && c == '\x07') // bell
				|| (c == '\x02')) {                 // STX terminates as well
				if (esclen) {
					*esclen = n;
				}
				return true;
			}
			else if (!final_csi && c == '\x1B' && str.size() > n && str[n] == '\\') {  // ST (ESC \)
				n++;
				if (esclen) {
					*esclen = n;
				}
				return true;
			}
		}
	}
	if (strchr(" #%()*+",str[1]) != nullptr) {
		// assume escape sequence of length 3 (like ESC % G)
		if (esclen) {
			*esclen = 2;
		}
		return true;
	}
	else {
		// assume single character escape code (like ESC 7)
		if (esclen) {
			*esclen = 2;
		}
		return true;
	}
	return false;
}

// The column width of a codepoint (0, 1, or 2)
static size_t grapheme_column_width(std::string_view str) {
	if (str.empty()) {
		return 0;
	}
	if (static_cast<byte_t>(str.front()) < ' ') {
		return 0;   // also for CSI escape sequences
	}
	size_t w = utf8_grapheme_code_point_count(str);
#ifdef OS_WINDOWS
	return std::max(size_t {1}, w); // windows console seems to use at least one column
#else
	return w;
#endif
}

// Offset to the next codepoint, treats CSI escape sequences as a single code point.
static std::tuple<size_t, size_t> next_column(std::string_view str, size_t pos, size_t column) {
	size_t offset = 0;
	if (pos <= str.size()) {
		if (!skip_esc(str.substr(pos), &offset)) {
			offset = utf8_code_point_length(str[pos]);
		}
	}
	if (str[pos] == '\t') {
		return { offset, term_get_tab_width(column) };
	}
	return { offset, grapheme_column_width(str.substr(pos)) };
}

static size_t to_input_pos(std::string_view str, const cursor_pos_t &cursor) {
	if (str.empty()) {
		return 0;
	}
	size_t pos = 0;
	cursor_pos_t cur = { 0, 0 };
	while (pos < str.size()) {
		if (str[pos] == '\n') {
			if (cur.row == cursor.row) {
				break;
			}
			cur.column = 0;
			cur.row++;
			pos++;
		}
		else {
			if (cur.row == cursor.row && cur.column == cursor.column) {
				break;
			}
			auto [offset, width] = next_column(str, pos, cur.column);
			if (!offset) {
				break;
			}
			cur.column += width;
			pos += offset;
		}
	}
	return pos;
}

static cursor_pos_t to_cursor_pos(std::string_view str, std::string_view::size_type length = std::string_view::npos) {
	cursor_pos_t cursor = { 0, 0 };
	if (str.empty()) {
		return cursor;
	}
	size_t pos = 0;
	while (pos < std::min(length, str.size())) {
		if (str[pos] == '\n') {
			cursor.column = 0;
			cursor.row++;
			pos++;
		}
		else {
			auto [offset, width] = next_column(str, pos, cursor.column);
			if (!offset) {
				break;
			}
			cursor.column += width;
			pos += offset;
		}
	}
	return cursor;
}

static size_t column_count(std::string_view str, std::string_view::size_type length = std::string_view::npos) {
	if (str.empty()) {
		return 0;
	}
	size_t count = 0, pos = 0;
	while (pos < std::min(length, str.size())) {
		auto [offset, width] = next_column(str, pos, count);
		if (!offset) {
			break;
		}
		count += width;
		pos += offset;
	}
	return count;
}

std::pair<std::string_view::size_type, bool> Terminal::find_matching_brace(size_t brace_pos) {

	if (m_braces_match) {
		return m_braces_match(m_input, brace_pos);
	}

	if (!m_auto_braces.empty()) {
		bool balanced = true;
		auto pos = std::string_view::npos;
		const byte_t brace = m_input[brace_pos];
		for (size_t b = 0; b < m_auto_braces.size(); b += 2) {
			const size_t open = m_auto_braces[b];
			const size_t close = m_auto_braces[b + 1];
			std::optional<size_t> open_count, close_count;
			std::vector<size_t> close_graph;
			size_t count = 0;
			for (size_t i = 0; i < m_input.size(); ++i) {
				if (m_input[i] == open) {
					if (open == close) {
						if (brace == open) {
							if (count) {
								if (open_count) {
									pos = i;
									open_count = std::nullopt;
								}
								else if (i == brace_pos) {
									pos = *close_count;
									close_count = std::nullopt;
								}
							}
							else {
								if (i == brace_pos) {
									open_count = i;
								}
								else {
									close_count = i;
								}
							}
						}
						count = !count;
					}
					else {
						if (brace == open && i == brace_pos) {
							open_count = count;
						}
						else if (brace == close) {
							close_graph.push_back(i);
						}
						++count;
					}
				}
				else if (m_input[i] == close) {
					--count;
					if (open_count && *open_count == count) {
						open_count = std::nullopt;
						pos = i;
					}
					if (brace == close) {
						if (i == brace_pos) {
							pos = close_graph.back();
						}
						close_graph.pop_back();
					}
				}
			}
			if (count) {
				balanced = false;
			}
		}
		return { pos, balanced };
	}

	return { std::string_view::npos, true };
}

void Terminal::edit_insert_auto_brace(byte_t c) {
	if (m_auto_braces.empty()) {
		return;
	}
	for (const byte_t *b = m_auto_braces.data(); *b != 0; b += 2) {
		if (*b == c) {
			const byte_t close = b[1];
			if (*b == close && m_pos < m_input.size() && m_input[m_pos] == c) {
				m_input.erase(m_pos, 1);
			}
			else {
				m_input.insert(m_pos, 1, close);
			}
			auto [_, balanced] = find_matching_brace(m_pos);
			if (!balanced) {
				m_input.erase(m_pos, 1);
			}
			return;
		}
		else if (b[1] == c) {
			// close brace, check if there we don't overwrite to the right
			if (m_input[m_pos] == c) {
				m_input.erase(m_pos, 1);
			}
			return;
		}
	}
}

void Terminal::edit_remove_auto_brace(size_t pos) {
	auto [offset, balanced] = find_matching_brace(pos);
	if (balanced && offset != std::string_view::npos && offset >= m_pos) {
		m_input.erase(offset, 1);
	}
}

static size_t indent_size(const std::string_view str, std::string_view::size_type pos) {
	size_t count = 0;
	auto offset = str.rfind('\n', pos - 2) + 1;
	while (str[offset++] == ' ') {
		count++;
	}
	return count;
}

void Terminal::edit_auto_indent(byte_t pre, byte_t post) {
	assert(m_pos > 0 && m_input[m_pos - 1] == '\n');
	if (m_pos > 1) {
		if (m_input[m_pos - 2] == pre && m_input[m_pos] == post) {
			size_t indent = indent_size(m_input, m_pos);
			m_input.insert(m_pos, indent + m_indent_size, ' ');
			m_pos += indent + m_indent_size;
			m_input.insert(m_pos, 1, '\n');
			m_input.insert(m_pos + 1, indent, ' ');
		}
		else if (size_t indent = indent_size(m_input, m_pos)) {
			m_input.insert(m_pos, indent, ' ');
			m_pos += indent;
		}
	}
}

bool Terminal::edit_pos_is_inside_multi_line() {
	auto pos = m_input.rfind('\n');
	return pos != std::string::npos && pos > m_pos;
}

bool Terminal::edit_pos_is_inside_braces() {

	if (m_braces_match) {
		return !m_braces_match(m_input.substr(0, m_pos), m_pos).second;
	}

	if (!m_auto_braces.empty()) {
		for (size_t b = 0; b < m_auto_braces.size(); b += 2) {
			const size_t open = m_auto_braces[b];
			const size_t close = m_auto_braces[b + 1];
			size_t count = 0;
			for (size_t i = 0; i < m_pos; ++i) {
				if (m_input[i] == open) {
					if (open == close) {
						count = !count;
					}
					else {
						++count;
					}
				}
				else if (m_input[i] == close) {
					--count;
				}
			}
			if (count) {
				return true;
			}
		}
		return false;
	}

	return false;
}

bool Terminal::edit_is_multi_line() {
	return m_input.find('\n') != std::string::npos;
}

void Terminal::edit_cursor_to_start() {
	m_pos = 0;
}

void Terminal::edit_cursor_to_end() {
	m_pos = m_input.size();
}

void Terminal::edit_cursor_line_start() {
	if (!m_input.empty()) {
		const auto from = m_input[m_pos] != '\n' ? m_pos : m_pos - 1;
		m_pos = m_input.rfind('\n', from) + 1;
	}
}

void Terminal::edit_cursor_line_end() {
	auto pos = m_input.find('\n', m_pos);
	if (pos == std::string::npos) {
		m_pos = m_input.length();
	}
	else {
		m_pos = pos;
	}
}

static bool is_word_delimiter(byte_t b) {
	static const std::string g_word_delimiter = "()\"'-,:;<>~!@#$%^&*|+=[]{}~?│";
	return g_word_delimiter.find(b) != std::string::npos || std::isspace(b);
}

void Terminal::edit_cursor_prev_word() {
	auto pos = utf8_previous_code_point_byte_index(m_input, m_pos);
	while (pos != std::string_view::npos && is_word_delimiter(m_input[pos])) {
		m_pos = pos;
		pos = utf8_previous_code_point_byte_index(m_input, pos);
	}
	while (pos != std::string_view::npos && !is_word_delimiter(m_input[pos])) {
		m_pos = pos;
		pos = utf8_previous_code_point_byte_index(m_input, pos);
	}
}

void Terminal::edit_cursor_next_word() {
	while (m_pos < m_input.size() && !is_word_delimiter(m_input[m_pos])) {
		m_pos = utf8_next_code_point_byte_index(m_input, m_pos);
	}
	while (m_pos < m_input.size() && is_word_delimiter(m_input[m_pos])) {
		m_pos = utf8_next_code_point_byte_index(m_input, m_pos);
	}
}

void Terminal::edit_cursor_row_up() {
	cursor_pos_t pos = to_cursor_pos(m_input, m_pos);
	if (pos.row == 0) {
		edit_history_prev();
	}
	else {
		pos.row--;
		m_pos = to_input_pos(m_input, pos);
	}
}

void Terminal::edit_cursor_row_down() {
	cursor_pos_t pos = to_cursor_pos(m_input, m_pos);
	if (pos.row == m_input_rows) {
		edit_history_next();
	}
	else {
		pos.row++;
		m_pos = to_input_pos(m_input, pos);
	}
}

void Terminal::edit_cursor_left() {
	if (m_pos) {
		m_pos = utf8_previous_code_point_byte_index(m_input, m_pos);
	}
}

void Terminal::edit_cursor_right() {
	if (m_pos < m_input.size()) {
		m_pos = utf8_next_code_point_byte_index(m_input, m_pos);
	}
}

void Terminal::edit_cursor_match_brace() {
	auto [pos, _] = find_matching_brace(m_pos);
	if (pos != std::string_view::npos) {
		m_pos = pos;
	}
}

void Terminal::edit_delete_to_start_of_line() {
	auto from = m_input.rfind('\n', m_pos);
	if (from == std::string::npos) {
		from = 0;
	}
	m_input.erase(from, m_pos - from);
	m_pos = from;
}

void Terminal::edit_delete_to_end_of_line() {
	auto to = m_input.find('\n', m_pos);
	if (to == std::string::npos) {
		to = m_input.size();
	}
	m_input.erase(m_pos, to - m_pos);
}

void Terminal::edit_delete_to_start_of_word() {
	auto from = m_pos;
	auto pos = utf8_previous_code_point_byte_index(m_input, from);
	while (pos != std::string_view::npos && is_word_delimiter(m_input[pos])) {
		from = pos;
		pos = utf8_previous_code_point_byte_index(m_input, pos);
	}
	while (pos != std::string_view::npos && !is_word_delimiter(m_input[pos])) {
		from = pos;
		pos = utf8_previous_code_point_byte_index(m_input, pos);
	}
	m_input.erase(from, m_pos - from);
	m_pos = from;
}

void Terminal::edit_delete_to_end_of_word() {
	auto to = m_pos;
	while (to < m_input.size() && !is_word_delimiter(m_input[to])) {
		to = utf8_next_code_point_byte_index(m_input, to);
	}
	while (to < m_input.size() && is_word_delimiter(m_input[to])) {
		to = utf8_next_code_point_byte_index(m_input, to);
	}
	m_input.erase(m_pos, m_pos + to);
}

void Terminal::edit_delete_indent() {
	if (!m_input.empty()) {
		const auto from = m_input[m_pos] != '\n' ? m_pos : m_pos - 1;
		auto pos = m_input.rfind('\n', from) + 1;
		for (size_t i = 0; i < m_indent_size && pos < m_input.size() && m_input[pos] == ' '; ++i) {
			m_input.erase(pos, 1);
			m_pos--;
		}
	}
}

void Terminal::edit_delete_char() {
	if (m_pos < m_input.size()) {
		edit_remove_auto_brace(m_pos);
		m_input.erase(m_pos, utf8_code_point_length(m_input[m_pos]));
	}
}

void Terminal::edit_delete_all() {
	m_input.clear();
	m_pos = 0;
}

void Terminal::edit_backspace() {
	if (m_pos) {
		edit_remove_auto_brace(m_pos - 1);
	}
	if (const size_t pos = m_pos) {
		const size_t prev = utf8_previous_code_point_byte_index(m_input, m_pos);
		m_input.erase(prev, pos - prev);
		m_pos = prev;
	}
}

void Terminal::edit_swap_char() {
	if (utf8_code_point_count(m_input) > 1) {
		if (m_pos == m_input.size()) {
			const size_t to = utf8_previous_code_point_byte_index(m_input, m_pos);
			const size_t from = utf8_previous_code_point_byte_index(m_input, to);
			m_input.insert(m_pos, m_input.substr(from, utf8_code_point_length(m_input[from])));
			m_input.erase(from, utf8_code_point_length(m_input[from]));
		}
		else if (m_pos) {
			const size_t from = utf8_previous_code_point_byte_index(m_input, m_pos);
			m_input.insert(utf8_next_code_point_byte_index(m_input, m_pos), m_input.substr(from, utf8_code_point_length(m_input[from])));
			m_input.erase(from, utf8_code_point_length(m_input[from]));
			m_pos = utf8_next_code_point_byte_index(m_input, m_pos);
		}
		else {
			const size_t to = utf8_code_point_length(m_input[m_pos]);
			m_input.insert(utf8_next_code_point_byte_index(m_input, to), m_input.substr(0, utf8_code_point_length(m_input.front())));
			m_input.erase(0, utf8_code_point_length(m_input.front()));
			m_pos = utf8_next_code_point_byte_index(m_input, m_pos);
		}
	}
}

void Terminal::edit_swap_line_up() {
	const auto pos = m_input[m_pos] == '\n' ? m_pos - 1 : m_pos;
	const auto from = m_input.rfind('\n', pos) + 1;
	const auto to = m_input.find('\n', pos);
	if (from > 1) {
		const auto length = to != std::string::npos ? to - from + 1 : to;
		const auto target = m_input.rfind('\n', from - 2) + 1;
		const auto line = m_input.substr(from, length);
		if (length != std::string::npos) {
			m_input.erase(from, length);
			m_input.insert(target, line);
		}
		else {
			m_input.erase(from - 1, length);
			m_input.insert(target, line + "\n");
		}
		edit_cursor_row_up();
	}
}

void Terminal::edit_swap_line_down() {
	const auto pos = m_input[m_pos] == '\n' ? m_pos - 1 : m_pos;
	const auto from = m_input.rfind('\n', pos) + 1;
	const auto to = m_input.find('\n', pos);
	if (to != std::string::npos) {
		const auto length = to - from + 1;
		const auto target = m_input.find('\n', to + 1) + 1;
		const auto line = m_input.substr(from, length);
		if (target) {
			m_input.insert(target, line);
			m_input.erase(from, length);
		}
		else {
			m_input.append("\n" + line);
			m_input.erase(from, length);
			m_input.pop_back();
		}
		edit_cursor_row_down();
	}
}

void Terminal::edit_insert_char(byte_t c) {
	m_input.insert(m_pos++, 1, c);
	edit_insert_auto_brace(c);
	if (c == '\n' && m_auto_braces.size() > 1) {
		edit_auto_indent(m_auto_braces[0], m_auto_braces[1]);
	}
}

void Terminal::edit_insert_indent() {
	m_input.insert(m_pos, m_indent_size, ' ');
	m_pos += m_indent_size;
}

void Terminal::edit_clear_screen() {
	move_cursor_up(to_cursor_pos(m_input, m_pos).row + 1);
	for (size_t row = 0; row < g_term.height; ++row) {
		if (row) {
			print(stdout, "\n");
		}
		clear_line();
	}
	move_cursor_up(g_term.height);
}

void Terminal::edit_history_prev() {
	if (m_history_idx > 0) {
		if (m_history_idx == m_history.size() - 1) {
			m_history.back() = m_input;
		}
		m_input = m_history[--m_history_idx];
		m_pos = m_input.size();
	}
}

void Terminal::edit_history_next() {
	if (m_history_idx < m_history.size() - 1) {
		m_input = m_history[++m_history_idx];
		m_pos = m_input.size();
	}
}

void Terminal::edit_history_search_backward() {
	/// \todo
}

void Terminal::edit_history_search_forward() {
	/// \todo
}

bool Terminal::edit_generate_completions() {

	if (m_generate_completions) {
		m_completions.clear();
		m_completions_idx = 0;
		return m_generate_completions(m_input, m_pos, m_completions);
	}

	return false;
}

void Terminal::edit_refresh(bool for_validation) {

	const bool has_trailing_new_line = !m_input.empty() && m_input.back() == '\n';
	const cursor_pos_t input_cursor = to_cursor_pos(m_input, m_pos);

	const std::string input = m_highlight ? m_highlight(m_input, m_pos) : m_input;
	std::vector<std::pair<std::string::size_type, bool>> line_breaks;
	std::vector<std::string> prompts;

	m_input_rows = 0;
	prompts.push_back(m_prompt ? m_prompt(m_input_rows) : "");
	size_t prompt_width = column_count(prompts.back());

	// calculate rows separation including word wrap
	for (size_t pos = 0, column = 0; pos < input.size();) {
		if (input[pos] == '\n') {
			line_breaks.push_back({pos, true});
			prompts.push_back(m_prompt ? m_prompt(++m_input_rows) : "");
			prompt_width = column_count(prompts.back());
			column = 0;
			pos++;
		}
		else {
			auto [offset, width] = next_column(input, pos, column);
			if (!offset) {
				break;
			}
			if (prompt_width + column + width < g_term.width) {
				column += width;
			}
			else {
				line_breaks.push_back({pos + offset, false});
				column = width - 1;
				prompt_width = 0;
			}
			pos += offset;
		}
	}

	line_breaks.push_back({std::string::npos, true});
	m_input_rows++;

	// move cursor back to start of input
	move_cursor_down(m_cursor_rows - m_cursor_row - 1);
	while (--m_cursor_rows) {
		clear_line();
		move_cursor_up();
	}

	size_t begin_row = 0;
	size_t end_row = m_input_rows + 1;
	size_t begin_completion = 0;
	size_t end_completion = m_completions.size();

	// calculate the new cursor row and total rows needed
	if (!for_validation && g_term.height < end_row + end_completion) {
		const size_t input_page_size = m_completions.empty() ? g_term.height : 2 * (g_term.height / 3);
		if (input_cursor.row < input_page_size) {
			end_row = std::min(input_page_size, m_input_rows + 1);
		}
		else {
			begin_row = (input_cursor.row / input_page_size) * input_page_size;
			end_row = std::min(begin_row + input_page_size, m_input_rows + 1);
		}
		if (!m_completions.empty()) {
			end_completion = g_term.height - end_row - begin_row - 2;
			if (m_completions_idx >= end_completion) {
				const size_t completion_page_size = end_completion - 1;
				begin_completion = (m_completions_idx / completion_page_size) * completion_page_size;
				end_completion = std::min(begin_completion + completion_page_size, m_completions.size());
			}
		}
	}

	size_t from = 0, row = 0, next_prompt = 0;
	prompt_width = column_count(prompts[input_cursor.row]);

	// render rows
	for (auto [to, new_line] : line_breaks) {

		if (for_validation && to == std::string::npos && has_trailing_new_line) {
			break;
		}

		if (row >= begin_row && row < end_row) {

			if (row == next_prompt && row == input_cursor.row) {
				m_cursor_row = m_cursor_rows + (prompt_width + input_cursor.column) / g_term.width;
			}

			if (m_cursor_rows++) {
				print(stdout, "\n");
			}
			else {
				clear_line();
			}

			if (row == next_prompt) {
				print(stdout, prompts[next_prompt++].c_str());
			}

			const std::string input_line = input.substr(from, to - from);
			print(stdout, input_line.c_str());
		}

		if (new_line) {
			from = to + 1;
			++row;
		}
		else {
			from = to;
		}
	}

	if (begin_completion) {
		m_cursor_rows++;
		print(stdout, "\n");
		print(stdout, "          ⮝          ");
	}

	for (size_t idx = begin_completion; idx < end_completion; ++idx) {
		m_cursor_rows++;
		print(stdout, "\n");
		if (idx == m_completions_idx) {
			printf(stdout, "\033[1;7m %s \033[0m", m_completions[idx].token.c_str());
		}
		else {
			printf(stdout, "\033[0m %s \033[0m", m_completions[idx].token.c_str());
		}
	}

	if (end_completion < m_completions.size()) {
		m_cursor_rows++;
		print(stdout, "\n");
		print(stdout, "          ⮟          ");
	}

	// move cursor back to edit position
	move_cursor_to_start_of_line();
	move_cursor_up(m_cursor_rows - m_cursor_row - 1);
	move_cursor_right((prompt_width + input_cursor.column) % g_term.width);

#ifdef OS_UNIX
	fflush(stdout);
#endif
}

std::optional<std::string> Terminal::edit() {

	// set up an edit buffer
	m_cursor_rows = 1;
	m_cursor_row = 0;
	m_input_rows = 1;
	m_input.clear();
	m_pos = 0;

	// always a history entry for the current input
	m_history_idx = m_history.size();
	m_history.push_back("");

	// process keys
	TtyEvent event;          // current key code

	for (bool done = false; !done;) {

		edit_refresh();
		event = wait_for_event();

		// Completion Operations
		if (!m_completions.empty()) {
			completion_t completion = m_completions[m_completions_idx];
			switch (static_cast<uint32_t>(event)) {
			// Operations that may return
			case EVENT_KEY_ENTER:
				m_input.replace(completion.offset, m_pos - completion.offset, completion.token);
				m_pos = completion.offset + completion.token.size();
				m_completions.clear();
				m_completions_idx = 0;
				continue;
			case EVENT_KEY_UP:
				if (m_completions_idx == 0) {
					m_completions_idx = m_completions.size() - 1;
				}
				else {
					m_completions_idx--;
				}
				continue;
			case EVENT_KEY_TAB:
			case EVENT_KEY_DOWN:
				m_completions_idx = (m_completions_idx + 1) % m_completions.size();
				continue;
			case EVENT_KEY_DEL:
			case EVENT_KEY_BACKSP:
				g_tty.event_buffer.push(EVENT_AUTOTAB);
				break;
			default:
				if (isascii(event) || (event & 0xEE000U) == 0xEE000U) {
					g_tty.event_buffer.push(EVENT_AUTOTAB);
				}
				else {
					m_completions.clear();
					m_completions_idx = 0;
				}
				break;
			}
		}

		// Editing Operations
		switch (static_cast<uint32_t>(event)) {
		// Operations that may return
		case EVENT_KEY_ENTER:
			if (edit_pos_is_inside_multi_line() || edit_pos_is_inside_braces()) {
				edit_insert_char('\n');
			}
			else {
				// otherwise done
				m_input += '\n';
				done = true;
			}
			break;
		case EVENT_KEY_CTRL_D:
			if (m_input.empty()) {
				// ctrl+D on empty quits with NULL
				done = true;
				break;
			}
			edit_delete_char();     // otherwise it is like delete
			break;
		case EVENT_KEY_CTRL_C:
		case EVENT_STOP:
			// ctrl+C or STOP event quits with NULL
			done = true;
			break;
		case EVENT_KEY_ESC:
			if (m_input.empty()) {
				// ESC on empty input returns with empty input
				done = true;
				break;
			}
			edit_delete_all();      // otherwise delete the current input
			// edit_delete_line();  // otherwise delete the current line
			break;
		case EVENT_KEY_BELL: // ^G
			// ctrl+G cancels (and returns empty input)
			edit_delete_all();
			done = true;
			break;

		// Events
		case EVENT_RESIZE:
			term_update_dim(&g_term);
			break;
		case EVENT_AUTOTAB:
			if (!edit_generate_completions()) {
				/// \todo on no completion available
			}
			break;

		// Completion, history, help
		case EVENT_KEY_TAB:
			if (!edit_generate_completions()) {
				edit_insert_indent();
			}
			break;
		case EVENT_KEY_MOD_ALT | '?':
			if (!edit_generate_completions()) {
				/// \todo on no completion available
			}
			break;
		case EVENT_KEY_CTRL_R:
			edit_history_search_backward();
			break;
		case EVENT_KEY_CTRL_S:
			edit_history_search_forward();
			break;
		case EVENT_KEY_CTRL_P:
			edit_history_prev();
			break;
		case EVENT_KEY_CTRL_N:
			edit_history_next();
			break;
		case EVENT_KEY_CTRL_L:
			edit_clear_screen();
			break;

		// Navigation
		case EVENT_KEY_LEFT:
		case EVENT_KEY_CTRL_B:
			edit_cursor_left();
			break;
		case EVENT_KEY_RIGHT:
		case EVENT_KEY_CTRL_F:
			if (m_pos == m_input.size()) {
				if (!edit_generate_completions()) {
					/// \todo on no completion available
				}
			}
			else {
				edit_cursor_right();
			}
			break;
		case EVENT_KEY_UP:
			if (edit_is_multi_line()) {
				edit_cursor_row_up();
			}
			else {
				edit_history_prev();
			}
			break;
		case EVENT_KEY_DOWN:
			if (edit_is_multi_line()) {
				edit_cursor_row_down();
			}
			else {
				edit_history_next();
			}
			break;
		case EVENT_KEY_HOME:
		case EVENT_KEY_CTRL_A:
			edit_cursor_line_start();
			break;
		case EVENT_KEY_END:
		case EVENT_KEY_CTRL_E:
			edit_cursor_line_end();
			break;
		case EVENT_KEY_MOD_CTRL | EVENT_KEY_LEFT:
		case EVENT_KEY_MOD_SHIFT | EVENT_KEY_LEFT:
		case EVENT_KEY_MOD_ALT | 'b':
			edit_cursor_prev_word();
			break;
		case EVENT_KEY_MOD_CTRL | EVENT_KEY_RIGHT:
		case EVENT_KEY_MOD_SHIFT | EVENT_KEY_RIGHT:
		case EVENT_KEY_MOD_ALT | 'f':
			if (m_pos == m_input.size()) {
				if (!edit_generate_completions()) {
					/// \todo on no completion available
				}
			}
			else {
				edit_cursor_next_word();
			}
			break;
		case EVENT_KEY_MOD_CTRL | EVENT_KEY_HOME:
		case EVENT_KEY_MOD_SHIFT | EVENT_KEY_HOME:
		case EVENT_KEY_PAGEUP:
		case EVENT_KEY_MOD_ALT | '<':
			edit_cursor_to_start();
			break;
		case EVENT_KEY_MOD_CTRL | EVENT_KEY_END:
		case EVENT_KEY_MOD_SHIFT | EVENT_KEY_END:
		case EVENT_KEY_PAGEDOWN:
		case EVENT_KEY_MOD_ALT | '>':
			edit_cursor_to_end();
			break;
		case EVENT_KEY_MOD_ALT | 'm':
			edit_cursor_match_brace();
			break;

		// Deletion
		case EVENT_KEY_BACKSP:
			edit_backspace();
			break;
		case EVENT_KEY_DEL:
			edit_delete_char();
			break;
		case EVENT_KEY_CTRL_W:
		case EVENT_KEY_MOD_ALT | EVENT_KEY_DEL:
		case EVENT_KEY_MOD_ALT | EVENT_KEY_BACKSP:
			edit_delete_to_start_of_word();
			break;
		case EVENT_KEY_MOD_ALT | 'd':
			edit_delete_to_end_of_word();
			break;
		case EVENT_KEY_CTRL_U:
			edit_delete_to_start_of_line();
			break;
		case EVENT_KEY_CTRL_K:
			edit_delete_to_end_of_line();
			break;
		case EVENT_KEY_MOD_SHIFT | EVENT_KEY_TAB:
			edit_delete_indent();
			break;
		case EVENT_KEY_CTRL_T:
			edit_swap_char();
			break;
		case EVENT_KEY_MOD_CTRL | EVENT_KEY_UP:
			edit_swap_line_up();
			break;
		case EVENT_KEY_MOD_CTRL | EVENT_KEY_DOWN:
			edit_swap_line_down();
			break;

		// Editing
		case EVENT_KEY_LINEFEED: // '\n' (ctrl+J, shift+enter)
			edit_insert_char('\n');
			break;
		default:
			if (isascii(event)) {
				edit_insert_char(static_cast<byte_t>(event));
			}
			else if (const size_t len = utf8_code_point_length(event)) {
				edit_insert_char(static_cast<byte_t>(event));
				for (size_t i = 1; i < len; ++i) {
					edit_insert_char(read_byte(0ms));
				}
			}
			break;
		}
	}

	// goto end
	m_pos = m_input.size();

	// refresh once more but without brace matching
	edit_refresh(true);
	print(stdout, "\n");

	// input was canceled ?
	if ((event == EVENT_KEY_CTRL_D && m_input.empty()) || event == EVENT_KEY_CTRL_C || event == EVENT_STOP) {
		return std::nullopt;
	}

	// update history
	m_history.pop_back();
	if (m_input.size() > 1) {
		add_history(m_input.substr(0, m_input.size() - 1));
	}

	return m_input;
}

bool mint::is_term(FILE *stream) {
	return isatty(fileno(stream));
}

bool mint::is_term(int fd) {
	return isatty(fd);
}
