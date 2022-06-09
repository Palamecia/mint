#include <memory/functiontool.h>
#include <memory/casttool.h>

#ifdef OS_WINDOWS
#include <Windows.h>
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
	HANDLE hPipe[2];
	SECURITY_ATTRIBUTES pipeAttributes;

	pipeAttributes.nLength = sizeof (SECURITY_ATTRIBUTES);
	pipeAttributes.bInheritHandle = true;
	pipeAttributes.lpSecurityDescriptor = nullptr;

	if (CreatePipe(hPipe + 0, hPipe + 1, &pipeAttributes, 0) != 0) {
		if ((hPipe[0] != INVALID_HANDLE_VALUE) && (hPipe[1] != INVALID_HANDLE_VALUE)) {
			WeakReference handles = create_iterator();
			iterator_insert(handles.data<Iterator>(), create_handle(hPipe[0]));
			iterator_insert(handles.data<Iterator>(), create_handle(hPipe[1]));
			helper.returnValue(move(handles));
		}
	}
#else
	int fd[2];

	if (pipe2(fd, O_NONBLOCK) == 0) {
		if ((fd[0] != -1) && (fd[1] != -1)) {
			WeakReference handles = create_iterator();
			iterator_insert(handles.data<Iterator>(), create_handle(fd[0]));
			iterator_insert(handles.data<Iterator>(), create_handle(fd[1]));
			helper.returnValue(move(handles));
		}
	}
#endif
}

MINT_FUNCTION(mint_pipe_close, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	mint::handle_t handle = to_handle(helper.popParameter());

#ifdef OS_WINDOWS
	CloseHandle(handle);
#else
	close(handle);
#endif
}

MINT_FUNCTION(mint_pipe_read, 2, cursor) {

	FunctionHelper helper(cursor, 2);

	WeakReference stream = move(helper.popParameter());

#ifdef OS_WINDOWS
	DWORD count;
	uint8_t read_buffer[1024];
	mint::handle_t h = to_handle(helper.popParameter());
	vector<uint8_t> *stream_buffer = stream.data<LibObject<vector<uint8_t>>>()->impl;

	while (ReadFile(h, read_buffer, sizeof(read_buffer), &count, nullptr)) {

		if (count < 0) {
			break;
		}

		copy_n(read_buffer, count, back_inserter(*stream_buffer));
	}
#else
	uint8_t read_buffer[1024];
	mint::handle_t fd = to_handle(helper.popParameter());
	vector<uint8_t> *stream_buffer = stream.data<LibObject<vector<uint8_t>>>()->impl;

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

	WeakReference stream = move(helper.popParameter());

#ifdef OS_WINDOWS
	DWORD count;
	mint::handle_t h = to_handle(helper.popParameter());
	vector<uint8_t> *buffer = stream.data<LibObject<vector<uint8_t>>>()->impl;

	WriteFile(h, buffer->data(), buffer->size(), &count, nullptr);
#else
	mint::handle_t fd = to_handle(helper.popParameter());
	vector<uint8_t> *buffer = stream.data<LibObject<vector<uint8_t>>>()->impl;

	write(fd, buffer->data(), buffer->size());
#endif
}

MINT_FUNCTION(mint_pipe_wait, 2, cursor) {

	FunctionHelper helper(cursor, 2);

	WeakReference timeout = move(helper.popParameter());

#ifdef OS_WINDOWS
	mint::handle_t h = to_handle(helper.popParameter());
	DWORD ret = WaitForSingleObjectEx(h, static_cast<DWORD>(to_integer(cursor, timeout)), true);
	helper.returnValue(create_boolean(ret == WAIT_OBJECT_0));
#else
	pollfd fds;
	fds.events = POLLIN;
	fds.fd = to_handle(helper.popParameter());

	int time_ms = -1;

	if (timeout.data()->format != Data::fmt_none) {
		time_ms = static_cast<int>(to_integer(cursor, timeout));
	}

	bool result = false;
	int ret = poll(&fds, 1, time_ms);

	if ((ret > 0) && (fds.revents & POLLIN)) {
		result = true;
	}

	helper.returnValue(create_boolean(result));
#endif
}
