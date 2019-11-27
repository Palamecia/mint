#include "system/terminal.h"

#include <map>
#include <string>
#include <cstdio>
#include <cstdlib>

#ifdef OS_WINDOWS
#include <Windows.h>
#else
#include <readline/readline.h>
#include <readline/history.h>
#endif

using namespace std;
using namespace mint;

#ifdef OS_WINDOWS
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

static bool vt100_enabled_for_console(HANDLE hTerminal) {

	DWORD dwMode = 0;

	if (GetConsoleMode(hTerminal, &dwMode)) {
		return SetConsoleMode(hTerminal, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
	}

	return false;
}

static void set_console_attributes(HANDLE hTerminal, const list<int> &attrs) {

	static WORD defaultAttributes = 0;
	static const map<int, pair<int, int>> Attributes = {
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
#endif

void mint::term_init() {
	/// \todo handle signals
	/// \todo enable indentation ?
}

char *mint::term_read_line(const char *prompt) {

#ifdef OS_WINDOWS
	size_t buffer_pos = 0;
	size_t buffer_length = 128;
	char *buffer = static_cast<char *>(malloc(buffer_length));

	term_cprint(stdout, prompt);

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
#else
	char *buffer = readline(prompt);

	if (buffer == nullptr) {
		exit(EXIT_SUCCESS);
	}

	rl_redisplay();
#endif

	size_t length = strlen(buffer);
	buffer = static_cast<char *>(realloc(buffer, length + 2));
	buffer[length + 0] = '\n';
	buffer[length + 1] = '\0';

	return buffer;
}

void mint::term_add_history(const char *line) {

#ifdef OS_WINDOWS
	/// \todo Windows term_add_history
#else
	size_t length = strlen(line);
	char buffer[length];
	strncpy(buffer, line, length - 1);
	buffer[length - 1] = '\0';
	add_history(buffer);
#endif
}

void mint::term_cprint(FILE *stream, const char *str) {

#ifdef OS_UNIX
	fprintf(stream, "%s", str);
#else
	HANDLE hTerminal = INVALID_HANDLE_VALUE;

	if (stream == stdout) {
		hTerminal = GetStdHandle(STD_OUTPUT_HANDLE);
	}
	else if (stream == stderr) {
		hTerminal = GetStdHandle(STD_ERROR_HANDLE);
	}
	else {
		return;
	}

	if (vt100_enabled_for_console(hTerminal)) {
		WriteConsole(hTerminal, str, static_cast<DWORD>(strlen(str)), nullptr, nullptr);
	}
	else {
		while (const char *cptr = strstr(str, "\033[")) {

			int attr = 0;
			list<int> attrs;
			WriteConsole(hTerminal, str, static_cast<DWORD>(cptr - str), nullptr, nullptr);
			cptr += 2;

			while (*cptr) {
				if (isdigit(*cptr)) {
					attr = (attr * 10) + (*cptr - '0');
				}
				else if (*cptr == ';') {
					attrs.push_back(attr);
					attr = 0;
				}
				else if (isalpha(*cptr)) {
					attrs.push_back(attr);
					str = cptr + 1;
					if (*cptr == 'm') {
						set_console_attributes(stdout, attrs);
					}
					break;
				}
				cptr++;
			}
		}

		if (*str) {
			WriteConsole(hTerminal, str, static_cast<DWORD>(strlen(str)), nullptr, nullptr);
		}
	}
#endif
}
