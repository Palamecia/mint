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

#include "mint/memory/builtin/iterator.h"
#include "mint/memory/builtin/libobject.h"
#include "mint/memory/data.h"
#include "mint/memory/function_tools.h"
#include "mint/memory/cast_tools.h"
#include "mint/memory/memory_tools.h"
#include "mint/memory/reference.h"
#include "mint/system/async_io.h"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <cstdint>
#include <array>
#include <iterator>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <type_traits>
#include <vector>

#ifdef MINT_OS_WINDOWS
#include <Windows.h>
#include <errhandlingapi.h>
#include <fileapi.h>
#include <handleapi.h>
#include <ioapiset.h>
#include <minwinbase.h>
#include <minwindef.h>
#include <synchapi.h>
#include <winbase.h>
#include <winerror.h>
#include <winnt.h>
#include <generator>
#else
#ifdef MINT_OS_LINUX
#include <stdio_ext.h>
#include <sys/file.h>
#include <sys/inotify.h>
#include <sys/epoll.h>
#else
#include <sys/event.h>
#endif
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#endif

namespace {

enum Changes : std::uint8_t {
	name = 0x01,
	data = 0x02,
	attributes = 0x04
};

Changes to_changes(mint::Cursor& cursor, const mint::Reference& value) {
	return static_cast<Changes>(to_integer<std::underlying_type_t<Changes>>(cursor, value));
}

struct FileWatcherData {
	mint::poll_event_t event = {};
#ifdef MINT_OS_WINDOWS
	HANDLE directory_handle = INVALID_HANDLE_VALUE;
	std::wstring target_name;
	DWORD notify_filter = 0;
	OVERLAPPED overlapped = {};
	std::array<BYTE, 4096> buffer = {};
#endif
};

#ifdef MINT_OS_WINDOWS
std::generator<FILE_NOTIFY_INFORMATION*> to_file_notify_informations(std::span<BYTE> buffer) {

	auto* notify_info = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(buffer.data());
	co_yield notify_info;

	while (notify_info->NextEntryOffset != 0) {
		notify_info = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(
		    std::next(reinterpret_cast<BYTE*>(notify_info), notify_info->NextEntryOffset));
		co_yield notify_info;
	}
}

void CALLBACK file_watcher_notification_callback(DWORD error_code, DWORD bytes_transferred, LPOVERLAPPED overlapped) {

	if (error_code != ERROR_SUCCESS || bytes_transferred == 0) {
		return;
	}

	auto* watcher = CONTAINING_RECORD(overlapped, FileWatcherData, overlapped);
	for (auto* notify_info : to_file_notify_informations(watcher->buffer)) {
		auto file_name = std::wstring(notify_info->FileName, notify_info->FileNameLength / sizeof(wchar_t));
		if (file_name == watcher->target_name) {
			SetEvent(watcher->event.handle);
			break;
		}
	}

	auto bytes_returned = DWORD();
	ReadDirectoryChangesW(watcher->directory_handle, watcher->buffer.data(), static_cast<DWORD>(watcher->buffer.size()),
	    false, watcher->notify_filter, &bytes_returned, &watcher->overlapped, &file_watcher_notification_callback);
}
#endif

mint::Reference mint_file_watcher_create(mint::Cursor& cursor, const mint::Reference& path, mint::Reference& flags) {
#ifdef MINT_OS_WINDOWS

	auto watcher = std::make_unique<FileWatcherData>();

	if (to_changes(cursor, flags) & Changes::name) {
		watcher->notify_filter |= FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME;
	}
	if (to_changes(cursor, flags) & Changes::data) {
		watcher->notify_filter |= FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_SIZE;
	}
	if (to_changes(cursor, flags) & Changes::attributes) {
		watcher->notify_filter |= FILE_NOTIFY_CHANGE_ATTRIBUTES;
	}

	bool is_directory_watch = false;
	auto dir_to_open = std::filesystem::path();
	auto path_to_watch = std::filesystem::path(mint::to_string(path));
	if (std::filesystem::is_directory(path_to_watch)) {
		dir_to_open = path_to_watch;
		is_directory_watch = true;
	}
	else {
		dir_to_open = path_to_watch.parent_path();
		watcher->target_name = path_to_watch.filename().generic_wstring();
		is_directory_watch = false;
	}

	watcher->directory_handle = CreateFileW(dir_to_open.generic_wstring().c_str(), FILE_LIST_DIRECTORY,
	    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
	    FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, nullptr);

	if (watcher->directory_handle == INVALID_HANDLE_VALUE) {
		return {};
	}

	auto bytes_returned = DWORD();
	if (is_directory_watch) {
		watcher->overlapped.hEvent = watcher->directory_handle;

		const auto success = ReadDirectoryChangesW(watcher->directory_handle, watcher->buffer.data(),
		    static_cast<DWORD>(watcher->buffer.size()), false, watcher->notify_filter, &bytes_returned,
		    &watcher->overlapped, nullptr);

		if (!success && GetLastError() != ERROR_IO_PENDING) {
			return {};
		}

		watcher->event.handle = watcher->directory_handle;
	}
	else {
		watcher->overlapped.hEvent = nullptr;

		const auto success = ReadDirectoryChangesW(watcher->directory_handle, watcher->buffer.data(),
		    static_cast<DWORD>(watcher->buffer.size()), false, watcher->notify_filter, &bytes_returned,
		    &watcher->overlapped, &file_watcher_notification_callback);

		if (!success && GetLastError() != ERROR_IO_PENDING) {
			return {};
		}

		watcher->event.handle = CreateEvent(nullptr, false, false, nullptr);
		if (watcher->event.handle == INVALID_HANDLE_VALUE) {
			return {};
		}
	}

	return mint::create_c_object(cursor.ast(), watcher.release());

#elifdef MINT_OS_LINUX

	auto watcher = std::make_unique<FileWatcherData>(FileWatcherData {
	    .event =
	        {
	            .fd = inotify_init1(IN_NONBLOCK),
	            .events = EPOLLIN,
	            .on_signal =
	                [](mint::poll_event_t& event) {
		                constexpr auto buf_len = 1024 * (sizeof(struct inotify_event) + 16);
		                auto buffer = std::array<char, buf_len>();
		                for (auto length = read(event.fd, buffer.data(), buffer.size()); length > 0;
		                    length = read(event.fd, buffer.data(), buffer.size())) {
		                }
	                },
	        },
	});

	if (watcher->event.fd == -1) {
		return {};
	}

	auto watch_flags = std::uint32_t();

	if (to_changes(cursor, flags) & Changes::name) {
		watch_flags |= IN_MOVE;
	}

	if (to_changes(cursor, flags) & Changes::data) {
		watch_flags |= IN_CREATE | IN_MODIFY | IN_DELETE | IN_DELETE_SELF;
	}

	if (to_changes(cursor, flags) & Changes::attributes) {
		watch_flags |= IN_ATTRIB;
	}

	if (!inotify_add_watch(watcher->event.fd, to_string(path).c_str(), watch_flags)) {
		return {};
	}

	return mint::create_c_object(cursor.ast(), watcher.release());

#elifdef MINT_OS_UNIX

	auto watcher = std::make_unique<FileWatcherData>(FileWatcherData {
	    .event =
	        {
#ifdef MINT_OS_MACOS
	            .fd = open(to_string(path).c_str(), O_RDONLY | O_EVTONLY | O_NONBLOCK),
#else
	            .fd = open(to_string(path).c_str(), O_RDONLY | O_NONBLOCK),
#endif
	            .filter = EVFILT_VNODE,
	        },
	});

	if (watcher->event.fd == -1) {
		return {};
	}

	if (to_changes(cursor, flags) & Changes::name) {
		watcher->event.fflags |= NOTE_RENAME;
	}
	if (to_changes(cursor, flags) & Changes::data) {
		watcher->event.fflags |= NOTE_WRITE | NOTE_DELETE;
	}
	if (to_changes(cursor, flags) & Changes::attributes) {
		watcher->event.fflags |= NOTE_ATTRIB;
	}

	return mint::create_c_object(cursor.ast(), watcher.release());
#endif
}

mint::Reference mint_file_watcher_delete(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr) {
	auto* watcher = d_ptr.data<mint::LibObject<FileWatcherData>>().ptr;
#ifdef MINT_OS_WINDOWS
	if (watcher->directory_handle != watcher->event.handle) {
		CancelIo(watcher->directory_handle);
		CloseHandle(watcher->directory_handle);
	}
	CloseHandle(watcher->event.handle);
#else
	close(watcher->event.fd);
#endif
	delete watcher;
	return {};
}

mint::Reference mint_file_watcher_get_handle(mint::Cursor& cursor, const mint::Reference& d_ptr) {
	auto* watcher = d_ptr.data<mint::LibObject<FileWatcherData>>().ptr;
#ifdef MINT_OS_WINDOWS
	return mint::create_handle(cursor.ast(), watcher->event.handle);
#else
	return mint::create_handle(cursor.ast(), watcher->event.fd);
#endif
}

mint::Reference mint_file_watcher_get_poll_event(mint::Cursor& cursor, const mint::Reference& d_ptr) {
	return mint::create_c_object(cursor.ast(), &d_ptr.data<mint::LibObject<FileWatcherData>>().ptr->event);
}

mint::Reference mint_file_watcher_wait(mint::Cursor& cursor, const mint::Reference& d_ptr,
    const mint::Reference& timeout) {

	auto* watcher = d_ptr.data<mint::LibObject<FileWatcherData>>().ptr;

#ifdef MINT_OS_WINDOWS

	const DWORD timeout_ms = mint::is_instance_of(timeout, mint::Data::Format::none)
	                             ? INFINITE
	                             : mint::to_integer<DWORD>(cursor, timeout);

	if (WaitForSingleObject(to_handle(d_ptr), timeout_ms) == WAIT_OBJECT_0) {
		ResetEvent(to_handle(d_ptr));
		return mint::create_boolean(true);
	}

	return mint::create_boolean(false);

#elifdef MINT_OS_LINUX

	const int timeout_ms = is_instance_of(timeout, mint::Data::Format::none) ? -1 : to_integer<int>(cursor, timeout);

	const auto context = epoll_create1(EPOLL_CLOEXEC);
	if (context == -1) {
		return mint::create_boolean(false);
	}

	auto changelist = epoll_event {
	    .events = watcher->event.events,
	    .data =
	        {
	            .fd = watcher->event.fd,
	        },
	};

	const auto ret = epoll_wait(context, &changelist, 1, timeout_ms);
	if (ret > 0 && changelist.events & EPOLLIN) {
		watcher->event.on_signal(watcher->event);
	}

	::close(context);
	return mint::create_boolean(ret > 0);

#elifdef MINT_OS_UNIX

	const auto timeout_ts = is_instance_of(timeout, mint::Data::Format::none)
	                            ? std::nullopt
	                            : std::optional<std::chrono::milliseconds>(mint::to_integer<int>(cursor, timeout))
	                                  .transform([](std::chrono::milliseconds ms) {
		                                  const auto sec = std::chrono::floor<std::chrono::seconds>(ms);
		                                  const auto nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(
		                                      ms - sec);
		                                  return timespec {
		                                      .tv_sec = sec.count(),
		                                      .tv_nsec = nsec.count(),
		                                  };
	                                  });

	int context = kqueue();
	if (context == -1) {
		return mint::create_boolean(false);
	}

	struct kevent changelist {
	    .ident = static_cast<std::uintptr_t>(watcher->event.fd),
	    .filter = watcher->event.filter,
	    .flags = watcher->event.flags,
	    .fflags = watcher->event.fflags,
	};

	struct kevent eventlist {};
	const auto ret = kevent(context, &changelist, 1, &eventlist, 1, timeout_ts ? std::to_address(timeout_ts) : nullptr);

	::close(context);
	return mint::create_boolean(ret > 0);

#endif
}

}

MINT_EXPORT_FUNCTION(mint_file_watcher_create, 2)
MINT_EXPORT_FUNCTION(mint_file_watcher_delete, 1)
MINT_EXPORT_FUNCTION(mint_file_watcher_get_handle, 1)
MINT_EXPORT_FUNCTION(mint_file_watcher_get_poll_event, 1)
MINT_EXPORT_FUNCTION(mint_file_watcher_wait, 2)
