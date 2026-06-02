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
#include "mint/memory/builtin/array.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/memory/cast_tools.h"
#include "mint/memory/data.h"
#include "mint/memory/function_tools.h"
#include "mint/memory/memory_tools.h"
#include "mint/memory/reference.h"
#include <cstddef>
#include <optional>

namespace {

mint::Reference mint_exception_stacktrace(mint::Cursor& cursor) {
	auto result = mint::create_iterator(cursor.ast());
	if (const auto* exception = cursor.get_exception()) {
		for (const auto& frame : exception->stacktrace) {
			mint::iterator_yield(cursor, result.data<mint::Iterator>(),
			    mint::create_iterator_from(cursor, mint::create_unsigned_number(frame.module_id()),
			        mint::create_string(cursor.ast(), frame.module_name()),
			        mint::create_unsigned_number(frame.line_number())));
		}
	}
	return result;
}

mint::Reference mint_exception_stacktrace_for(mint::Cursor& cursor, const mint::Reference& cause,
    const mint::Reference& depth) {
	const auto chain_depth = mint::is_instance_of(depth, mint::Data::Format::none)
	                             ? std::nullopt
	                             : std::optional<std::size_t>(mint::to_integer<std::size_t>(cursor, depth));
	for (const auto* exception = cursor.get_exception(); exception != nullptr; exception = exception->cause.get()) {
		if (chain_depth && *chain_depth != exception->depth) {
			continue;
		}
		if (&exception->object.data() != &cause.data()) {
			continue;
		}
		auto result = mint::create_iterator(cursor.ast());
		for (const auto& frame : exception->stacktrace) {
			mint::iterator_yield(cursor, result.data<mint::Iterator>(),
			    mint::create_iterator_from(cursor, mint::create_unsigned_number(frame.module_id()),
			        mint::create_string(cursor.ast(), frame.module_name()),
			        mint::create_unsigned_number(frame.line_number())));
		}
		return result;
	}
	return {};
}

mint::Reference mint_exception_cause_for(mint::Cursor& cursor, const mint::Reference& error,
    const mint::Reference& depth) {
	const auto chain_depth = mint::is_instance_of(depth, mint::Data::Format::none)
	                             ? std::nullopt
	                             : std::optional<std::size_t>(mint::to_integer<std::size_t>(cursor, depth));
	for (const auto* exception = cursor.get_exception(); exception != nullptr; exception = exception->cause.get()) {
		if (chain_depth && *chain_depth != exception->depth) {
			continue;
		}
		if (&exception->object.data() != &error.data()) {
			continue;
		}
		if (const auto* cause = exception->cause.get()) {
			return mint::create_iterator_from(cursor, cause->object, mint::create_unsigned_number(cause->depth));
		}
	}
	return {};
}

mint::Reference mint_exception_causes_for(mint::Cursor& cursor, const mint::Reference& error,
    const mint::Reference& depth) {
	const auto chain_depth = mint::is_instance_of(depth, mint::Data::Format::none)
	                             ? std::nullopt
	                             : std::optional<std::size_t>(mint::to_integer<std::size_t>(cursor, depth));
	for (const auto* exception = cursor.get_exception(); exception != nullptr; exception = exception->cause.get()) {
		if (chain_depth && *chain_depth != exception->depth) {
			continue;
		}
		if (&exception->object.data() != &error.data()) {
			continue;
		}
		auto result = mint::create_iterator(cursor.ast());
		for (const auto* cause = exception->cause.get(); cause != nullptr; cause = cause->cause.get()) {
			mint::iterator_yield(cursor, result.data<mint::Iterator>(),
			    mint::create_iterator_from(cursor, cause->object, mint::create_unsigned_number(cause->depth)));
		}
		return result;
	}
	return {};
}

}

MINT_EXPORT_FUNCTION(mint_exception_stacktrace, 0)
MINT_EXPORT_FUNCTION(mint_exception_stacktrace_for, 2)
MINT_EXPORT_FUNCTION(mint_exception_cause_for, 2)
MINT_EXPORT_FUNCTION(mint_exception_causes_for, 2)
