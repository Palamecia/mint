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

#ifndef MINT_SYSTEM_ASYNC_IO_H
#define MINT_SYSTEM_ASYNC_IO_H

#include "mint/config.h"
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <optional>
#include <system_error>
#include <unordered_set>
#include <variant>

#if defined(MINT_OS_WINDOWS)
#include <Windows.h>
#elifdef MINT_OS_MAC
#include <sys/event.h>
#elifdef MINT_OS_LINUX
#include <sys/epoll.h>
#if HAS_IO_URING
#include <liburing.h>
#include <liburing/io_uring.h>
#endif
#elif HAS_IO_URING
#include <liburing.h>
#include <liburing/io_uring.h>
#endif

namespace mint {

#ifdef MINT_OS_WINDOWS
#define MINT_ASYNC_BACKEND_IOCP
using handle_t = HANDLE;
using async_operation_t = OVERLAPPED;
inline const handle_t invalid_handle = INVALID_HANDLE_VALUE;
#else
using handle_t = int;
inline const handle_t invalid_handle = -1;
#ifdef MINT_OS_MAC
#define MINT_ASYNC_BACKEND_KQUEUE

using async_operation_t = struct KqueueOperation {
	std::uint16_t filter = 0;
	std::uint16_t flags = 0;
	std::intptr_t result = 0;
	bool pending = false;
};
#elifdef MINT_OS_LINUX
#if HAS_IO_URING
#define MINT_ASYNC_BACKEND_IO_URING

struct IoUringOperation {
	io_uring_sqe* sqe = nullptr;
};
#endif
#define MINT_ASYNC_BACKEND_EPOLL

struct EPollContext {

	int fd = -1;

	std::deque<class AsyncOperation*> ready_operations;
	std::array<epoll_event, 16> events = {};
	std::size_t event_count = 0;
	std::size_t event_index = 0;
};

struct EPollOperation {
	std::uint32_t events = 0;
	std::int64_t result = 0;
	bool started = false;
	bool pending = false;
};

using async_operation_t = std::variant<
#ifdef MINT_ASYNC_BACKEND_IO_URING
    IoUringOperation,
#endif
#ifdef MINT_ASYNC_BACKEND_EPOLL
    EPollOperation,
#endif
    std::monostate>;
#else
#error "AsyncOperation is not implemented for this platform"
#endif
#endif

class MINT_EXPORT AsyncOperation : public async_operation_t {
	handle_t _handle;
public:
	AsyncOperation(handle_t handle);
	AsyncOperation(const AsyncOperation&) = delete;
	AsyncOperation(AsyncOperation&&) = delete;
	virtual ~AsyncOperation() = default;

	AsyncOperation& operator=(const AsyncOperation&) = delete;
	AsyncOperation& operator=(AsyncOperation&&) = delete;

	[[nodiscard]] handle_t get_handle() const;

	virtual std::error_code start() = 0;
	virtual void complete(std::error_code error, std::size_t bytes_transferred) = 0;
};

class MINT_EXPORT AsyncRuntime {
public:
	AsyncRuntime();
	AsyncRuntime(const AsyncRuntime&) = delete;
	AsyncRuntime(AsyncRuntime&&) = delete;
	~AsyncRuntime();

	AsyncRuntime& operator=(const AsyncRuntime&) = delete;
	AsyncRuntime& operator=(AsyncRuntime&&) = delete;

	std::error_code register_handle(handle_t handle);

	bool submit(AsyncOperation& operation);
	bool cancel(AsyncOperation& operation);
	std::error_code post_deferred_completion(AsyncOperation& operation);

	AsyncOperation* poll(std::optional<std::chrono::milliseconds> timeout = std::nullopt);


private:
#ifdef MINT_ASYNC_BACKEND_IOCP
	HANDLE _context = INVALID_HANDLE_VALUE;
#elifdef MINT_ASYNC_BACKEND_KQUEUE
	int _context = -1;
#elifdef MINT_OS_LINUX
	std::variant<
#ifdef MINT_ASYNC_BACKEND_IO_URING
	    io_uring,
#endif
#ifdef MINT_ASYNC_BACKEND_EPOLL
	    EPollContext,
#endif
	    std::monostate>
	    _context;
#else
#error "AsyncRuntime is not implemented for this platform"
#endif
	std::unordered_set<AsyncOperation*> _operations;
	std::mutex _mutex;
};

}

#endif // MINT_SYSTEM_ASYNC_IO_H
