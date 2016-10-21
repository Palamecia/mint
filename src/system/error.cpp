#include "system/error.h"
#include <cstdio>
#include <cstdlib>
#include <cstdarg>

void error(const char *format, ...) {

	va_list args;

#ifndef _WIN32
	fprintf(stderr, "\033[1;31m");
#endif

	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);

#ifndef _WIN32
	fprintf(stderr, "\033[0m");
#endif

	fprintf(stderr, "\n");

	exit(1);
}
