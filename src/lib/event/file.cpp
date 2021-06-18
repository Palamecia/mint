#include <memory/functiontool.h>
#include <memory/casttool.h>
#include <system/filesystem.h>

#ifdef OS_WINDOWS
#include <Windows.h>
using handle_data_t = std::remove_pointer<HANDLE>::type;
#else
#include <sys/file.h>
#include <sys/inotify.h>
#include <stdio_ext.h>
#include <unistd.h>
#include <poll.h>
#endif

using namespace std;
using namespace mint;

enum Changes { name, data, attributes };

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

	switch (*mode) {
	case '+':	/* [rwa]\+ means read and write */
		m = O_RDWR;
		break;
	case '\0':	/* no more flags */
		break;
	default:	/* illegal mode */
		errno = EINVAL;
		return false;
	}

	*optr = m | o;
	return true;
}

bool reset_event(int event_fd) {

	size_t len = 0;
	bool reseted = false;
	uint8_t read_buffer[BUFSIZ];

	while (ssize_t count = read(event_fd, read_buffer, sizeof(read_buffer))) {

		if (count < 0) {
			break;
		}

		for (uint8_t *ptr = read_buffer; ptr < read_buffer + count; ptr += len) {
			inotify_event *event = reinterpret_cast<inotify_event *>(ptr);
			reseted = reseted || (event->mask != 0);
			len = sizeof(inotify_event) + event->len;
		}
	}

	return reseted;
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

MINT_FUNCTION(mint_file_create, 3, cursor) {

	FunctionHelper helper(cursor, 3);

	SharedReference flags = move(helper.popParameter());
	SharedReference mode = move(helper.popParameter());
	SharedReference path = move(helper.popParameter());

	SharedReference handles = create_iterator();

#ifdef OS_WINDOWS
	DWORD dwDesiredAccess, dwCreationDisposition, dwNotifyFilter;

	dwNotifyFilter = 0;

	if (static_cast<intmax_t>(to_number(cursor, flags)) & Changes::name) {
		dwNotifyFilter |= FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME;
	}

	if (static_cast<intmax_t>(to_number(cursor, flags)) & Changes::data) {
		dwNotifyFilter |= FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_SIZE;
	}

	if (static_cast<intmax_t>(to_number(cursor, flags)) & Changes::attributes) {
		dwNotifyFilter |= FILE_NOTIFY_CHANGE_ATTRIBUTES;
	}

	if (mint_sflags(to_string(mode).c_str(), &dwDesiredAccess, &dwCreationDisposition)) {
		HANDLE fd = CreateFileW(string_to_windows_path(to_string(path)).c_str(), dwDesiredAccess, dwDesiredAccess, nullptr, dwCreationDisposition, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (fd != INVALID_HANDLE_VALUE) {
			iterator_insert(handles->data<Iterator>(), create_object(fd));
			HANDLE fe = FindFirstChangeNotificationW(string_to_windows_path(to_string(path)).c_str(), TRUE, dwNotifyFilter);
			if (fe != INVALID_HANDLE_VALUE) {
				iterator_insert(handles->data<Iterator>(), create_object(fe));
			}
		}
	}
#else
	int open_flags = 0;
	uint32_t watch_flags = 0;

	if (static_cast<intmax_t>(to_number(cursor, flags)) & name) {
		watch_flags |= IN_MOVE;
	}

	if (static_cast<intmax_t>(to_number(cursor, flags)) & data) {
		watch_flags |= IN_CREATE | IN_MODIFY | IN_DELETE | IN_DELETE_SELF;
	}

	if (static_cast<intmax_t>(to_number(cursor, flags)) & attributes) {
		watch_flags |= IN_ATTRIB;
	}

	if (mint_sflags(to_string(mode).c_str(), &open_flags)) {
		int fd = open(to_string(path).c_str(), open_flags | O_NONBLOCK);
		if (fd != -1) {
			iterator_insert(handles->data<Iterator>(), create_number(fd));
			int fe = inotify_init1(IN_NONBLOCK);
			if (fe != -1) {
				if (inotify_add_watch(fe, to_string(path).c_str(), watch_flags)) {
					iterator_insert(handles->data<Iterator>(), create_number(fe));
				}
			}
		}
	}
#endif
	helper.returnValue(move(handles));
}

MINT_FUNCTION(mint_file_close_file, 1, cursor) {

	FunctionHelper helper(cursor, 1);

#ifdef OS_WINDOWS
	CloseHandle(helper.popParameter()->data<LibObject<handle_data_t>>()->impl);
#else
	close(static_cast<int>(to_number(cursor, helper.popParameter())));
#endif
}

MINT_FUNCTION(mint_file_close_event, 1, cursor) {

	FunctionHelper helper(cursor, 1);

#ifdef OS_WINDOWS
	CloseHandle(helper.popParameter()->data<LibObject<handle_data_t>>()->impl);
#else
	close(static_cast<int>(to_number(cursor, helper.popParameter())));
#endif
}

MINT_FUNCTION(mint_file_read, 3, cursor) {

	FunctionHelper helper(cursor, 3);

	SharedReference stream = move(helper.popParameter());

#ifdef OS_WINDOWS
	DWORD count = 0;
	uint8_t read_buffer[BUFSIZ];
	HANDLE event_handle = helper.popParameter()->data<LibObject<handle_data_t>>()->impl;
	HANDLE file_handle = helper.popParameter()->data<LibObject<handle_data_t>>()->impl;
	vector<uint8_t> *stream_buffer = stream->data<LibObject<vector<uint8_t>>>()->impl;

	while (ReadFile(file_handle, read_buffer, sizeof(read_buffer), &count, nullptr)) {
		copy_n(read_buffer, count, back_inserter(*stream_buffer));
	}

	ResetEvent(event_handle);
#else
	uint8_t read_buffer[BUFSIZ];
	int fe = static_cast<int>(to_number(cursor, helper.popParameter()));
	int fd = static_cast<int>(to_number(cursor, helper.popParameter()));
	vector<uint8_t> *stream_buffer = stream->data<LibObject<vector<uint8_t>>>()->impl;

	while (ssize_t count = read(fd, read_buffer, sizeof (read_buffer))) {

		if (count < 0) {
			break;
		}

		copy_n(read_buffer, count, back_inserter(*stream_buffer));
	}

	reset_event(fe);
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

MINT_FUNCTION(mint_file_wait, 2, cursor) {

	FunctionHelper helper(cursor, 2);

	SharedReference timeout = move(helper.popParameter());

#ifdef OS_WINDOWS

	DWORD time_ms = INFINITE;
	HANDLE handle = helper.popParameter()->data<LibObject<handle_data_t>>()->impl;

	if (timeout->data()->format != Data::fmt_none) {
		time_ms = static_cast<int>(to_number(cursor, timeout));
	}

	bool result = false;

	if (WaitForSingleObject(handle, time_ms) == WAIT_OBJECT_0) {
		ResetEvent(handle);
		result = true;
	}

	helper.returnValue(create_boolean(result));
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
		result = reset_event(fds.fd);
	}

	helper.returnValue(create_boolean(result));
#endif
}
