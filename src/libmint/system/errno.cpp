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

#include "mint/system/errno.h"

#include <unordered_map>

#ifdef OS_WINDOWS
#include <Windows.h>
#endif

using namespace mint;

SystemError::SystemError(bool status) :
	SystemError(status, status ? 0 : errno) {

}

SystemError::SystemError(const SystemError &other) noexcept :
	SystemError(other.m_status, other.m_errno) {

}

SystemError::SystemError(bool _status, int _errno) :
	m_status(_status),
	m_errno(_errno) {

}

SystemError &SystemError::operator =(const SystemError &other) noexcept {
	m_status = other.m_status;
	m_errno = other.m_errno;
	return *this;
}

#ifdef OS_WINDOWS
SystemError SystemError::from_windows_last_error() {
	return SystemError(false, errno_from_windows_last_error());
}
#endif

SystemError::operator bool() const {
	return !m_status;
}

int SystemError::get_errno() const {
	return m_errno;
}

#ifdef OS_WINDOWS
int mint::errno_from_windows_last_error() {

	static const std::unordered_map<int, int> g_errno_for = {
		{ ERROR_ACCESS_DENIED,             EACCES },
		{ ERROR_ACTIVE_CONNECTIONS,        EAGAIN },
		{ ERROR_ALREADY_EXISTS,            EEXIST },
		{ ERROR_BAD_DEVICE,                ENODEV },
		{ ERROR_BAD_EXE_FORMAT,            ENOEXEC },
		{ ERROR_BAD_NETPATH,               ENOENT },
		{ ERROR_BAD_NET_NAME,              ENOENT },
		{ ERROR_BAD_NET_RESP,              ENOSYS },
		{ ERROR_BAD_PATHNAME,              ENOENT },
		{ ERROR_BAD_PIPE,                  EINVAL },
		{ ERROR_BAD_UNIT,                  ENODEV },
		{ ERROR_BAD_USERNAME,              EINVAL },
		{ ERROR_BEGINNING_OF_MEDIA,        EIO },
		{ ERROR_BROKEN_PIPE,               EPIPE },
		{ ERROR_BUSY,                      EBUSY },
		{ ERROR_BUS_RESET,                 EIO },
		{ ERROR_CALL_NOT_IMPLEMENTED,      ENOSYS },
		{ ERROR_CANCELLED,                 EINTR },
		{ ERROR_CANNOT_MAKE,               EPERM },
		{ ERROR_CHILD_NOT_COMPLETE,        EBUSY },
		{ ERROR_COMMITMENT_LIMIT,          EAGAIN },
		{ ERROR_CONNECTION_REFUSED,        ECONNREFUSED },
		{ ERROR_CRC,                       EIO },
		{ ERROR_DEVICE_DOOR_OPEN,          EIO },
		{ ERROR_DEVICE_IN_USE,             EAGAIN },
		{ ERROR_DEVICE_REQUIRES_CLEANING,  EIO },
		{ ERROR_DEV_NOT_EXIST,             ENOENT },
		{ ERROR_DIRECTORY,                 ENOTDIR },
		{ ERROR_DIR_NOT_EMPTY,             ENOTEMPTY },
		{ ERROR_DISK_CORRUPT,              EIO },
		{ ERROR_DISK_FULL,                 ENOSPC },
		{ ERROR_DS_GENERIC_ERROR,          EIO },
		// { ERROR_DUP_NAME,                  ENOTUNIQ },
		{ ERROR_EAS_DIDNT_FIT,             ENOSPC },
		{ ERROR_EAS_NOT_SUPPORTED,         ENOTSUP },
		{ ERROR_EA_LIST_INCONSISTENT,      EINVAL },
		{ ERROR_EA_TABLE_FULL,             ENOSPC },
		{ ERROR_END_OF_MEDIA,              ENOSPC },
		{ ERROR_EOM_OVERFLOW,              EIO },
		{ ERROR_EXE_MACHINE_TYPE_MISMATCH, ENOEXEC },
		{ ERROR_EXE_MARKED_INVALID,        ENOEXEC },
		{ ERROR_FILEMARK_DETECTED,         EIO },
		{ ERROR_FILENAME_EXCED_RANGE,      ENAMETOOLONG },
		{ ERROR_FILE_CORRUPT,              EEXIST },
		{ ERROR_FILE_EXISTS,               EEXIST },
		{ ERROR_FILE_INVALID,              ENXIO },
		{ ERROR_FILE_NOT_FOUND,            ENOENT },
		{ ERROR_HANDLE_DISK_FULL,          ENOSPC },
		{ ERROR_HANDLE_EOF,                ENODATA },
		{ ERROR_INVALID_ADDRESS,           EINVAL },
		{ ERROR_INVALID_AT_INTERRUPT_TIME, EINTR },
		{ ERROR_INVALID_BLOCK_LENGTH,      EIO },
		{ ERROR_INVALID_DATA,              EINVAL },
		{ ERROR_INVALID_DRIVE,             ENODEV },
		{ ERROR_INVALID_EA_NAME,           EINVAL },
		{ ERROR_INVALID_EXE_SIGNATURE,     ENOEXEC },
		// { ERROR_INVALID_FUNCTION,          EBADRQC },
		{ ERROR_INVALID_HANDLE,            EBADF },
		{ ERROR_INVALID_NAME,              ENOENT },
		{ ERROR_INVALID_PARAMETER,         EINVAL },
		{ ERROR_INVALID_SIGNAL_NUMBER,     EINVAL },
		{ ERROR_IOPL_NOT_ENABLED,          ENOEXEC },
		{ ERROR_IO_DEVICE,                 EIO },
		{ ERROR_IO_INCOMPLETE,             EAGAIN },
		{ ERROR_IO_PENDING,                EAGAIN },
		{ ERROR_LOCK_VIOLATION,            EBUSY },
		{ ERROR_MAX_THRDS_REACHED,         EAGAIN },
		{ ERROR_META_EXPANSION_TOO_LONG,   EINVAL },
		{ ERROR_MOD_NOT_FOUND,             ENOENT },
		{ ERROR_MORE_DATA,                 EMSGSIZE },
		{ ERROR_NEGATIVE_SEEK,             EINVAL },
		{ ERROR_NETNAME_DELETED,           ENOENT },
		{ ERROR_NOACCESS,                  EFAULT },
		{ ERROR_NONE_MAPPED,               EINVAL },
		{ ERROR_NONPAGED_SYSTEM_RESOURCES, EAGAIN },
		{ ERROR_NOT_CONNECTED,             ENOLINK },
		{ ERROR_NOT_ENOUGH_MEMORY,         ENOMEM },
		{ ERROR_NOT_ENOUGH_QUOTA,          EIO },
		{ ERROR_NOT_OWNER,                 EPERM },
		// { ERROR_NOT_READY,                 ENOMEDIUM },
		{ ERROR_NOT_SAME_DEVICE,           EXDEV },
		{ ERROR_NOT_SUPPORTED,             ENOSYS },
		{ ERROR_NO_DATA,                   EPIPE },
		{ ERROR_NO_DATA_DETECTED,          EIO },
		// { ERROR_NO_MEDIA_IN_DRIVE,         ENOMEDIUM },
		// { ERROR_NO_MORE_FILES,             ENMFILE },
		// { ERROR_NO_MORE_ITEMS,             ENMFILE },
		{ ERROR_NO_MORE_SEARCH_HANDLES,    ENFILE },
		{ ERROR_NO_PROC_SLOTS,             EAGAIN },
		{ ERROR_NO_SIGNAL_SENT,            EIO },
		{ ERROR_NO_SYSTEM_RESOURCES,       EFBIG },
		{ ERROR_NO_TOKEN,                  EINVAL },
		{ ERROR_OPEN_FAILED,               EIO },
		{ ERROR_OPEN_FILES,                EAGAIN },
		{ ERROR_OUTOFMEMORY,               ENOMEM },
		{ ERROR_PAGED_SYSTEM_RESOURCES,    EAGAIN },
		{ ERROR_PAGEFILE_QUOTA,            EAGAIN },
		{ ERROR_PATH_NOT_FOUND,            ENOENT },
		{ ERROR_PIPE_BUSY,                 EBUSY },
		{ ERROR_PIPE_CONNECTED,            EBUSY },
		// { ERROR_PIPE_LISTENING,            ECOMM },
		// { ERROR_PIPE_NOT_CONNECTED,        ECOMM },
		{ ERROR_POSSIBLE_DEADLOCK,         EDEADLOCK },
		{ ERROR_PRIVILEGE_NOT_HELD,        EPERM },
		{ ERROR_PROCESS_ABORTED,           EFAULT },
		{ ERROR_PROC_NOT_FOUND,            ESRCH },
		// { ERROR_REM_NOT_LIST,              ENONET },
		{ ERROR_SECTOR_NOT_FOUND,          EINVAL },
		{ ERROR_SEEK,                      EINVAL },
		{ ERROR_SERVICE_REQUEST_TIMEOUT,   EBUSY },
		{ ERROR_SETMARK_DETECTED,          EIO },
		{ ERROR_SHARING_BUFFER_EXCEEDED,   ENOLCK },
		{ ERROR_SHARING_VIOLATION,         EBUSY },
		{ ERROR_SIGNAL_PENDING,            EBUSY },
		{ ERROR_SIGNAL_REFUSED,            EIO },
		// { ERROR_SXS_CANT_GEN_ACTCTX,       ELIBBAD },
		{ ERROR_THREAD_1_INACTIVE,         EINVAL },
		{ ERROR_TIMEOUT,                   EBUSY },
		{ ERROR_TOO_MANY_LINKS,            EMLINK },
		{ ERROR_TOO_MANY_OPEN_FILES,       EMFILE },
		{ ERROR_UNEXP_NET_ERR,             EIO },
		{ ERROR_WAIT_NO_CHILDREN,          ECHILD },
		{ ERROR_WORKING_SET_QUOTA,         EAGAIN },
		{ ERROR_WRITE_PROTECT,             EROFS },
	};

	auto i = g_errno_for.find(GetLastError());
	return (i != g_errno_for.end()) ? i->second : EINVAL;
}
#endif
