#ifndef ITERATOR_GENERATOR_H
#define ITERATOR_GENERATOR_H

#include "iterator_items.h"

#include "ast/savedstate.h"

namespace _mint_iterator {

class generator_data : public items_data {
public:
	generator_data(mint::Cursor *cursor, size_t stack_size);
	generator_data(const generator_data &other);

	void mark() override;

	mint::Iterator::ctx_type::type getType() override;
	data *copy() const override;

	data_iterator *begin() override;

	mint::Iterator::ctx_type::value_type &back() override;

	void emplace_back(mint::Iterator::ctx_type::value_type &&value) override;

	void pop_front() override;
	void pop_back() override;

	void finalize() override;

private:
	std::unique_ptr<mint::SavedState> m_state;
	mint::Cursor *m_cursor;

	std::vector<mint::WeakReference> m_storedStack;
	size_t m_stackSize;
};

}

#endif // ITERATOR_GENERATOR_H
