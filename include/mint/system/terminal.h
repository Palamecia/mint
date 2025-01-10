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

#ifndef MINT_TERMINAL_H
#define MINT_TERMINAL_H

#include "mint/system/stdio.h"
#include "mint/config.h"

#include <functional>
#include <optional>
#include <vector>
#include <string>
#include <chrono>
#include <queue>

#define MINT_TERM_RESET       "\033[0m"
#define MINT_TERM_BOLD        "\033[1m"
#define MINT_TERM_DARK        "\033[2m"
#define MINT_TERM_ITALIC      "\033[3m"
#define MINT_TERM_UNDERLINE   "\033[4m"
#define MINT_TERM_BLINK       "\033[5m"
#define MINT_TERM_REVERSE     "\033[7m"
#define MINT_TERM_CONCEALED   "\033[8m"
#define MINT_TERM_CROSSED     "\033[9m"
#define MINT_TERM_FG_GREY     "\033[30m"
#define MINT_TERM_FG_RED      "\033[31m"
#define MINT_TERM_FG_GREEN    "\033[32m"
#define MINT_TERM_FG_YELLOW   "\033[33m"
#define MINT_TERM_FG_BLUE     "\033[34m"
#define MINT_TERM_FG_MAGENTA  "\033[35m"
#define MINT_TERM_FG_CYAN     "\033[36m"
#define MINT_TERM_FG_WHITE    "\033[37m"
#define MINT_TERM_BG_GREY     "\033[40m"
#define MINT_TERM_BG_RED      "\033[41m"
#define MINT_TERM_BG_GREEN    "\033[42m"
#define MINT_TERM_BG_YELLOW   "\033[43m"
#define MINT_TERM_BG_BLUE     "\033[44m"
#define MINT_TERM_BG_MAGENTA  "\033[45m"
#define MINT_TERM_BG_CYAN     "\033[46m"
#define MINT_TERM_BG_WHITE    "\033[47m"

#define MINT_TERM_RESET_OPTION           "0"
#define MINT_TERM_BOLD_OPTION            "1"
#define MINT_TERM_DARK_OPTION            "2"
#define MINT_TERM_ITALIC_OPTION          "3"
#define MINT_TERM_UNDERLINE_OPTION       "4"
#define MINT_TERM_BLINK_OPTION           "5"
#define MINT_TERM_REVERSE_OPTION         "7"
#define MINT_TERM_CONCEALED_OPTION       "8"
#define MINT_TERM_CROSSED_OPTION         "9"
#define MINT_TERM_FG_GREY_WITH(_opt)     "\033[" _opt ";30m"
#define MINT_TERM_FG_RED_WITH(_opt)      "\033[" _opt ";31m"
#define MINT_TERM_FG_GREEN_WITH(_opt)    "\033[" _opt ";32m"
#define MINT_TERM_FG_YELLOW_WITH(_opt)   "\033[" _opt ";33m"
#define MINT_TERM_FG_BLUE_WITH(_opt)     "\033[" _opt ";34m"
#define MINT_TERM_FG_MAGENTA_WITH(_opt)  "\033[" _opt ";35m"
#define MINT_TERM_FG_CYAN_WITH(_opt)     "\033[" _opt ";36m"
#define MINT_TERM_FG_WHITE_WITH(_opt)    "\033[" _opt ";37m"
#define MINT_TERM_BG_GREY_WITH(_opt)     "\033[" _opt ";40m"
#define MINT_TERM_BG_RED_WITH(_opt)      "\033[" _opt ";41m"
#define MINT_TERM_BG_GREEN_WITH(_opt)    "\033[" _opt ";42m"
#define MINT_TERM_BG_YELLOW_WITH(_opt)   "\033[" _opt ";43m"
#define MINT_TERM_BG_BLUE_WITH(_opt)     "\033[" _opt ";44m"
#define MINT_TERM_BG_MAGENTA_WITH(_opt)  "\033[" _opt ";45m"
#define MINT_TERM_BG_CYAN_WITH(_opt)     "\033[" _opt ";46m"
#define MINT_TERM_BG_WHITE_WITH(_opt)    "\033[" _opt ";47m"

namespace mint {

enum StdStreamFileNo {
	STDIN_FILE_NO = 0,
	STDOUT_FILE_NO = 1,
	STDERR_FILE_NO = 2
};

enum TtyEvent : uint32_t {
	EVENT_KEY_MOD_SHIFT     = 0x10000000U,
	EVENT_KEY_MOD_ALT       = 0x20000000U,
	EVENT_KEY_MOD_CTRL      = 0x40000000U,

	EVENT_KEY_NONE          = 0,
	EVENT_KEY_CTRL_A        = 1,
	EVENT_KEY_CTRL_B        = 2,
	EVENT_KEY_CTRL_C        = 3,
	EVENT_KEY_CTRL_D        = 4,
	EVENT_KEY_CTRL_E        = 5,
	EVENT_KEY_CTRL_F        = 6,
	EVENT_KEY_BELL          = 7,
	EVENT_KEY_BACKSP        = 8,
	EVENT_KEY_TAB           = 9,
	EVENT_KEY_LINEFEED      = 10,   // ctrl/shift + enter is considered KEY_LINEFEED
	EVENT_KEY_CTRL_K        = 11,
	EVENT_KEY_CTRL_L        = 12,
	EVENT_KEY_ENTER         = 13,
	EVENT_KEY_CTRL_N        = 14,
	EVENT_KEY_CTRL_O        = 15,
	EVENT_KEY_CTRL_P        = 16,
	EVENT_KEY_CTRL_Q        = 17,
	EVENT_KEY_CTRL_R        = 18,
	EVENT_KEY_CTRL_S        = 19,
	EVENT_KEY_CTRL_T        = 20,
	EVENT_KEY_CTRL_U        = 21,
	EVENT_KEY_CTRL_V        = 22,
	EVENT_KEY_CTRL_W        = 23,
	EVENT_KEY_CTRL_X        = 24,
	EVENT_KEY_CTRL_Y        = 25,
	EVENT_KEY_CTRL_Z        = 26,
	EVENT_KEY_ESC           = 27,
	EVENT_KEY_SPACE         = 32,
	EVENT_KEY_RUBOUT        = 127,  // always translated to KEY_BACKSP
	EVENT_KEY_UNICODE_MAX   = 0x0010FFFFU,

	EVENT_KEY_VIRT          = 0x01000000U,
	EVENT_KEY_UP            = EVENT_KEY_VIRT + 0,
	EVENT_KEY_DOWN          = EVENT_KEY_VIRT + 1,
	EVENT_KEY_LEFT          = EVENT_KEY_VIRT + 2,
	EVENT_KEY_RIGHT         = EVENT_KEY_VIRT + 3,
	EVENT_KEY_HOME          = EVENT_KEY_VIRT + 4,
	EVENT_KEY_END           = EVENT_KEY_VIRT + 5,
	EVENT_KEY_DEL           = EVENT_KEY_VIRT + 6,
	EVENT_KEY_PAGEUP        = EVENT_KEY_VIRT + 7,
	EVENT_KEY_PAGEDOWN      = EVENT_KEY_VIRT + 8,
	EVENT_KEY_INS           = EVENT_KEY_VIRT + 9,

	EVENT_KEY_F1            = EVENT_KEY_VIRT + 11,
	EVENT_KEY_F2            = EVENT_KEY_VIRT + 12,
	EVENT_KEY_F3            = EVENT_KEY_VIRT + 13,
	EVENT_KEY_F4            = EVENT_KEY_VIRT + 14,
	EVENT_KEY_F5            = EVENT_KEY_VIRT + 15,
	EVENT_KEY_F6            = EVENT_KEY_VIRT + 16,
	EVENT_KEY_F7            = EVENT_KEY_VIRT + 17,
	EVENT_KEY_F8            = EVENT_KEY_VIRT + 18,
	EVENT_KEY_F9            = EVENT_KEY_VIRT + 19,
	EVENT_KEY_F10           = EVENT_KEY_VIRT + 20,
	EVENT_KEY_F11           = EVENT_KEY_VIRT + 21,
	EVENT_KEY_F12           = EVENT_KEY_VIRT + 22,

	EVENT_BASE    = 0x02000000U,
	EVENT_RESIZE  = EVENT_BASE + 1,
	EVENT_AUTOTAB = EVENT_BASE + 2,
	EVENT_STOP    = EVENT_BASE + 3
};

struct completion_t {
	std::string::size_type offset;
	std::string token;
	std::string hint;
};

struct tty_t {
	std::queue<TtyEvent> event_buffer;
	std::queue<byte_t> byte_buffer;
};

struct term_t {
	size_t width = 80;
	size_t height = 25;
};

struct cursor_pos_t {
	size_t row;
	size_t column;
};

class MINT_EXPORT Terminal {
public:
	Terminal() = default;

	static size_t get_width();
	static size_t get_height();

	static size_t get_cursor_row();
	static size_t get_cursor_column();

	static cursor_pos_t get_cursor_pos();
	static void set_cursor_pos(const cursor_pos_t &pos);
	static void set_cursor_pos(size_t row, size_t column);
	static void move_cursor_left(size_t count = 1);
	static void move_cursor_right(size_t count = 1);
	static void move_cursor_up(size_t count = 1);
	static void move_cursor_down(size_t count = 1);
	static void move_cursor_to_start_of_line();

	void set_prompt(std::function<std::string(size_t)> prompt);
	void set_auto_braces(const std::string &auto_braces);
	void set_highlighter(std::function<std::string(std::string_view, std::string_view::size_type)> highlight);
	void set_completion_generator(std::function<bool(std::string_view, std::string_view::size_type, std::vector<completion_t> &)> generator);
	void set_brace_matcher(std::function<std::pair<std::string_view::size_type, bool>(std::string_view, std::string_view::size_type)> matcher);

	void add_history(const std::string &line);
	std::optional<std::string> read_line();

	static int printf(FILE *stream, const char *format, ...) __attribute__((format(printf, 2, 3)));
	static int vprintf(FILE *stream, const char *format, va_list args);
	static int print(FILE *stream, const char *str);

	static void clear_to_end_of_line();
	static void clear_line();

protected:
	static TtyEvent wait_for_event(std::optional<std::chrono::milliseconds> timeout = std::nullopt);
	static TtyEvent event_from_esc(std::optional<std::chrono::milliseconds> timeout);
	static TtyEvent event_from_osc(byte_t peek, std::optional<std::chrono::milliseconds> timeout);
	static TtyEvent event_from_csi(byte_t c1, byte_t peek, uint32_t mods0, std::optional<std::chrono::milliseconds> timeout);
	static byte_t read_byte(std::optional<std::chrono::milliseconds> timeout);

	std::pair<std::string_view::size_type, bool> find_matching_brace(size_t brace_pos);
	void edit_insert_auto_brace(byte_t c);
	void edit_remove_auto_brace(size_t pos);
	void edit_auto_indent(byte_t pre, byte_t post);

	bool edit_pos_is_inside_multi_line();
	bool edit_pos_is_inside_braces();
	bool edit_is_multi_line();

	void edit_cursor_to_start();
	void edit_cursor_to_end();
	void edit_cursor_line_start();
	void edit_cursor_line_end();
	void edit_cursor_prev_word();
	void edit_cursor_next_word();
	void edit_cursor_row_up();
	void edit_cursor_row_down();
	void edit_cursor_left();
	void edit_cursor_right();
	void edit_cursor_match_brace();

	void edit_delete_to_start_of_line();
	void edit_delete_to_end_of_line();
	void edit_delete_to_start_of_word();
	void edit_delete_to_end_of_word();
	void edit_delete_indent();
	void edit_delete_char();
	void edit_delete_all();
	void edit_backspace();
	void edit_swap_char();
	void edit_swap_line_up();
	void edit_swap_line_down();
	void edit_insert_char(byte_t c);
	void edit_insert_indent();
	void edit_clear_screen();

	void edit_history_prev();
	void edit_history_next();
	void edit_history_search_backward();
	void edit_history_search_forward();

	bool edit_generate_completions();
	void edit_refresh(bool for_validation = false);

	std::optional<std::string> edit();

private:
	static term_t g_term;
	static tty_t g_tty;

	std::string m_input;
	size_t m_pos = 0;
	size_t m_input_rows = 1;
	size_t m_cursor_row = 0;
	size_t m_cursor_rows = 1;
	size_t m_indent_size = 4;
	size_t m_history_idx = 0;
	std::vector<std::string> m_history;
	size_t m_completions_idx = 0;
	std::vector<completion_t> m_completions;
	std::function<std::string(size_t)> m_prompt;
	std::basic_string<byte_t> m_auto_braces;
	std::function<std::string(std::string_view, std::string_view::size_type)> m_highlight;
	std::function<std::pair<std::string_view::size_type, bool>(std::string_view, std::string_view::size_type)> m_braces_match;
	std::function<bool(std::string_view, std::string_view::size_type, std::vector<completion_t> &)> m_generate_completions;
};

MINT_EXPORT bool is_term(FILE *stream);
MINT_EXPORT bool is_term(int fd);

}

#endif // MINT_TERMINAL_H
