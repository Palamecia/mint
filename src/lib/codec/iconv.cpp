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
#include "mint/ast/symbol.h"
#include "mint/memory/builtin/libobject.h"
#include "mint/memory/cast_tools.h"
#include "mint/memory/object.h"
#include "mint/memory/reference.h"
#include "mint/memory/function_tools.h"
#include "mint/memory/builtin/string.h"
#include "mint/system/utf8.h"
#include <algorithm>
#include <array>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <iterator>
#include <iconv.h>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#ifdef MINT_OS_WINDOWS
#ifndef WINICONV_CONST
#define WINICONV_CONST
#endif
#endif

namespace {

struct IconvContext {
	iconv_t decode_cd;
	iconv_t encode_cd;
};

constexpr auto iconv_failed = static_cast<std::size_t>(-1);

mint::Reference mint_iconv_open(mint::Cursor& cursor, const mint::Reference& encoding) {
	return mint::create_c_object(cursor.ast(),
	    new IconvContext {
	        .decode_cd = iconv_open("UTF-8", encoding.data<mint::String>().str.c_str()),
	        .encode_cd = iconv_open(encoding.data<mint::String>().str.c_str(), "UTF-8"),
	    });
}

mint::Reference mint_iconv_close(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr) {
	iconv_close(d_ptr.data<mint::LibObject<IconvContext>>().ptr->decode_cd);
	iconv_close(d_ptr.data<mint::LibObject<IconvContext>>().ptr->encode_cd);
	delete d_ptr.data<mint::LibObject<IconvContext>>().ptr;
	return {};
}

mint::Reference mint_iconv_decode(mint::Cursor& cursor, const mint::Reference& d_ptr, const mint::Reference& buffer,
    const mint::Reference& pos) {

	const auto offset = mint::to_integer<std::size_t>(cursor, pos);
	auto bytes = std::span(*buffer.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr).subspan(offset);
	auto* cd = d_ptr.data<mint::LibObject<IconvContext>>().ptr->decode_cd;
	auto decoded = std::string();

#ifdef MINT_OS_WINDOWS
	WINICONV_CONST auto* inbuf = (WINICONV_CONST char*)(bytes.data());
#else
	auto* inbuf = (char*)(bytes.data());
#endif
	auto inlen = bytes.size();

	auto outbuf = std::array<char, BUFSIZ>();
	auto outlen = outbuf.size();

	for (;;) {

		auto* outptr = outbuf.data();
		const auto count = iconv(cd, &inbuf, &inlen, &outptr, &outlen);

		if (count == iconv_failed) {
			switch (errno) {
			case E2BIG:
				decoded.append_range(std::span(outbuf.data(), outbuf.size() - outlen));
				outlen = BUFSIZ;
				break;
			case EILSEQ:
				decoded.append_range(std::span(outbuf.data(), outbuf.size() - outlen));
				pos.data<mint::Number>().value = mint::to_number(offset + decoded.size());
				return mint::create_iterator_from(cursor, mint::create_number(1),
				    mint::create_string(cursor.ast(), decoded));
			case EINVAL:
				decoded.append_range(std::span(outbuf.data(), outbuf.size() - outlen));
				pos.data<mint::Number>().value = mint::to_number(offset + decoded.size());
				return mint::create_iterator_from(cursor, mint::create_number(-1), pos);
			default:
				break;
			}
		}
		else {
			decoded.append_range(std::span(outbuf.data(), outbuf.size() - outlen));
			pos.data<mint::Number>().value = mint::to_number(offset + decoded.size());
			return mint::create_iterator_from(cursor, mint::create_number(0),
			    mint::create_string(cursor.ast(), decoded));
		}
	}
}

mint::Reference mint_iconv_encode(mint::Cursor& cursor, const mint::Reference& d_ptr, const mint::Reference& str,
    const mint::Reference& buffer, const mint::Reference& pos) {

	auto* output = buffer.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr;
	auto* cd = d_ptr.data<mint::LibObject<IconvContext>>().ptr->encode_cd;
	auto input_string = mint::to_string(str);
	const auto offset = mint::utf8_code_point_index_to_byte_index(input_string,
	    mint::to_integer<std::size_t>(cursor, pos));
	auto input_view = std::string_view(input_string).substr(offset);
	auto bytes_pos = 0uz;

#ifdef MINT_OS_WINDOWS
	WINICONV_CONST auto* inbuf = (WINICONV_CONST char*)(input_view.data());
#else
	auto* inbuf = (char*)(input_view.data());
#endif
	std::size_t inlen = input_view.size();

	auto outbuf = std::array<char, BUFSIZ>();
	auto outlen = outbuf.size();

	for (;;) {

		auto* outptr = outbuf.data();
		const auto count = iconv(cd, &inbuf, &inlen, &outptr, &outlen);

		if (count == iconv_failed) {
			switch (errno) {
			case E2BIG:
				output->append_range(std::span(outbuf.data(), outbuf.size() - outlen));
				bytes_pos += outbuf.size() - outlen;
				outlen = outbuf.size();
				break;
			case EILSEQ:
				output->append_range(std::span(outbuf.data(), outbuf.size() - outlen));
				bytes_pos += outbuf.size() - outlen;
				pos.data<mint::Number>().value = mint::to_number(offset + bytes_pos);
				return mint::create_number(1);
			case EINVAL:
				output->append_range(std::span(outbuf.data(), outbuf.size() - outlen));
				bytes_pos += outbuf.size() - outlen;
				pos.data<mint::Number>().value = mint::to_number(offset + bytes_pos);
				return mint::create_number(-1);
			default:
				break;
			}
		}
		else {
			output->append_range(std::span(outbuf.data(), outbuf.size() - outlen));
			bytes_pos += outbuf.size() - outlen;
			pos.data<mint::Number>().value = mint::to_number(offset + bytes_pos);
			return mint::create_number(0);
		}
	}
}

}

MINT_EXPORT_FUNCTION(mint_iconv_open, 1)
MINT_EXPORT_FUNCTION(mint_iconv_close, 1)
MINT_EXPORT_FUNCTION(mint_iconv_decode, 3)
MINT_EXPORT_FUNCTION(mint_iconv_encode, 4)
