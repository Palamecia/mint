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

#include "mint/memory/functiontool.h"
#include "mint/memory/operatortool.h"
#include "mint/memory/memorytool.h"
#include "mint/memory/casttool.h"
#include "mint/system/utf8.h"
#include "mint/system/stdio.h"
#include "mint/system/terminal.h"
#include "mint/ast/fileprinter.h"
#include "mint/ast/cursor.h"
#include "mint/scheduler/scheduler.h"
#include "mint/scheduler/process.h"

#ifdef OS_UNIX
#include <poll.h>
#include <unistd.h>
#include <termios.h>
#else
#include <Windows.h>
#include <io.h>
#endif

using namespace mint;

namespace symbols {

static const Symbol d_ptr("d_ptr");

static const std::string DataStream("Serializer.DataStream");

}

static WeakReference get_d_ptr(Reference &reference) {

	Object *object = reference.data<Object>();
	auto it = object->metadata->members().find(symbols::d_ptr);

	if (it != object->metadata->members().end()) {
		return WeakReference::share(Class::MemberInfo::get(it->second, object));
	}

	return WeakReference();
}

MINT_FUNCTION(mint_terminal_new, 0, cursor) {
	FunctionHelper helper(cursor, 0);
	helper.return_value(create_object(new Terminal));
}

MINT_FUNCTION(mint_terminal_delete, 1, cursor) {
	FunctionHelper helper(cursor, 1);
	const Reference &self = helper.pop_parameter();
	delete self.data<LibObject<Terminal>>()->impl;
}

MINT_FUNCTION(mint_terminal_get_width, 0, cursor) {
	FunctionHelper helper(cursor, 0);
	helper.return_value(create_number(Terminal::get_width()));
}

MINT_FUNCTION(mint_terminal_get_height, 0, cursor) {
	FunctionHelper helper(cursor, 0);
	helper.return_value(create_number(Terminal::get_height()));
}

MINT_FUNCTION(mint_terminal_get_cursor_row, 0, cursor) {
	FunctionHelper helper(cursor, 0);
	helper.return_value(create_number(Terminal::get_cursor_row()));
}

MINT_FUNCTION(mint_terminal_get_cursor_column, 0, cursor) {
	FunctionHelper helper(cursor, 0);
	helper.return_value(create_number(Terminal::get_cursor_column()));
}

MINT_FUNCTION(mint_terminal_set_cursor_pos, 2, cursor) {
	FunctionHelper helper(cursor, 2);
	Reference &column = helper.pop_parameter();
	Reference &row = helper.pop_parameter();
	Terminal::set_cursor_pos(to_integer(cursor, row), to_integer(cursor, column));
}

MINT_FUNCTION(mint_terminal_move_cursor_left, 1, cursor) {
	FunctionHelper helper(cursor, 1);
	Reference &count = helper.pop_parameter();
	Terminal::move_cursor_left(to_integer(cursor, count));
}

MINT_FUNCTION(mint_terminal_move_cursor_right, 1, cursor) {
	FunctionHelper helper(cursor, 1);
	Reference &count = helper.pop_parameter();
	Terminal::move_cursor_right(to_integer(cursor, count));
}

MINT_FUNCTION(mint_terminal_move_cursor_up, 1, cursor) {
	FunctionHelper helper(cursor, 1);
	Reference &count = helper.pop_parameter();
	Terminal::move_cursor_up(to_integer(cursor, count));
}

MINT_FUNCTION(mint_terminal_move_cursor_down, 1, cursor) {
	FunctionHelper helper(cursor, 1);
	Reference &count = helper.pop_parameter();
	Terminal::move_cursor_down(to_integer(cursor, count));
}

MINT_FUNCTION(mint_terminal_move_cursor_to_start_of_line, 0, cursor) {
	FunctionHelper helper(cursor, 0);
	Terminal::move_cursor_to_start_of_line();
}

MINT_FUNCTION(mint_terminal_set_prompt, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &function = helper.pop_parameter();
	Reference &self = helper.pop_parameter();

	struct callback_t {
		explicit callback_t(WeakReference &&function) :
			function(std::make_shared<StrongReference>(std::move(function))) {

		}
		std::string operator ()(size_t row_number) {
			if (has_signature(*function, 1)) {
				return to_string(Scheduler::instance()->invoke(*function, create_number(row_number)));
			}
			return to_string(*function);
		}
	private:
		std::shared_ptr<StrongReference> function;
	};

	self.data<LibObject<Terminal>>()->impl->set_prompt(callback_t { std::move(function) });
}

MINT_FUNCTION(mint_terminal_set_highlighter, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &function = helper.pop_parameter();
	Reference &self = helper.pop_parameter();

	struct callback_t {
		explicit callback_t(WeakReference &&function) :
			function(std::make_shared<StrongReference>(std::move(function))) {

		}
		std::string operator ()(std::string_view str, std::string_view::size_type pos) {
			return to_string(Scheduler::instance()->invoke(*function, create_string(str), create_number(pos)));
		}
	private:
		std::shared_ptr<StrongReference> function;
	};

	self.data<LibObject<Terminal>>()->impl->set_highlighter(callback_t { std::move(function) });
}

MINT_FUNCTION(mint_terminal_set_completion_generator, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &function = helper.pop_parameter();
	Reference &self = helper.pop_parameter();

	struct callback_t {
		explicit callback_t(WeakReference &&function) :
			function(std::make_shared<StrongReference>(std::move(function))) {

		}
		bool operator ()(std::string_view str, std::string_view::size_type pos, std::vector<completion_t> &results) {
			WeakReference result = Scheduler::instance()->invoke(*function, create_string(str), create_number(pos));
			if (is_instance_of(result, Data::FMT_NONE)) {
				return false;
			}
			Iterator *it = iterator_init(result);
			while (std::optional<WeakReference> item = iterator_next(it)) {
				if (std::optional<WeakReference> token = iterator_next(item->data<Iterator>())) {
					completion_t completion;
					completion.token = to_string(token.value());
					completion.offset = to_integer(Scheduler::instance()->current_process()->cursor(), iterator_next(item->data<Iterator>()).value_or(create_number(std::string_view::npos)));
					results.push_back(completion);
				}
			}
			return true;
		}
	private:
		std::shared_ptr<StrongReference> function;
	};

	self.data<LibObject<Terminal>>()->impl->set_completion_generator(callback_t { std::move(function) });
}

MINT_FUNCTION(mint_terminal_set_brace_matcher, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &function = helper.pop_parameter();
	Reference &self = helper.pop_parameter();

	if (has_signature(function, 2)) {

		struct callback_t {
			explicit callback_t(WeakReference &&function) :
				function(std::make_shared<StrongReference>(std::move(function))) {

			}
			std::pair<std::string_view::size_type, bool> operator ()(std::string_view str, std::string_view::size_type pos) {
				Iterator *result = iterator_init(Scheduler::instance()->invoke(*function, create_string(str), create_number(pos)));
				const std::string_view::size_type offset = to_integer(Scheduler::instance()->current_process()->cursor(), iterator_next(result).value_or(create_number(std::string_view::npos)));
				const bool balanced = to_boolean(Scheduler::instance()->current_process()->cursor(), iterator_next(result).value_or(create_boolean(false)));
				return { offset, balanced };
			}
		private:
			std::shared_ptr<StrongReference> function;
		};

		self.data<LibObject<Terminal>>()->impl->set_brace_matcher(callback_t { std::move(function) });
	}
	else {
		self.data<LibObject<Terminal>>()->impl->set_auto_braces(to_string(function));
	}
}

MINT_FUNCTION(mint_terminal_edit_line, 1, cursor) {
	FunctionHelper helper(cursor, 1);
	const Reference &self = helper.pop_parameter();
	if (auto input = self.data<LibObject<Terminal>>()->impl->read_line()) {
		helper.return_value(create_string(*input));
	}
}

MINT_FUNCTION(mint_terminal_flush, 0, cursor) {
	FunctionHelper helper(cursor, 0);
	fflush(stdout);
	fflush(stderr);
}

MINT_FUNCTION(mint_terminal_is_terminal, 1, cursor) {
	FunctionHelper helper(cursor, 1);
	Reference &stream = helper.pop_parameter();
	helper.return_value(create_boolean(is_term(to_integer(cursor, stream))));
}

MINT_FUNCTION(mint_terminal_readchar, 0, cursor) {

	FunctionHelper helper(cursor, 0);

	int fd = fileno(stdin);

	char buffer[5];

	if (read(fd, buffer, 1) > 0) {
		
		size_t length = utf8_code_point_length(static_cast<byte_t>(*buffer));

		if (length > 1) {
			if (read(fd, buffer + 1, length - 1) > 0) {
				helper.return_value(create_string(std::string(buffer, length)));
			}
		}
		else {
			helper.return_value(create_string(std::string(buffer, 1)));
		}
	}
}

MINT_FUNCTION(mint_terminal_readline, 0, cursor) {

	FunctionHelper helper(cursor, 0);

	size_t size = 0;
	char *buffer = nullptr;

	if (getline(&buffer, &size, stdin) != -1) {
		helper.return_value(create_string(buffer));
		free(buffer);
	}
}

MINT_FUNCTION(mint_terminal_read, 1, cursor) {

	FunctionHelper helper(cursor, 1);

	size_t size = 0;
	char *buffer = nullptr;
	std::string delim = to_string(helper.pop_parameter());

	if (getdelim(&buffer, &size, delim.front(), stdin) != -1) {
		helper.return_value(create_string(buffer));
		free(buffer);
	}
}

static int write_binary_data(FILE *stream, const std::vector<uint8_t> *data) {
	return fwrite(data->data(), sizeof(uint8_t), data->size(), stream);
}

static int write_string_data(FILE *stream, const std::string &data) {
	return mint::print(stream, data.c_str());
}

MINT_FUNCTION(mint_terminal_write, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &data = helper.pop_parameter();
	int amount = EOF;

	if (is_instance_of(data, symbols::DataStream)) {
		amount = write_binary_data(stdout, get_d_ptr(data).data<LibObject<std::vector<uint8_t>>>()->impl);
	}
	else {
		amount = write_string_data(stdout, to_string(data));
	}

	helper.return_value(create_iterator(create_number(static_cast<double>(amount)),
										(amount == EOF) ? create_number(errno) : WeakReference::create<None>()));
}

MINT_FUNCTION(mint_terminal_write_error, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &data = helper.pop_parameter();
	int amount = EOF;

	if (is_instance_of(data, symbols::DataStream)) {
		amount = write_binary_data(stderr, get_d_ptr(data).data<LibObject<std::vector<uint8_t>>>()->impl);
	}
	else {
		amount = write_string_data(stderr, to_string(data));
	}

	helper.return_value(create_iterator(create_number(static_cast<double>(amount)),
										(amount == EOF) ? create_number(errno) : WeakReference::create<None>()));
}

MINT_FUNCTION(mint_terminal_change_attribute, 1, cursor) {

	std::string attr = to_string(cursor->stack().back());
	FILE *stream = stdout;

	cursor->stack().back() = WeakReference::create<None>();
	cursor->exit_call();
	cursor->exit_call();

	if (const FilePrinter *printer = dynamic_cast<FilePrinter *>(cursor->printer())) {
		stream = printer->file();
	}

	if (is_term(stream)) {
		Terminal::print(stream, attr.c_str());
	}
}

MINT_FUNCTION(mint_terminal_get_stdin_handle, 0, cursor) {

	FunctionHelper helper(cursor, 0);

#ifdef OS_WINDOWS
	helper.return_value(create_handle(GetStdHandle(STD_INPUT_HANDLE)));
#else
	helper.return_value(create_handle(STDIN_FILE_NO));
#endif
}

MINT_FUNCTION(mint_terminal_wait, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &timeout = helper.pop_parameter();

#ifdef OS_WINDOWS
	mint::handle_t h = GetStdHandle(STD_INPUT_HANDLE);
	DWORD dwMilliseconds = INFINITE;

	if (timeout.data()->format != Data::FMT_NONE) {
		dwMilliseconds = static_cast<DWORD>(to_integer(cursor, timeout));
	}

	DWORD ret = WaitForSingleObjectEx(h, dwMilliseconds, true);
	helper.return_value(create_boolean(ret == WAIT_OBJECT_0));
#else
	pollfd fds;
	fds.events = POLLIN;
	fds.fd = STDIN_FILE_NO;

	int time_ms = -1;

	if (timeout.data()->format != Data::FMT_NONE) {
		time_ms = static_cast<int>(to_integer(cursor, timeout));
	}

	bool result = false;
	int ret = poll(&fds, 1, time_ms);

	if ((ret > 0) && (fds.revents & POLLIN)) {
		result = true;
	}

	helper.return_value(create_boolean(result));
#endif
}

MINT_FUNCTION(mint_terminal_clear_to_end_of_line, 0, cursor) {
	FunctionHelper helper(cursor, 0);
	Terminal::clear_to_end_of_line();
}

MINT_FUNCTION(mint_terminal_clear_line, 0, cursor) {
	FunctionHelper helper(cursor, 0);
	Terminal::clear_line();
}
