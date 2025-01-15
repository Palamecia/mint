#ifndef ITERATOR_GENERATOR_H
#define ITERATOR_GENERATOR_H

#include "iterator_items.h"
#include "mint/ast/savedstate.h"

namespace mint::internal {

class generator_data : public items_data {
public:
	generator_data(size_t stack_size);
	generator_data(const generator_data &other) = delete;

	void mark() override;

	mint::Iterator::ctx_type::type getType() override;
	data *copy() const override;

	data_iterator *begin() override;

	mint::Iterator::ctx_type::value_type &back() override;

	void emplace(mint::Iterator::ctx_type::value_type &&value) override;
	void pop() override;

	void finalize() override;

private:
	enum ExecutionMode {
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
