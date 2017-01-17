#include "system/terminal.h"

#include <cstdio>

char *readline(const char *prompt) {

	size_t size = 0;
	char *buffer = nullptr;

	fprintf(stdout, prompt);
	fflush(stdout);

	getline(&buffer, &size, stdin);

	return buffer;
}

void add_history(const char *line) {
	((void)line);
}
