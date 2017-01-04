#include "system/error.h"

#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <list>

using namespace std;

static list<function<void(void)>> g_error_callbacks;

void error(const char *format, ...) {

	for (auto callback : g_error_callbacks) {
		fprintf(stderr, "Traceback thread %d : \n", /** \todo thread id */0);
		callback();
	}

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

int add_error_callback(function<void(void)> on_error) {
	g_error_callbacks.push_back(on_error);
	return 0; /// \todo callback unique id
}

void remove_error_callback(int id) {
	/// \todo remove callback
}
