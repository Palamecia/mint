#include <memory/functiontool.h>
#include <memory/builtin/string.h>
#include <algorithm>
#include <iterator>
#include <iconv.h>

#ifdef OS_WINDOWS
#ifndef WINICONV_CONST
#define WINICONV_CONST
#endif
#endif

using namespace std;
using namespace mint;

struct iconv_context_t {
	iconv_t decode_cd;
	iconv_t encode_cd;
};

static constexpr size_t iconv_failed = size_t(-1);

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
	WeakReference encoding = move(helper.popParameter());

	iconv_context_t *context = new iconv_context_t;
	context->decode_cd = iconv_open("UTF-8", encoding.data<String>()->str.c_str());
	context->encode_cd = iconv_open(encoding.data<String>()->str.c_str(), "UTF-8");

	helper.returnValue(create_object(context));
}

MINT_FUNCTION(mint_iconv_close, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference context = move(helper.popParameter());

	iconv_close(context.data<LibObject<iconv_context_t>>()->impl->decode_cd);
	iconv_close(context.data<LibObject<iconv_context_t>>()->impl->encode_cd);
}

MINT_FUNCTION(mint_iconv_decode, 3, cursor) {

	FunctionHelper helper(cursor, 3);
	WeakReference stream = move(helper.popParameter());
	WeakReference buffer = move(helper.popParameter());
	WeakReference context = move(helper.popParameter());

	bool finished = false;
	iconv_t cd = context.data<LibObject<iconv_context_t>>()->impl->decode_cd;
	auto State = helper.reference(symbols::Codec).member(symbols::Iconv).member(symbols::State);

#ifdef OS_WINDOWS
	WINICONV_CONST char *inbuf = (WINICONV_CONST char *)(stream.data<LibObject<vector<uint8_t>>>()->impl->data());
#else
	char *inbuf = (char *)(stream.data<LibObject<vector<uint8_t>>>()->impl->data());
#endif
	size_t inlen = stream.data<LibObject<vector<uint8_t>>>()->impl->size();

	char *outbuf = static_cast<char *>(alloca(BUFSIZ));
	size_t outlen = BUFSIZ;

	while (!finished) {

		char* outptr = outbuf;
		size_t count = iconv(cd, &inbuf, &inlen, &outptr, &outlen);

		if (count == iconv_failed) {
			switch (errno) {
			case E2BIG:
				copy_n(outbuf, BUFSIZ - outlen, back_inserter(buffer.data<String>()->str));
				outlen = BUFSIZ;
				break;

			case EILSEQ:
				helper.returnValue(State.member(symbols::Invalid));
				finished = true;
				break;

			case EINVAL:
				helper.returnValue(State.member(symbols::NeedMore));
				finished = true;
				break;

			default:
				break;
			}
		}
		else {
			copy_n(outbuf, BUFSIZ - outlen, back_inserter(buffer.data<String>()->str));
			helper.returnValue(State.member(symbols::Success));
			finished = true;
		}
	}
}

MINT_FUNCTION(mint_iconv_encode, 3, cursor) {

	FunctionHelper helper(cursor, 3);
	WeakReference stream = move(helper.popParameter());
	WeakReference buffer = move(helper.popParameter());
	WeakReference context = move(helper.popParameter());

	bool finished = false;
	iconv_t cd = context.data<LibObject<iconv_context_t>>()->impl->encode_cd;
	auto State = helper.reference(symbols::Codec).member(symbols::Iconv).member(symbols::State);

#ifdef OS_WINDOWS
	WINICONV_CONST char *inbuf = (WINICONV_CONST char *)(buffer.data<String>()->str.c_str());
#else
	char *inbuf = (char *)(buffer.data<String>()->str.c_str());
#endif
	size_t inlen = buffer.data<String>()->str.size();

	char *outbuf = static_cast<char *>(alloca(BUFSIZ));
	size_t outlen = BUFSIZ;

	while (!finished) {

		char* outptr = outbuf;
		size_t count = iconv(cd, &inbuf, &inlen, &outptr, &outlen);

		if (count == iconv_failed) {
			switch (errno) {
			case E2BIG:
				copy_n(reinterpret_cast<uint8_t *>(outbuf), BUFSIZ - outlen, back_inserter(*stream.data<LibObject<vector<uint8_t>>>()->impl));
				outlen = BUFSIZ;
				break;

			case EILSEQ:
				helper.returnValue(State.member(symbols::Invalid));
				finished = true;
				break;

			case EINVAL:
				helper.returnValue(State.member(symbols::NeedMore));
				finished = true;
				break;

			default:
				break;
			}
		}
		else {
			copy_n(reinterpret_cast<uint8_t *>(outbuf), BUFSIZ - outlen, back_inserter(*stream.data<LibObject<vector<uint8_t>>>()->impl));
			stream.data<LibObject<vector<uint8_t>>>()->impl->push_back('\0');
			helper.returnValue(State.member(symbols::Success));
			finished = true;
		}
	}
}
