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

#include "mint/system/async_io.h"
#include "mint/system/errno.h"

#include <Windows.h>
#include <basetsd.h>
#include <cassert>
#include <chrono>
#include <errhandlingapi.h>
#include <handleapi.h>
#include <ioapiset.h>
#include <minwinbase.h>
#include <minwindef.h>
#include <optional>
#include <mutex>
#include <system_error>
#include <winbase.h>
#include <winerror.h>

#ifdef MINT_ASYNC_BACKEND_IOCP

mint::AsyncOperation::AsyncOperation(handle_t handle) :
    async_operation_t({}),
    _handle(handle) {}

mint::AsyncRuntime::AsyncRuntime() :
    _context(CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0)) {
	if (!_context) {
		_context = INVALID_HANDLE_VALUE;
	}
}

mint::AsyncRuntime::~AsyncRuntime() {
	if (_context && _context != INVALID_HANDLE_VALUE) {
		CloseHandle(_context);
	}
}

std::error_code mint::AsyncRuntime::register_handle(handle_t handle) {

	if (_context == INVALID_HANDLE_VALUE) [[unlikely]] {
		return std::make_error_code(std::errc::state_not_recoverable);
	}

	if (!CreateIoCompletionPort(handle, _context, 0, 0)) {
		return mint::last_error_code();
	}

	return {};
}

bool mint::AsyncRuntime::submit(AsyncOperation& operation) {

	if (_context == INVALID_HANDLE_VALUE) [[unlikely]] {
		return false;
	}

	if (const auto error = operation.start()) {
		operation.complete(error, 0);
		return false;
	}

	const std::scoped_lock lock(_mutex);
	_operations.emplace(&operation);
	return true;
}

bool mint::AsyncRuntime::cancel(AsyncOperation& operation) {

	if (_context == INVALID_HANDLE_VALUE) [[unlikely]] {
		return false;
	}

	const std::scoped_lock lock(_mutex);

	if (auto it = _operations.find(&operation); it != _operations.end()) {
		_operations.erase(it);
		return true;
	}

	return false;
}

std::error_code mint::AsyncRuntime::post_deferred_completion(AsyncOperation& operation) {
	if (!PostQueuedCompletionStatus(_context, 0, 0, &operation)) {
		return last_error_code();
	}
	return {};
}

mint::AsyncOperation* mint::AsyncRuntime::poll(std::optional<std::chrono::milliseconds> timeout) {

	if (_context == INVALID_HANDLE_VALUE) [[unlikely]] {
		return nullptr;
	}

	DWORD bytes_transferred = 0;
	ULONG_PTR completion_key = 0;
	LPOVERLAPPED overlapped = nullptr;

	const auto error = GetQueuedCompletionStatus(_context, &bytes_transferred, &completion_key, &overlapped,
	                       timeout
	                           .transform([](std::chrono::milliseconds ms) {
		                           return static_cast<DWORD>(ms.count());
	                           })
	                           .value_or(INFINITE))
	                       ? std::error_code()
	                       : last_error_code();

	auto* operation = static_cast<AsyncOperation*>(overlapped);
	if (operation == nullptr) {
		return nullptr;
	}

	const std::scoped_lock lock(_mutex);
	if (auto it = _operations.find(operation); it != _operations.end()) {
		_operations.erase(it);
		operation->complete(error, bytes_transferred);
		return operation;
	}

	return nullptr;
}

#else
#error "AsyncRuntime is not implemented for this platform"
#endif
