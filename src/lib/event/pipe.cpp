#include <memory/functiontool.h>
#include <memory/casttool.h>

#ifdef OS_WINDOWS
#include <Windows.h>
using handle_data_t = std::remove_pointer<HANDLE>::type;
#else
#include <sys/file.h>
#include <unistd.h>
#include <poll.h>
#endif

using namespace std;
using namespace mint;

MINT_FUNCTION(mint_pipe_create, 0, cursor) {

	FunctionHelper helper(cursor, 0);

#ifdef OS_WINDOWS
	HANDLE h[2];

	if (CreatePipe(h + 0, h + 1, nullptr, 0) != 0) {
		if ((h[0] != INVALID_HANDLE_VALUE) && (h[1] != INVALID_HANDLE_VALUE)) {
			SharedReference handles = SharedReference::unique(Reference::create<Iterator>());
			iterator_insert(handles->data<Iterator>(), create_object(h[0]));
			iterator_insert(handles->data<Iterator>(), create_object(h[1]));
			handles->data<Iterator>()->construct();
			helper.returnValue(move(handles));
		}
	}
#else
	int fd[2];

	if (pipe2(fd, O_NONBLOCK) == 0) {
		if ((fd[0] != -1) && (fd[1] != -1)) {
			SharedReference handles = SharedReference::unique(Reference::create<Iterator>());
			iterator_insert(handles->data<Iterator>(), create_number(fd[0]));
			iterator_insert(handles->data<Iterator>(), create_number(fd[1]));
			handles->data<Iterator>()->construct();
			helper.returnValue(move(handles));
		}
	}
#endif
}

MINT_FUNCTION(mint_pipe_close, 1, cursor) {

	FunctionHelper helper(cursor, 1);

#ifdef OS_WINDOWS
	HANDLE h = helper.popParameter()->data<LibObject<handle_data_t>>()->impl;
	CloseHandle(h);
#else
	int fd = static_cast<int>(to_number(cursor, helper.popParameter()));
	close(fd);
#endif
}

MINT_FUNCTION(mint_pipe_read, 2, cursor) {

	FunctionHelper helper(cursor, 2);

	SharedReference stream = move(helper.popParameter());

#ifdef OS_WINDOWS
	DWORD count;
	uint8_t read_buffer[1024];
	HANDLE h = helper.popParameter()->data<LibObject<handle_data_t>>()->impl;
	vector<uint8_t> *stream_buffer = stream->data<LibObject<vector<uint8_t>>>()->impl;

	while (ReadFile(h, read_buffer, sizeof(read_buffer), &count, nullptr)) {

		if (count < 0) {
			break;
		}

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

MINT_FUNCTION(mint_pipe_write, 2, cursor) {

	FunctionHelper helper(cursor, 2);

	SharedReference stream = move(helper.popParameter());

#ifdef OS_WINDOWS
	DWORD count;
	HANDLE h = helper.popParameter()->data<LibObject<handle_data_t>>()->impl;
	vector<uint8_t> *buffer = stream->data<LibObject<vector<uint8_t>>>()->impl;

	WriteFile(h, buffer->data(), buffer->size(), &count, nullptr);
#else
	int fd = static_cast<int>(to_number(cursor, helper.popParameter()));
	vector<uint8_t> *buffer = stream->data<LibObject<vector<uint8_t>>>()->impl;

	write(fd, buffer->data(), buffer->size());
#endif
}

MINT_FUNCTION(mint_pipe_wait, 2, cursor) {

	FunctionHelper helper(cursor, 2);

	SharedReference timeout = move(helper.popParameter());

#ifdef OS_WINDOWS
	HANDLE h = helper.popParameter()->data<LibObject<handle_data_t>>()->impl;
	DWORD ret = WaitForSingleObjectEx(h, timeout, true);
	helper.returnValue(create_boolean(ret == WAIT_OBJECT_0));
#else
	pollfd fds;
	fds.events = POLLIN;
	fds.fd = static_cast<int>(to_number(cursor, helper.popParameter()));

	int time_ms = -1;

	if (timeout->data()->format != Data::fmt_none) {
		time_ms = static_cast<int>(to_number(cursor, timeout));
	}

	bool result = false;
	int ret = poll(&fds, 1, time_ms);

	if ((ret > 0) && (fds.revents & POLLIN)) {
		result = true;
	}

	helper.returnValue(create_boolean(result));
#endif
}
