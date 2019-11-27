#include <memory/functiontool.h>
#include <memory/casttool.h>
#include <system/filesystem.h>

#ifdef OS_WINDOWS
#include <Windows.h>
using handle_data_t = std::remove_pointer<HANDLE>::type;
#else
#include <sys/file.h>
#include <stdio_ext.h>
#include <unistd.h>
#include <poll.h>
#endif

using namespace std;
using namespace mint;

#ifdef OS_UNIX
static bool mint_sflags(const char *mode, int *optr) {

	int m, o;

	switch (*mode++) {
	case 'r':	/* open for reading */
		m = O_RDONLY;
		o = 0;
		break;
	case 'w':	/* open for writing */
		m = O_WRONLY;
		o = O_CREAT | O_TRUNC;
		break;
	case 'a':	/* open for appending */
		m = O_WRONLY;
		o = O_CREAT | O_APPEND;
		break;
	default:	/* illegal mode */
		errno = EINVAL;
		return false;
	}

	/* [rwa]\+ or [rwa]b\+ means read and write */
	if (*mode == '+' || (*mode == 'b' && mode[1] == '+')) {
		m = O_RDWR;
	}

	*optr = m | o;
	return true;
}
#else
static bool mint_sflags(const char *mode, DWORD *dwDesiredAccess, DWORD *dwCreationDisposition) {

	switch (*mode++) {
	case 'r':	/* open for reading */
		*dwDesiredAccess = GENERIC_READ;
		*dwCreationDisposition = OPEN_EXISTING;
		break;
	case 'w':	/* open for writing */
		*dwDesiredAccess = GENERIC_WRITE;
		*dwCreationDisposition = TRUNCATE_EXISTING;
		break;
	case 'a':	/* open for appending */
		*dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
		*dwCreationDisposition = CREATE_ALWAYS;
		break;
	default:	/* illegal mode */
		errno = EINVAL;
		return false;
	}

	/* [rwa]\+ or [rwa]b\+ means read and write */
	if (*mode == '+' || (*mode == 'b' && mode[1] == '+')) {
		*dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
	}

	return true;
}
#endif

MINT_FUNCTION(mint_file_create, 2, cursor) {

	FunctionHelper helper(cursor, 2);

	SharedReference mode = move(helper.popParameter());
	SharedReference path = move(helper.popParameter());

#ifdef OS_WINDOWS
	DWORD dwDesiredAccess, dwCreationDisposition;

	if (mint_sflags(to_string(mode).c_str(), &dwDesiredAccess, &dwCreationDisposition)) {
		helper.returnValue(create_object(CreateFileW(string_to_windows_path(to_string(path)).c_str(), dwDesiredAccess, dwDesiredAccess, nullptr, dwCreationDisposition, FILE_ATTRIBUTE_NORMAL, nullptr)));
	}
#else
	int flags = 0;

	if (mint_sflags(to_string(mode).c_str(), &flags)) {
		int fd = open(to_string(path).c_str(), flags | O_NONBLOCK);
		if (fd != -1) {
			helper.returnValue(create_number(fd));
		}
	}
#endif
}

MINT_FUNCTION(mint_file_close, 1, cursor) {

	FunctionHelper helper(cursor, 1);

#ifdef OS_WINDOWS
	CloseHandle(helper.popParameter()->data<LibObject<handle_data_t>>()->impl);
#else
	close(static_cast<int>(to_number(cursor, helper.popParameter())));
#endif
}

MINT_FUNCTION(mint_file_read, 2, cursor) {

	FunctionHelper helper(cursor, 2);

	SharedReference stream = move(helper.popParameter());

#ifdef OS_WINDOWS
	DWORD count = 0;
	uint8_t read_buffer[1024];
	HANDLE handle = helper.popParameter()->data<LibObject<handle_data_t>>()->impl;
	vector<uint8_t> *stream_buffer = stream->data<LibObject<vector<uint8_t>>>()->impl;

	while (ReadFile(handle, read_buffer, sizeof(read_buffer), &count, nullptr)) {
		copy_n(read_buffer, count, back_inserter(*stream_buffer));
	}
#else
	uint8_t read_buffer[1024];
	int fd = static_cast<int>(to_number(cursor, helper.popParameter()));
	vector<uint8_t> *stream_buffer = stream->data<LibObject<vector<uint8_t>>>()->impl;

	while (ssize_t count = read(fd, read_buffer, sizeof (read_buffer))) {

		if (count < 0) {
			break;
		}

		copy_n(read_buffer, count, back_inserter(*stream_buffer));
	}
#endif
}

MINT_FUNCTION(mint_file_write, 2, cursor) {

	FunctionHelper helper(cursor, 2);

	SharedReference stream = move(helper.popParameter());

#ifdef OS_WINDOWS
	HANDLE handle = helper.popParameter()->data<LibObject<handle_data_t>>()->impl;
	vector<uint8_t> *buffer = stream->data<LibObject<vector<uint8_t>>>()->impl;

	WriteFile(handle, buffer->data(), buffer->size(), nullptr, nullptr);
#else
	int fd = static_cast<int>(to_number(cursor, helper.popParameter()));
	vector<uint8_t> *buffer = stream->data<LibObject<vector<uint8_t>>>()->impl;

	write(fd, buffer->data(), buffer->size());
#endif
}
