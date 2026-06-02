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

#include "mint/ast/symbol.h"
#include "mint/config.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/memory/builtin/libobject.h"
#include "mint/memory/class.h"
#include "mint/memory/data.h"
#include "mint/memory/function_tools.h"
#include "mint/memory/operator_tools.h"
#include "mint/memory/memory_tools.h"
#include "mint/memory/cast_tools.h"
#include "mint/memory/reference.h"
#include "mint/system/utf8.h"
#include "mint/system/stdio.h"
#include "mint/system/terminal.h"
#include "mint/ast/file_printer.h"
#include "mint/ast/cursor.h"
#include "mint/scheduler/scheduler.h"
#include "mint/scheduler/process.h"
#include "mint/system/errno.h"
#include "mint/system/pipe.h"
#include <array>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <stdio.h>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#ifdef MINT_OS_WINDOWS
#include <Windows.h>
#include <corecrt_io.h>
#include <io.h>
#include <minwindef.h>
#include <processenv.h>
#include <synchapi.h>
#include <winbase.h>
#else
#include <cstddef>
#include <poll.h>
#include <sys/poll.h>
#include <termios.h>
#include <unistd.h>
#endif

namespace symbols {

static const mint::Symbol d_ptr("d_ptr");

static const std::string data_stream("Serializer.DataStream");

}

namespace {

mint::Reference get_d_ptr(const mint::Reference& reference) {
	if (auto& object = reference.data<mint::Object>(); auto* member = object.metadata.find_member(symbols::d_ptr)) {
		return mint::Class::MemberInfo::get(*member, object);
	}
	return {};
}

std::size_t write_to_term(FILE* stream, const std::vector<std::uint8_t>& data) {
	const auto amount = fwrite(data.data(), sizeof(std::uint8_t), data.size(), stream);
	if (amount != data.size()) {
		throw std::system_error(mint::last_error_code());
	}
	return amount;
}

std::size_t write_to_term(FILE* stream, const std::string& data) {
	if (mint::is_term(stream)) {
		return mint::Terminal::write(stream, data);
	}
	if (mint::is_pipe(stream)) {
		return mint::Pipe::write(stream, data);
	}
	const auto amount = std::fputs(data.data(), stream);
	if (amount == EOF) {
		throw std::system_error(mint::last_error_code());
	}
	return static_cast<std::size_t>(amount);
}

mint::Reference mint_terminal_new(mint::Cursor& cursor) {
	return mint::create_c_object(cursor.ast(), new mint::Terminal);
}

mint::Reference mint_terminal_delete(mint::Cursor& /*cursor*/, const mint::Reference& self) {
	delete self.data<mint::LibObject<mint::Terminal>>().ptr;
	return {};
}

mint::Reference mint_terminal_get_width(mint::Cursor& /*cursor*/) {
	return mint::create_unsigned_number(mint::Terminal::get_width());
}

mint::Reference mint_terminal_get_height(mint::Cursor& /*cursor*/) {
	return mint::create_unsigned_number(mint::Terminal::get_height());
}

mint::Reference mint_terminal_get_cursor_row(mint::Cursor& /*cursor*/) {
	return mint::create_unsigned_number(mint::Terminal::get_cursor_row());
}

mint::Reference mint_terminal_get_cursor_column(mint::Cursor& /*cursor*/) {
	return mint::create_unsigned_number(mint::Terminal::get_cursor_column());
}

mint::Reference mint_terminal_set_cursor_pos(mint::Cursor& cursor, const mint::Reference& row,
    const mint::Reference& column) {
	mint::Terminal::set_cursor_pos(to_integer<std::size_t>(cursor, row), to_integer<std::size_t>(cursor, column));
	return {};
}

mint::Reference mint_terminal_move_cursor_left(mint::Cursor& cursor, const mint::Reference& count) {
	mint::Terminal::move_cursor_left(to_integer<std::size_t>(cursor, count));
	return {};
}

mint::Reference mint_terminal_move_cursor_right(mint::Cursor& cursor, const mint::Reference& count) {
	mint::Terminal::move_cursor_right(to_integer<std::size_t>(cursor, count));
	return {};
}

mint::Reference mint_terminal_move_cursor_up(mint::Cursor& cursor, const mint::Reference& count) {
	mint::Terminal::move_cursor_up(to_integer<std::size_t>(cursor, count));
	return {};
}

mint::Reference mint_terminal_move_cursor_down(mint::Cursor& cursor, const mint::Reference& count) {
	mint::Terminal::move_cursor_down(to_integer<std::size_t>(cursor, count));
	return {};
}

mint::Reference mint_terminal_move_cursor_to_start_of_line(mint::Cursor& /*cursor*/) {
	mint::Terminal::move_cursor_to_start_of_line();
	return {};
}

mint::Reference mint_terminal_set_prompt(mint::FunctionHelper& helper, const mint::Reference& self,
    mint::Reference& function) {

	struct Callback {
		Callback(mint::Scheduler& scheduler, mint::Reference&& function) :
		    _scheduler(scheduler),
		    _function(std::make_shared<mint::RootReference>(std::move(function))) {}

		std::string operator()(std::size_t row_number) {
			if (has_signature(*_function, 1)) {
				return to_string(_scheduler.get().invoke(*_function, mint::create_unsigned_number(row_number)));
			}
			return to_string(*_function);
		}

	private:
		std::reference_wrapper<mint::Scheduler> _scheduler;
		std::shared_ptr<mint::RootReference> _function;
	};

	self.data<mint::LibObject<mint::Terminal>>().ptr->set_prompt(Callback(helper.scheduler(), std::move(function)));
	return {};
}

mint::Reference mint_terminal_set_highlighter(mint::FunctionHelper& helper, const mint::Reference& self,
    mint::Reference& function) {

	struct Callback {
		Callback(mint::Scheduler& scheduler, mint::Reference&& function) :
		    _scheduler(scheduler),
		    _function(std::make_shared<mint::RootReference>(std::move(function))) {}

		std::string operator()(std::string_view str, std::string_view::size_type pos) {
			return to_string(_scheduler.get().invoke(*_function, mint::create_string(_scheduler.get().ast(), str),
			    mint::create_unsigned_number(pos)));
		}

	private:
		std::reference_wrapper<mint::Scheduler> _scheduler;
		std::shared_ptr<mint::RootReference> _function;
	};

	self.data<mint::LibObject<mint::Terminal>>().ptr->set_highlighter(Callback(helper.scheduler(), std::move(function)));
	return {};
}

mint::Reference mint_terminal_set_completion_generator(mint::FunctionHelper& helper, const mint::Reference& self,
    mint::Reference& function) {

	struct Callback {
		Callback(mint::Scheduler& scheduler, mint::Reference&& function) :
		    _scheduler(scheduler),
		    _function(std::make_shared<mint::RootReference>(std::move(function))) {}

		std::optional<std::vector<mint::Completion>> operator()(std::string_view str, std::string_view::size_type pos) {
			auto result = _scheduler.get().invoke(*_function, mint::create_string(_scheduler.get().ast(), str),
			    mint::create_unsigned_number(pos));
			if (mint::is_instance_of(result, mint::Data::Format::none)) {
				return std::nullopt;
			}
			auto results = std::vector<mint::Completion>();
			auto& cursor = mint::Scheduler::current_process()->cursor();
			auto it = mint::create_iterator_over(cursor, result);
			while (std::optional<mint::Reference> item = mint::iterator_next(cursor, it.data<mint::Iterator>())) {
				if (std::optional<mint::Reference> token = iterator_next(cursor, item->data<mint::Iterator>())) {
					results.push_back({
					    .offset = to_integer<std::string::size_type>(mint::Scheduler::current_process()->cursor(),
					        iterator_next(cursor, item->data<mint::Iterator>())
					            .value_or(mint::create_unsigned_number(std::string_view::npos))),
					    .token = to_string(token.value()),
					});
				}
			}
			return results;
		}

	private:
		std::reference_wrapper<mint::Scheduler> _scheduler;
		std::shared_ptr<mint::RootReference> _function;
	};

	self.data<mint::LibObject<mint::Terminal>>().ptr->set_completion_generator(
	    Callback(helper.scheduler(), std::move(function)));
	return {};
}

mint::Reference mint_terminal_set_brace_matcher(mint::FunctionHelper& helper, const mint::Reference& self,
    mint::Reference& function) {

	if (has_signature(function, 2)) {

		struct Callback {
			Callback(mint::Scheduler& scheduler, mint::Reference&& function) :
			    _scheduler(scheduler),
			    _function(std::make_shared<mint::RootReference>(std::move(function))) {}

			std::pair<std::string_view::size_type, bool> operator()(std::string_view str,
			    std::string_view::size_type pos) {
				auto& cursor = mint::Scheduler::current_process()->cursor();
				auto result = mint::create_iterator_over(cursor,
				    _scheduler.get().invoke(*_function, mint::create_string(_scheduler.get().ast(), str),
				        mint::create_unsigned_number(pos)));
				const auto offset = to_integer<std::string_view::size_type>(mint::Scheduler::current_process()->cursor(),
				    mint::iterator_next(cursor, result.data<mint::Iterator>())
				        .value_or(mint::create_unsigned_number(std::string_view::npos)));
				const bool balanced = to_boolean(
				    mint::iterator_next(cursor, result.data<mint::Iterator>()).value_or(mint::create_boolean(false)));
				return {offset, balanced};
			}
		private:
			std::reference_wrapper<mint::Scheduler> _scheduler;
			std::shared_ptr<mint::RootReference> _function;
		};

		self.data<mint::LibObject<mint::Terminal>>().ptr->set_brace_matcher(
		    Callback(helper.scheduler(), std::move(function)));
	}
	else {
		self.data<mint::LibObject<mint::Terminal>>().ptr->set_auto_braces(to_string(function));
	}
	return {};
}

mint::Reference mint_terminal_edit_line(mint::Cursor& cursor, const mint::Reference& self) {
	if (auto input = self.data<mint::LibObject<mint::Terminal>>().ptr->read_line()) {
		return mint::create_string(cursor.ast(), *input);
	}
	return {};
}

mint::Reference mint_terminal_flush(mint::Cursor& /*cursor*/) {
	std::fflush(stdout);
	std::fflush(stderr);
	return {};
}

mint::Reference mint_terminal_is_terminal(mint::Cursor& cursor, const mint::Reference& stream) {
	return mint::create_boolean(mint::is_term(to_integer<int>(cursor, stream)));
}

mint::Reference mint_terminal_readchar(mint::Cursor& cursor) {

	const int fd = fileno(stdin);
	std::array<char, mint::utf8_code_point_length_max + 1> buffer = {};

	if (read(fd, buffer.data(), sizeof(char)) > 0) {
		if (const std::size_t length = mint::utf8_code_point_length(static_cast<std::uint8_t>(buffer[0])); length > 1) {
			if (read(fd, std::next(buffer.data(), 1), static_cast<int>(buffer.size()) - 1) > 0) {
				return mint::create_string(cursor.ast(), std::string(buffer.data(), length));
			}
		}
		else {
			return mint::create_string(cursor.ast(), std::string(buffer.data(), 1));
		}
	}

	return {};
}

mint::Reference mint_terminal_readline(mint::Cursor& cursor) {
	if (!std::feof(stdin)) {
		return mint::create_string(cursor.ast(), mint::get_line(stdin));
	}
	return {};
}

mint::Reference mint_terminal_read(mint::Cursor& cursor, const mint::Reference& delim) {
	if (!std::feof(stdin)) {
		return mint::create_string(cursor.ast(), mint::get_delim(mint::to_string(delim).front(), stdin));
	}
	return {};
}

mint::Reference mint_terminal_write(mint::Cursor& cursor, const mint::Reference& data) {
	try {
		if (is_instance_of(data, symbols::data_stream)) {
			return create_iterator_from(cursor,
			    mint::create_unsigned_number(
			        write_to_term(stdout, *get_d_ptr(data).data<mint::LibObject<std::vector<std::uint8_t>>>().ptr)),
			    mint::create_none());
		}
		return create_iterator_from(cursor, mint::create_unsigned_number(write_to_term(stdout, to_string(data))),
		    mint::create_none());
	}
	catch (std::system_error& error) {
		return create_iterator_from(cursor, mint::create_signed_number(EOF),
		    mint::create_number(mint::errno_from_error_code(error.code())));
	}
}

mint::Reference mint_terminal_write_error(mint::Cursor& cursor, const mint::Reference& data) {
	try {
		if (is_instance_of(data, symbols::data_stream)) {
			return create_iterator_from(cursor,
			    mint::create_unsigned_number(
			        write_to_term(stderr, *get_d_ptr(data).data<mint::LibObject<std::vector<std::uint8_t>>>().ptr)),
			    mint::create_none());
		}
		return create_iterator_from(cursor, mint::create_unsigned_number(write_to_term(stderr, to_string(data))),
		    mint::create_none());
	}
	catch (std::system_error& error) {
		return create_iterator_from(cursor, mint::create_signed_number(EOF),
		    mint::create_number(mint::errno_from_error_code(error.code())));
	}
}

mint::Reference mint_terminal_get_stdin_handle(mint::Cursor& cursor) {
#ifdef MINT_OS_WINDOWS
	return mint::create_handle(cursor.ast(), GetStdHandle(STD_INPUT_HANDLE));
#else
	return mint::create_handle(cursor.ast(), mint::stdin_file_no);
#endif
}

mint::Reference mint_terminal_wait(mint::Cursor& cursor, const mint::Reference& timeout) {
#ifdef MINT_OS_WINDOWS
	mint::handle_t handle = GetStdHandle(STD_INPUT_HANDLE);
	const DWORD time_ms = mint::is_instance_of(timeout, mint::Data::Format::none)
	                          ? INFINITE
	                          : mint::to_integer<DWORD>(cursor, timeout);

	return mint::create_boolean(WaitForSingleObjectEx(handle, time_ms, true) == WAIT_OBJECT_0);
#else
	pollfd fds {
	    .fd = mint::stdin_file_no,
	    .events = POLLIN,
	};

	const int time_ms = is_instance_of(timeout, mint::Data::Format::none) ? -1 : to_integer<int>(cursor, timeout);

	if (const auto ret = poll(&fds, 1, time_ms); (ret > 0) && (fds.revents & POLLIN)) {
		return mint::create_boolean(true);
	}

	return mint::create_boolean(false);
#endif
}

mint::Reference mint_terminal_clear_to_end_of_line(mint::Cursor& /*cursor*/) {
	mint::Terminal::clear_to_end_of_line();
	return {};
}

mint::Reference mint_terminal_clear_line(mint::Cursor& /*cursor*/) {
	mint::Terminal::clear_line();
	return {};
}

}

MINT_EXPORT_FUNCTION(mint_terminal_new, 0)
MINT_EXPORT_FUNCTION(mint_terminal_delete, 1)
MINT_EXPORT_FUNCTION(mint_terminal_get_width, 0)
MINT_EXPORT_FUNCTION(mint_terminal_get_height, 0)
MINT_EXPORT_FUNCTION(mint_terminal_get_cursor_row, 0)
MINT_EXPORT_FUNCTION(mint_terminal_get_cursor_column, 0)
MINT_EXPORT_FUNCTION(mint_terminal_set_cursor_pos, 2)
MINT_EXPORT_FUNCTION(mint_terminal_move_cursor_left, 1)
MINT_EXPORT_FUNCTION(mint_terminal_move_cursor_right, 1)
MINT_EXPORT_FUNCTION(mint_terminal_move_cursor_up, 1)
MINT_EXPORT_FUNCTION(mint_terminal_move_cursor_down, 1)
MINT_EXPORT_FUNCTION(mint_terminal_move_cursor_to_start_of_line, 0)
MINT_EXPORT_FUNCTION(mint_terminal_set_prompt, 2)
MINT_EXPORT_FUNCTION(mint_terminal_set_highlighter, 2)
MINT_EXPORT_FUNCTION(mint_terminal_set_completion_generator, 2)
MINT_EXPORT_FUNCTION(mint_terminal_set_brace_matcher, 2)
MINT_EXPORT_FUNCTION(mint_terminal_edit_line, 1)
MINT_EXPORT_FUNCTION(mint_terminal_flush, 0)
MINT_EXPORT_FUNCTION(mint_terminal_is_terminal, 1)
MINT_EXPORT_FUNCTION(mint_terminal_readchar, 0)
MINT_EXPORT_FUNCTION(mint_terminal_readline, 0)
MINT_EXPORT_FUNCTION(mint_terminal_read, 1)
MINT_EXPORT_FUNCTION(mint_terminal_write, 1)
MINT_EXPORT_FUNCTION(mint_terminal_write_error, 1)

MINT_RAW_FUNCTION(mint_terminal_change_attribute, 1, cursor) {

	const auto attr = to_string(cursor.stack().back());
	FILE* stream = stdout;

	cursor.stack().back() = mint::create_none();
	cursor.exit_call();
	cursor.exit_call();

	if (const mint::FilePrinter* printer = dynamic_cast<mint::FilePrinter*>(cursor.printer())) {
		stream = printer->stream();
	}

	if (mint::is_term(stream)) {
		mint::Terminal::print(stream, attr);
	}
}

MINT_EXPORT_FUNCTION(mint_terminal_get_stdin_handle, 0)
MINT_EXPORT_FUNCTION(mint_terminal_wait, 1)
MINT_EXPORT_FUNCTION(mint_terminal_clear_to_end_of_line, 0)
MINT_EXPORT_FUNCTION(mint_terminal_clear_line, 0)
