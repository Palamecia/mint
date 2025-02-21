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

#ifndef MINT_PLUGIN_H
#define MINT_PLUGIN_H

#include "mint/config.h"

#include <filesystem>
#include <string>

#ifdef OS_WINDOWS
#include <Windows.h>
#endif

namespace mint {

class Cursor;

class MINT_EXPORT Plugin {
public:
#ifdef OS_WINDOWS
	using handle_type = HMODULE;
#else
	using handle_type = void *;
#endif

	explicit Plugin(const std::filesystem::path &path);
	Plugin(const Plugin &) = delete;
	Plugin(Plugin &&) = delete;
	~Plugin();

	Plugin &operator=(const Plugin &) = delete;
	Plugin &operator=(Plugin &&) = delete;

	static Plugin *load(const std::string &plugin);
	static std::string function_name(const std::string &name, int signature);

	bool call(const std::string &function, int signature, Cursor *cursor);

	[[nodiscard]] std::filesystem::path get_path() const;

protected:
	using function_type = void (*)(Cursor *);

	function_type get_function(const std::string &name);

private:
	std::filesystem::path m_path;
	handle_type m_handle;
};

}

#endif // MINT_PLUGIN_H
