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

#include "terminal.h"
#include "mint/memory/function_tools.h"
#include "mint/system/terminal.h"
#include "mint/system/errno.h"
#include "mint/system/string.h"

#include <consoleapi2.h>
#include <limits>
#include <list>
#include <optional>
#include <ranges>
#include <string_view>
#include <string>
#include <unordered_map>
#include <Windows.h>

namespace ntdef {

#include <ntdef.h>

}

#define BUFFER_SIZE (32 + 17)

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

using namespace mint;

namespace {

consteval int decode(std::string_view code) {
	int value = 0;
	for (const char ch : code) {
		value *= mint::decimal_base;
		value += ch - '0';
	}
	return value;
}

struct Attributes {
	std::optional<WORD> foreground;
	std::optional<WORD> background;
};

constexpr inline WORD attr_reset = std::numeric_limits<WORD>::max();
constexpr inline WORD attr_fg_mask = 0x0F;
constexpr inline WORD attr_bg_mask = 0xF0;

void set_console_attributes(HANDLE terminal, const std::list<int>& attrs) {

	static std::unordered_map<HANDLE, WORD> g_default_attributes;
	static const std::unordered_map<int, Attributes> attributes {
	    {decode(MINT_TERM_RESET),
	        {
	            .foreground = attr_reset,
	            .background = attr_reset,
	        }},
	    {decode(MINT_TERM_BOLD), {/*TODO*/}},
	    {decode(MINT_TERM_DARK), {/*TODO*/}},
	    {decode(MINT_TERM_ITALIC), {/*TODO*/}},
	    {decode(MINT_TERM_UNDERLINE), {/*TODO*/}},
	    {decode(MINT_TERM_BLINK), {/*TODO*/}},
	    {decode(MINT_TERM_REVERSE), {/*TODO*/}},
	    {decode(MINT_TERM_CONCEALED), {/*TODO*/}},
	    {decode(MINT_TERM_CROSSED), {/*TODO*/}},
	    {decode(MINT_TERM_RESET_BOLD), {/*TODO*/}},
	    {decode(MINT_TERM_RESET_DARK), {/*TODO*/}},
	    {decode(MINT_TERM_RESET_ITALIC), {/*TODO*/}},
	    {decode(MINT_TERM_RESET_UNDERLINE), {/*TODO*/}},
	    {decode(MINT_TERM_RESET_BLINK), {/*TODO*/}},
	    {decode(MINT_TERM_RESET_REVERSE), {/*TODO*/}},
	    {decode(MINT_TERM_RESET_CONCEALED), {/*TODO*/}},
	    {decode(MINT_TERM_RESET_CROSSED), {/*TODO*/}},
	    {decode(MINT_TERM_FG_GREY),
	        {
	            .foreground = 0,
	        }},
	    {decode(MINT_TERM_FG_RED),
	        {
	            .foreground = FOREGROUND_RED,
	        }},
	    {decode(MINT_TERM_FG_GREEN),
	        {
	            .foreground = FOREGROUND_GREEN,
	        }},
	    {decode(MINT_TERM_FG_YELLOW),
	        {
	            .foreground = FOREGROUND_GREEN | FOREGROUND_RED,
	        }},
	    {decode(MINT_TERM_FG_BLUE),
	        {
	            .foreground = FOREGROUND_BLUE,
	        }},
	    {decode(MINT_TERM_FG_MAGENTA),
	        {
	            .foreground = FOREGROUND_BLUE | FOREGROUND_RED,
	        }},
	    {decode(MINT_TERM_FG_CYAN),
	        {
	            .foreground = FOREGROUND_BLUE | FOREGROUND_GREEN,
	        }},
	    {decode(MINT_TERM_FG_WHITE),
	        {
	            .foreground = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED,
	        }},
	    {decode(MINT_TERM_FG_RESET),
	        {
	            .foreground = attr_reset,
	        }},
	    {decode(MINT_TERM_BG_GREY),
	        {
	            .background = 0,
	        }},
	    {decode(MINT_TERM_BG_RED),
	        {
	            .background = BACKGROUND_RED,
	        }},
	    {decode(MINT_TERM_BG_GREEN),
	        {
	            .background = BACKGROUND_GREEN,
	        }},
	    {decode(MINT_TERM_BG_YELLOW),
	        {
	            .background = BACKGROUND_GREEN | BACKGROUND_RED,
	        }},
	    {decode(MINT_TERM_BG_BLUE),
	        {
	            .background = BACKGROUND_BLUE,
	        }},
	    {decode(MINT_TERM_BG_MAGENTA),
	        {
	            .background = BACKGROUND_BLUE | BACKGROUND_RED,
	        }},
	    {decode(MINT_TERM_BG_CYAN),
	        {
	            .background = BACKGROUND_BLUE | BACKGROUND_GREEN,
	        }},
	    {decode(MINT_TERM_BG_WHITE),
	        {
	            .background = BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED,
	        }},
	    {decode(MINT_TERM_BG_RESET),
	        {
	            .background = attr_reset,
	        }},
	    {decode(MINT_TERM_FG_DARK_GRAY),
	        {
	            .foreground = FOREGROUND_INTENSITY,
	        }},
	    {decode(MINT_TERM_FG_DARK_RED),
	        {
	            .foreground = FOREGROUND_INTENSITY | BACKGROUND_RED,
	        }},
	    {decode(MINT_TERM_FG_DARK_GREEN),
	        {
	            .foreground = FOREGROUND_INTENSITY | BACKGROUND_GREEN,
	        }},
	    {decode(MINT_TERM_FG_DARK_YELLOW),
	        {
	            .foreground = FOREGROUND_INTENSITY | BACKGROUND_GREEN | BACKGROUND_RED,
	        }},
	    {decode(MINT_TERM_FG_DARK_BLUE),
	        {
	            .foreground = FOREGROUND_INTENSITY | BACKGROUND_BLUE,
	        }},
	    {decode(MINT_TERM_FG_DARK_MAGENTA),
	        {
	            .foreground = FOREGROUND_INTENSITY | BACKGROUND_BLUE | BACKGROUND_RED,
	        }},
	    {decode(MINT_TERM_FG_DARK_CYAN),
	        {
	            .foreground = FOREGROUND_INTENSITY | BACKGROUND_BLUE | BACKGROUND_GREEN,
	        }},
	    {decode(MINT_TERM_FG_DARK_WHITE),
	        {
	            .foreground = FOREGROUND_INTENSITY | BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED,
	        }},
	    {decode(MINT_TERM_BG_DARK_GREY),
	        {
	            .background = BACKGROUND_INTENSITY,
	        }},
	    {decode(MINT_TERM_BG_DARK_RED),
	        {
	            .background = BACKGROUND_INTENSITY | BACKGROUND_RED,
	        }},
	    {decode(MINT_TERM_BG_DARK_GREEN),
	        {
	            .background = BACKGROUND_INTENSITY | BACKGROUND_GREEN,
	        }},
	    {decode(MINT_TERM_BG_DARK_YELLOW),
	        {
	            .background = BACKGROUND_INTENSITY | BACKGROUND_GREEN | BACKGROUND_RED,
	        }},
	    {decode(MINT_TERM_BG_DARK_BLUE),
	        {
	            .background = BACKGROUND_INTENSITY | BACKGROUND_BLUE,
	        }},
	    {decode(MINT_TERM_BG_DARK_MAGENTA),
	        {
	            .background = BACKGROUND_INTENSITY | BACKGROUND_BLUE | BACKGROUND_RED,
	        }},
	    {decode(MINT_TERM_BG_DARK_CYAN),
	        {
	            .background = BACKGROUND_INTENSITY | BACKGROUND_BLUE | BACKGROUND_GREEN,
	        }},
	    {decode(MINT_TERM_BG_DARK_WHITE),
	        {
	            .background = BACKGROUND_INTENSITY | BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED,
	        }},
	};

	auto default_attributes = g_default_attributes.find(terminal);
	if (default_attributes == g_default_attributes.end()) {
		CONSOLE_SCREEN_BUFFER_INFO info;
		if (!GetConsoleScreenBufferInfo(terminal, &info)) {
			return;
		}
		default_attributes = g_default_attributes.emplace(terminal, info.wAttributes).first;
	}

	CONSOLE_SCREEN_BUFFER_INFO info;
	if (!GetConsoleScreenBufferInfo(terminal, &info)) {
		return;
	}

	for (const auto& attributes : std::views::transform(attrs, [](int attr) -> const Attributes& {
		     if (auto it = attributes.find(attr); it != attributes.end()) {
			     return it->second;
		     }
		     static Attributes g_default;
		     return g_default;
	     })) {
		if (attributes.foreground) {
			info.wAttributes &= ~(info.wAttributes & attr_fg_mask);
			if (*attributes.foreground == attr_reset) {
				info.wAttributes |= (default_attributes->second & attr_fg_mask);
			}
			else {
				info.wAttributes |= *attributes.foreground;
			}
		}
		if (attributes.background) {
			info.wAttributes &= ~(info.wAttributes & attr_bg_mask);
			if (*attributes.background == attr_reset) {
				info.wAttributes |= (default_attributes->second & attr_bg_mask);
			}
			else {
				info.wAttributes |= *attributes.background;
			}
		}
	}

	SetConsoleTextAttribute(terminal, info.wAttributes);
}

}

DWORD mint::term_setup_mode() {
	DWORD mode = 0;
	HANDLE hTty = GetStdHandle(STD_INPUT_HANDLE);
	GetConsoleMode(hTty, &mode);
	SetConsoleMode(hTty, ENABLE_QUICK_EDIT_MODE | ENABLE_WINDOW_INPUT);
	return mode;
}

void mint::term_reset_mode(DWORD mode) {
	SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), mode);
}

char* mint::term_readline(const char* prompt) {

	std::size_t buffer_pos = 0;
	std::size_t buffer_length = 128;
	char* buffer = static_cast<char*>(malloc(buffer_length));

	Terminal::print(stdout, prompt);

	if (buffer) {
		for (int c = getchar(); (c != '\n') && (c != EOF); c = getchar()) {
			buffer[buffer_pos++] = static_cast<char>(c);
			if (buffer_pos == buffer_length) {
				buffer_length += 128;
				buffer = static_cast<char*>(realloc(buffer, buffer_length));
			}
		}

		buffer[buffer_pos] = '\0';
	}

	return buffer;
}

namespace {

//-------------------------------------------------------------
// Push escape codes (used on Windows to insert keys)
//-------------------------------------------------------------

void tty_push_bytes(Tty* tty, const std::string& bytes) {
	for (char ch : bytes) {
		tty->byte_buffer.push(static_cast<byte_t>(ch));
	}
}

unsigned csi_mods(uint32_t mods) {
	unsigned m = 1;
	if (mods & event_key_mod_shift) {
		m += 1;
	}
	if (mods & event_key_mod_alt) {
		m += 2;
	}
	if (mods & event_key_mod_ctrl) {
		m += 4;
	}
	return m;
}

// Push ESC [ <vtcode> ; <mods> ~
void tty_cpush_csi_vt(Tty* tty, uint32_t mods, uint32_t vtcode) {
	tty_push_bytes(tty, "\033[" + std::to_string(vtcode) + ";" + std::to_string(csi_mods(mods)) + "~");
}

// push ESC [ 1 ; <mods> <xcmd>
void tty_cpush_csi_xterm(Tty* tty, uint32_t mods, char xcode) {
	tty_push_bytes(tty, "\033[1;" + std::to_string(csi_mods(mods)) + std::string(1, xcode));
}

// push ESC [ <unicode> ; <mods> u
void tty_cpush_csi_unicode(Tty* tty, uint32_t mods, uint32_t unicode) {
	if ((unicode < 0x80 && mods == 0)
	    || (mods == event_key_mod_ctrl && unicode < ' ' && unicode != event_key_tab && unicode != event_key_enter
	        && unicode != event_key_linefeed && unicode != event_key_backsp)
	    || (mods == event_key_mod_shift && unicode >= ' ' && unicode <= event_key_rubout)) {
		tty->byte_buffer.push(static_cast<byte_t>(unicode));
	}
	else if (mods == 0) {
		if (unicode < 0x0800) {
			tty->byte_buffer.push(static_cast<byte_t>((unicode >> 6 & 0x1F) | 0xC0));
			tty->byte_buffer.push(static_cast<byte_t>((unicode >> 0 & 0x3F) | 0x80));
		}
		else if (unicode < 0x010000) {
			tty->byte_buffer.push(static_cast<byte_t>((unicode >> 12 & 0x0F) | 0xE0));
			tty->byte_buffer.push(static_cast<byte_t>((unicode >> 6 & 0x3F) | 0x80));
			tty->byte_buffer.push(static_cast<byte_t>((unicode >> 0 & 0x3F) | 0x80));
		}
		else if (unicode < 0x110000) {
			tty->byte_buffer.push(static_cast<byte_t>((unicode >> 18 & 0x07) | 0xF0));
			tty->byte_buffer.push(static_cast<byte_t>((unicode >> 12 & 0x3F) | 0x80));
			tty->byte_buffer.push(static_cast<byte_t>((unicode >> 6 & 0x3F) | 0x80));
			tty->byte_buffer.push(static_cast<byte_t>((unicode >> 0 & 0x3F) | 0x80));
		}
	}
	else {
		tty_push_bytes(tty, "\033[" + std::to_string(unicode) + ";" + std::to_string(csi_mods(mods)) + "u");
	}
}

}

// Read from the console input events and push escape codes into the tty cbuffer.
void mint::term_read_input(Tty* tty, std::optional<std::chrono::milliseconds> timeout) {

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
				tty_cpush_csi_vt(tty, mods, 3);
				return;
			case VK_PRIOR:
				tty_cpush_csi_vt(tty, mods, 5);
				return; //page up
			case VK_NEXT:
				tty_cpush_csi_vt(tty, mods, 6);
				return; //page down
			case VK_TAB:
				tty_cpush_csi_unicode(tty, mods, 9);
				return;
			case VK_RETURN:
				tty_cpush_csi_unicode(tty, mods, 13);
				return;
			default:
				{
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
			tty_cpush_csi_unicode(tty, mods, chr);
			surrogate_hi = 0;
			return;
		}
		// regular character
		else {
			tty_cpush_csi_unicode(tty, mods, chr);
			return;
		}
	}
}

bool mint::term_update_dim(TerminalInfo* term) {

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

bool mint::term_get_cursor_pos(CursorPos* pos) {
	CONSOLE_SCREEN_BUFFER_INFO sbinfo;
	if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &sbinfo)) {
		pos->column = sbinfo.dwCursorPosition.X;
		pos->row = sbinfo.dwCursorPosition.Y;
		return true;
	}
	return false;
}

bool mint::term_set_cursor_pos(const CursorPos& pos) {
	COORD dwCursorPosition = {static_cast<SHORT>(pos.column), static_cast<SHORT>(pos.row)};
	return SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), dwCursorPosition);
}

std::size_t mint::term_get_tab_width(std::size_t column) {
	const std::size_t tab_width = 8; /// \todo get console info
	return tab_width - column % tab_width;
}

int mint::WriteMultiByteToConsoleW(HANDLE hConsoleOutput, const char* str, int cbMultiByte) {

	std::wstring buffer(MultiByteToWideChar(CP_UTF8, 0, str, cbMultiByte, nullptr, 0), L'\0');
	DWORD number_of_chars_written = 0;

	if (MultiByteToWideChar(CP_UTF8, 0, str, cbMultiByte, buffer.data(), static_cast<int>(buffer.length()))) {
		if (WriteConsoleW(hConsoleOutput, buffer.data(), static_cast<DWORD>(buffer.length()), &number_of_chars_written,
		        nullptr)) {
			return static_cast<int>(number_of_chars_written);
		}
	}

	errno = errno_from_error_code(last_error_code());
	return EOF;
}

int mint::WriteCharsToConsoleW(HANDLE hConsoleOutput, wchar_t wc, int cbRepeat) {

	std::wstring buffer(cbRepeat, wc);
	DWORD numberOfCharsWritten = 0;

	if (WriteConsoleW(hConsoleOutput, buffer.c_str(), static_cast<DWORD>(buffer.length()), &numberOfCharsWritten,
	        nullptr)) {
		return static_cast<int>(numberOfCharsWritten);
	}

	return EOF;
}

bool mint::term_vt100_enabled_for_console(HANDLE terminal) {
	if (DWORD mode = 0; GetConsoleMode(terminal, &mode)) {
		return SetConsoleMode(terminal, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
	}
	return false;
}

std::string_view mint::term_handle_vt100_sequence(HANDLE terminal, std::string_view str) {

	int attr = 0;
	std::list<int> attrs;

	for (std::size_t i = 0; i < str.size(); ++i) {
		if (isdigit(str[i])) {
			attr = (attr * 10) + (str[i] - '0');
		}
		else if (str[i] == ';') {
			attrs.push_back(attr);
			attr = 0;
		}
		else if (isalpha(str[i])) {
			attrs.push_back(attr);
			if (str[i] == 'm') {
				set_console_attributes(terminal, attrs);
			}
			return str.substr(i + 1);
		}
	}

	return {};
}
