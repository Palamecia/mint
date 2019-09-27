#include <memory/functiontool.h>
#include <memory/casttool.h>

#ifdef OS_WINDOWS
#include <Windows.h>
#else
#include <sys/file.h>
#include <stdio_ext.h>
#include <poll.h>
#endif

#include <unistd.h>

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
#endif

MINT_FUNCTION(mint_file_create, 2, cursor) {

	FunctionHelper helper(cursor, 2);

	SharedReference mode = helper.popParameter();
	SharedReference path = helper.popParameter();

#ifdef OS_WINDOWS
	helper.returnValue(create_object(CreateFile()));
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

#else
	int fd = static_cast<int>(to_number(cursor, helper.popParameter()));
	close(fd);
#endif
}

MINT_FUNCTION(mint_file_read, 2, cursor) {

	FunctionHelper helper(cursor, 2);

	SharedReference stream = helper.popParameter();

#ifdef OS_WINDOWS

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

	SharedReference stream = helper.popParameter();

#ifdef OS_WINDOWS

#else
	int fd = static_cast<int>(to_number(cursor, helper.popParameter()));
	vector<uint8_t> *buffer = stream->data<LibObject<vector<uint8_t>>>()->impl;

	write(fd, buffer->data(), buffer->size());
#endif
}
