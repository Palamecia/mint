#include "system/terminal.h"

#include <cstdio>
#include <cstdlib>

#include <readline/readline.h>
#include <readline/history.h>

using namespace mint;

void mint::term_init() {
	/// \todo handle signals
	/// \todo enable indentation ?
}

char *mint::term_read_line(const char *prompt) {

	char *buffer = readline(prompt);

	if (buffer == nullptr) {
		exit(EXIT_SUCCESS);
	}

	rl_redisplay();

	size_t length = strlen(buffer);
	buffer = (char *)realloc(buffer, length + 2);
	buffer[length + 0] = '\n';
	buffer[length + 1] = '\0';

	return buffer;
}

void mint::term_add_history(const char *line) {

	size_t length = strlen(line);
	char buffer[length];
	strncpy(buffer, line, length - 1);
	buffer[length - 1] = '\0';
	add_history(buffer);
}
