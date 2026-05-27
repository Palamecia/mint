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

#include "mint/memory/garbage_collector.h"
#include "mint/memory/cast_tools.h"
#include "mint/memory/reference.h"
#include "mint/memory/function_tools.h"
#include <cstddef>

namespace {

mint::Reference mint_garbage_collector_collect(mint::Cursor& /*cursor*/) {
	return mint::create_unsigned_number(mint::GarbageCollector::instance().collect());
}

mint::Reference mint_garbage_collector_get_threshold(mint::Cursor& /*cursor*/) {
	return mint::create_unsigned_number(mint::GarbageCollector::instance().get_threshold());
}

mint::Reference mint_garbage_collector_set_threshold(mint::Cursor& cursor, const mint::Reference& threshold) {
	mint::GarbageCollector::instance().set_threshold(mint::to_integer<std::size_t>(cursor, threshold));
	return {};
}

mint::Reference mint_garbage_collector_get_refcount(mint::Cursor& /*cursor*/, const mint::Reference& object) {
	return mint::create_unsigned_number(mint::GarbageCollector::get_refcount(object.data()));
}

mint::Reference mint_garbage_collector_get_count(mint::Cursor& /*cursor*/) {
	return mint::create_unsigned_number(mint::GarbageCollector::instance().get_count());
}

}

MINT_EXPORT_FUNCTION(mint_garbage_collector_collect, 0)
MINT_EXPORT_FUNCTION(mint_garbage_collector_get_threshold, 0)
MINT_EXPORT_FUNCTION(mint_garbage_collector_set_threshold, 1)
MINT_EXPORT_FUNCTION(mint_garbage_collector_get_refcount, 1)
MINT_EXPORT_FUNCTION(mint_garbage_collector_get_count, 0)
