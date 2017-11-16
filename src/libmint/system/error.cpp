#include "system/error.h"

#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <map>

using namespace std;

static int g_next_error_callback_id = 0;
static map<int, function<void(void)>> g_error_callbacks;
static function<void(void)> g_exit_callback = bind(exit, EXIT_FAILURE);

MintSystemError::MintSystemError() {

}

void error(const char *format, ...) {

	for (auto callback : g_error_callbacks) {
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

	g_exit_callback();
}

int add_error_callback(function<void(void)> on_error) {

	if (g_error_callbacks.emplace(++g_next_error_callback_id, on_error).second) {
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

void set_exit_callback(function<void(void)> on_exit) {
	g_exit_callback = on_exit;
}
