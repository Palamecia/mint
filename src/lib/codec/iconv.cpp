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
#include <algorithm>
#include <iterator>
#include <iconv.h>

#ifdef OS_WINDOWS
#ifndef WINICONV_CONST
#define WINICONV_CONST
#endif
#endif

using namespace mint;

struct iconv_context_t {
	iconv_t decode_cd;
	iconv_t encode_cd;
};

static constexpr size_t ICONV_FAILED = size_t(-1);

namespace symbols {

static const Symbol Codec("Codec");
static const Symbol Iconv("Iconv");
static const Symbol State("State");

static const Symbol Invalid("Invalid");
static const Symbol Success("Success");
static const Symbol NeedMore("NeedMore");

}

MINT_FUNCTION(mint_iconv_open, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &encoding = helper.pop_parameter();

	iconv_context_t *context = new iconv_context_t;
	context->decode_cd = iconv_open("UTF-8", encoding.data<String>()->str.c_str());
	context->encode_cd = iconv_open(encoding.data<String>()->str.c_str(), "UTF-8");
	
	helper.return_value(create_object(context));
}

MINT_FUNCTION(mint_iconv_close, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &context = helper.pop_parameter();

	iconv_close(context.data<LibObject<iconv_context_t>>()->impl->decode_cd);
	iconv_close(context.data<LibObject<iconv_context_t>>()->impl->encode_cd);
}

MINT_FUNCTION(mint_iconv_decode, 3, cursor) {

	FunctionHelper helper(cursor, 3);
	const Reference &stream = helper.pop_parameter();
	const Reference &buffer = helper.pop_parameter();
	const Reference &context = helper.pop_parameter();

	bool finished = false;
	iconv_t cd = context.data<LibObject<iconv_context_t>>()->impl->decode_cd;
	auto State = helper.reference(symbols::Codec).member(symbols::Iconv).member(symbols::State);

#ifdef OS_WINDOWS
	WINICONV_CONST char *inbuf = (WINICONV_CONST char *)(stream.data<LibObject<std::vector<uint8_t>>>()->impl->data());
#else
	char *inbuf = (char *)(stream.data<LibObject<std::vector<uint8_t>>>()->impl->data());
#endif
	size_t inlen = stream.data<LibObject<std::vector<uint8_t>>>()->impl->size();

	char outbuf[BUFSIZ];
	size_t outlen = BUFSIZ;

	while (!finished) {

		char* outptr = outbuf;
		size_t count = iconv(cd, &inbuf, &inlen, &outptr, &outlen);

		if (count == ICONV_FAILED) {
			switch (errno) {
			case E2BIG:
				copy_n(outbuf, BUFSIZ - outlen, back_inserter(buffer.data<String>()->str));
				outlen = BUFSIZ;
				break;

			case EILSEQ:
				helper.return_value(State.member(symbols::Invalid));
				finished = true;
				break;

			case EINVAL:
				helper.return_value(State.member(symbols::NeedMore));
				finished = true;
				break;

			default:
				break;
			}
		}
		else {
			copy_n(outbuf, BUFSIZ - outlen, back_inserter(buffer.data<String>()->str));
			helper.return_value(State.member(symbols::Success));
			finished = true;
		}
	}
}

MINT_FUNCTION(mint_iconv_encode, 3, cursor) {

	FunctionHelper helper(cursor, 3);
	const Reference &stream = helper.pop_parameter();
	Reference &buffer = helper.pop_parameter();
	const Reference &context = helper.pop_parameter();

	bool finished = false;
	iconv_t cd = context.data<LibObject<iconv_context_t>>()->impl->encode_cd;
	auto State = helper.reference(symbols::Codec).member(symbols::Iconv).member(symbols::State);

#ifdef OS_WINDOWS
	WINICONV_CONST char *inbuf = (WINICONV_CONST char *)(buffer.data<String>()->str.c_str());
#else
	char *inbuf = (char *)(buffer.data<String>()->str.c_str());
#endif
	size_t inlen = buffer.data<String>()->str.size();

	char outbuf[BUFSIZ];
	size_t outlen = BUFSIZ;

	while (!finished) {

		char* outptr = outbuf;
		size_t count = iconv(cd, &inbuf, &inlen, &outptr, &outlen);

		if (count == ICONV_FAILED) {
			switch (errno) {
			case E2BIG:
				copy_n(reinterpret_cast<uint8_t *>(outbuf), BUFSIZ - outlen, back_inserter(*stream.data<LibObject<std::vector<uint8_t>>>()->impl));
				outlen = BUFSIZ;
				break;

			case EILSEQ:
				helper.return_value(State.member(symbols::Invalid));
				finished = true;
				break;

			case EINVAL:
				helper.return_value(State.member(symbols::NeedMore));
				finished = true;
				break;

			default:
				break;
			}
		}
		else {
			copy_n(reinterpret_cast<uint8_t *>(outbuf), BUFSIZ - outlen, back_inserter(*stream.data<LibObject<std::vector<uint8_t>>>()->impl));
			stream.data<LibObject<std::vector<uint8_t>>>()->impl->push_back('\0');
			helper.return_value(State.member(symbols::Success));
			finished = true;
		}
	}
}
