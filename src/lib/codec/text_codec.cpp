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
#include "mint/memory/cast_tools.h"
#include "mint/memory/function_tools.h"
#include "mint/memory/object.h"
#include "mint/memory/reference.h"
#include "mint/system/utf8.h"
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {

mint::Reference mint_text_codec_decode(mint::Cursor& cursor, const mint::Reference& buffer, const mint::Reference& pos) {

	const auto offset = mint::to_integer<std::size_t>(cursor, pos);
	auto bytes = std::span(*buffer.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr).subspan(offset);
	auto decoded = std::string();

	if (bytes.empty()) {
		return mint::create_iterator_from(cursor, mint::create_number(1), mint::create_string(cursor.ast(), decoded));
	}

	while (!bytes.empty()) {
		const auto length = mint::utf8_code_point_length(bytes.front());
		if (length > bytes.size()) {
			pos.data<mint::Number>().value = mint::to_number(offset + decoded.size());
			return mint::create_iterator_from(cursor, mint::create_number(1),
			    mint::create_string(cursor.ast(), decoded));
		}
		decoded.append_range(std::span(bytes.data(), length));
		bytes = bytes.subspan(length);
	}

	pos.data<mint::Number>().value = mint::to_number(offset + decoded.size());
	return mint::create_iterator_from(cursor, mint::create_number(0), mint::create_string(cursor.ast(), decoded));
}

mint::Reference mint_text_codec_encode(mint::Cursor& cursor, const mint::Reference& str, const mint::Reference& buffer,
    const mint::Reference& pos) {

	auto* output = buffer.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr;
	auto input_string = mint::to_string(str);
	const auto offset = mint::utf8_code_point_index_to_byte_index(input_string,
	    mint::to_integer<std::size_t>(cursor, pos));
	auto input_view = std::string_view(input_string).substr(offset);
	auto bytes_pos = 0uz;

	while (bytes_pos < input_view.size()) {
		const auto length = mint::utf8_code_point_length(input_view.at(bytes_pos));
		if (bytes_pos + length > input_view.size()) {
			pos.data<mint::Number>().value = mint::to_number(offset + bytes_pos);
			return mint::create_number(1);
		}
		output->append_range(input_view.substr(bytes_pos, length));
		bytes_pos += length;
	}

	pos.data<mint::Number>().value = mint::to_number(offset + bytes_pos);
	return mint::create_number(0);
}

}

MINT_EXPORT_FUNCTION(mint_text_codec_decode, 2)
MINT_EXPORT_FUNCTION(mint_text_codec_encode, 3)
