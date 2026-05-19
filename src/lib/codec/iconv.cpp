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
#include "mint/memory/reference.h"
#include "mint/memory/functiontool.h"
#include "mint/memory/builtin/string.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <iterator>
#include <iconv.h>
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

constexpr std::size_t iconv_failed = std::size_t(-1);

namespace symbols {

const mint::Symbol codec_type("Codec");
const mint::Symbol iconv_type("Iconv");
const mint::Symbol state_type("State");

const mint::Symbol invalid("Invalid");
const mint::Symbol success("Success");
const mint::Symbol need_more("NeedMore");

}

mint::Reference mint_iconv_open(mint::Cursor& cursor, const mint::Reference& encoding) {
	return mint::create_c_object(cursor.ast(),
	    new IconvContext {
	        .decode_cd = iconv_open("UTF-8", encoding.data<mint::String>().str.c_str()),
	        .encode_cd = iconv_open(encoding.data<mint::String>().str.c_str(), "UTF-8"),
	    });
}

mint::Reference mint_iconv_close(mint::Cursor& /*cursor*/, const mint::Reference& context) {
	iconv_close(context.data<mint::LibObject<IconvContext>>().ptr->decode_cd);
	iconv_close(context.data<mint::LibObject<IconvContext>>().ptr->encode_cd);
	delete context.data<mint::LibObject<IconvContext>>().ptr;
	return {};
}

mint::Reference mint_iconv_decode(mint::FunctionHelper& helper, const mint::Reference& context,
    mint::Reference& buffer, const mint::Reference& stream) {

	iconv_t cd = context.data<mint::LibObject<IconvContext>>().ptr->decode_cd;
	auto state_type = helper.reference(symbols::codec_type).member(symbols::iconv_type).member(symbols::state_type);

#ifdef MINT_OS_WINDOWS
	WINICONV_CONST auto* inbuf =
	    (WINICONV_CONST char*)(stream.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr->data());
#else
	auto* inbuf = (char*)(stream.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr->data());
#endif
	std::size_t inlen = stream.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr->size();

	std::array<char, BUFSIZ> outbuf = {};
	std::size_t outlen = BUFSIZ;

	for (;;) {

		auto* outptr = outbuf.data();
		const auto count = iconv(cd, &inbuf, &inlen, &outptr, &outlen);

		if (count == iconv_failed) {
			switch (errno) {
			case E2BIG:
				std::copy_n(outbuf.data(), BUFSIZ - outlen, std::back_inserter(buffer.data<mint::String>().str));
				outlen = BUFSIZ;
				break;
			case EILSEQ:
				return state_type.member(symbols::invalid).share();
			case EINVAL:
				return state_type.member(symbols::need_more).share();
			default:
				break;
			}
		}
		else {
			std::copy_n(outbuf.data(), BUFSIZ - outlen, std::back_inserter(buffer.data<mint::String>().str));
			return state_type.member(symbols::success).share();
		}
	}
}

mint::Reference mint_iconv_encode(mint::FunctionHelper& helper, const mint::Reference& context,
    mint::Reference& buffer, const mint::Reference& stream) {

	iconv_t cd = context.data<mint::LibObject<IconvContext>>().ptr->encode_cd;
	auto state_type = helper.reference(symbols::codec_type).member(symbols::iconv_type).member(symbols::state_type);

#ifdef MINT_OS_WINDOWS
	WINICONV_CONST auto* inbuf = (WINICONV_CONST char*)(buffer.data<mint::String>().str.c_str());
#else
	auto* inbuf = (char*)(buffer.data<mint::String>().str.c_str());
#endif
	std::size_t inlen = buffer.data<mint::String>().str.size();

	std::array<char, BUFSIZ> outbuf = {};
	std::size_t outlen = BUFSIZ;

	for (;;) {

		auto* outptr = outbuf.data();
		const auto count = iconv(cd, &inbuf, &inlen, &outptr, &outlen);

		if (count == iconv_failed) {
			switch (errno) {
			case E2BIG:
				std::copy_n(reinterpret_cast<std::uint8_t*>(outbuf.data()), BUFSIZ - outlen,
				    std::back_inserter(*stream.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr));
				outlen = BUFSIZ;
				break;

			case EILSEQ:
				return state_type.member(symbols::invalid).share();
			case EINVAL:
				return state_type.member(symbols::need_more).share();
			default:
				break;
			}
		}
		else {
			std::copy_n(reinterpret_cast<std::uint8_t*>(outbuf.data()), BUFSIZ - outlen,
			    std::back_inserter(*stream.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr));
			stream.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr->push_back('\0');
			return state_type.member(symbols::success).share();
		}
	}
}

}

MINT_EXPORT_FUNCTION(mint_iconv_open, 1)
MINT_EXPORT_FUNCTION(mint_iconv_close, 1)
MINT_EXPORT_FUNCTION(mint_iconv_decode, 3)
MINT_EXPORT_FUNCTION(mint_iconv_encode, 3)
