#include "system/error.h"

#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <map>

using namespace std;

static int g_next_error_callback_id = 0;
static map<int, function<void(void)>> g_error_callbacks;

void error(const char *format, ...) {

	int thread_id = 0;
	for (auto callback : g_error_callbacks) {
		fprintf(stderr, "Traceback thread %d : \n", thread_id++);
		callback.second();
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

	exit(EXIT_FAILURE);
}

int add_error_callback(function<void(void)> on_error) {

	if (g_error_callbacks.insert({++g_next_error_callback_id, on_error}).second) {
		return g_next_error_callback_id;
	}

	return add_error_callback(on_error);
}

void remove_error_callback(int id) {

	auto callback = g_error_callbacks.find(id);
	if (callback != g_error_callbacks.end()) {
		g_error_callbacks.erase(callback);
	}
}
