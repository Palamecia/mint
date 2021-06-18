#include "system/error.h"
#include "system/terminal.h"

#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <mutex>
#include <map>

using namespace std;
using namespace mint;

static mutex g_error_callback_mutex;
static int g_next_error_callback_id = 0;
static map<int, function<void(void)>> g_error_callbacks;
static function<void(void)> g_exit_callback = bind(&exit, EXIT_FAILURE);

void mint::error(const char *format, ...) {

	unique_lock<mutex> lock(g_error_callback_mutex);

	for (auto callback : g_error_callbacks) {
		callback.second();
	}

	va_list args;

	term_cprint(stderr, "\033[1;31m");

	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);

	term_cprint(stderr, "\033[0m");
	fprintf(stderr, "\n");

	auto exit_callback = g_exit_callback;
	lock.unlock();
	exit_callback();

	throw MintSystemError();
}

int mint::add_error_callback(function<void(void)> on_error) {

	unique_lock<mutex> lock(g_error_callback_mutex);

	if (g_error_callbacks.emplace(++g_next_error_callback_id, on_error).second) {
		return g_next_error_callback_id;
	}

	return add_error_callback(on_error);
}

void mint::remove_error_callback(int id) {

	unique_lock<mutex> lock(g_error_callback_mutex);

	auto callback = g_error_callbacks.find(id);
	if (callback != g_error_callbacks.end()) {
		g_error_callbacks.erase(callback);
	}
}

void mint::call_error_callbacks() {

	unique_lock<mutex> lock(g_error_callback_mutex);

	for (auto callback : g_error_callbacks) {
		callback.second();
	}
}

void mint::set_exit_callback(function<void(void)> on_exit) {
	g_exit_callback = on_exit;
}

void mint::call_exit_callback() {
	g_error_callback_mutex.lock();
	auto exit_callback = g_exit_callback;
	g_error_callback_mutex.unlock();
	exit_callback();
}
