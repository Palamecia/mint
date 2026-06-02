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
#include "mint/memory/data.h"
#include "mint/memory/object.h"
#include "mint/memory/reference.h"
#include "mint/memory/function_tools.h"
#include "mint/memory/memory_tools.h"
#include "mint/memory/cast_tools.h"
#include <ranges>
#include <vector>

#ifdef MINT_OS_WINDOWS
#include <Windows.h>
#else
#include <poll.h>
#endif

namespace symbols {

static const mint::Symbol handle("handle");
static const mint::Symbol activated("activated");

}

namespace {

mint::Reference mint_poll(mint::Cursor& cursor, const mint::Reference& event_set, const mint::Reference& timeout) {
#ifdef MINT_OS_WINDOWS

	std::vector<HANDLE> fdset(std::from_range,
	    std::views::transform(mint::to_array(event_set), [&ast = cursor.ast()](auto& item) {
		    return to_handle(mint::get_member_ignore_visibility(ast, item, symbols::handle));
	    }));

	const DWORD time_ms = mint::is_instance_of(timeout, mint::Data::Format::none)
	                          ? INFINITE
	                          : mint::to_integer<DWORD>(cursor, timeout);

	DWORD status = WaitForMultipleObjectsEx(static_cast<DWORD>(fdset.size()), fdset.data(), false, time_ms, true);
	while (status == WAIT_IO_COMPLETION) {
		status = WaitForMultipleObjectsEx(static_cast<DWORD>(fdset.size()), fdset.data(), false, 0, true);
	}

	for (const auto& [fd, event] :
	    std::views::drop(std::views::zip(fdset, mint::to_array(event_set)), status - WAIT_OBJECT_0 + 1)) {
		mint::get_member_ignore_visibility(cursor.ast(), event, symbols::activated).data<mint::Boolean>().value =
		    (WaitForSingleObjectEx(fd, 0, true) == WAIT_OBJECT_0);
	}
#else
	std::vector<pollfd> fdset(std::from_range,
	    std::views::transform(mint::to_array(event_set), [&ast = cursor.ast()](auto& item) {
		    return pollfd {
		        .fd = to_handle(mint::get_member_ignore_visibility(ast, item, symbols::handle)),
		        .events = POLLIN,
		    };
	    }));

	const int time_ms = is_instance_of(timeout, mint::Data::Format::none) ? -1 : to_integer<int>(cursor, timeout);

	poll(fdset.data(), fdset.size(), time_ms);

	for (const auto& [fd, event] : std::views::zip(fdset, mint::to_array(event_set))) {
		mint::get_member_ignore_visibility(cursor.ast(), event, symbols::activated).data<mint::Boolean>().value =
		    fd.revents & POLLIN;
	}
#endif
	return {};
}

}

MINT_EXPORT_FUNCTION(mint_poll, 2)
