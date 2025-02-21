/**
 * Copyright (c) 2025 Gauvain CHERY.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "mint/system/error.h"
#include "mint/system/pipe.h"
#include "mint/system/string.h"
#include "mint/system/terminal.h"
#include "mint/system/mintsystemerror.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <mutex>
#include <map>

using namespace mint;

static std::string g_error_message;
static std::mutex g_error_callback_mutex;
static int g_next_error_callback_id = 0;
static std::map<int, std::function<void(void)>> g_error_callbacks;
static std::function<void(void)> g_exit_callback = [] {
	exit(EXIT_FAILURE);
};

void mint::error(const char *format, ...) {

	std::unique_lock<std::mutex> lock(g_error_callback_mutex);

	va_list args;
	va_start(args, format);
	g_error_message = mint::vformat(format, args);
	va_end(args);

	for (auto &callback : g_error_callbacks) {
		callback.second();
	}

	if (is_term(stderr)) {
		Terminal::print(stderr, MINT_TERM_FG_RED_WITH(MINT_TERM_BOLD_OPTION));
		Terminal::print(stderr, g_error_message.c_str());
		Terminal::print(stderr, MINT_TERM_RESET);
		Terminal::print(stderr, "\n");
	}
	else if (is_pipe(stderr)) {
		Pipe::print(stderr, g_error_message.c_str());
		Pipe::print(stderr, "\n");
	}
	else {
		fputs(g_error_message.c_str(), stderr);
		fputc('\n', stderr);
	}

	auto exit_callback = g_exit_callback;
	lock.unlock();
	exit_callback();

	throw MintSystemError(g_error_message);
}

std::string_view mint::get_error_message() {
	return g_error_message;
}

int mint::add_error_callback(std::function<void(void)> on_error) {

	std::unique_lock<std::mutex> lock(g_error_callback_mutex);

	if (g_error_callbacks.emplace(++g_next_error_callback_id, on_error).second) {
		return g_next_error_callback_id;
	}

	return add_error_callback(on_error);
}

void mint::remove_error_callback(int id) {

	std::unique_lock<std::mutex> lock(g_error_callback_mutex);

	auto callback = g_error_callbacks.find(id);
	if (callback != g_error_callbacks.end()) {
		g_error_callbacks.erase(callback);
	}
}

void mint::call_error_callbacks() {

	std::unique_lock<std::mutex> lock(g_error_callback_mutex);

	for (auto &callback : g_error_callbacks) {
		callback.second();
	}
}

void mint::set_exit_callback(const std::function<void(void)> &on_exit) {
	g_exit_callback = on_exit;
}

void mint::call_exit_callback() {
	g_error_callback_mutex.lock();
	auto exit_callback = g_exit_callback;
	g_error_callback_mutex.unlock();
	exit_callback();
}
