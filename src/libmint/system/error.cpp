/**
 * Copyright (c) 2026 Gauvain CHERY.
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
#include "mint/system/terminal.h"
#include "mint/system/mintruntimeerror.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <mutex>
#include <map>
#include <string>
#include <utility>
#include <vector>

using namespace mint;

namespace {

struct {
	std::mutex callback_mutex;
	int next_callback_id = 0;
	std::vector<std::pair<int, std::function<void(const std::string&)>>> callbacks;
	std::function<void(void)> exit_callback = []() {
		std::exit(EXIT_FAILURE);
	};
} g_error;

}

void mint::on_error(std::string message) {

	std::unique_lock lock(g_error.callback_mutex);

	for (const auto& callback : g_error.callbacks) {
		callback.second(message);
	}

	if (is_term(stderr)) {
		Terminal::println(stderr,
		    MINT_TERM_OPT(MINT_TERM_BOLD, MINT_TERM_FG_RED) + message + MINT_TERM_OPT(MINT_TERM_RESET));
	}
	else if (is_pipe(stderr)) {
		Pipe::print(stderr, message);
		Pipe::print(stderr, "\n");
	}
	else {
		std::fputs(message.data(), stderr);
		std::fputc('\n', stderr);
	}

	auto exit_callback = g_error.exit_callback;
	lock.unlock();
	exit_callback();

	throw MintRuntimeError(message);
}

int mint::add_error_callback(const std::function<void(const std::string&)>& callback) {

	const std::unique_lock _(g_error.callback_mutex);

	do {
		++g_error.next_callback_id;
	}
	while (!g_error.callbacks.emplace_back(g_error.next_callback_id, callback).second);

	return g_error.next_callback_id;
}

void mint::call_error_callbacks(const std::string& message) {

	const std::unique_lock _(g_error.callback_mutex);

	for (auto& callback : g_error.callbacks) {
		callback.second(message);
	}
}

void mint::remove_error_callback(int id) {

	using CallbackList = decltype(g_error.callbacks);

	const std::unique_lock _(g_error.callback_mutex);

	if (auto it = std::ranges::find(g_error.callbacks, id, &CallbackList::value_type::first);
	    it != g_error.callbacks.end()) {
		g_error.callbacks.erase(it);
	}
}

std::vector<std::pair<int, std::function<void(const std::string&)>>> mint::take_error_callbacks() {
	return std::move(g_error.callbacks);
}

void mint::restore_error_callbacks(std::vector<std::pair<int, std::function<void(const std::string&)>>>&& callbacks) {
	g_error.callbacks.append_range(std::move(callbacks));
}

std::function<void(void)> mint::get_exit_callback() {
	return g_error.exit_callback;
}

void mint::set_exit_callback(const std::function<void(void)>& callback) {
	g_error.exit_callback = callback;
}

void mint::call_exit_callback() {
	g_error.callback_mutex.lock();
	auto exit_callback = g_error.exit_callback;
	g_error.callback_mutex.unlock();
	exit_callback();
}
