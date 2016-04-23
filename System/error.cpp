#include "System/error.h"
#include <cstdio>
#include <cstdlib>
#include <cstdarg>

void error(const char *format, ...) {

	va_list args;

	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);

	fprintf(stderr, "\n");

	/// \todo print callstack

	abort();
}
