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

#ifndef LIBMINT_MEMORY_BUILTIN_ITERATOR_GENERATOR_H
#define LIBMINT_MEMORY_BUILTIN_ITERATOR_GENERATOR_H

#include "iterator_items.h"
#include "iterator_p.h"
#include "mint/ast/cursor.h"
#include "mint/ast/saved_state.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/memory/reference.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace mint::internal {

class GeneratorData : public ItemsIteratorData {
public:
	GeneratorData(std::size_t stack_size);
	GeneratorData(GeneratorData&&) = delete;
	GeneratorData(const GeneratorData& other);
	~GeneratorData() override = default;

	GeneratorData& operator=(GeneratorData&&) = delete;
	GeneratorData& operator=(const GeneratorData&) = delete;

	[[nodiscard]] std::unique_ptr<IteratorData> copy() override;
	void mark() override;

	[[nodiscard]] mint::Iterator::Context::Type get_type() const override;

	void yield(mint::Cursor& cursor, mint::Iterator::Context::value_type&& value,
	    Iterator::ResumeKind resume_kind) override;
	void next(mint::Cursor& cursor) override;

	void finalize(mint::Cursor& cursor) override;

private:
	enum class ExecutionMode : std::uint8_t {
		single_pass,
		interruptible
	};

	ExecutionMode _execution_mode = ExecutionMode::interruptible;
	std::unique_ptr<mint::SavedState> _state;

	std::vector<mint::Reference> _stored_stack;
	std::size_t _stack_size;
};

}

#endif // LIBMINT_MEMORY_BUILTIN_ITERATOR_GENERATOR_H
