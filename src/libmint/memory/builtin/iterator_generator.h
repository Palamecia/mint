#ifndef ITERATOR_GENERATOR_H
#define ITERATOR_GENERATOR_H

#include "iterator_items.h"
#include "mint/ast/savedstate.h"
#include <cstdint>

namespace mint::internal {

class GeneratorData : public ItemsIteratorData {
public:
	GeneratorData(size_t stack_size);
	GeneratorData(GeneratorData &&) = delete;
	GeneratorData(const GeneratorData &other);
	~GeneratorData() override = default;

	GeneratorData &operator=(GeneratorData &&) = delete;
	GeneratorData &operator=(const GeneratorData &) = delete;

	[[nodiscard]] IteratorData *copy() override;
	void mark() override;

	[[nodiscard]] mint::Iterator::Context::Type get_type() const override;
	[[nodiscard]] mint::Iterator::Context::value_type &last() override;

	void yield(mint::Iterator::Context::value_type &&value) override;
	void next() override;

	void finalize() override;

private:
	enum ExecutionMode : std::uint8_t {
		SINGLE_PASS,
		INTERRUPTIBLE
	};

	ExecutionMode m_execution_mode = INTERRUPTIBLE;
	std::unique_ptr<mint::SavedState> m_state;

	std::vector<mint::WeakReference> m_stored_stack;
	size_t m_stack_size;
};
}

#endif // ITERATOR_GENERATOR_H
