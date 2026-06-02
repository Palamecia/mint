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

#include "mint/ast/cursor.h"
#include "mint/config.h"
#include "mint/memory/builtin/libobject.h"
#include "mint/memory/cast_tools.h"
#include "mint/memory/class.h"
#include "mint/memory/data.h"
#include "mint/memory/function_tools.h"
#include "mint/memory/memory_tools.h"
#include "mint/memory/reference.h"
#include "mint/scheduler/processor.h"
#include "mint/system/async_io.h"
#include "mint/system/errno.h"
#include "socket.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <format>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#ifdef MINT_OS_WINDOWS
#include <Windows.h>
#include <WinSock2.h>
#include <winsock.h>
#include <MSWSock.h>
#include <afunix.h>
#include <errhandlingapi.h>
#include <expected>
#include <fileapi.h>
#include <future>
#include <handleapi.h>
#include <minwindef.h>
#include <namedpipeapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <winbase.h>
#include <winerror.h>
#include <winnt.h>
#else
#ifdef MINT_ASYNC_BACKEND_IO_URING
#include <liburing.h>
#endif
#ifdef MINT_ASYNC_BACKEND_EPOLL
#include <sys/epoll.h>
#endif
#ifdef MINT_OS_LINUX
#include <linux/sockios.h>
#endif
#include <asm-generic/socket.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <variant>
#define UNIX_PATH_MAX sizeof(sockaddr_un::sun_path)
#endif

namespace {

enum class IOStatus : std::uint8_t {
	success,
	would_block,
	closed
};

class LocalSocket {
public:
	LocalSocket() = default;
	LocalSocket(const LocalSocket&) = default;
	LocalSocket(LocalSocket&&) = delete;
	virtual ~LocalSocket() = default;

	LocalSocket& operator=(const LocalSocket&) = default;
	LocalSocket& operator=(LocalSocket&&) = delete;

	[[nodiscard]] virtual SOCKET get_socket() const = 0;

	virtual std::unique_ptr<mint::MintAsyncOperation> connect_async(mint::FunctionHelper& helper, mint::Reference self,
	    mint::AsyncRuntime* scheduler, const std::string& name, const sockaddr* address, socklen_t address_length) = 0;
	virtual IOStatus connect(const std::string& name, const sockaddr* address, socklen_t address_length) = 0;

	virtual std::unique_ptr<mint::MintAsyncOperation> read_some_async(mint::FunctionHelper& helper,
	    mint::Reference self, std::vector<std::uint8_t>* buf, std::size_t count) = 0;
	virtual IOStatus read_some(std::vector<std::uint8_t>* buf, std::size_t count) = 0;

	virtual std::unique_ptr<mint::MintAsyncOperation> read_async(mint::FunctionHelper& helper, mint::Reference self,
	    std::vector<std::uint8_t>* buf) = 0;
	virtual IOStatus read(std::vector<std::uint8_t>* buf) = 0;

	virtual std::unique_ptr<mint::MintAsyncOperation> write_async(mint::FunctionHelper& helper, mint::Reference self,
	    std::vector<std::uint8_t>* buf) = 0;
	virtual std::tuple<IOStatus, std::size_t> write(const std::vector<std::uint8_t>* buf) = 0;

	virtual void listen(const std::string& name, const sockaddr* address, socklen_t address_length) = 0;

	virtual std::unique_ptr<mint::MintAsyncOperation> accept_async(mint::Cursor& cursor, mint::Reference self,
	    mint::Reference handle) = 0;
	virtual std::unique_ptr<LocalSocket> accept(mint::Reference& handle) = 0;

	virtual void close() = 0;

	virtual void set_non_blocking(bool enabled) = 0;
};

#ifdef MINT_OS_WINDOWS
class NamedPipeLocalSocket : public LocalSocket {
	SOCKET _socket = INVALID_SOCKET;
	std::filesystem::path _path;
	bool _connected = false;
	bool _blocking = true;
public:
	NamedPipeLocalSocket() = default;

	NamedPipeLocalSocket(SOCKET socket, std::filesystem::path path) :
	    _socket(socket),
	    _path(std::move(path)),
	    _connected(true),
	    _blocking(mint_network::SocketManager::instance().is_socket_blocking(socket)) {}

	[[nodiscard]] SOCKET get_socket() const override {
		return _socket;
	}

	void set_socket(SOCKET socket) {
		_socket = socket;
	}

	std::unique_ptr<mint::MintAsyncOperation> connect_async(mint::FunctionHelper& helper, mint::Reference self,
	    mint::AsyncRuntime* scheduler, const std::string& name, const sockaddr* /*address*/,
	    socklen_t /*address_length*/) override {
		class AsyncConnectOperation : public mint::MintAsyncOperation {
			std::reference_wrapper<mint::AsyncRuntime> _scheduler;
			std::reference_wrapper<mint::Cursor> _cursor;
			NamedPipeLocalSocket* _socket;
			mint::Reference _io_status;
			std::filesystem::path _path;
			std::future<void> _future;
			std::expected<IOStatus, std::error_code> _result;
		public:
			AsyncConnectOperation(mint::FunctionHelper& helper, mint::Reference self, mint::AsyncRuntime& scheduler,
			    NamedPipeLocalSocket* socket, std::filesystem::path path) :
			    mint::MintAsyncOperation(std::move(self), std::bit_cast<mint::handle_t>(socket->get_socket())),
			    _scheduler(scheduler),
			    _cursor(helper.cursor()),
			    _socket(socket),
			    _io_status(helper.reference(mint_network::symbols::network)
			            .member(mint_network::symbols::socket)
			            .member(mint_network::symbols::io_status)
			            .get()),
			    _path(std::move(path)) {}

			std::error_code start() override {
#ifdef MINT_ASYNC_BACKEND_IOCP
				_future = std::async([this]() {
					_result = std::invoke([this]() -> std::expected<IOStatus, std::error_code> {
						while (WaitNamedPipeW(_path.wstring().data(), NMPWAIT_WAIT_FOREVER)) {
							if (auto* handle = CreateFileW(_path.wstring().data(), GENERIC_READ | GENERIC_WRITE, 0,
							        nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
							    handle != mint::invalid_handle) {
								auto socket_fd = mint_network::SocketManager::instance().open_socket_from_handle(handle);
								mint_network::SocketManager::instance().set_socket_listening(socket_fd, false);
								_socket->set_socket(socket_fd);
								return IOStatus::success;
							}
							switch (const auto error = mint::last_error_code(); mint::errno_from_error_code(error)) {
							case EBUSY:
								continue;
							case EPIPE:
								return IOStatus::closed;
							default:
								return std::unexpected(error);
							}
						}
						std::unreachable();
					});
					_scheduler.get().post_deferred_completion(*this);
				});
#else
#error "This operation is not implemented for this platform"
#endif
				return {};
			}

			void complete(std::error_code error, std::size_t /*bytes_transferred*/) override {
				if (error) {
					done(mint::create_iterator_from(_cursor,
					    mint::get_global_ignore_visibility(_io_status.data<mint::Object>(),
					        mint_network::symbols::io_error),
					    mint::create_null(), mint::create_number(error.value())));
				}
				else {
#ifdef MINT_ASYNC_BACKEND_IOCP
					if (_result) {
						const auto socket_fd = _socket->get_socket();
						switch (*_result) {
						case IOStatus::success:
							done(mint::create_iterator_from(_cursor,
							    mint::get_global_ignore_visibility(_io_status.data<mint::Object>(),
							        mint_network::symbols::io_success),
							    mint::create_c_object<LocalSocket>(_cursor.get().ast(), _socket),
							    mint_network::create_socket(_cursor.get().ast(), socket_fd)));
							break;
						case IOStatus::would_block:
							done(mint::create_iterator_from(_cursor,
							    mint::get_global_ignore_visibility(_io_status.data<mint::Object>(),
							        mint_network::symbols::io_would_block),
							    mint::create_c_object<LocalSocket>(_cursor.get().ast(), _socket),
							    mint_network::create_socket(_cursor.get().ast(), socket_fd)));
							break;
						case IOStatus::closed:
							done(mint::create_iterator_from(_cursor,
							    mint::get_global_ignore_visibility(_io_status.data<mint::Object>(),
							        mint_network::symbols::io_closed),
							    mint::create_null()));
							break;
						}
					}
					else {
						done(mint::create_iterator_from(_cursor,
						    mint::get_global_ignore_visibility(_io_status.data<mint::Object>(),
						        mint_network::symbols::io_error),
						    mint::create_null(), mint::create_number(_result.error().value())));
					}
#else
#error "This operation is not implemented for this platform"
#endif
				}
			}
		};

		return std::make_unique<AsyncConnectOperation>(helper, std::move(self), *scheduler, this, make_path(name));
	}

	IOStatus connect(const std::string& name, const sockaddr* /*address*/, socklen_t /*address_length*/) override {
		_path = make_path(name);
		if (auto* handle = CreateFileW(_path.wstring().data(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING,
		        FILE_FLAG_OVERLAPPED, nullptr);
		    handle != mint::invalid_handle) {
			_socket = mint_network::SocketManager::instance().open_socket_from_handle(handle);
			mint_network::SocketManager::instance().set_socket_listening(_socket, false);
			return IOStatus::success;
		}
		switch (const auto error = mint::last_error_code(); mint::errno_from_error_code(error)) {
		case EBUSY:
			return IOStatus::would_block;
		case EPIPE:
			return IOStatus::closed;
		default:
			throw std::system_error(error);
		}
	}

	std::unique_ptr<mint::MintAsyncOperation> read_some_async(mint::FunctionHelper& helper, mint::Reference self,
	    std::vector<std::uint8_t>* buf, std::size_t count) override {
		class AsyncReadSomeOperation : public mint::MintAsyncOperation {
			std::reference_wrapper<mint::Cursor> _cursor;
			mint::Reference _io_status;
			std::vector<std::uint8_t>* _buf;
			std::unique_ptr<char[]> _local_buffer;
			std::size_t _buffer_length;
		public:
			AsyncReadSomeOperation(mint::FunctionHelper& helper, mint::Reference self, SOCKET socket_fd,
			    std::vector<std::uint8_t>* buf, socklen_t buffer_length) :
			    mint::MintAsyncOperation(std::move(self), std::bit_cast<mint::handle_t>(socket_fd)),
			    _cursor(helper.cursor()),
			    _io_status(helper.reference(mint_network::symbols::network)
			            .member(mint_network::symbols::socket)
			            .member(mint_network::symbols::io_status)
			            .get()),
			    _buf(buf),
			    _local_buffer(std::make_unique<char[]>(buffer_length)),
			    _buffer_length(buffer_length) {}

			std::error_code start() override {
#ifdef MINT_ASYNC_BACKEND_IOCP
				if (!::ReadFile(get_handle(), _local_buffer.get(), static_cast<DWORD>(_buffer_length), nullptr, this)) {
					switch (GetLastError()) {
					case ERROR_IO_PENDING:
						break;
					default:
						return mint::last_error_code();
					}
				}
#else
#error "This operation is not implemented for this platform"
#endif
				return {};
			}

			void complete(std::error_code error, std::size_t bytes_transferred) override {
				if (error) {
					done(mint::create_iterator_from(_cursor,
					    mint::get_global_ignore_visibility(_io_status.data<mint::Object>(),
					        mint_network::symbols::io_error),
					    mint::create_number(error.value())));
				}
				else {
					_buf->append_range(std::span(_local_buffer.get(), bytes_transferred));
					done(mint::create_iterator_from(_cursor,
					    mint::get_global_ignore_visibility(_io_status.data<mint::Object>(),
					        mint_network::symbols::io_success)));
				}
			}
		};

		return std::make_unique<AsyncReadSomeOperation>(helper, std::move(self), _socket, buf,
		    static_cast<socklen_t>(count));
	}

	IOStatus read_some(std::vector<std::uint8_t>* buf, std::size_t count) override {

		auto bytes_read = DWORD();
		auto read_buffer = std::make_unique<std::uint8_t[]>(count);

		mint::unlock_processor();
		const auto read_result = ReadFile(std::bit_cast<HANDLE>(_socket), read_buffer.get(), static_cast<DWORD>(count),
		    &bytes_read, nullptr);
		mint::lock_processor();

		if (read_result) {
			buf->append_range(std::span(read_buffer.get(), bytes_read));
			return IOStatus::success;
		}
		switch (const auto error = mint::last_error_code(); mint::errno_from_error_code(error)) {
		case EWOULDBLOCK:
		case EINPROGRESS:
			mint_network::SocketManager::instance().set_socket_blocked(_socket, true);
			return IOStatus::would_block;
		case EPIPE:
			return IOStatus::closed;
		default:
			throw std::system_error(error);
		}
	}

	std::unique_ptr<mint::MintAsyncOperation> read_async(mint::FunctionHelper& helper, mint::Reference self,
	    std::vector<std::uint8_t>* buf) override {
		class AsyncReadOperation : public mint::MintAsyncOperation {
			std::reference_wrapper<mint::Cursor> _cursor;
			mint::Reference _io_status;
			std::vector<std::uint8_t>* _buf;
			std::array<char, BUFSIZ> _local_buffer {};
		public:
			AsyncReadOperation(mint::FunctionHelper& helper, mint::Reference self, SOCKET socket_fd,
			    std::vector<std::uint8_t>* buf) :
			    mint::MintAsyncOperation(std::move(self), std::bit_cast<mint::handle_t>(socket_fd)),
			    _cursor(helper.cursor()),
			    _io_status(helper.reference(mint_network::symbols::network)
			            .member(mint_network::symbols::socket)
			            .member(mint_network::symbols::io_status)
			            .get()),
			    _buf(buf) {}

			std::error_code start() override {
#ifdef MINT_ASYNC_BACKEND_IOCP
				if (!::ReadFile(get_handle(), _local_buffer.data(), static_cast<DWORD>(_local_buffer.size()), nullptr,
				        this)) {
					switch (GetLastError()) {
					case ERROR_IO_PENDING:
						break;
					default:
						return mint::last_error_code();
					}
				}
#else
#error "This operation is not implemented for this platform"
#endif
				return {};
			}

			void complete(std::error_code error, std::size_t bytes_transferred) override {
				if (error) {
					done(mint::create_iterator_from(_cursor,
					    mint::get_global_ignore_visibility(_io_status.data<mint::Object>(),
					        mint_network::symbols::io_error),
					    mint::create_number(error.value())));
				}
				else {
					_buf->append_range(std::span(_local_buffer.data(), bytes_transferred));
					done(mint::create_iterator_from(_cursor,
					    mint::get_global_ignore_visibility(_io_status.data<mint::Object>(),
					        mint_network::symbols::io_success)));
				}
			}
		};

		return std::make_unique<AsyncReadOperation>(helper, std::move(self), _socket, buf);
	}

	IOStatus read(std::vector<std::uint8_t>* buf) override {

		auto bytes_read = DWORD();
		auto read_buffer = std::array<std::uint8_t, BUFSIZ>();

		mint::unlock_processor();
		const auto read_result = ReadFile(std::bit_cast<HANDLE>(_socket), read_buffer.data(),
		    static_cast<DWORD>(read_buffer.size()), &bytes_read, nullptr);
		mint::lock_processor();

		if (read_result) {
			buf->append_range(std::span(read_buffer.data(), bytes_read));
			return IOStatus::success;
		}
		switch (const auto error = mint::last_error_code(); mint::errno_from_error_code(error)) {
		case EWOULDBLOCK:
		case EINPROGRESS:
			mint_network::SocketManager::instance().set_socket_blocked(_socket, true);
			return IOStatus::would_block;
		case EPIPE:
			return IOStatus::closed;
		default:
			throw std::system_error(error);
		}
	}

	std::unique_ptr<mint::MintAsyncOperation> write_async(mint::FunctionHelper& helper, mint::Reference self,
	    std::vector<std::uint8_t>* buf) override {
		class AsyncWriteOperation : public mint::MintAsyncOperation {
			std::reference_wrapper<mint::Cursor> _cursor;
			mint::Reference _io_status;
			std::span<std::uint8_t> _buffer;
			std::size_t _offset = 0;
		public:
			AsyncWriteOperation(mint::FunctionHelper& helper, mint::Reference self, SOCKET socket_fd,
			    std::vector<std::uint8_t>* buffer) :
			    mint::MintAsyncOperation(std::move(self), std::bit_cast<mint::handle_t>(socket_fd)),
			    _cursor(helper.cursor()),
			    _io_status(helper.reference(mint_network::symbols::network)
			            .member(mint_network::symbols::socket)
			            .member(mint_network::symbols::io_status)
			            .get()),
			    _buffer(*buffer) {}

			std::error_code start() override {
#ifdef MINT_ASYNC_BACKEND_IOCP
				if (!WriteFile(get_handle(), _buffer.subspan(_offset).data(),
				        static_cast<DWORD>(_buffer.size() - _offset), nullptr, this)) {
					switch (GetLastError()) {
					case ERROR_IO_PENDING:
						break;
					default:
						return mint::last_error_code();
					}
				}
#else
#error "This operation is not implemented for this platform"
#endif
				return {};
			}

			void complete(std::error_code error, std::size_t bytes_transferred) override {
				if (error) {
					done(mint::create_iterator_from(_cursor,
					    mint::get_global_ignore_visibility(_io_status.data<mint::Object>(),
					        mint_network::symbols::io_error),
					    mint::create_number(error.value())));
				}
				else {
					_offset += bytes_transferred;
					done(mint::create_iterator_from(_cursor,
					    mint::get_global_ignore_visibility(_io_status.data<mint::Object>(),
					        mint_network::symbols::io_success),
					    mint::create_unsigned_number(bytes_transferred)));
				}
			}
		};

		return std::make_unique<AsyncWriteOperation>(helper, std::move(self), _socket, buf);
	}

	std::tuple<IOStatus, std::size_t> write(const std::vector<std::uint8_t>* buf) override {
		auto count = DWORD();
		if (WriteFile(std::bit_cast<HANDLE>(_socket), buf->data(), static_cast<DWORD>(buf->size()), &count, nullptr)) {
			return {IOStatus::success, static_cast<std::size_t>(count)};
		}
		switch (const auto error = mint::last_error_code(); mint::errno_from_error_code(error)) {
		case EWOULDBLOCK:
		case EINPROGRESS:
			mint_network::SocketManager::instance().set_socket_blocked(_socket, true);
			return {IOStatus::would_block, 0uz};
		case EPIPE:
			return {IOStatus::closed, 0uz};
		default:
			throw std::system_error(error);
		}
	}

	void listen(const std::string& name, const sockaddr* /*address*/, socklen_t /*address_length*/) override {
		_path = make_path(name);
		auto* handle = create_listener();
		if (handle == mint::invalid_handle) {
			throw std::system_error(mint::last_error_code());
		}
		_socket = mint_network::SocketManager::instance().open_socket_from_handle(handle);
		mint_network::SocketManager::instance().set_socket_listening(_socket, true);
	}

	std::unique_ptr<mint::MintAsyncOperation> accept_async(mint::Cursor& cursor, mint::Reference self,
	    mint::Reference handle) override {
		class AsyncAcceptOperation : public mint::MintAsyncOperation {
			std::reference_wrapper<NamedPipeLocalSocket> _socket;
			std::reference_wrapper<mint::Cursor> _cursor;
			std::reference_wrapper<SOCKET> _socket_fd;
			mint::Reference _handle;
		public:
			AsyncAcceptOperation(mint::Cursor& cursor, mint::Reference self, SOCKET& socket_fd,
			    NamedPipeLocalSocket& data, mint::Reference handle) :
			    mint::MintAsyncOperation(std::move(self), std::bit_cast<mint::handle_t>(socket_fd)),
			    _socket(data),
			    _cursor(cursor),
			    _socket_fd(socket_fd),
			    _handle(std::move(handle)) {}

			std::error_code start() override {
				try {
#ifdef MINT_ASYNC_BACKEND_IOCP
					if (!ConnectNamedPipe(get_handle(), this)) {
						switch (GetLastError()) {
						case ERROR_IO_PENDING:
						case ERROR_PIPE_CONNECTED:
							break;
						default:
							return mint_network::last_socket_error_code();
						}
					}
#else
#error "This operation is not implemented for this platform"
#endif
				}
				catch (const std::system_error& error) {
					return error.code();
				}
				return {};
			}

			void complete(std::error_code error, [[maybe_unused]] std::size_t bytes_transferred) override {
				if (error) {
					done(mint::create_iterator_from(_cursor, mint::create_number(error.value())));
				}
				else {
#ifdef MINT_ASYNC_BACKEND_IOCP
					auto client_fd = _socket_fd.get();
					mint_network::SocketManager::instance().set_socket_listening(client_fd, false);
					auto* handle = _socket.get().create_listener();
					if (handle == mint::invalid_handle) {
						throw std::system_error(mint::last_error_code());
					}
					_socket_fd.get() = mint_network::SocketManager::instance().open_socket_from_handle(handle);
					_handle.data<mint::LibObject<std::remove_pointer_t<mint::handle_t>>>().ptr =
					    std::bit_cast<mint::handle_t>(_socket_fd.get());
					mint_network::SocketManager::instance().set_socket_listening(_socket_fd, true);
					done(mint::create_iterator_from(_cursor, mint::create_number(0),
					    mint::create_c_object<LocalSocket>(_cursor.get().ast(),
					        new NamedPipeLocalSocket(client_fd, _socket.get().get_path())),
					    mint_network::create_socket(_cursor.get().ast(), client_fd)));
#else
#error "This operation is not implemented for this platform"
#endif
				}
			}
		};

		return std::make_unique<AsyncAcceptOperation>(cursor, std::move(self), _socket, *this, std::move(handle));
	}

	std::unique_ptr<LocalSocket> accept(mint::Reference& handle_ref) override {

		mint::unlock_processor();
		const auto connect_result = ConnectNamedPipe(std::bit_cast<HANDLE>(_socket), nullptr);
		mint::lock_processor();

		if (!connect_result) {
			switch (GetLastError()) {
			case ERROR_IO_PENDING:
				mint_network::SocketManager::instance().set_socket_blocked(_socket, true);
				return {};
			case ERROR_PIPE_CONNECTED:
				break;
			default:
				throw std::system_error(mint::last_error_code());
			}
		}

		auto client_fd = _socket;
		mint_network::SocketManager::instance().set_socket_listening(client_fd, false);
		auto* handle = create_listener();
		if (handle == mint::invalid_handle) {
			throw std::system_error(mint::last_error_code());
		}
		handle_ref.data<mint::LibObject<std::remove_pointer_t<mint::handle_t>>>().ptr = handle;
		_socket = mint_network::SocketManager::instance().open_socket_from_handle(handle);
		mint_network::SocketManager::instance().set_socket_listening(_socket, true);
		return std::make_unique<NamedPipeLocalSocket>(client_fd, _path);
	}

	void close() override {
		if (_connected) {
			if (!DisconnectNamedPipe(std::bit_cast<HANDLE>(_socket))) {
				throw std::system_error(mint::last_error_code());
			}
			_connected = false;
		}
		mint_network::SocketManager::instance().close_socket_from_handle(std::bit_cast<mint::handle_t>(_socket));
	}

	void set_non_blocking(bool enabled) override {
		DWORD mode = PIPE_READMODE_BYTE | (enabled ? PIPE_NOWAIT : PIPE_WAIT);
		if (!SetNamedPipeHandleState(std::bit_cast<HANDLE>(_socket), &mode, nullptr, nullptr)) {
			throw std::system_error(mint::last_error_code());
		}
		mint_network::SocketManager::instance().set_socket_blocking(_socket, !enabled);
		_blocking = !enabled;
	}

	[[nodiscard]] const std::filesystem::path& get_path() const {
		return _path;
	}

	[[nodiscard]] mint::handle_t create_listener() const {
		return CreateNamedPipeW(_path.wstring().data(), PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
		    PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_REJECT_REMOTE_CLIENTS | (_blocking ? PIPE_WAIT : PIPE_NOWAIT),
		    PIPE_UNLIMITED_INSTANCES, BUFSIZ, BUFSIZ, 0, nullptr);
	}

private:
	static std::filesystem::path make_path(const std::string& name) {
		return std::format(R"(\\.\pipe\{})", name);
	}
};
#endif

#ifdef AF_UNIX
class UnixLocalSocket : public LocalSocket {
	SOCKET _socket = INVALID_SOCKET;
	bool _server = false;
public:
	UnixLocalSocket() :
	    _socket(mint_network::SocketManager::instance().open_socket(AF_UNIX, SOCK_STREAM, 0)) {
		if (_socket == INVALID_SOCKET) {
			throw std::system_error(mint_network::last_socket_error_code());
		}
	}

	UnixLocalSocket(SOCKET socket) :
	    _socket(socket) {}

	[[nodiscard]] SOCKET get_socket() const override {
		return _socket;
	}

	std::unique_ptr<mint::MintAsyncOperation> connect_async(mint::FunctionHelper& helper, mint::Reference self,
	    mint::AsyncRuntime* scheduler, const std::string& /*name*/, const sockaddr* address,
	    socklen_t address_length) override {
		class AsyncConnectOperation : public mint::MintAsyncOperation {
			std::reference_wrapper<mint::AsyncRuntime> _scheduler;
			std::reference_wrapper<mint::Cursor> _cursor;
			UnixLocalSocket* _socket;
			mint::Reference _io_status;
			const sockaddr* _remote_address;
			socklen_t _remote_address_length;
#ifdef MINT_ASYNC_BACKEND_IOCP
			std::future<void> _future;
			std::expected<IOStatus, std::error_code> _result;
#endif
		public:
			AsyncConnectOperation(mint::FunctionHelper& helper, mint::Reference self, mint::AsyncRuntime& scheduler,
			    UnixLocalSocket* socket, const sockaddr* address, socklen_t address_length) :
			    mint::MintAsyncOperation(std::move(self), std::bit_cast<mint::handle_t>(socket->get_socket())),
			    _scheduler(scheduler),
			    _cursor(helper.cursor()),
			    _socket(socket),
			    _io_status(helper.reference(mint_network::symbols::network)
			            .member(mint_network::symbols::socket)
			            .member(mint_network::symbols::io_status)
			            .get()),
			    _remote_address(address),
			    _remote_address_length(address_length) {}

			std::error_code start() override {

				const auto socket_fd = reinterpret_cast<SOCKET>(get_handle());

#ifdef MINT_ASYNC_BACKEND_IOCP
				_future = std::async([this, socket_fd]() {
					_result = std::invoke([this, socket_fd]() -> std::expected<IOStatus, std::error_code> {
						try {
							const auto set_blocking = mint_network::SocketBlockingModeGuard<false>(socket_fd);
							if (::connect(socket_fd, _remote_address, _remote_address_length)) {
								return IOStatus::success;
							}
							switch (const auto error = mint_network::last_socket_error_code();
							    mint::errno_from_error_code(error)) {
							case EINPROGRESS:
							case EWOULDBLOCK:
								break;
							default:
								return std::unexpected(error);
							}
							auto pfd = WSAPOLLFD {
							    .fd = socket_fd,
							    .events = POLLOUT,
							};
							for (;;) {
								if (WSAPoll(&pfd, 1, -1) == SOCKET_ERROR) {
									return std::unexpected(mint_network::last_socket_error_code());
								}
								if (pfd.revents & POLLOUT) {
									if (const auto error = mint_network::get_socket_option<int>(socket_fd, SO_ERROR)) {
										return std::unexpected(std::error_code(error, std::generic_category()));
									}
									return IOStatus::success;
								}
								if (pfd.revents & POLLHUP) {
									return IOStatus::closed;
								}
								if (pfd.revents & POLLERR) {
									return std::unexpected(mint_network::last_socket_error_code());
								}
							}
						}
						catch (const std::system_error& error) {
							return std::unexpected(error.code());
						}
						std::unreachable();
					});
					_scheduler.get().post_deferred_completion(*this);
				});
#elifdef MINT_ASYNC_BACKEND_KQUEUE
				epoll.filter = EVFILT_WRITE;
				if (epoll.pending) {
					int socket_error = 0;
					socklen_t socket_error_length = sizeof(socket_error);
					if (getsockopt(socket_fd, SOL_SOCKET, SO_ERROR, &socket_error, &socket_error_length) < 0) {
						return mint::last_error_code();
					}
					if (socket_error != 0) {
						return {socket_error, std::generic_category()};
					}
					return {};
				}
				if (connect(socket_fd, _remote_address, _remote_address_length) < 0) {
					const auto error = errno;
					if (error != EINPROGRESS && error != EALREADY && error != EWOULDBLOCK) {
						return mint::last_error_code();
					}
					return {EAGAIN, std::generic_category()};
				}
#elifdef MINT_OS_LINUX
				return std::visit(mint::Overloaded {
#ifdef MINT_ASYNC_BACKEND_IO_URING
				                      [&](mint::IoUringOperation& self) -> std::error_code {
					                      io_uring_prep_connect(self.sqe, socket_fd, _remote_address,
					                          _remote_address_length);
					                      return {};
				                      },
#endif
#ifdef MINT_ASYNC_BACKEND_EPOLL
				                      [&](mint::EPollOperation& self) -> std::error_code {
					                      self.events = EPOLLOUT;
					                      if (self.started) {
						                      int socket_error = 0;
						                      socklen_t socket_error_length = sizeof(socket_error);
						                      if (getsockopt(socket_fd, SOL_SOCKET, SO_ERROR, &socket_error,
						                              &socket_error_length)
						                          < 0) {
							                      return mint::last_error_code();
						                      }
						                      if (socket_error != 0) {
							                      return {socket_error, std::generic_category()};
						                      }
						                      return {};
					                      }
					                      self.started = true;
					                      const auto _ = mint_network::SocketBlockingModeGuard<false>(socket_fd);
					                      if (::connect(socket_fd, _remote_address, _remote_address_length) < 0) {
						                      const auto error = errno;
						                      if (error != EINPROGRESS && error != EALREADY && error != EWOULDBLOCK) {
							                      return mint::last_error_code();
						                      }
						                      return {EAGAIN, std::generic_category()};
					                      }
					                      return {};
				                      },
#endif
				                      [](std::monostate) -> std::error_code {
					                      return std::make_error_code(std::errc::not_supported);
				                      },
				                  },
				    *this);
#else
#error "This operation is not implemented for this platform"
#endif
				return {};
			}

			void complete(std::error_code error, std::size_t /*bytes_transferred*/) override {
				if (error) {
					done(mint::create_iterator_from(_cursor,
					    mint::get_global_ignore_visibility(_io_status.data<mint::Object>(),
					        mint_network::symbols::io_error),
					    mint::create_null(), mint::create_number(error.value())));
				}
				else {
					const auto socket_fd = reinterpret_cast<SOCKET>(get_handle());
#ifdef MINT_ASYNC_BACKEND_IOCP
					if (_result) {
						switch (*_result) {
						case IOStatus::success:
							done(mint::create_iterator_from(_cursor,
							    mint::get_global_ignore_visibility(_io_status.data<mint::Object>(),
							        mint_network::symbols::io_success),
							    mint::create_c_object<LocalSocket>(_cursor.get().ast(), _socket),
							    mint_network::create_socket(_cursor.get().ast(), socket_fd)));
							break;
						case IOStatus::would_block:
							done(mint::create_iterator_from(_cursor,
							    mint::get_global_ignore_visibility(_io_status.data<mint::Object>(),
							        mint_network::symbols::io_would_block),
							    mint::create_c_object<LocalSocket>(_cursor.get().ast(), _socket),
							    mint_network::create_socket(_cursor.get().ast(), socket_fd)));
							break;
						case IOStatus::closed:
							done(mint::create_iterator_from(_cursor,
							    mint::get_global_ignore_visibility(_io_status.data<mint::Object>(),
							        mint_network::symbols::io_closed),
							    mint::create_null()));
							break;
						}
					}
					else {
						done(mint::create_iterator_from(_cursor,
						    mint::get_global_ignore_visibility(_io_status.data<mint::Object>(),
						        mint_network::symbols::io_error),
						    mint::create_null(), mint::create_number(_result.error().value())));
					}
#elifdef MINT_OS_LINUX
					done(mint::create_iterator_from(_cursor,
					    mint::get_global_ignore_visibility(_io_status.data<mint::Object>(),
					        mint_network::symbols::io_success),
					    mint::create_c_object<LocalSocket>(_cursor.get().ast(), _socket),
					    mint_network::create_socket(_cursor.get().ast(), socket_fd)));
#else
#error "This operation is not implemented for this platform"
#endif
				}
			}
		};

		return std::make_unique<AsyncConnectOperation>(helper, std::move(self), *scheduler, this, address,
		    address_length);
	}

	IOStatus connect(const std::string& /*name*/, const sockaddr* address, socklen_t address_length) override {

		mint_network::SocketManager::instance().set_socket_listening(_socket, false);

		mint::unlock_processor();
		const auto connect_result = ::connect(_socket, address, address_length);
		mint::lock_processor();

		if (connect_result == 0) {
			return IOStatus::success;
		}
		switch (const auto error = mint_network::last_socket_error_code(); mint::errno_from_error_code(error)) {
		case EINPROGRESS:
		case EWOULDBLOCK:
			mint_network::SocketManager::instance().set_socket_blocked(_socket, true);
			return IOStatus::would_block;
		default:
			throw std::system_error(error);
		}
	}

	std::unique_ptr<mint::MintAsyncOperation> read_some_async(mint::FunctionHelper& helper, mint::Reference self,
	    std::vector<std::uint8_t>* buf, std::size_t count) override {
		class AsyncRecvSomeOperation : public mint::MintAsyncOperation {
			std::reference_wrapper<mint::Cursor> _cursor;
			mint::Reference _io_status;
			std::vector<std::uint8_t>* _buf;
			std::unique_ptr<std::uint8_t[]> _local_buffer;
			socklen_t _local_buffer_length;
#ifdef MINT_ASYNC_BACKEND_IOCP
			WSABUF _buffer_desc {};
#endif
		public:
			AsyncRecvSomeOperation(mint::FunctionHelper& helper, mint::Reference self, SOCKET socket_fd,
			    std::vector<std::uint8_t>* buf, socklen_t buffer_length) :
			    mint::MintAsyncOperation(std::move(self), std::bit_cast<mint::handle_t>(socket_fd)),
			    _cursor(helper.cursor()),
			    _io_status(helper.reference(mint_network::symbols::network)
			            .member(mint_network::symbols::socket)
			            .member(mint_network::symbols::io_status)
			            .get()),
			    _buf(buf),
			    _local_buffer(std::make_unique<std::uint8_t[]>(buffer_length)),
			    _local_buffer_length(buffer_length) {}

			std::error_code start() override {

				const auto socket_fd = reinterpret_cast<SOCKET>(get_handle());

#ifdef MINT_ASYNC_BACKEND_IOCP
				DWORD flags = 0;
				_buffer_desc = WSABUF {
				    .len = static_cast<ULONG>(_local_buffer_length),
				    .buf = reinterpret_cast<char*>(_local_buffer.get()),
				};

				switch (WSARecv(socket_fd, &_buffer_desc, 1, nullptr, &flags, this, nullptr)) {
				case SOCKET_ERROR:
					switch (WSAGetLastError()) {
					case WSA_IO_PENDING:
						break;
					default:
						return mint_network::last_socket_error_code();
					}
					break;
				default:
					break;
				}
#elifdef MINT_ASYNC_BACKEND_KQUEUE
				filter = EVFILT_READ;
				result = recv(socket_fd, reinterpret_cast<char*>(_local_buffer.get()),
				    static_cast<int>(_local_buffer_length), 0);
				if (result < 0) {
					return mint::last_error_code();
				}
#elifdef MINT_OS_LINUX
				return std::visit(mint::Overloaded {
#ifdef MINT_ASYNC_BACKEND_IO_URING
				                      [&](mint::IoUringOperation& self) -> std::error_code {
					                      const auto flags = 0;
					                      io_uring_prep_recv(self.sqe, socket_fd, _local_buffer.get(),
					                          _local_buffer_length, flags);
					                      return {};
				                      },
#endif
#ifdef MINT_ASYNC_BACKEND_EPOLL
				                      [&](mint::EPollOperation& self) -> std::error_code {
					                      self.events = EPOLLIN;
					                      self.result = recv(socket_fd, reinterpret_cast<char*>(_local_buffer.get()),
					                          static_cast<int>(_local_buffer_length), MSG_DONTWAIT);
					                      if (self.result < 0) {
						                      return mint::last_error_code();
					                      }
					                      return {};
				                      },
#endif
				                      [](std::monostate) -> std::error_code {
					                      return std::make_error_code(std::errc::not_supported);
				                      },
				                  },
				    *this);
#else
#error "This operation is not implemented for this platform"
#endif

				return {};
			}

			void complete(std::error_code error, std::size_t bytes_transferred) override {
				if (error) {
					done(mint::create_iterator_from(_cursor,
					    mint::get_global_ignore_visibility(_io_status.data<mint::Object>(),
					        mint_network::symbols::io_error),
					    mint::create_number(error.value())));
				}
				else if (bytes_transferred == 0) {
					done(mint::create_iterator_from(_cursor,
					    mint::get_global_ignore_visibility(_io_status.data<mint::Object>(),
					        mint_network::symbols::io_closed)));
				}
				else {
					_buf->append_range(std::span(_local_buffer.get(), bytes_transferred));
					done(mint::create_iterator_from(_cursor,
					    mint::get_global_ignore_visibility(_io_status.data<mint::Object>(),
					        mint_network::symbols::io_success)));
				}
			}
		};

		return std::make_unique<AsyncRecvSomeOperation>(helper, std::move(self), _socket, buf, count);
	}

	IOStatus read_some(std::vector<std::uint8_t>* buf, std::size_t count) override {

		auto local_buffer = std::make_unique<std::uint8_t[]>(count);
		mint::unlock_processor();
		const auto bytes_transferred = recv(_socket, reinterpret_cast<char*>(local_buffer.get()),
		    static_cast<int>(count), 0);
		mint::lock_processor();

		switch (bytes_transferred) {
		case -1:
			switch (const auto error = mint::last_error_code(); mint::errno_from_error_code(error)) {
			case EINPROGRESS:
			case EWOULDBLOCK:
				mint_network::SocketManager::instance().set_socket_blocked(_socket, true);
				return IOStatus::would_block;
			case EPIPE:
				return IOStatus::closed;
			default:
				throw std::system_error(error);
			}
			break;
		case 0:
			return IOStatus::closed;
		default:
			buf->append_range(std::span(local_buffer.get(), bytes_transferred));
			return IOStatus::success;
		}

		return {};
	}

	std::unique_ptr<mint::MintAsyncOperation> read_async(mint::FunctionHelper& helper, mint::Reference self,
	    std::vector<std::uint8_t>* buf) override {
		class AsyncRecvOperation : public mint::MintAsyncOperation {
			std::reference_wrapper<mint::Cursor> _cursor;
			mint::Reference _io_status;
			std::vector<std::uint8_t>* _buf;
			std::array<std::uint8_t, BUFSIZ> _local_buffer {};
#ifdef MINT_ASYNC_BACKEND_IOCP
			WSABUF _buffer_desc {};
#endif
		public:
			AsyncRecvOperation(mint::FunctionHelper& helper, mint::Reference self, SOCKET socket_fd,
			    std::vector<std::uint8_t>* buf) :
			    mint::MintAsyncOperation(std::move(self), std::bit_cast<mint::handle_t>(socket_fd)),
			    _cursor(helper.cursor()),
			    _io_status(helper.reference(mint_network::symbols::network)
			            .member(mint_network::symbols::socket)
			            .member(mint_network::symbols::io_status)
			            .get()),
			    _buf(buf) {}

			std::error_code start() override {

				const auto socket_fd = reinterpret_cast<SOCKET>(get_handle());

#ifdef MINT_ASYNC_BACKEND_IOCP
				DWORD flags = 0;
				_buffer_desc = WSABUF {
				    .len = static_cast<ULONG>(_local_buffer.size()),
				    .buf = reinterpret_cast<char*>(_local_buffer.data()),
				};

				switch (WSARecv(socket_fd, &_buffer_desc, 1, nullptr, &flags, this, nullptr)) {
				case SOCKET_ERROR:
					switch (WSAGetLastError()) {
					case WSA_IO_PENDING:
						break;
					default:
						return mint_network::last_socket_error_code();
					}
					break;
				default:
					break;
				}
#elifdef MINT_ASYNC_BACKEND_KQUEUE
				filter = EVFILT_READ;
				result = recv(socket_fd, reinterpret_cast<char*>(_local_buffer.data()),
				    static_cast<int>(_local_buffer.size()), 0);
				if (result < 0) {
					return mint::last_error_code();
				}
#elifdef MINT_OS_LINUX
				return std::visit(mint::Overloaded {
#ifdef MINT_ASYNC_BACKEND_IO_URING
				                      [&](mint::IoUringOperation& self) -> std::error_code {
					                      const auto flags = 0;
					                      io_uring_prep_recv(self.sqe, socket_fd, _local_buffer.data(),
					                          _local_buffer.size(), flags);
					                      return {};
				                      },
#endif
#ifdef MINT_ASYNC_BACKEND_EPOLL
				                      [&](mint::EPollOperation& self) -> std::error_code {
					                      self.events = EPOLLIN;
					                      self.result = recv(socket_fd, reinterpret_cast<char*>(_local_buffer.data()),
					                          _local_buffer.size(), MSG_DONTWAIT);
					                      if (self.result < 0) {
						                      return mint::last_error_code();
					                      }
					                      return {};
				                      },
#endif
				                      [](std::monostate) -> std::error_code {
					                      return std::make_error_code(std::errc::not_supported);
				                      },
				                  },
				    *this);
#else
#error "This operation is not implemented for this platform"
#endif

				return {};
			}

			void complete(std::error_code error, std::size_t bytes_transferred) override {
				if (error) {
					done(mint::create_iterator_from(_cursor,
					    mint::get_global_ignore_visibility(_io_status.data<mint::Object>(),
					        mint_network::symbols::io_error),
					    mint::create_number(error.value())));
				}
				else if (bytes_transferred == 0) {
					done(mint::create_iterator_from(_cursor,
					    mint::get_global_ignore_visibility(_io_status.data<mint::Object>(),
					        mint_network::symbols::io_closed)));
				}
				else {
					_buf->append_range(std::span(_local_buffer.data(), bytes_transferred));
					done(mint::create_iterator_from(_cursor,
					    mint::get_global_ignore_visibility(_io_status.data<mint::Object>(),
					        mint_network::symbols::io_success)));
				}
			}
		};

		return std::make_unique<AsyncRecvOperation>(helper, std::move(self), _socket, buf);
	}

	IOStatus read(std::vector<std::uint8_t>* buf) override {

		socklen_t length = 0;
#ifdef MINT_OS_UNIX
		if (ioctl(_socket, SIOCINQ, &length) == -1) {
			throw std::system_error(mint::last_error_code());
		}
#else
		length = BUFSIZ; // TODO: get better value
#endif

		auto local_buffer = std::make_unique<std::uint8_t[]>(length);
		mint::unlock_processor();
		const auto bytes_transferred = recv(_socket, reinterpret_cast<char*>(local_buffer.get()),
		    static_cast<int>(length), 0);
		mint::lock_processor();

		switch (bytes_transferred) {
		case -1:
			switch (const auto error = mint::last_error_code(); mint::errno_from_error_code(error)) {
			case EINPROGRESS:
			case EWOULDBLOCK:
				mint_network::SocketManager::instance().set_socket_blocked(_socket, true);
				return IOStatus::would_block;
			case EPIPE:
				return IOStatus::closed;
			default:
				throw std::system_error(error);
			}
			break;
		case 0:
			return IOStatus::closed;
		default:
			buf->append_range(std::span(local_buffer.get(), bytes_transferred));
			return IOStatus::success;
		}

		return {};
	}

	std::unique_ptr<mint::MintAsyncOperation> write_async(mint::FunctionHelper& helper, mint::Reference self,
	    std::vector<std::uint8_t>* buf) override {
		class AsyncSendOperation : public mint::MintAsyncOperation {
			std::reference_wrapper<mint::Cursor> _cursor;
			mint::Reference _io_status;
			std::span<std::uint8_t> _buffer;
			std::size_t _offset = 0;
#ifdef MINT_ASYNC_BACKEND_IOCP
			WSABUF _buffer_desc {};
#endif
		public:
			AsyncSendOperation(mint::FunctionHelper& helper, mint::Reference self, SOCKET socket_fd,
			    std::vector<std::uint8_t>* buffer) :
			    mint::MintAsyncOperation(std::move(self), std::bit_cast<mint::handle_t>(socket_fd)),
			    _cursor(helper.cursor()),
			    _io_status(helper.reference(mint_network::symbols::network)
			            .member(mint_network::symbols::socket)
			            .member(mint_network::symbols::io_status)
			            .get()),
			    _buffer(*buffer) {}

			std::error_code start() override {

				const auto socket_fd = reinterpret_cast<SOCKET>(get_handle());

#ifdef MINT_ASYNC_BACKEND_IOCP
				const DWORD flags = 0;
				_buffer_desc = WSABUF {
				    .len = static_cast<ULONG>(_buffer.size() - _offset),
				    .buf = reinterpret_cast<char*>(_buffer.subspan(_offset).data()),
				};

				switch (WSASend(socket_fd, &_buffer_desc, 1, nullptr, flags, this, nullptr)) {
				case SOCKET_ERROR:
					switch (WSAGetLastError()) {
					case WSA_IO_PENDING:
						break;
					default:
						return mint_network::last_socket_error_code();
					}
					break;
				default:
					break;
				}
#elifdef MINT_ASYNC_BACKEND_KQUEUE
				filter = EVFILT_WRITE;
				result = send(socket_fd, reinterpret_cast<const char*>(_buffer.data()),
				    static_cast<int>(_buffer.size()), MSG_NOSIGNAL);
				if (result < 0) {
					return mint::last_error_code();
				}
#elifdef MINT_OS_LINUX
				return std::visit(mint::Overloaded {
#ifdef MINT_ASYNC_BACKEND_IO_URING
				                      [&](mint::IoUringOperation& self) -> std::error_code {
					                      const auto flags = MSG_NOSIGNAL;
					                      io_uring_prep_send(self.sqe, socket_fd, _buffer.data(), _buffer.size(), flags);
					                      return {};
				                      },
#endif
#ifdef MINT_ASYNC_BACKEND_EPOLL
				                      [&](mint::EPollOperation& self) -> std::error_code {
					                      self.events = EPOLLOUT;
					                      self.result = send(socket_fd, reinterpret_cast<const char*>(_buffer.data()),
					                          static_cast<int>(_buffer.size()), MSG_NOSIGNAL | MSG_DONTWAIT);
					                      if (self.result < 0) {
						                      return mint::last_error_code();
					                      }
					                      return {};
				                      },
#endif
				                      [](std::monostate) -> std::error_code {
					                      return std::make_error_code(std::errc::not_supported);
				                      },
				                  },
				    *this);
#else
#error "This operation is not implemented for this platform"
#endif

				return {};
			}

			void complete(std::error_code error, std::size_t bytes_transferred) override {
				if (error) {
					done(mint::create_iterator_from(_cursor,
					    mint::get_global_ignore_visibility(_io_status.data<mint::Object>(),
					        mint_network::symbols::io_error),
					    mint::create_number(error.value())));
				}
				else {
					_offset += bytes_transferred;
					done(mint::create_iterator_from(_cursor,
					    mint::get_global_ignore_visibility(_io_status.data<mint::Object>(),
					        mint_network::symbols::io_success),
					    mint::create_unsigned_number(bytes_transferred)));
				}
			}
		};

		return std::make_unique<AsyncSendOperation>(helper, std::move(self), _socket, buf);
	}

	std::tuple<IOStatus, std::size_t> write(const std::vector<std::uint8_t>* buf) override {

#ifdef MINT_OS_WINDOWS
		const auto flags = 0;
#else
		const auto flags = MSG_NOSIGNAL;
#endif

		mint::unlock_processor();
		const auto bytes_transferred = send(_socket, reinterpret_cast<const char*>(buf->data()),
		    static_cast<int>(buf->size()), flags);
		mint::lock_processor();

		switch (bytes_transferred) {
		case -1:
			switch (const auto error = mint::last_error_code(); mint::errno_from_error_code(error)) {
			case EINPROGRESS:
			case EWOULDBLOCK:
				mint_network::SocketManager::instance().set_socket_blocked(_socket, true);
				return {IOStatus::would_block, 0};
			case EPIPE:
				return {IOStatus::closed, 0};
			default:
				throw std::system_error(error);
			}
			break;
		case 0:
			return {IOStatus::closed, 0};
			break;
		default:
			return {IOStatus::success, bytes_transferred};
		}

		return {};
	}

	void listen(const std::string& /*name*/, const sockaddr* address, socklen_t address_length) override {

		if (::bind(_socket, address, address_length) != 0) {
			throw std::system_error(mint_network::last_socket_error_code());
		}

		mint_network::SocketManager::instance().set_socket_listening(_socket, true);

		if (::listen(_socket, SOMAXCONN) != 0) {
			throw std::system_error(mint_network::last_socket_error_code());
		}

		_server = true;
	}

	std::unique_ptr<mint::MintAsyncOperation> accept_async(mint::Cursor& cursor, mint::Reference self,
	    mint::Reference /*handle*/) override {

#ifdef MINT_ASYNC_BACKEND_IOCP
		static constexpr auto address_length = sizeof(sockaddr_storage) + 16;
#endif

		class AsyncAcceptOperation : public mint::MintAsyncOperation {
			std::reference_wrapper<mint::Cursor> _cursor;
#ifdef MINT_ASYNC_BACKEND_IOCP
			SOCKET _client_fd = INVALID_SOCKET;
			std::array<char, 2 * address_length> _accept_buffer {};
#elifdef MINT_OS_LINUX
			sockaddr_storage _remote_address {};
			socklen_t _remote_address_length = static_cast<socklen_t>(sizeof(_remote_address));
#endif
		public:
			AsyncAcceptOperation(mint::Cursor& cursor, mint::Reference self, SOCKET socket_fd) :
			    mint::MintAsyncOperation(std::move(self), std::bit_cast<mint::handle_t>(socket_fd)),
			    _cursor(cursor) {}

			std::error_code start() override {
				const auto socket_fd = reinterpret_cast<SOCKET>(get_handle());

				try {
#ifdef MINT_ASYNC_BACKEND_IOCP
					const auto type = mint_network::get_socket_option<int>(socket_fd, SO_TYPE);
					const auto info = mint_network::get_socket_option<WSAPROTOCOL_INFOW>(socket_fd, SO_PROTOCOL_INFOW);

					_client_fd = WSASocketW(info.iAddressFamily, type, info.iProtocol, nullptr, 0, WSA_FLAG_OVERLAPPED);

					auto bytes_received = DWORD();
					auto* address_buffer = _accept_buffer.data();
					auto address_buffer_length = static_cast<DWORD>(address_length);
					if (!AcceptEx(socket_fd, _client_fd, address_buffer, 0, address_buffer_length,
					        address_buffer_length, &bytes_received, this)) {
						switch (WSAGetLastError()) {
						case WSA_IO_PENDING:
							break;
						default:
							return mint_network::last_socket_error_code();
						}
					}
#elifdef MINT_ASYNC_BACKEND_KQUEUE
					filter = EVFILT_READ;
					result = ::accept(socket_fd, reinterpret_cast<sockaddr*>(&_remote_address), &_remote_address_length);
					if (result < 0) {
						return mint::last_error_code();
					}
#elifdef MINT_OS_LINUX
					return std::visit(mint::Overloaded {
#ifdef MINT_ASYNC_BACKEND_IO_URING
					                      [&](mint::IoUringOperation& self) -> std::error_code {
						                      io_uring_prep_accept(self.sqe, socket_fd,
						                          reinterpret_cast<sockaddr*>(&_remote_address),
						                          &_remote_address_length, 0);
						                      return {};
					                      },
#endif
#ifdef MINT_ASYNC_BACKEND_EPOLL
					                      [&](mint::EPollOperation& self) -> std::error_code {
						                      const auto _ = mint_network::SocketBlockingModeGuard<false>(socket_fd);
						                      self.events = EPOLLIN;
						                      self.result = ::accept(socket_fd,
						                          reinterpret_cast<sockaddr*>(&_remote_address),
						                          &_remote_address_length);
						                      if (self.result < 0) {
							                      return mint::last_error_code();
						                      }
						                      return {};
					                      },
#endif
					                      [](std::monostate) -> std::error_code {
						                      return std::make_error_code(std::errc::not_supported);
					                      },
					                  },
					    *this);
#else
#error "This operation is not implemented for this platform"
#endif
				}
				catch (const std::system_error& error) {
					return error.code();
				}
				return {};
			}

			void complete(std::error_code error, [[maybe_unused]] std::size_t bytes_transferred) override {
				if (error) {
					done(mint::create_iterator_from(_cursor, mint::create_number(error.value())));
				}
				else {
#ifdef MINT_ASYNC_BACKEND_IOCP
					auto socket_fd = reinterpret_cast<SOCKET>(get_handle());
					auto* address_buffer = _accept_buffer.data();
					auto address_buffer_length = static_cast<DWORD>(address_length);
					socklen_t local_address_length = sizeof(sockaddr);
					LPSOCKADDR local_address = nullptr;
					socklen_t remote_address_length = sizeof(sockaddr);
					LPSOCKADDR remote_address = nullptr;

					GetAcceptExSockaddrs(address_buffer, 0, address_buffer_length, address_buffer_length,
					    &local_address, &local_address_length, &remote_address, &remote_address_length);

					try {
						mint_network::set_socket_option(_client_fd, SO_UPDATE_ACCEPT_CONTEXT, socket_fd);
						mint_network::SocketManager::instance().accept_socket(_client_fd);
						done(mint::create_iterator_from(_cursor, mint::create_number(0),
						    mint::create_c_object<LocalSocket>(_cursor.get().ast(), new UnixLocalSocket(_client_fd)),
						    mint_network::create_socket(_cursor.get().ast(), _client_fd)));
					}
					catch (const std::system_error& error) {
						done(mint::create_iterator_from(_cursor, mint::create_number(error.code().value())));
					}
#elifdef MINT_OS_LINUX
					const auto client_fd = static_cast<SOCKET>(bytes_transferred);
					mint_network::SocketManager::instance().accept_socket(client_fd);
					done(mint::create_iterator_from(_cursor, mint::create_number(0),
					    mint::create_c_object<LocalSocket>(_cursor.get().ast(), new UnixLocalSocket(client_fd)),
					    mint_network::create_socket(_cursor.get().ast(), client_fd)));
#else
#error "This operation is not implemented for this platform"
#endif
				}
			}
		};

		return std::make_unique<AsyncAcceptOperation>(cursor, std::move(self), _socket);
	}

	std::unique_ptr<LocalSocket> accept(mint::Reference& /*handle*/) override {

		sockaddr remote_address {};
		socklen_t remote_address_length = sizeof(remote_address);

		mint::unlock_processor();
		const SOCKET client_fd = ::accept(_socket, &remote_address, &remote_address_length);
		mint::lock_processor();

		if (client_fd != INVALID_SOCKET) {
			mint_network::SocketManager::instance().accept_socket(client_fd);
			return std::make_unique<UnixLocalSocket>(client_fd);
		}

		switch (const auto error = mint::last_error_code(); mint::errno_from_error_code(error)) {
		case EINPROGRESS:
		case EWOULDBLOCK:
			mint_network::SocketManager::instance().set_socket_blocked(_socket, true);
			break;
		default:
			throw std::system_error(error);
		}

		return {};
	}

	void close() override {
		if (_server) {

			auto address = sockaddr_storage();
			auto address_length = static_cast<socklen_t>(sizeof(address));

			if (getsockname(_socket, reinterpret_cast<sockaddr*>(&address), &address_length) != 0) {
				throw std::system_error(mint_network::last_socket_error_code());
			}

			mint_network::SocketManager::instance().close_socket(_socket);

			if (unlink(reinterpret_cast<sockaddr_un*>(&address)->sun_path) != 0) {
				throw std::system_error(mint::last_error_code());
			}

			_server = true;
		}
		else {
			mint_network::SocketManager::instance().close_socket(_socket);
		}
	}

	void set_non_blocking(bool enabled) override {
		mint_network::set_socket_non_blocking(_socket, enabled);
		mint_network::SocketManager::instance().set_socket_blocking(_socket, !enabled);
	}
};
#endif

std::unique_ptr<LocalSocket> make_local_socket() {
	try {
#ifdef AF_UNIX
		return std::make_unique<UnixLocalSocket>();
#else
		throw std::system_error(std::make_error_code(std::errc::not_supported));
#endif
	}
	catch (const std::system_error& error) {
#ifdef MINT_OS_WINDOWS
		if (error.code() == std::errc::not_supported) {
			return std::make_unique<NamedPipeLocalSocket>();
		}
#endif
		throw;
	}
}

mint::Reference mint_local_endpoint_create(mint::Cursor& cursor, const mint::Reference& name) {
#ifdef AF_UNIX
	auto d_ptr = std::make_unique<sockaddr_un>(sockaddr_un {
	    .sun_family = AF_UNIX,
	});
#ifdef MINT_OS_WINDOWS
	auto temp_path = std::array<wchar_t, _MAX_PATH + 1>();
	auto temp_path_length = static_cast<std::size_t>(
	    GetTempPath2W(static_cast<DWORD>(temp_path.size()), temp_path.data()));
	const auto path_str = (std::filesystem::path(std::wstring_view(temp_path.data(), temp_path_length))
	                       / std::format("{}.sock", mint::to_string(name)))
	                          .string();
#else
	auto* temp_path = std::getenv("TMPDIR");
	if (temp_path == nullptr) {
		temp_path = "/tmp";
	}
	const auto path_str = (std::filesystem::path(temp_path) / std::format("{}.sock", mint::to_string(name))).string();
#endif
	if (path_str.size() < UNIX_PATH_MAX) {
		std::ranges::copy(path_str, d_ptr->sun_path);
		return mint::create_c_object<sockaddr>(cursor.ast(), reinterpret_cast<sockaddr*>(d_ptr.release()));
	}
#endif
	return {};
}

mint::Reference mint_local_endpoint_delete(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr) {
	if (mint::is_instance_of(d_ptr, mint::Class::Metatype::libobject)) {
		delete reinterpret_cast<sockaddr_un*>(d_ptr.data<mint::LibObject<sockaddr>>().ptr);
	}
	return {};
}

mint::Reference mint_local_endpoint_get_path(mint::Cursor& cursor, const mint::Reference& d_ptr) {
	if (mint::is_instance_of(d_ptr, mint::Class::Metatype::libobject)) {
		return mint::create_string(cursor.ast(),
		    reinterpret_cast<sockaddr_un*>(d_ptr.data<mint::LibObject<sockaddr>>().ptr)->sun_path);
	}
	return {};
}

mint::Reference mint_local_socket_connect(mint::FunctionHelper& helper, const mint::Reference& name,
    const mint::Reference& endpoint) {

	auto io_status = helper.reference(mint_network::symbols::network)
	                     .member(mint_network::symbols::socket)
	                     .member(mint_network::symbols::io_status);

	try {
		auto socket = make_local_socket();
		const auto [address, address_length] = mint::is_instance_of(endpoint, mint::Class::Metatype::libobject)
		                                           ? mint_network::to_sockaddr(endpoint)
		                                           : std::make_tuple<sockaddr*, socklen_t>(nullptr, 0);
		const auto status = socket->connect(mint::to_string(name), address, address_length);
		const auto socket_fd = socket->get_socket();
		switch (status) {
		case IOStatus::success:
			return mint::create_iterator_from(helper.cursor(),
			    io_status.member(mint_network::symbols::io_success).share(),
			    mint::create_c_object(helper.cursor().ast(), socket.release()),
			    mint_network::create_socket(helper.cursor().ast(), socket_fd));
		case IOStatus::would_block:
			return mint::create_iterator_from(helper.cursor(),
			    io_status.member(mint_network::symbols::io_would_block).share(),
			    mint::create_c_object(helper.cursor().ast(), socket.release()),
			    mint_network::create_socket(helper.cursor().ast(), socket_fd));
		case IOStatus::closed:
			return mint::create_iterator_from(helper.cursor(),
			    io_status.member(mint_network::symbols::io_closed).share(), mint::create_null());
		}
	}
	catch (const std::system_error& error) {
		return mint::create_iterator_from(helper.cursor(), io_status.member(mint_network::symbols::io_error).share(),
		    mint::create_number(error.code().value()));
	}
	return {};
}

mint::Reference mint_local_socket_connect_async(mint::FunctionHelper& helper, mint::Reference& self,
    const mint::Reference& scheduler, const mint::Reference& name, const mint::Reference& endpoint) {
	try {

		auto socket = make_local_socket();
		const auto [target, length] = mint::is_instance_of(endpoint, mint::Class::Metatype::libobject)
		                                  ? mint_network::to_sockaddr(endpoint)
		                                  : std::make_tuple<sockaddr*, socklen_t>(nullptr, 0);

		return mint::create_iterator_from(helper.cursor(), mint::create_number(0),
		    mint::create_async_operation(helper.cursor().ast(),
		        socket.release()
		            ->connect_async(helper, std::move(self), scheduler.data<mint::LibObject<mint::AsyncRuntime>>().ptr,
		                mint::to_string(name), target, length)
		            .release()));
	}
	catch (const std::system_error& error) {
		return mint::create_iterator_from(helper.cursor(), mint::create_number(error.code().value()));
	}
}

mint::Reference mint_local_socket_close(mint::Cursor& /*cursor*/, mint::Reference& d_ptr) {
	try {
		if (!mint::is_instance_of(d_ptr, mint::Data::Format::null)) {
			d_ptr.data<mint::LibObject<LocalSocket>>().ptr->close();
			delete d_ptr.data<mint::LibObject<LocalSocket>>().ptr;
			d_ptr.move_data(mint::create_null());
		}
	}
	catch (const std::system_error& error) {
		return mint::create_number(error.code().value());
	}
	return {};
}

mint::Reference mint_local_socket_recv_some(mint::FunctionHelper& helper, const mint::Reference& d_ptr,
    mint::Reference& buffer, mint::Reference& count) {

	auto io_status = helper.reference(mint_network::symbols::network)
	                     .member(mint_network::symbols::socket)
	                     .member(mint_network::symbols::io_status);

	try {
		switch (d_ptr.data<mint::LibObject<LocalSocket>>()
		        .ptr->read_some(buffer.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr,
		            mint::to_integer<std::size_t>(helper.cursor(), count))) {
		case IOStatus::success:
			return mint::create_iterator_from(helper.cursor(),
			    io_status.member(mint_network::symbols::io_success).share());
		case IOStatus::would_block:
			return mint::create_iterator_from(helper.cursor(),
			    io_status.member(mint_network::symbols::io_would_block).share());
		case IOStatus::closed:
			return mint::create_iterator_from(helper.cursor(),
			    io_status.member(mint_network::symbols::io_closed).share());
		}
	}
	catch (const std::system_error& error) {
		return mint::create_iterator_from(helper.cursor(), io_status.member(mint_network::symbols::io_error).share(),
		    mint::create_number(error.code().value()));
	}
	return {};
}

mint::Reference mint_local_socket_recv_some_async(mint::FunctionHelper& helper, mint::Reference& self,
    const mint::Reference& d_ptr, mint::Reference& buffer, mint::Reference& count) {
	return mint::create_async_operation(helper.cursor().ast(),
	    d_ptr.data<mint::LibObject<LocalSocket>>()
	        .ptr
	        ->read_some_async(helper, std::move(self), buffer.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr,
	            mint::to_integer<std::size_t>(helper.cursor(), count))
	        .release());
}

mint::Reference mint_local_socket_recv(mint::FunctionHelper& helper, const mint::Reference& d_ptr,
    mint::Reference& buffer) {

	auto io_status = helper.reference(mint_network::symbols::network)
	                     .member(mint_network::symbols::socket)
	                     .member(mint_network::symbols::io_status);

	try {
		switch (d_ptr.data<mint::LibObject<LocalSocket>>().ptr->read(
		    buffer.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr)) {
		case IOStatus::success:
			return mint::create_iterator_from(helper.cursor(),
			    io_status.member(mint_network::symbols::io_success).share());
		case IOStatus::would_block:
			return mint::create_iterator_from(helper.cursor(),
			    io_status.member(mint_network::symbols::io_would_block).share());
		case IOStatus::closed:
			return mint::create_iterator_from(helper.cursor(),
			    io_status.member(mint_network::symbols::io_closed).share());
		}
	}
	catch (const std::system_error& error) {
		return mint::create_iterator_from(helper.cursor(), io_status.member(mint_network::symbols::io_error).share(),
		    mint::create_number(error.code().value()));
	}
	return {};
}

mint::Reference mint_local_socket_recv_async(mint::FunctionHelper& helper, mint::Reference& self,
    const mint::Reference& d_ptr, mint::Reference& buffer) {
	return mint::create_async_operation(helper.cursor().ast(),
	    d_ptr.data<mint::LibObject<LocalSocket>>()
	        .ptr->read_async(helper, std::move(self), buffer.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr)
	        .release());
}

mint::Reference mint_local_socket_send(mint::FunctionHelper& helper, const mint::Reference& d_ptr,
    mint::Reference& buffer) {

	auto io_status = helper.reference(mint_network::symbols::network)
	                     .member(mint_network::symbols::socket)
	                     .member(mint_network::symbols::io_status);

	try {
		switch (const auto [status, count] = d_ptr.data<mint::LibObject<LocalSocket>>().ptr->write(
		            buffer.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr);
		    status) {
		case IOStatus::success:
			return mint::create_iterator_from(helper.cursor(),
			    io_status.member(mint_network::symbols::io_success).share(), mint::create_unsigned_number(count));
		case IOStatus::would_block:
			return mint::create_iterator_from(helper.cursor(),
			    io_status.member(mint_network::symbols::io_would_block).share());
		case IOStatus::closed:
			return mint::create_iterator_from(helper.cursor(),
			    io_status.member(mint_network::symbols::io_closed).share());
		}
	}
	catch (const std::system_error& error) {
		return mint::create_iterator_from(helper.cursor(), io_status.member(mint_network::symbols::io_error).share(),
		    mint::create_number(error.code().value()));
	}
	return {};
}

mint::Reference mint_local_socket_send_async(mint::FunctionHelper& helper, mint::Reference& self,
    const mint::Reference& d_ptr, mint::Reference& buffer) {
	return mint::create_async_operation(helper.cursor().ast(),
	    d_ptr.data<mint::LibObject<LocalSocket>>()
	        .ptr->write_async(helper, std::move(self), buffer.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr)
	        .release());
}

mint::Reference mint_local_socket_listen(mint::Cursor& cursor, const mint::Reference& name,
    const mint::Reference& endpoint) {
	try {
		auto socket = make_local_socket();
		const auto [address, address_length] = mint::is_instance_of(endpoint, mint::Class::Metatype::libobject)
		                                           ? mint_network::to_sockaddr(endpoint)
		                                           : std::make_tuple<sockaddr*, socklen_t>(nullptr, 0);
		socket->listen(mint::to_string(name), address, address_length);
		const auto socket_fd = socket->get_socket();
		return mint::create_iterator_from(cursor, mint::create_number(0),
		    mint::create_c_object(cursor.ast(), socket.release()),
		    mint_network::create_socket(cursor.ast(), socket_fd));
	}
	catch (const std::system_error& error) {
		return mint::create_iterator_from(cursor, mint::create_number(error.code().value()), mint::create_null());
	}
}

mint::Reference mint_local_socket_accept(mint::Cursor& cursor, const mint::Reference& d_ptr, mint::Reference& handle) {
	try {
		auto socket = d_ptr.data<mint::LibObject<LocalSocket>>().ptr->accept(handle);
		if (!socket) {
			return mint::create_iterator_from(cursor, mint::create_number(0), mint::create_null());
		}
		const auto socket_fd = socket->get_socket();
		return mint::create_iterator_from(cursor, mint::create_number(0),
		    mint::create_c_object(cursor.ast(), socket.release()),
		    mint_network::create_socket(cursor.ast(), socket_fd));
	}
	catch (const std::system_error& error) {
		return mint::create_iterator_from(cursor, mint::create_number(error.code().value()), mint::create_null());
	}
}

mint::Reference mint_local_socket_accept_async(mint::Cursor& cursor, mint::Reference& self,
    const mint::Reference& d_ptr, mint::Reference& handle) {
	return mint::create_async_operation(cursor.ast(), d_ptr.data<mint::LibObject<LocalSocket>>()
	                                                      .ptr->accept_async(cursor, std::move(self), std::move(handle))
	                                                      .release());
}

mint::Reference mint_local_socket_set_non_blocking(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr,
    mint::Reference& enabled) {
	try {
		d_ptr.data<mint::LibObject<LocalSocket>>().ptr->set_non_blocking(to_boolean(enabled));
	}
	catch (const std::system_error& error) {
		return mint::create_number(error.code().value());
	}
	return {};
}

}

MINT_EXPORT_FUNCTION(mint_local_endpoint_create, 1)
MINT_EXPORT_FUNCTION(mint_local_endpoint_delete, 1)
MINT_EXPORT_FUNCTION(mint_local_endpoint_get_path, 1)

MINT_EXPORT_FUNCTION(mint_local_socket_connect, 2)
MINT_EXPORT_FUNCTION(mint_local_socket_connect_async, 4)
MINT_EXPORT_FUNCTION(mint_local_socket_close, 1)
MINT_EXPORT_FUNCTION(mint_local_socket_recv_some, 3)
MINT_EXPORT_FUNCTION(mint_local_socket_recv_some_async, 4)
MINT_EXPORT_FUNCTION(mint_local_socket_recv, 2)
MINT_EXPORT_FUNCTION(mint_local_socket_recv_async, 3)
MINT_EXPORT_FUNCTION(mint_local_socket_send, 2)
MINT_EXPORT_FUNCTION(mint_local_socket_send_async, 3)
MINT_EXPORT_FUNCTION(mint_local_socket_listen, 2)
MINT_EXPORT_FUNCTION(mint_local_socket_accept, 2)
MINT_EXPORT_FUNCTION(mint_local_socket_accept_async, 3)
MINT_EXPORT_FUNCTION(mint_local_socket_set_non_blocking, 2)
