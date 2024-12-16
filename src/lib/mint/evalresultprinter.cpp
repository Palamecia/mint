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

#include "evalresultprinter.h"

#include <mint/memory/builtin/iterator.h>
#include <mint/memory/functiontool.h>

using namespace mint;

void EvalResultPrinter::print(Reference &reference) {
	m_results.emplace_back(WeakReference::share(reference));
}

bool EvalResultPrinter::global() const {
	return true;
}

WeakReference EvalResultPrinter::result() {
	switch (m_results.size()) {
	case 0:
		return WeakReference::create<None>();

	case 1:
		return std::move(m_results.front());

	default:
		break;
	}

	WeakReference reference = create_iterator();

	for (Reference &item : m_results) {
		iterator_insert(reference.data<Iterator>(), std::move(item));
	}

	return reference;
}
