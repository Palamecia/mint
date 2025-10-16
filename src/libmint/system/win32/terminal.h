#ifndef WIN32_TERMINAL_H
#define WIN32_TERMINAL_H

#include "mint/config.h"

#include <Windows.h>
#include <optional>
#include <chrono>
#include <string_view>

namespace mint {

struct Tty;
struct TerminalInfo;
struct CursorPos;

DWORD term_setup_mode();
void term_reset_mode(DWORD mode);

char* term_readline(const char* prompt);
void term_read_input(Tty* tty, std::optional<std::chrono::milliseconds> timeout = std::nullopt);

bool term_update_dim(TerminalInfo* term);
bool term_get_cursor_pos(CursorPos* pos);
bool term_set_cursor_pos(const CursorPos& pos);

std::size_t term_get_tab_width(std::size_t column);

int WriteMultiByteToConsoleW(HANDLE hConsoleOutput, const char* str, int cbMultiByte = -1);
int WriteCharsToConsoleW(HANDLE hConsoleOutput, wchar_t wc, int cbRepeat = 1);

bool term_vt100_enabled_for_console(HANDLE terminal);
std::string_view term_handle_vt100_sequence(HANDLE terminal, std::string_view str);

};

#endif // WIN32_TERMINAL_H
