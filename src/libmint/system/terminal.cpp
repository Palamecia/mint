#include "system/terminal.h"
#include "system/errno.h"

#include <string>
#include <cstdio>
#include <cstdlib>

#ifdef OS_WINDOWS
#include "winterminal.h"
#include <io.h>
#else
#include <readline/readline.h>
#include <readline/history.h>
#include <unistd.h>
#include <stdarg.h>
#endif

using namespace std;
using namespace mint;

void mint::term_init() {
	/// \todo handle signals
	/// \todo enable indentation ?
}

char *mint::term_read_line(const char *prompt) {

#ifdef OS_WINDOWS
	size_t buffer_pos = 0;
	size_t buffer_length = 128;
	char *buffer = static_cast<char *>(malloc(buffer_length));

	term_print(stdout, prompt);

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

int mint::term_print(FILE *stream, const char *str) {

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

	if (vt100_enabled_for_console(hTerminal)) {
		return WriteMultiByteToConsoleW(hTerminal, str);
	}

	int written_all = 0;

	while (const char *cptr = strstr(str, "\033[")) {

		int written = WriteMultiByteToConsoleW(hTerminal, str, static_cast<int>(cptr - str));

		if (written == EOF) {
			errno = errno_from_windows_last_error();
			return written;
		}

		str = handle_vt100_sequence(hTerminal, cptr + 2);
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

int mint::term_printf(FILE *stream, const char *format, ...) {

	va_list args;
	int result;

	va_start(args, format);
	result = term_vprintf(stream, format, args);
	va_end(args);
	return result;
}

int mint::term_vprintf(FILE *stream, const char *format, va_list args) {

#ifdef OS_UNIX
	return vfprintf(stream, format, args);
#else
	HANDLE hTerminal = INVALID_HANDLE_VALUE;

	switch (int fd = fileno(stream)) {
	case stdout_fileno:
		if (isatty(fd)) {
			hTerminal = GetStdHandle(STD_OUTPUT_HANDLE);
		}
		else {
			return vfprintf(stream, format, args);
		}
		break;
	case stderr_fileno:
		if (isatty(fd)) {
			hTerminal = GetStdHandle(STD_ERROR_HANDLE);
		}
		else {
			return vfprintf(stream, format, args);
		}
		break;
	default:
		return vfprintf(stream, format, args);
	}

	int written = 0;
	int written_all = 0;

	if (vt100_enabled_for_console(hTerminal)) {
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
			written = handle_format_flags(hTerminal, &format, &args);
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
				written = handle_format_flags(hTerminal, &format, &args);
				if (written == EOF) {
					errno = errno_from_windows_last_error();
					return written;
				}
				written_all += written;
				break;
			case '\033':
				if (cptr[1] == '[') {
					format = handle_vt100_sequence(hTerminal, cptr + 2);
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

bool mint::is_term(FILE *stream) {
	return isatty(fileno(stream));
}

bool mint::is_term(int fd) {
	return isatty(fd);
}
