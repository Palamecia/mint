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

#ifndef MINT_SYSTEM_TERMINAL_H
#define MINT_SYSTEM_TERMINAL_H

#include "mint/config.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>
#include <string>
#include <chrono>
#include <queue>

#define MINT_TERM_OPT_JOIN_15(__opt) __opt
#define MINT_TERM_OPT_JOIN_14(__opt, ...) __opt __VA_OPT__(";" MINT_TERM_OPT_JOIN_15(__VA_ARGS__))
#define MINT_TERM_OPT_JOIN_13(__opt, ...) __opt __VA_OPT__(";" MINT_TERM_OPT_JOIN_14(__VA_ARGS__))
#define MINT_TERM_OPT_JOIN_12(__opt, ...) __opt __VA_OPT__(";" MINT_TERM_OPT_JOIN_13(__VA_ARGS__))
#define MINT_TERM_OPT_JOIN_11(__opt, ...) __opt __VA_OPT__(";" MINT_TERM_OPT_JOIN_12(__VA_ARGS__))
#define MINT_TERM_OPT_JOIN_10(__opt, ...) __opt __VA_OPT__(";" MINT_TERM_OPT_JOIN_11(__VA_ARGS__))
#define MINT_TERM_OPT_JOIN_9(__opt, ...) __opt __VA_OPT__(";" MINT_TERM_OPT_JOIN_10(__VA_ARGS__))
#define MINT_TERM_OPT_JOIN_8(__opt, ...) __opt __VA_OPT__(";" MINT_TERM_OPT_JOIN_9(__VA_ARGS__))
#define MINT_TERM_OPT_JOIN_7(__opt, ...) __opt __VA_OPT__(";" MINT_TERM_OPT_JOIN_8(__VA_ARGS__))
#define MINT_TERM_OPT_JOIN_6(__opt, ...) __opt __VA_OPT__(";" MINT_TERM_OPT_JOIN_7(__VA_ARGS__))
#define MINT_TERM_OPT_JOIN_5(__opt, ...) __opt __VA_OPT__(";" MINT_TERM_OPT_JOIN_6(__VA_ARGS__))
#define MINT_TERM_OPT_JOIN_4(__opt, ...) __opt __VA_OPT__(";" MINT_TERM_OPT_JOIN_5(__VA_ARGS__))
#define MINT_TERM_OPT_JOIN_3(__opt, ...) __opt __VA_OPT__(";" MINT_TERM_OPT_JOIN_4(__VA_ARGS__))
#define MINT_TERM_OPT_JOIN_2(__opt, ...) __opt __VA_OPT__(";" MINT_TERM_OPT_JOIN_3(__VA_ARGS__))
#define MINT_TERM_OPT_JOIN_1(__opt, ...) __opt __VA_OPT__(";" MINT_TERM_OPT_JOIN_2(__VA_ARGS__))
#define MINT_TERM_OPT_JOIN(__opt, ...) __opt __VA_OPT__(";" MINT_TERM_OPT_JOIN_1(__VA_ARGS__))
#define MINT_TERM_OPT(...) "\033[" MINT_TERM_OPT_JOIN(__VA_ARGS__) "m"
#define MINT_TERM_STDSTR(...) __VA_ARGS__ + "\033[0m"
#define MINT_TERM_STR(...) __VA_ARGS__ "\033[0m"

#define MINT_TERM_RESET "0"
#define MINT_TERM_BOLD "1"
#define MINT_TERM_DARK "2"
#define MINT_TERM_ITALIC "3"
#define MINT_TERM_UNDERLINE "4"
#define MINT_TERM_BLINK "5"
#define MINT_TERM_REVERSE "7"
#define MINT_TERM_CONCEALED "8"
#define MINT_TERM_CROSSED "9"
#define MINT_TERM_RESET_BOLD "21"
#define MINT_TERM_RESET_DARK "22"
#define MINT_TERM_RESET_ITALIC "23"
#define MINT_TERM_RESET_UNDERLINE "24"
#define MINT_TERM_RESET_BLINK "25"
#define MINT_TERM_RESET_REVERSE "27"
#define MINT_TERM_RESET_CONCEALED "28"
#define MINT_TERM_RESET_CROSSED "29"
#define MINT_TERM_FG_GREY "30"
#define MINT_TERM_FG_RED "31"
#define MINT_TERM_FG_GREEN "32"
#define MINT_TERM_FG_YELLOW "33"
#define MINT_TERM_FG_BLUE "34"
#define MINT_TERM_FG_MAGENTA "35"
#define MINT_TERM_FG_CYAN "36"
#define MINT_TERM_FG_WHITE "37"
#define MINT_TERM_FG_RESET "39"
#define MINT_TERM_BG_GREY "40"
#define MINT_TERM_BG_RED "41"
#define MINT_TERM_BG_GREEN "42"
#define MINT_TERM_BG_YELLOW "43"
#define MINT_TERM_BG_BLUE "44"
#define MINT_TERM_BG_MAGENTA "45"
#define MINT_TERM_BG_CYAN "46"
#define MINT_TERM_BG_WHITE "47"
#define MINT_TERM_BG_RESET "49"
#define MINT_TERM_FG_DARK_GRAY "90"
#define MINT_TERM_FG_DARK_RED "91"
#define MINT_TERM_FG_DARK_GREEN "92"
#define MINT_TERM_FG_DARK_YELLOW "93"
#define MINT_TERM_FG_DARK_BLUE "94"
#define MINT_TERM_FG_DARK_MAGENTA "95"
#define MINT_TERM_FG_DARK_CYAN "96"
#define MINT_TERM_FG_DARK_WHITE "97"
#define MINT_TERM_BG_DARK_GREY "100"
#define MINT_TERM_BG_DARK_RED "101"
#define MINT_TERM_BG_DARK_GREEN "102"
#define MINT_TERM_BG_DARK_YELLOW "103"
#define MINT_TERM_BG_DARK_BLUE "104"
#define MINT_TERM_BG_DARK_MAGENTA "105"
#define MINT_TERM_BG_DARK_CYAN "106"
#define MINT_TERM_BG_DARK_WHITE "107"

namespace mint {

enum StdStreamFileNo : std::uint8_t {
	stdin_file_no = 0,
	stdout_file_no = 1,
	stderr_file_no = 2
};

enum TtyEvent : std::uint32_t {
	event_none = 0,
	event_key_ctrl_a = 1,
	event_key_ctrl_b = 2,
	event_key_ctrl_c = 3,
	event_key_ctrl_d = 4,
	event_key_ctrl_e = 5,
	event_key_ctrl_f = 6,
	event_key_bell = 7,
	event_key_backsp = 8,
	event_key_tab = 9,
	event_key_linefeed = 10, // ctrl/shift + enter is considered key_linefeed
	event_key_ctrl_k = 11,
	event_key_ctrl_l = 12,
	event_key_enter = 13,
	event_key_ctrl_n = 14,
	event_key_ctrl_o = 15,
	event_key_ctrl_p = 16,
	event_key_ctrl_q = 17,
	event_key_ctrl_r = 18,
	event_key_ctrl_s = 19,
	event_key_ctrl_t = 20,
	event_key_ctrl_u = 21,
	event_key_ctrl_v = 22,
	event_key_ctrl_w = 23,
	event_key_ctrl_x = 24,
	event_key_ctrl_y = 25,
	event_key_ctrl_z = 26,
	event_key_esc = 27,
	event_key_space = 32,
	event_key_rubout = 127, // always translated to key_backsp
	event_key_unicode_max = 0x0010ffffU,

	event_key_virt = 0x01000000U,
	event_key_up = event_key_virt + 0,
	event_key_down = event_key_virt + 1,
	event_key_left = event_key_virt + 2,
	event_key_right = event_key_virt + 3,
	event_key_home = event_key_virt + 4,
	event_key_end = event_key_virt + 5,
	event_key_del = event_key_virt + 6,
	event_key_pageup = event_key_virt + 7,
	event_key_pagedown = event_key_virt + 8,
	event_key_ins = event_key_virt + 9,

	event_key_f1 = event_key_virt + 11,
	event_key_f2 = event_key_virt + 12,
	event_key_f3 = event_key_virt + 13,
	event_key_f4 = event_key_virt + 14,
	event_key_f5 = event_key_virt + 15,
	event_key_f6 = event_key_virt + 16,
	event_key_f7 = event_key_virt + 17,
	event_key_f8 = event_key_virt + 18,
	event_key_f9 = event_key_virt + 19,
	event_key_f10 = event_key_virt + 20,
	event_key_f11 = event_key_virt + 21,
	event_key_f12 = event_key_virt + 22,

	event_key_mod_shift = 0x10000000U,
	event_key_mod_alt = 0x20000000U,
	event_key_mod_ctrl = 0x40000000U,

	event_base = 0x02000000U,
	event_resize = event_base + 1,
	event_autotab = event_base + 2,
	event_stop = event_base + 3
};

struct Completion {
	std::string::size_type offset;
	std::string token;
	std::string hint;
};

struct Tty {
	std::queue<TtyEvent> event_buffer;
	std::queue<byte_t> byte_buffer;
};

struct TerminalInfo {
	std::size_t width = 80;
	std::size_t height = 25;
};

struct CursorPos {
	std::size_t row;
	std::size_t column;
};

class MINT_EXPORT Terminal {
public:
	using HighlighterFunction = std::function<std::string(std::string_view, std::string_view::size_type)>;
	using CompletionGeneratorFunction =
	    std::function<std::optional<std::vector<Completion>>(std::string_view, std::string_view::size_type)>;
	using BraceMatcherFunction =
	    std::function<std::pair<std::string_view::size_type, bool>(std::string_view, std::string_view::size_type)>;

	Terminal() = default;

	static std::size_t get_width();
	static std::size_t get_height();

	static std::size_t get_cursor_row();
	static std::size_t get_cursor_column();

	static CursorPos get_cursor_pos();
	static void set_cursor_pos(const CursorPos& pos);
	static void set_cursor_pos(std::size_t row, std::size_t column);
	static void move_cursor_left(std::size_t count = 1);
	static void move_cursor_right(std::size_t count = 1);
	static void move_cursor_up(std::size_t count = 1);
	static void move_cursor_down(std::size_t count = 1);
	static void move_cursor_to_start_of_line();

	void set_prompt(std::function<std::string(std::size_t)> prompt);
	void set_auto_braces(const std::string& auto_braces);
	void set_highlighter(HighlighterFunction highlight);
	void set_completion_generator(CompletionGeneratorFunction generator);
	void set_brace_matcher(BraceMatcherFunction matcher);

	void add_history(const std::string& line);
	std::optional<std::string> read_line();

	static std::size_t write(FILE* stream, const std::string& str);
	static void print(FILE* stream, const std::string& str);
	static void println(FILE* stream, const std::string& str);

	static void clear_to_end_of_line();
	static void clear_line();

protected:
	static TtyEvent wait_for_event(std::optional<std::chrono::milliseconds> timeout = std::nullopt);
	static TtyEvent event_from_esc(std::optional<std::chrono::milliseconds> timeout);
	static TtyEvent event_from_osc(byte_t peek, std::optional<std::chrono::milliseconds> timeout);
	static TtyEvent event_from_csi(byte_t c1, byte_t peek, uint32_t mods0,
	    std::optional<std::chrono::milliseconds> timeout);
	static byte_t read_byte(std::optional<std::chrono::milliseconds> timeout);

	std::pair<std::string_view::size_type, bool> find_matching_brace(std::size_t brace_pos);
	void edit_insert_auto_brace(byte_t c);
	void edit_remove_auto_brace(std::size_t pos);
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
	static TerminalInfo g_term;
	static Tty g_tty;

	std::string _input;
	std::size_t _pos = 0;
	std::size_t _input_rows = 1;
	std::size_t _cursor_row = 0;
	std::size_t _cursor_rows = 1;
	std::size_t _indent_size = 4;
	std::size_t _history_idx = 0;
	std::vector<std::string> _history;
	std::size_t _completions_idx = 0;
	std::vector<Completion> _completions;
	std::function<std::string(std::size_t)> _prompt;
	std::basic_string<byte_t> _auto_braces;
	HighlighterFunction _highlight;
	BraceMatcherFunction _braces_match;
	CompletionGeneratorFunction _generate_completions;
};

MINT_EXPORT bool is_term(FILE* stream);
MINT_EXPORT bool is_term(int fd);

}

#endif // MINT_SYSTEM_TERMINAL_H
