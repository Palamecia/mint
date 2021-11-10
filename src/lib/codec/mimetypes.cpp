#include <memory/functiontool.h>
#include <memory/builtin/string.h>

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

	if (FindMimeFromData(NULL, NULL, const_cast<LPVOID>(buffer), length, NULL, 0, &swContentType, 0) == S_OK) {

		int mime_type_length = WideCharToMultiByte(CP_UTF8, 0, swContentType, -1, nullptr, 0, nullptr, nullptr);
		char *mime_type = static_cast<char *>(alloca(mime_type_length * sizeof(char)));

		if (WideCharToMultiByte(CP_UTF8, 0, swContentType, -1, mime_type, mime_type_length, nullptr, nullptr)) {
			return mime_type;
		}

		wstring buffer = swContentType;
		return string(buffer.begin(), buffer.end());
	}
#else
	magic_t cookie = magic_open(MAGIC_MIME);
	const char *mime_type = magic_buffer(cookie, buffer, length);
	magic_close(cookie);

	if (mime_type) {
		return mime_type;
	}
#endif

	return string();
}

MINT_FUNCTION(mint_mime_type_from_buffer, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference data = move(helper.popParameter());
	helper.returnValue(create_string(mime_type_from_data(data.data<LibObject<vector<uint8_t>>>()->impl->data(), data.data<LibObject<vector<uint8_t>>>()->impl->size())));
}

MINT_FUNCTION(mint_mime_type_from_string, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference data = move(helper.popParameter());
	helper.returnValue(create_string(mime_type_from_data(data.data<String>()->str.data(), data.data<String>()->str.size())));
}
