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

#include "mint/ast/cursor.h"
#include "mint/memory/builtin/libobject.h"
#include "mint/memory/reference.h"
#include "mint/memory/functiontool.h"
#include "mint/memory/builtin/string.h"
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#ifdef MINT_OS_WINDOWS
#include <urlmon.h>
#else
#include <magic.h>
#endif

namespace {

std::string mime_type_from_data(const void* buffer, std::size_t length) {
#ifdef MINT_OS_WINDOWS
	LPWSTR content_type = nullptr;

	if (FindMimeFromData(nullptr, nullptr, const_cast<LPVOID>(buffer), static_cast<DWORD>(length), nullptr, 0,
	        &content_type, 0)
	    == S_OK) {

		const int length = WideCharToMultiByte(CP_UTF8, 0, content_type, -1, nullptr, 0, nullptr, nullptr);
		if (std::string mime_type(length, '\0');
		    WideCharToMultiByte(CP_UTF8, 0, content_type, -1, mime_type.data(), length, nullptr, nullptr)) {
			return mime_type;
		}

		return {};
	}
#else
	magic_t cookie = magic_open(MAGIC_MIME);
	const char* mime_type = magic_buffer(cookie, buffer, length);
	magic_close(cookie);

	if (mime_type) {
		return mime_type;
	}
#endif

	return {};
}

mint::WeakReference mint_mime_type_from_buffer(mint::Cursor& cursor, const mint::Reference& data) {
	return mint::create_string(cursor.ast(),
	    mime_type_from_data(data.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr->data(),
	        data.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr->size()));
}

mint::WeakReference mint_mime_type_from_string(mint::Cursor& cursor, const mint::Reference& data) {
	return mint::create_string(cursor.ast(),
	    mime_type_from_data(data.data<mint::String>().str.data(), data.data<mint::String>().str.size()));
}

}

MINT_EXPORT_FUNCTION(mint_mime_type_from_buffer, 1)
MINT_EXPORT_FUNCTION(mint_mime_type_from_string, 1)
