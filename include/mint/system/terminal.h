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
	stdin_fileno = 0,
	stdout_fileno = 1,
	stderr_fileno = 2
};

enum TtyEvent : uint32_t {
	event_key_mod_shift     = 0x10000000U,
	event_key_mod_alt       = 0x20000000U,
	event_key_mod_ctrl      = 0x40000000U,

	event_key_none          = 0,
	event_key_ctrl_a        = 1,
	event_key_ctrl_b        = 2,
	event_key_ctrl_c        = 3,
	event_key_ctrl_d        = 4,
	event_key_ctrl_e        = 5,
	event_key_ctrl_f        = 6,
	event_key_bell          = 7,
	event_key_backsp        = 8,
	event_key_tab           = 9,
	event_key_linefeed      = 10,   // ctrl/shift + enter is considered KEY_LINEFEED
	event_key_ctrl_k        = 11,
	event_key_ctrl_l        = 12,
	event_key_enter         = 13,
	event_key_ctrl_n        = 14,
	event_key_ctrl_o        = 15,
	event_key_ctrl_p        = 16,
	event_key_ctrl_q        = 17,
	event_key_ctrl_r        = 18,
	event_key_ctrl_s        = 19,
	event_key_ctrl_t        = 20,
	event_key_ctrl_u        = 21,
	event_key_ctrl_v        = 22,
	event_key_ctrl_w        = 23,
	event_key_ctrl_x        = 24,
	event_key_ctrl_y        = 25,
	event_key_ctrl_z        = 26,
	event_key_esc           = 27,
	event_key_space         = 32,
	event_key_rubout        = 127,  // always translated to KEY_BACKSP
	event_key_unicode_max   = 0x0010FFFFU,

	event_key_virt          = 0x01000000U,
	event_key_up            = event_key_virt + 0,
	event_key_down          = event_key_virt + 1,
	event_key_left          = event_key_virt + 2,
	event_key_right         = event_key_virt + 3,
	event_key_home          = event_key_virt + 4,
	event_key_end           = event_key_virt + 5,
	event_key_del           = event_key_virt + 6,
	event_key_pageup        = event_key_virt + 7,
	event_key_pagedown      = event_key_virt + 8,
	event_key_ins           = event_key_virt + 9,

	event_key_f1            = event_key_virt + 11,
	event_key_f2            = event_key_virt + 12,
	event_key_f3            = event_key_virt + 13,
	event_key_f4            = event_key_virt + 14,
	event_key_f5            = event_key_virt + 15,
	event_key_f6            = event_key_virt + 16,
	event_key_f7            = event_key_virt + 17,
	event_key_f8            = event_key_virt + 18,
	event_key_f9            = event_key_virt + 19,
	event_key_f10           = event_key_virt + 20,
	event_key_f11           = event_key_virt + 21,
	event_key_f12           = event_key_virt + 22,

	event_base    = 0x02000000U,
	event_resize  = event_base + 1,
	event_autotab = event_base + 2,
	event_stop    = event_base + 3
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
	void set_higlighter(std::function<std::string(std::string_view, std::string_view::size_type)> highlight);
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
