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

#include <sys/select.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <cstring>

using namespace mint;
using namespace std;

termios mint::term_setup_mode() {
	termios mode;
	tcgetattr(STDIN_FILE_NO, &mode);
	termios raw_mode = mode;
	// input: no break signal, no \r to \n, no parity check, no 8-bit to 7-bit, no flow control
	raw_mode.c_iflag &= ~(unsigned long)(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	// control: allow 8-bit
	raw_mode.c_cflag |= CS8;
	// local: no echo, no line-by-line (canonical), no extended input processing, no signals for ^z,^c
	raw_mode.c_lflag &= ~(unsigned long)(ECHO | ICANON | IEXTEN | ISIG);
	// 1 byte at a time, no delay
	raw_mode.c_cc[VTIME] = 0;
	raw_mode.c_cc[VMIN] = 1;
	tcsetattr(STDIN_FILE_NO, TCSAFLUSH, &raw_mode);
	return mode;
}

void mint::term_reset_mode(termios mode) {
	tcsetattr(STDIN_FILE_NO, TCSAFLUSH, &mode);
}

bool mint::term_update_dim(TerminalInfo *term) {

	ssize_t cols = 0;
	ssize_t rows = 0;

	struct winsize ws;
	if (ioctl(STDOUT_FILE_NO, TIOCGWINSZ, &ws) >= 0) {
		// ioctl succeeded
		cols = ws.ws_col; // debuggers return 0 for the column
		rows = ws.ws_row;
	}
	else {
		// determine width by querying the cursor position
		CursorPos pos = Terminal::get_cursor_pos();
		Terminal::set_cursor_pos(999, 999);
		CursorPos pos1 = Terminal::get_cursor_pos();
		Terminal::set_cursor_pos(pos);
		cols = pos1.column;
		rows = pos1.row;
	}

	// update width and return whether it changed.
	bool changed = (term->width != cols || term->height != rows);
	if (cols > 0) {
		term->width = cols;
		term->height = rows;
	}
	return changed;
}

bool mint::term_get_cursor_pos(CursorPos *pos) {
	char buf[128];
	auto mode = term_setup_mode();
	if (fputs("\033[6n", stdout) == EOF) {
		term_reset_mode(mode);
		return false;
	}
	if (fflush(stdout) == EOF) {
		term_reset_mode(mode);
		return false;
	}
	for (size_t len = 0, count = 0; !len || buf[len - 1] != 'R'; len += count) {
		if (!(count = read(STDIN_FILE_NO, &buf[len], 1))) {
			term_reset_mode(mode);
			return false;
		}
	}
	if (!sscanf(buf + 2, "%zd;%zd", &pos->row, &pos->column)) {
		term_reset_mode(mode);
		return false;
	}
	term_reset_mode(mode);
	return true;
}

bool mint::term_set_cursor_pos(const CursorPos &pos) {
	return fprintf(stdout, "\033[%zd;%zdH", pos.row, pos.column) != EOF;
}

size_t mint::term_get_tab_width(size_t column) {
	const size_t tab_width = 8; /// \todo get console info
	return tab_width - column % tab_width;
}

// non blocking read -- with a small timeout used for reading escape sequences.
void mint::term_read_input(Tty *tty, optional<chrono::milliseconds> timeout) {

	// blocking read?
	if (!timeout.has_value()) {
		char c = 0;
		if (read(STDIN_FILE_NO, &c, 1) == 1) {
			tty->byte_buffer.push(c);
		}
		return;
	}

	// if supported, peek first if any char is available.
#if defined(FIONREAD)
	{
		int navail = 0;
		if (ioctl(0, FIONREAD, &navail) == 0) {
			if (navail >= 1) {
				char c = 0;
				if (read(STDIN_FILE_NO, &c, 1) == 1) {
					tty->byte_buffer.push(c);
				}
				return;
			}
			else if (!timeout.value_or(0ms).count()) {
				return; // return early if there is no input available (with a zero timeout)
			}
		}
	}
#endif

	// we can use select to detect when input becomes available
	fd_set readset;
	struct timeval time;
	FD_ZERO(&readset);
	FD_SET(STDIN_FILE_NO, &readset);
	time.tv_sec = (timeout.has_value() ? timeout.value().count() / 1000 : 0);
	time.tv_usec = (timeout.has_value() ? 1000 * (timeout.value().count() % 1000) : 0);
	if (select(STDIN_FILE_NO + 1, &readset, nullptr, nullptr, &time) == 1) {
		char c = 0;
		if (read(STDIN_FILE_NO, &c, 1) == 1) {
			tty->byte_buffer.push(c);
		}
	}
}
