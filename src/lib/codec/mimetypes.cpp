/**
 * Copyright (c) 2024 Gauvain CHERY.
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

#include <mint/memory/functiontool.h>
#include <mint/memory/builtin/string.h>

#ifdef OS_WINDOWS
#include <urlmon.h>
#else
#include <magic.h>
#endif

using namespace mint;
using namespace std;

static string mime_type_from_data(const void *buffer, size_t length) {
#ifdef OS_WINDOWS
	LPWSTR swContentType = 0;

	if (FindMimeFromData(NULL, NULL, const_cast<LPVOID>(buffer), static_cast<DWORD>(length), NULL, 0, &swContentType, 0) == S_OK) {

		string mime_type(WideCharToMultiByte(CP_UTF8, 0, swContentType, -1, nullptr, 0, nullptr, nullptr), '\0');

		if (WideCharToMultiByte(CP_UTF8, 0, swContentType, -1, mime_type.data(), mime_type.length(), nullptr, nullptr)) {
			return mime_type.data();
		}

		return {};
	}
#else
	magic_t cookie = magic_open(MAGIC_MIME);
	const char *mime_type = magic_buffer(cookie, buffer, length);
	magic_close(cookie);

	if (mime_type) {
		return mime_type;
	}
#endif

	return {};
}

MINT_FUNCTION(mint_mime_type_from_buffer, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &data = helper.pop_parameter();
	helper.return_value(create_string(mime_type_from_data(data.data<LibObject<vector<uint8_t>>>()->impl->data(), data.data<LibObject<vector<uint8_t>>>()->impl->size())));
}

MINT_FUNCTION(mint_mime_type_from_string, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &data = helper.pop_parameter();
	helper.return_value(create_string(mime_type_from_data(data.data<String>()->str.data(), data.data<String>()->str.size())));
}
