/**
 * Copyright (c) 2026 Gauvain CHERY.
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
#include "mint/config.h"
#include "mint/system/utf8.h"
#include "mint/system/assert.h"
#include "mint/system/errno.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <ctype.h>
#include <functional>
#include <optional>
#include <print>
#include <stdio.h>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string_view>
#include <system_error>
#include <tuple>
#include <utility>
#include <vector>

#ifdef MINT_OS_WINDOWS
#include "win32/terminal.h"
#include <Windows.h>
#include <corecrt_io.h>
#include <handleapi.h>
#include <io.h>
#include <processenv.h>
#include <winbase.h>
#include <winnt.h>
#else
#include "unix/terminal.h"
#include <unistd.h>
#include <stdarg.h>
#endif

using namespace mint;
using namespace std::chrono_literals;

TerminalInfo Terminal::g_term;
Tty Terminal::g_tty;

std::size_t Terminal::get_width() {
	TerminalInfo term;
	term_update_dim(&term);
	return term.width;
}

std::size_t Terminal::get_height() {
	TerminalInfo term;
	term_update_dim(&term);
	return term.height;
}

std::size_t Terminal::get_cursor_row() {
	CursorPos pos = {
	    .row = 0,
	    .column = 0,
	};
	term_get_cursor_pos(&pos);
	return pos.row;
}

std::size_t Terminal::get_cursor_column() {
	CursorPos pos = {
	    .row = 0,
	    .column = 0,
	};
	term_get_cursor_pos(&pos);
	return pos.column;
}

CursorPos Terminal::get_cursor_pos() {
	CursorPos pos = {
	    .row = 0,
	    .column = 0,
	};
	term_get_cursor_pos(&pos);
	return pos;
}

void Terminal::set_cursor_pos(const CursorPos& pos) {
	term_set_cursor_pos(pos);
}

void Terminal::set_cursor_pos(std::size_t row, std::size_t column) {
	term_set_cursor_pos({
	    .row = row,
	    .column = column,
	});
}

void Terminal::move_cursor_left(std::size_t count) {
	if (count) {
		std::print(stdout, "\033[{}D", count);
	}
}

void Terminal::move_cursor_right(std::size_t count) {
	if (count) {
		std::print(stdout, "\033[{}C", count);
	}
}

void Terminal::move_cursor_up(std::size_t count) {
	if (count) {
		std::print(stdout, "\033[{}A", count);
	}
}

void Terminal::move_cursor_down(std::size_t count) {
	if (count) {
		std::print(stdout, "\033[{}B", count);
	}
}

void Terminal::move_cursor_to_start_of_line() {
	print(stdout, "\r");
}

void Terminal::set_prompt(std::function<std::string(std::size_t)> prompt) {
	_prompt = std::move(prompt);
}

void Terminal::set_auto_braces(const std::string& auto_braces) {
	_auto_braces = reinterpret_cast<const byte_t*>(auto_braces.data());
}

void Terminal::set_highlighter(HighlighterFunction highlight) {
	_highlight = std::move(highlight);
}

void Terminal::set_completion_generator(CompletionGeneratorFunction generator) {
	_generate_completions = std::move(generator);
}

void Terminal::set_brace_matcher(BraceMatcherFunction matcher) {
	_braces_match = std::move(matcher);
}

void Terminal::add_history(const std::string& line) {
	auto it = _history.begin();
	while (it != _history.end()) {
		if (*it == line) {
			it = _history.erase(it);
		}
		else {
			++it;
		}
	}
	_history.push_back(line);
}

std::optional<std::string> Terminal::read_line() {
	auto mode = term_setup_mode();
	auto buffer = edit();
	term_reset_mode(mode);
	return buffer;
}

std::size_t Terminal::write(FILE* stream, const std::string& str) {
#ifdef MINT_OS_WINDOWS
	if (str.empty()) {
		return 0;
	}

	HANDLE terminal = INVALID_HANDLE_VALUE;

	if (stream == stdout) {
		terminal = GetStdHandle(STD_OUTPUT_HANDLE);
	}
	else if (stream == stderr) {
		terminal = GetStdHandle(STD_ERROR_HANDLE);
	}
	else {
		const auto amount = std::fputs(str.data(), stream);
		if (amount == EOF) {
			throw std::system_error(last_error_code());
		}
		return amount;
	}

	if (term_vt100_enabled_for_console(terminal)) {
		const auto amount = WriteMultiByteToConsoleW(terminal, str.data(), static_cast<int>(str.size()));
		if (amount == EOF) {
			throw std::system_error(last_error_code());
		}
		return amount;
	}

	std::size_t written_all = 0;
	auto view = std::string_view(str);

	for (auto pos = view.find("\033["); pos != std::string_view::npos; pos = view.find("\033[")) {
		const auto buffer = view.substr(0, pos);
		const auto written = WriteMultiByteToConsoleW(terminal, buffer.data(), static_cast<int>(buffer.size()));
		if (written == EOF) {
			throw std::system_error(last_error_code());
		}
		view = term_handle_vt100_sequence(terminal, view.substr(pos + 2));
		written_all += written;
	}

	if (!view.empty()) {
		const auto written = WriteMultiByteToConsoleW(terminal, view.data(), static_cast<int>(view.size()));
		if (written == EOF) {
			throw std::system_error(last_error_code());
		}
		written_all += written;
	}

	return written_all;
#else
	const auto amount = std::fputs(str.data(), stream);
	if (amount == EOF) {
		throw std::system_error(last_error_code());
	}
	return static_cast<std::size_t>(amount);
#endif
}

void Terminal::print(FILE* stream, const std::string& str) {
#ifdef MINT_OS_WINDOWS
	if (str.empty()) {
		return;
	}

	HANDLE terminal = INVALID_HANDLE_VALUE;

	if (stream == stdout) {
		terminal = GetStdHandle(STD_OUTPUT_HANDLE);
	}
	else if (stream == stderr) {
		terminal = GetStdHandle(STD_ERROR_HANDLE);
	}
	else {
		if (std::fputs(str.data(), stream) == EOF) {
			throw std::system_error(last_error_code());
		}
		return;
	}

	if (term_vt100_enabled_for_console(terminal)) {
		if (WriteMultiByteToConsoleW(terminal, str.data(), static_cast<int>(str.size())) == EOF) {
			throw std::system_error(last_error_code());
		}
	}
	else {
		auto view = std::string_view(str);
		for (auto pos = view.find("\033["); pos != std::string_view::npos; pos = view.find("\033[")) {
			const auto buffer = view.substr(0, pos);
			if (WriteMultiByteToConsoleW(terminal, buffer.data(), static_cast<int>(buffer.size())) == EOF) {
				throw std::system_error(last_error_code());
			}
			view = term_handle_vt100_sequence(terminal, view.substr(pos + 2));
		}
		if (!view.empty()) {
			if (WriteMultiByteToConsoleW(terminal, view.data(), static_cast<int>(view.size())) == EOF) {
				throw std::system_error(last_error_code());
			}
		}
	}
#else
	std::print(stream, "{}", str);
#endif
}

void Terminal::println(FILE* stream, const std::string& str) {
	print(stream, str);
	print(stream, "\n");
}

void Terminal::clear_to_end_of_line() {
	print(stdout, "\033[K");
}

void Terminal::clear_line() {
	print(stdout, "\r\033[K");
}

TtyEvent Terminal::wait_for_event(std::optional<std::chrono::milliseconds> timeout) {

	TtyEvent event = event_none;

	// is there a push_count back code?
	if (!g_tty.event_buffer.empty()) {
		event = g_tty.event_buffer.front();
		g_tty.event_buffer.pop();
		return event;
	}

	// read a single char/byte from a character stream
	const auto byte = read_byte(timeout);
	if (!byte) {
		return event_none;
	}

	if (byte == event_key_esc) {
		event = event_from_esc(100ms);
	}
	else if (isascii(byte)) {
		event = static_cast<TtyEvent>(byte);
	}
	else {
		// utf8 sequence
		event = static_cast<TtyEvent>(0xEE000U + byte);
	}

	auto key = event & 0x0FFFFFFFU;
	auto mods = event & 0xF0000000U;

	// treat event_key_rubout (0x7F) as event_key_backsp
	if (key == event_key_rubout) {
		event = static_cast<TtyEvent>(event_key_backsp | mods);
	}
	// ctrl+'_' is translated to '\x1F' on Linux, translate it back
	else if (key == '\x1F' && (mods & event_key_mod_alt) == 0) {
		key = '_';
		event = static_cast<TtyEvent>(event_key_mod_ctrl | '_');
	}
	// treat ctrl/shift + enter always as event_key_linefeed for portability
	else if (key == event_key_enter
	         && (mods == event_key_mod_shift || mods == event_key_mod_alt || mods == event_key_mod_ctrl)) {
		event = event_key_linefeed;
	}
	// treat ctrl+tab always as shift+tab for portability
	else if (event == (event_key_mod_ctrl | event_key_tab)) {
		event = static_cast<TtyEvent>(event_key_mod_shift | event_key_tab);
	}
	// treat ctrl+end/alt+>/alt-down and ctrl+home/alt+</alt-up always as pagedown/pageup for portability
	else if (event == (event_key_mod_alt | event_key_down) || event == (event_key_mod_alt | '>')
	         || event == (event_key_mod_ctrl | event_key_end)) {
		event = event_key_pagedown;
	}
	else if (event == (event_key_mod_alt | event_key_up) || event == (event_key_mod_alt | '<')
	         || event == (event_key_mod_ctrl | event_key_home)) {
		event = event_key_pageup;
	}

	// treat C0 codes without EVENT_KEY_MOD_CTRL
	if (key < ' ' && (mods & event_key_mod_ctrl) != 0) {
		event = static_cast<TtyEvent>(event & ~event_key_mod_ctrl);
	}

	return event;
}

TtyEvent Terminal::event_from_esc(std::optional<std::chrono::milliseconds> timeout) {

	uint32_t mods = 0;
	byte_t peek = 0;

	// lone ESC?
	if (!(peek = read_byte(timeout))) {
		return event_key_esc;
	}

	// treat ESC ESC as Alt modifier (macOS sends ESC ESC [ [A-D] for alt-<cursor>)
	if (peek == event_key_esc) {
		if (!(peek = read_byte(timeout.transform([](std::chrono::milliseconds value) {
			    return value / 10;
		    })))) {
			return static_cast<TtyEvent>(event_key_esc | event_key_mod_alt); // ESC <anychar>
		}
		mods |= event_key_mod_alt;
	}

	// CSI ?
	if (peek == '[') {
		if (!(peek = read_byte(timeout.transform([](std::chrono::milliseconds value) {
			    return value / 10;
		    })))) {
			return static_cast<TtyEvent>('[' | event_key_mod_alt); // ESC <anychar>
		}
		return event_from_csi('[', peek, mods, timeout.transform([](std::chrono::milliseconds value) {
			return value / 10;
		})); // ESC [ ...
	}

	// SS3?
	if (peek == 'O' || peek == 'o' || peek == '?' /*vt52*/) {
		const std::uint8_t c1 = peek;
		if (!(peek = read_byte(timeout.transform([](std::chrono::milliseconds value) {
			    return value / 10;
		    })))) {
			return static_cast<TtyEvent>(c1 | event_key_mod_alt); // ESC <anychar>
		}
		if (c1 == 'o') {
			// ETerm uses this for ctrl+<cursor>
			mods |= event_key_mod_ctrl;
		}
		// treat all as standard SS3 'O'
		return event_from_csi('O', peek, mods, timeout.transform([](std::chrono::milliseconds value) {
			return value / 10;
		})); // ESC [Oo?] ...
	}

	// OSC: we may get a delayed query response; ensure it is ignored
	if (peek == ']') {
		if (!(peek = read_byte(timeout.transform([](std::chrono::milliseconds value) {
			    return value / 10;
		    })))) {
			return static_cast<TtyEvent>(']' | event_key_mod_alt); // ESC <anychar>
		}
		return event_from_osc(peek, timeout.transform([](std::chrono::milliseconds value) {
			return value / 10;
		})); // ESC ] ...
	}

	// Alt+<char>
	return static_cast<TtyEvent>(peek | event_key_mod_alt); // ESC <anychar>
}

TtyEvent Terminal::event_from_osc(byte_t peek, std::optional<std::chrono::milliseconds> timeout) {

	// keep reading until termination: OSC is terminated by BELL, or ESC \ (ST)  (and STX)
	for (;;) {
		if (peek <= '\x07') { // BELL and anything below (STX, ^C, ^D)
			if (peek != '\x07') {
				g_tty.byte_buffer.push(peek);
			}
			break;
		}
		if (peek == '\x1B') {
			if (!(peek = read_byte(timeout))) {
				break;
			}
			const byte_t c1 = peek;
			if (c1 == '\\') {
				break;
			}
			g_tty.byte_buffer.push(c1);
		}
		if (!(peek = read_byte(timeout))) {
			break;
		}
	}
	return event_none;
}

//-------------------------------------------------------------
// Decode escape sequences
//-------------------------------------------------------------

static TtyEvent esc_decode_vt(uint32_t vt_code) {
	switch (vt_code) {
	case 1:
		return event_key_home;
	case 2:
		return event_key_ins;
	case 3:
		return event_key_del;
	case 4:
		return event_key_end;
	case 5:
		return event_key_pageup;
	case 6:
		return event_key_pagedown;
	case 7:
		return event_key_home;
	case 8:
		return event_key_end;
	case 10:
		return event_key_f1;
	case 11:
		return event_key_f2;
	case 12:
		return event_key_f3;
	case 13:
		return event_key_f4;
	case 14:
		return event_key_f5;
	case 15:
		return event_key_f6;
	case 16:
		return event_key_f5; // minicom
	default:
		if (vt_code >= 17 && vt_code <= 21) {
			return static_cast<TtyEvent>(event_key_f1 + 5 + (vt_code - 17));
		}
		if (vt_code >= 23 && vt_code <= 26) {
			return static_cast<TtyEvent>(event_key_f1 + 10 + (vt_code - 23));
		}
		if (vt_code >= 28 && vt_code <= 29) {
			return static_cast<TtyEvent>(event_key_f1 + 14 + (vt_code - 28));
		}
		if (vt_code >= 31 && vt_code <= 34) {
			return static_cast<TtyEvent>(event_key_f1 + 16 + (vt_code - 31));
		}
	}
	return event_none;
}

static TtyEvent esc_decode_xterm(std::uint8_t xcode) {
	// ESC [
	switch (xcode) {
	case 'A':
		return event_key_up;
	case 'B':
		return event_key_down;
	case 'C':
		return event_key_right;
	case 'D':
		return event_key_left;
	case 'E':
		return static_cast<TtyEvent>('5'); // numpad 5
	case 'F':
		return event_key_end;
	case 'H':
		return event_key_home;
	case 'Z':
		return static_cast<TtyEvent>(event_key_tab | event_key_mod_shift);
	// Freebsd:
	case 'I':
		return event_key_pageup;
	case 'L':
		return event_key_ins;
	case 'M':
		return event_key_f1;
	case 'N':
		return event_key_f2;
	case 'O':
		return event_key_f3;
	case 'P':
		return event_key_f4; // note: differs from <https://en.wikipedia.org/wiki/ANSI_escape_code#CSI_(Control_Sequence_Introducer)_sequences>
	case 'Q':
		return event_key_f5;
	case 'R':
		return event_key_f6;
	case 'S':
		return event_key_f7;
	case 'T':
		return event_key_f8;
	case 'U':
		return event_key_pagedown; // Mach
	case 'V':
		return event_key_pageup; // Mach
	case 'W':
		return event_key_f11;
	case 'X':
		return event_key_f12;
	case 'Y':
		return event_key_end; // Mach
	}
	return event_none;
}

static TtyEvent esc_decode_ss3(std::uint8_t ss3_code) {
	// ESC O
	switch (ss3_code) {
	case 'A':
		return event_key_up;
	case 'B':
		return event_key_down;
	case 'C':
		return event_key_right;
	case 'D':
		return event_key_left;
	case 'E':
		return static_cast<TtyEvent>('5'); // numpad 5
	case 'F':
		return event_key_end;
	case 'H':
		return event_key_home;
	case 'I':
		return event_key_tab;
	case 'Z':
		return static_cast<TtyEvent>(event_key_tab | event_key_mod_shift);
	case 'M':
		return event_key_linefeed;
	case 'P':
		return event_key_f1;
	case 'Q':
		return event_key_f2;
	case 'R':
		return event_key_f3;
	case 'S':
		return event_key_f4;
	// on Mach
	case 'T':
		return event_key_f5;
	case 'U':
		return event_key_f6;
	case 'V':
		return event_key_f7;
	case 'W':
		return event_key_f8;
	case 'X':
		return event_key_f9; // '=' on vt220
	case 'Y':
		return event_key_f10;
	// numpad
	case 'a':
		return event_key_up;
	case 'b':
		return event_key_down;
	case 'c':
		return event_key_right;
	case 'd':
		return event_key_left;
	case 'j':
		return static_cast<TtyEvent>('*');
	case 'k':
		return static_cast<TtyEvent>('+');
	case 'l':
		return static_cast<TtyEvent>(',');
	case 'm':
		return static_cast<TtyEvent>('-');
	case 'n':
		return event_key_del; // '.'
	case 'o':
		return static_cast<TtyEvent>('/');
	case 'p':
		return event_key_ins;
	case 'q':
		return event_key_end;
	case 'r':
		return event_key_down;
	case 's':
		return event_key_pagedown;
	case 't':
		return event_key_left;
	case 'u':
		return static_cast<TtyEvent>('5');
	case 'v':
		return event_key_right;
	case 'w':
		return event_key_home;
	case 'x':
		return event_key_up;
	case 'y':
		return event_key_pageup;
	}
	return event_none;
}

TtyEvent Terminal::event_from_csi(byte_t c1, byte_t peek, uint32_t mods0,
    std::optional<std::chrono::milliseconds> timeout) {

	// CSI starts with 0x9b (c1=='[') | ESC [ (c1=='[') | ESC [Oo?] (c1 == 'O')  /* = SS3 */

	// check for extra starter '[' (Linux sends ESC [ [ 15 ~  for F5 for example)
	if (c1 == '[' && strchr("[Oo", static_cast<char>(peek)) != nullptr) {
		std::uint8_t cx = peek;
		if (!(peek = read_byte(timeout))) {
			c1 = cx;
		}
	}

	// "special" characters ('?' is used for private sequences)
	std::uint8_t special = 0;
	if (strchr(":<=>?", static_cast<char>(peek)) != nullptr) {
		special = peek;
		if (!(peek = read_byte(timeout))) {
			g_tty.byte_buffer.push(special);
			return static_cast<TtyEvent>(c1 | event_key_mod_alt); // Alt+<anychar>
		}
	}

	static auto read_csi_num = [read_byte = &Terminal::read_byte](std::uint8_t* ppeek,
	                               std::optional<std::chrono::milliseconds> timeout) -> uint32_t {
		uint32_t i = 0;
		std::size_t count = 0;
		while (isdigit(*ppeek) && count < 16) {
			std::uint8_t digit = *ppeek - '0';
			if ((*ppeek = read_byte(timeout.transform([](std::chrono::milliseconds value) {
				    return value / 10;
			    })))) {
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
			return event_none;
		}
		num2 = read_csi_num(&peek, timeout);
	}

	// the final character (we do not allow 'intermediate characters')
	std::uint8_t final = peek;
	uint32_t modifiers = mods0;

	// Adjust special cases into standard ones.
	if ((final == '@' || final == '9') && c1 == '[' && num1 == 1) {
		// ESC [ @, ESC [ 9  : on Mach
		if (final == '@') {
			num1 = 3; // DEL
		}
		else if (final == '9') {
			num1 = 2; // INS
		}
		final = '~';
	}
	else if (final == '^' || final == '$' || final == '@') {
		// Eterm/rxvt/urxt
		if (final == '^') {
			modifiers |= event_key_mod_ctrl;
		}
		if (final == '$') {
			modifiers |= event_key_mod_shift;
		}
		if (final == '@') {
			modifiers |= event_key_mod_shift | event_key_mod_ctrl;
		}
		final = '~';
	}
	else if (c1 == '[' && final >= 'a' && final <= 'd') { // note: do not catch ESC [ .. u  (for unicode)
		// ESC [ [a-d]  : on Eterm for shift+ cursor
		modifiers |= event_key_mod_shift;
		final = 'A' + (final - 'a');
	}

	if (((c1 == 'O') || (c1 == '[' && final != '~' && final != 'u')) && (num2 == 1 && num1 > 1 && num1 <= 8)) {
		// on haiku the modifier can be parameter 1, make it parameter 2 instead
		num2 = num1;
		num1 = 1;
	}

	// parameter 2 determines the modifiers
	if (num2 > 1 && num2 <= 9) {
		if (num2 == 9) {
			num2 = 3; // iTerm2 in xterm mode
		}
		num2--;
		if (num2 & 0x1) {
			modifiers |= event_key_mod_shift;
		}
		if (num2 & 0x2) {
			modifiers |= event_key_mod_alt;
		}
		if (num2 & 0x4) {
			modifiers |= event_key_mod_ctrl;
		}
	}

	// and translate
	TtyEvent event = event_none;
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
		event = event_none;
	}

	return (event != event_none ? static_cast<TtyEvent>(event | modifiers) : event_none);
}

byte_t Terminal::read_byte(std::optional<std::chrono::milliseconds> timeout) {

	// any events in the input queue?
	if (g_tty.byte_buffer.empty()) {
		term_read_input(&g_tty, timeout);
	}

	// in our pushback buffer?
	if (!g_tty.byte_buffer.empty()) {
		const byte_t byte = g_tty.byte_buffer.front();
		g_tty.byte_buffer.pop();
		return byte;
	}

	return 0;
}

// skip an escape sequence
// <https://www.xfree86.org/current/ctlseqs.html>
static bool skip_esc(std::string_view str, std::size_t* esclen) {
	if (str.empty() || str.size() <= 1 || str[0] != '\033') {
		return false;
	}
	if (esclen) {
		*esclen = 0;
	}
	if (strchr("[PX^_]", str[1]) != nullptr) {
		// CSI (ESC [), DCS (ESC P), SOS (ESC X), PM (ESC ^), APC (ESC _), and OSC (ESC ]): terminated with a special sequence
		bool final_csi = (str[1] == '['); // CSI terminates with 0x40-0x7F; otherwise ST (bell or ESC \)
		std::size_t n = 2;
		while (str.size() > n) {
			byte_t c = str[n++];
			if ((final_csi && c >= 0x40 && c <= 0x7F) // terminating byte: @A–Z[\]^_`a–z{|}~
			    || (!final_csi && c == '\x07')        // bell
			    || (c == '\x02')) {                   // STX terminates as well
				if (esclen) {
					*esclen = n;
				}
				return true;
			}
			else if (!final_csi && c == '\x1B' && str.size() > n && str[n] == '\\') { // ST (ESC \)
				n++;
				if (esclen) {
					*esclen = n;
				}
				return true;
			}
		}
	}
	if (strchr(" #%()*+", str[1]) != nullptr) {
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
static std::size_t grapheme_column_width(std::string_view str) {
	if (str.empty()) {
		return 0;
	}
	if (static_cast<byte_t>(str.front()) < ' ') {
		return 0; // also for CSI escape sequences
	}
	std::size_t w = utf8_grapheme_code_point_count(str);
#ifdef MINT_OS_WINDOWS
	return std::max(std::size_t {1}, w); // windows console seems to use at least one column
#else
	return w;
#endif
}

// Offset to the next codepoint, treats CSI escape sequences as a single code point.
static std::tuple<std::size_t, std::size_t> next_column(std::string_view str, std::size_t pos, std::size_t column) {
	std::size_t offset = 0;
	if (pos <= str.size()) {
		if (!skip_esc(str.substr(pos), &offset)) {
			offset = utf8_code_point_length(str[pos]);
		}
	}
	if (str[pos] == '\t') {
		return {offset, term_get_tab_width(column)};
	}
	return {offset, grapheme_column_width(str.substr(pos))};
}

static std::size_t to_input_pos(std::string_view str, const CursorPos& cursor) {
	if (str.empty()) {
		return 0;
	}
	std::size_t pos = 0;
	CursorPos cur = {0, 0};
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

static CursorPos to_cursor_pos(std::string_view str, std::string_view::size_type length = std::string_view::npos) {
	CursorPos cursor = {
	    .row = 0,
	    .column = 0,
	};
	if (str.empty()) {
		return cursor;
	}
	std::size_t pos = 0;
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

namespace {

std::size_t column_count(std::string_view str, std::string_view::size_type length = std::string_view::npos) {

	if (str.empty()) {
		return 0;
	}

	length = std::min(length, str.length());
	std::size_t count = 0;
	std::size_t pos = 0;

	while (pos < length) {
		auto [offset, width] = next_column(str, pos, count);
		if (!offset) {
			break;
		}
		count += width;
		pos += offset;
	}

	return count;
}

}

std::pair<std::string_view::size_type, bool> Terminal::find_matching_brace(std::size_t brace_pos) {

	if (_braces_match) {
		return _braces_match(_input, brace_pos);
	}

	if (!_auto_braces.empty()) {
		bool balanced = true;
		auto pos = std::string_view::npos;
		const byte_t brace = _input[brace_pos];
		for (std::size_t b = 0; b < _auto_braces.size(); b += 2) {
			const std::size_t open = _auto_braces[b];
			const std::size_t close = _auto_braces[b + 1];
			std::optional<std::size_t> open_count, close_count;
			std::vector<std::size_t> close_graph;
			std::size_t count = 0;
			for (std::size_t i = 0; i < _input.size(); ++i) {
				if (_input[i] == open) {
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
				else if (_input[i] == close) {
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
		return {pos, balanced};
	}

	return {std::string_view::npos, true};
}

void Terminal::edit_insert_auto_brace(byte_t c) {
	if (_auto_braces.empty()) {
		return;
	}
	for (const byte_t* b = _auto_braces.data(); *b != 0; b += 2) {
		if (*b == c) {
			const byte_t close = b[1];
			if (*b == close && _pos < _input.size() && _input[_pos] == c) {
				_input.erase(_pos, 1);
			}
			else {
				_input.insert(_pos, 1, close);
			}
			auto [_, balanced] = find_matching_brace(_pos);
			if (!balanced) {
				_input.erase(_pos, 1);
			}
			return;
		}
		else if (b[1] == c) {
			// close brace, check if there we don't overwrite to the right
			if (_input[_pos] == c) {
				_input.erase(_pos, 1);
			}
			return;
		}
	}
}

void Terminal::edit_remove_auto_brace(std::size_t pos) {
	auto [offset, balanced] = find_matching_brace(pos);
	if (balanced && offset != std::string_view::npos && offset >= _pos) {
		_input.erase(offset, 1);
	}
}

static std::size_t indent_size(const std::string_view str, std::string_view::size_type pos) {
	std::size_t count = 0;
	auto offset = str.rfind('\n', pos - 2) + 1;
	while (str[offset++] == ' ') {
		count++;
	}
	return count;
}

void Terminal::edit_auto_indent(byte_t pre, byte_t post) {
	assert(_pos > 0 && _input[_pos - 1] == '\n');
	if (_pos > 1) {
		if (_input[_pos - 2] == pre && _input[_pos] == post) {
			std::size_t indent = indent_size(_input, _pos);
			_input.insert(_pos, indent + _indent_size, ' ');
			_pos += indent + _indent_size;
			_input.insert(_pos, 1, '\n');
			_input.insert(_pos + 1, indent, ' ');
		}
		else if (std::size_t indent = indent_size(_input, _pos)) {
			_input.insert(_pos, indent, ' ');
			_pos += indent;
		}
	}
}

bool Terminal::edit_pos_is_inside_multi_line() {
	auto pos = _input.rfind('\n');
	return pos != std::string::npos && pos > _pos;
}

bool Terminal::edit_pos_is_inside_braces() {

	if (_braces_match) {
		return !_braces_match(_input.substr(0, _pos), _pos).second;
	}

	if (!_auto_braces.empty()) {
		for (std::size_t b = 0; b < _auto_braces.size(); b += 2) {
			const std::size_t open = _auto_braces[b];
			const std::size_t close = _auto_braces[b + 1];
			std::size_t count = 0;
			for (std::size_t i = 0; i < _pos; ++i) {
				if (_input[i] == open) {
					if (open == close) {
						count = !count;
					}
					else {
						++count;
					}
				}
				else if (_input[i] == close) {
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
	return _input.find('\n') != std::string::npos;
}

void Terminal::edit_cursor_to_start() {
	_pos = 0;
}

void Terminal::edit_cursor_to_end() {
	_pos = _input.size();
}

void Terminal::edit_cursor_line_start() {
	if (!_input.empty()) {
		const auto from = _input[_pos] != '\n' ? _pos : _pos - 1;
		_pos = _input.rfind('\n', from) + 1;
	}
}

void Terminal::edit_cursor_line_end() {
	auto pos = _input.find('\n', _pos);
	if (pos == std::string::npos) {
		_pos = _input.length();
	}
	else {
		_pos = pos;
	}
}

static bool is_word_delimiter(byte_t b) {
	static const std::string g_word_delimiter = "()\"'-,:;<>~!@#$%^&*|+=[]{}~?│";
	return g_word_delimiter.find(b) != std::string::npos || std::isspace(b);
}

void Terminal::edit_cursor_prev_word() {
	auto pos = utf8_previous_code_point_byte_index(_input, _pos);
	while (pos != std::string_view::npos && is_word_delimiter(_input[pos])) {
		_pos = pos;
		pos = utf8_previous_code_point_byte_index(_input, pos);
	}
	while (pos != std::string_view::npos && !is_word_delimiter(_input[pos])) {
		_pos = pos;
		pos = utf8_previous_code_point_byte_index(_input, pos);
	}
}

void Terminal::edit_cursor_next_word() {
	while (_pos < _input.size() && !is_word_delimiter(_input[_pos])) {
		_pos = utf8_next_code_point_byte_index(_input, _pos);
	}
	while (_pos < _input.size() && is_word_delimiter(_input[_pos])) {
		_pos = utf8_next_code_point_byte_index(_input, _pos);
	}
}

void Terminal::edit_cursor_row_up() {
	CursorPos pos = to_cursor_pos(_input, _pos);
	if (pos.row == 0) {
		edit_history_prev();
	}
	else {
		pos.row--;
		_pos = to_input_pos(_input, pos);
	}
}

void Terminal::edit_cursor_row_down() {
	CursorPos pos = to_cursor_pos(_input, _pos);
	if (pos.row == _input_rows) {
		edit_history_next();
	}
	else {
		pos.row++;
		_pos = to_input_pos(_input, pos);
	}
}

void Terminal::edit_cursor_left() {
	if (_pos) {
		_pos = utf8_previous_code_point_byte_index(_input, _pos);
	}
}

void Terminal::edit_cursor_right() {
	if (_pos < _input.size()) {
		_pos = utf8_next_code_point_byte_index(_input, _pos);
	}
}

void Terminal::edit_cursor_match_brace() {
	auto [pos, _] = find_matching_brace(_pos);
	if (pos != std::string_view::npos) {
		_pos = pos;
	}
}

void Terminal::edit_delete_to_start_of_line() {
	auto from = _input.rfind('\n', _pos);
	if (from == std::string::npos) {
		from = 0;
	}
	_input.erase(from, _pos - from);
	_pos = from;
}

void Terminal::edit_delete_to_end_of_line() {
	auto to = _input.find('\n', _pos);
	if (to == std::string::npos) {
		to = _input.size();
	}
	_input.erase(_pos, to - _pos);
}

void Terminal::edit_delete_to_start_of_word() {
	auto from = _pos;
	auto pos = utf8_previous_code_point_byte_index(_input, from);
	while (pos != std::string_view::npos && is_word_delimiter(_input[pos])) {
		from = pos;
		pos = utf8_previous_code_point_byte_index(_input, pos);
	}
	while (pos != std::string_view::npos && !is_word_delimiter(_input[pos])) {
		from = pos;
		pos = utf8_previous_code_point_byte_index(_input, pos);
	}
	_input.erase(from, _pos - from);
	_pos = from;
}

void Terminal::edit_delete_to_end_of_word() {
	auto to = _pos;
	while (to < _input.size() && !is_word_delimiter(_input[to])) {
		to = utf8_next_code_point_byte_index(_input, to);
	}
	while (to < _input.size() && is_word_delimiter(_input[to])) {
		to = utf8_next_code_point_byte_index(_input, to);
	}
	_input.erase(_pos, _pos + to);
}

void Terminal::edit_delete_indent() {
	if (!_input.empty()) {
		const auto from = _input[_pos] != '\n' ? _pos : _pos - 1;
		auto pos = _input.rfind('\n', from) + 1;
		for (std::size_t i = 0; i < _indent_size && pos < _input.size() && _input[pos] == ' '; ++i) {
			_input.erase(pos, 1);
			_pos--;
		}
	}
}

void Terminal::edit_delete_char() {
	if (_pos < _input.size()) {
		edit_remove_auto_brace(_pos);
		_input.erase(_pos, utf8_code_point_length(_input[_pos]));
	}
}

void Terminal::edit_delete_all() {
	_input.clear();
	_pos = 0;
}

void Terminal::edit_backspace() {
	if (_pos) {
		edit_remove_auto_brace(_pos - 1);
	}
	if (const std::size_t pos = _pos) {
		const std::size_t prev = utf8_previous_code_point_byte_index(_input, _pos);
		_input.erase(prev, pos - prev);
		_pos = prev;
	}
}

void Terminal::edit_swap_char() {
	if (utf8_code_point_count(_input) > 1) {
		if (_pos == _input.size()) {
			const std::size_t to = utf8_previous_code_point_byte_index(_input, _pos);
			const std::size_t from = utf8_previous_code_point_byte_index(_input, to);
			_input.insert(_pos, _input.substr(from, utf8_code_point_length(_input[from])));
			_input.erase(from, utf8_code_point_length(_input[from]));
		}
		else if (_pos) {
			const std::size_t from = utf8_previous_code_point_byte_index(_input, _pos);
			_input.insert(utf8_next_code_point_byte_index(_input, _pos),
			    _input.substr(from, utf8_code_point_length(_input[from])));
			_input.erase(from, utf8_code_point_length(_input[from]));
			_pos = utf8_next_code_point_byte_index(_input, _pos);
		}
		else {
			const std::size_t to = utf8_code_point_length(_input[_pos]);
			_input.insert(utf8_next_code_point_byte_index(_input, to),
			    _input.substr(0, utf8_code_point_length(_input.front())));
			_input.erase(0, utf8_code_point_length(_input.front()));
			_pos = utf8_next_code_point_byte_index(_input, _pos);
		}
	}
}

void Terminal::edit_swap_line_up() {
	const auto pos = _input[_pos] == '\n' ? _pos - 1 : _pos;
	const auto from = _input.rfind('\n', pos) + 1;
	const auto to = _input.find('\n', pos);
	if (from > 1) {
		const auto length = to != std::string::npos ? to - from + 1 : to;
		const auto target = _input.rfind('\n', from - 2) + 1;
		const auto line = _input.substr(from, length);
		if (length != std::string::npos) {
			_input.erase(from, length);
			_input.insert(target, line);
		}
		else {
			_input.erase(from - 1, length);
			_input.insert(target, line + "\n");
		}
		edit_cursor_row_up();
	}
}

void Terminal::edit_swap_line_down() {
	const auto pos = _input[_pos] == '\n' ? _pos - 1 : _pos;
	const auto from = _input.rfind('\n', pos) + 1;
	const auto to = _input.find('\n', pos);
	if (to != std::string::npos) {
		const auto length = to - from + 1;
		const auto target = _input.find('\n', to + 1) + 1;
		const auto line = _input.substr(from, length);
		if (target) {
			_input.insert(target, line);
			_input.erase(from, length);
		}
		else {
			_input.append("\n" + line);
			_input.erase(from, length);
			_input.pop_back();
		}
		edit_cursor_row_down();
	}
}

void Terminal::edit_insert_char(byte_t c) {
	_input.insert(_pos++, 1, c);
	edit_insert_auto_brace(c);
	if (c == '\n' && _auto_braces.size() > 1) {
		edit_auto_indent(_auto_braces[0], _auto_braces[1]);
	}
}

void Terminal::edit_insert_indent() {
	_input.insert(_pos, _indent_size, ' ');
	_pos += _indent_size;
}

void Terminal::edit_clear_screen() {
	move_cursor_up(to_cursor_pos(_input, _pos).row + 1);
	for (std::size_t row = 0; row < g_term.height; ++row) {
		if (row) {
			print(stdout, "\n");
		}
		clear_line();
	}
	move_cursor_up(g_term.height);
}

void Terminal::edit_history_prev() {
	if (_history_idx > 0) {
		if (_history_idx == _history.size() - 1) {
			_history.back() = _input;
		}
		_input = _history[--_history_idx];
		_pos = _input.size();
	}
}

void Terminal::edit_history_next() {
	if (_history_idx < _history.size() - 1) {
		_input = _history[++_history_idx];
		_pos = _input.size();
	}
}

void Terminal::edit_history_search_backward() {
	/// \todo
}

void Terminal::edit_history_search_forward() {
	/// \todo
}

bool Terminal::edit_generate_completions() {

	if (_generate_completions) {
		_completions.clear();
		_completions_idx = 0;
		if (auto completions = _generate_completions(_input, _pos)) {
			_completions = std::move(*completions);
			return true;
		}
	}

	return false;
}

void Terminal::edit_refresh(bool for_validation) {

	const bool has_trailing_new_line = !_input.empty() && _input.back() == '\n';
	const CursorPos input_cursor = to_cursor_pos(_input, _pos);

	const std::string input = _highlight ? _highlight(_input, _pos) : _input;
	std::vector<std::pair<std::string::size_type, bool>> line_breaks;
	std::vector<std::string> prompts;

	_input_rows = 0;
	prompts.push_back(_prompt ? _prompt(_input_rows) : "");
	std::size_t prompt_width = column_count(prompts.back());

	// calculate rows separation including word wrap
	for (std::size_t pos = 0, column = 0; pos < input.size();) {
		if (input[pos] == '\n') {
			line_breaks.emplace_back(pos, true);
			prompts.push_back(_prompt ? _prompt(++_input_rows) : "");
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
				line_breaks.emplace_back(pos + offset, false);
				column = width - 1;
				prompt_width = 0;
			}
			pos += offset;
		}
	}

	line_breaks.emplace_back(std::string::npos, true);
	_input_rows++;

	// move cursor back to start of input
	move_cursor_down(_cursor_rows - _cursor_row - 1);
	while (--_cursor_rows) {
		clear_line();
		move_cursor_up();
	}

	std::size_t begin_row = 0;
	std::size_t end_row = _input_rows + 1;
	std::size_t begin_completion = 0;
	std::size_t end_completion = _completions.size();

	// calculate the new cursor row and total rows needed
	if (!for_validation && g_term.height < end_row + end_completion) {
		const std::size_t input_page_size = _completions.empty() ? g_term.height : 2 * (g_term.height / 3);
		if (input_cursor.row < input_page_size) {
			end_row = std::min(input_page_size, _input_rows + 1);
		}
		else {
			begin_row = (input_cursor.row / input_page_size) * input_page_size;
			end_row = std::min(begin_row + input_page_size, _input_rows + 1);
		}
		if (!_completions.empty()) {
			end_completion = g_term.height - end_row - begin_row - 2;
			if (_completions_idx >= end_completion) {
				const std::size_t completion_page_size = end_completion - 1;
				begin_completion = (_completions_idx / completion_page_size) * completion_page_size;
				end_completion = std::min(begin_completion + completion_page_size, _completions.size());
			}
		}
	}

	std::size_t from = 0;
	std::size_t row = 0;
	std::size_t next_prompt = 0;
	prompt_width = column_count(prompts[input_cursor.row]);

	// render rows
	for (auto [to, new_line] : line_breaks) {

		if (for_validation && to == std::string::npos && has_trailing_new_line) {
			break;
		}

		if (row >= begin_row && row < end_row) {

			if (row == next_prompt && row == input_cursor.row) {
				_cursor_row = _cursor_rows + ((prompt_width + input_cursor.column) / g_term.width);
			}

			if (_cursor_rows++) {
				print(stdout, "\n");
			}
			else {
				clear_line();
			}

			if (row == next_prompt) {
				print(stdout, prompts[next_prompt++]);
			}

			print(stdout, input.substr(from, to - from));
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
		_cursor_rows++;
		print(stdout, "\n");
		print(stdout, "          ⮝          ");
	}

	for (std::size_t idx = begin_completion; idx < end_completion; ++idx) {
		_cursor_rows++;
		print(stdout, "\n");
		if (idx == _completions_idx) {
			std::print(stdout, "\033[1;7m {} \033[0m", _completions[idx].token);
		}
		else {
			std::print(stdout, "\033[0m {} \033[0m", _completions[idx].token);
		}
	}

	if (end_completion < _completions.size()) {
		_cursor_rows++;
		print(stdout, "\n");
		print(stdout, "          ⮟          ");
	}

	// move cursor back to edit position
	move_cursor_to_start_of_line();
	move_cursor_up(_cursor_rows - _cursor_row - 1);
	move_cursor_right((prompt_width + input_cursor.column) % g_term.width);

#ifdef MINT_OS_UNIX
	fflush(stdout);
#endif
}

std::optional<std::string> Terminal::edit() {

	// set up an edit buffer
	_cursor_rows = 1;
	_cursor_row = 0;
	_input_rows = 1;
	_input.clear();
	_pos = 0;

	// always a history entry for the current input
	_history_idx = _history.size();
	_history.emplace_back("");

	// process keys
	TtyEvent event; // current key code

	for (bool done = false; !done;) {

		edit_refresh();
		event = wait_for_event();

		// Completion Operations
		if (!_completions.empty()) {
			Completion completion = _completions[_completions_idx];
			switch (static_cast<uint32_t>(event)) {
			// Operations that may return
			case event_key_enter:
				_input.replace(completion.offset, _pos - completion.offset, completion.token);
				_pos = completion.offset + completion.token.size();
				_completions.clear();
				_completions_idx = 0;
				continue;
			case event_key_up:
				if (_completions_idx == 0) {
					_completions_idx = _completions.size() - 1;
				}
				else {
					_completions_idx--;
				}
				continue;
			case event_key_tab:
			case event_key_down:
				_completions_idx = (_completions_idx + 1) % _completions.size();
				continue;
			case event_key_del:
			case event_key_backsp:
				g_tty.event_buffer.push(event_autotab);
				break;
			default:
				if (isascii(event) || (event & 0xEE000U) == 0xEE000U) {
					g_tty.event_buffer.push(event_autotab);
				}
				else {
					_completions.clear();
					_completions_idx = 0;
				}
				break;
			}
		}

		// Editing Operations
		switch (static_cast<uint32_t>(event)) {
		// Operations that may return
		case event_key_enter:
			if (edit_pos_is_inside_multi_line() || edit_pos_is_inside_braces()) {
				edit_insert_char('\n');
			}
			else {
				// otherwise done
				_input += '\n';
				done = true;
			}
			break;
		case event_key_ctrl_d:
			if (_input.empty()) {
				// ctrl+D on empty quits with NULL
				done = true;
				break;
			}
			edit_delete_char(); // otherwise it is like delete
			break;
		case event_key_ctrl_c:
		case event_stop:
			// ctrl+C or STOP event quits with NULL
			done = true;
			break;
		case event_key_esc:
			if (_input.empty()) {
				// ESC on empty input returns with empty input
				done = true;
				break;
			}
			edit_delete_all(); // otherwise delete the current input
			// edit_delete_line();  // otherwise delete the current line
			break;
		case event_key_bell: // ^G
			// ctrl+G cancels (and returns empty input)
			edit_delete_all();
			done = true;
			break;

		// Events
		case event_resize:
			term_update_dim(&g_term);
			break;
		case event_autotab:
			if (!edit_generate_completions()) {
				/// \todo on no completion available
			}
			break;

		// Completion, history, help
		case event_key_tab:
			if (!edit_generate_completions()) {
				edit_insert_indent();
			}
			break;
		case event_key_mod_alt | '?':
			if (!edit_generate_completions()) {
				/// \todo on no completion available
			}
			break;
		case event_key_ctrl_r:
			edit_history_search_backward();
			break;
		case event_key_ctrl_s:
			edit_history_search_forward();
			break;
		case event_key_ctrl_p:
			edit_history_prev();
			break;
		case event_key_ctrl_n:
			edit_history_next();
			break;
		case event_key_ctrl_l:
			edit_clear_screen();
			break;

		// Navigation
		case event_key_left:
		case event_key_ctrl_b:
			edit_cursor_left();
			break;
		case event_key_right:
		case event_key_ctrl_f:
			if (_pos == _input.size()) {
				if (!edit_generate_completions()) {
					/// \todo on no completion available
				}
			}
			else {
				edit_cursor_right();
			}
			break;
		case event_key_up:
			if (edit_is_multi_line()) {
				edit_cursor_row_up();
			}
			else {
				edit_history_prev();
			}
			break;
		case event_key_down:
			if (edit_is_multi_line()) {
				edit_cursor_row_down();
			}
			else {
				edit_history_next();
			}
			break;
		case event_key_home:
		case event_key_ctrl_a:
			edit_cursor_line_start();
			break;
		case event_key_end:
		case event_key_ctrl_e:
			edit_cursor_line_end();
			break;
		case event_key_mod_ctrl | event_key_left:
		case event_key_mod_shift | event_key_left:
		case event_key_mod_alt | 'b':
			edit_cursor_prev_word();
			break;
		case event_key_mod_ctrl | event_key_right:
		case event_key_mod_shift | event_key_right:
		case event_key_mod_alt | 'f':
			if (_pos == _input.size()) {
				if (!edit_generate_completions()) {
					/// \todo on no completion available
				}
			}
			else {
				edit_cursor_next_word();
			}
			break;
		case event_key_mod_ctrl | event_key_home:
		case event_key_mod_shift | event_key_home:
		case event_key_pageup:
		case event_key_mod_alt | '<':
			edit_cursor_to_start();
			break;
		case event_key_mod_ctrl | event_key_end:
		case event_key_mod_shift | event_key_end:
		case event_key_pagedown:
		case event_key_mod_alt | '>':
			edit_cursor_to_end();
			break;
		case event_key_mod_alt | 'm':
			edit_cursor_match_brace();
			break;

		// Deletion
		case event_key_backsp:
			edit_backspace();
			break;
		case event_key_del:
			edit_delete_char();
			break;
		case event_key_ctrl_w:
		case event_key_mod_alt | event_key_del:
		case event_key_mod_alt | event_key_backsp:
			edit_delete_to_start_of_word();
			break;
		case event_key_mod_alt | 'd':
			edit_delete_to_end_of_word();
			break;
		case event_key_ctrl_u:
			edit_delete_to_start_of_line();
			break;
		case event_key_ctrl_k:
			edit_delete_to_end_of_line();
			break;
		case event_key_mod_shift | event_key_tab:
			edit_delete_indent();
			break;
		case event_key_ctrl_t:
			edit_swap_char();
			break;
		case event_key_mod_ctrl | event_key_up:
			edit_swap_line_up();
			break;
		case event_key_mod_ctrl | event_key_down:
			edit_swap_line_down();
			break;

		// Editing
		case event_key_linefeed: // '\n' (ctrl+J, shift+enter)
			edit_insert_char('\n');
			break;
		default:
			if (isascii(event)) {
				edit_insert_char(static_cast<byte_t>(event));
			}
			else if (const std::size_t len = utf8_code_point_length(event)) {
				edit_insert_char(static_cast<byte_t>(event));
				for (std::size_t i = 1; i < len; ++i) {
					edit_insert_char(read_byte(0ms));
				}
			}
			break;
		}
	}

	// goto end
	_pos = _input.size();

	// refresh once more but without brace matching
	edit_refresh(true);
	print(stdout, "\n");

	// input was canceled ?
	if ((event == event_key_ctrl_d && _input.empty()) || event == event_key_ctrl_c || event == event_stop) {
		return std::nullopt;
	}

	// update history
	_history.pop_back();
	if (_input.size() > 1) {
		add_history(_input.substr(0, _input.size() - 1));
	}

	return _input;
}

bool mint::is_term(FILE* stream) {
	return isatty(fileno(stream));
}

bool mint::is_term(int fd) {
	return isatty(fd);
}
