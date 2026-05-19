#ifndef LIBMINT_MEMORY_BUILTIN_ITERATOR_GENERATOR_H
#define LIBMINT_MEMORY_BUILTIN_ITERATOR_GENERATOR_H

#include "iterator_items.h"
#include "iterator_p.h"
#include "mint/ast/cursor.h"
#include "mint/ast/savedstate.h"
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
