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

#ifndef MINT_SYSTEM_ERROR_H
#define MINT_SYSTEM_ERROR_H

#include "mint/config.h"

#include <format>
#include <string>
#include <functional>

namespace mint {

[[noreturn]] MINT_EXPORT void on_error(std::string message);

template<typename... Args>
[[noreturn]] void error(std::format_string<Args...> fmt, Args&&... args) {
	on_error(std::vformat(fmt.get(), std::make_format_args(args...)));
}

MINT_EXPORT int add_error_callback(const std::function<void(const std::string&)>& callback);
MINT_EXPORT void call_error_callbacks(const std::string& message);
MINT_EXPORT void remove_error_callback(int id);

MINT_EXPORT void set_exit_callback(const std::function<void(void)>& callback);
MINT_EXPORT void call_exit_callback();

}

#endif // MINT_SYSTEM_ERROR_H
