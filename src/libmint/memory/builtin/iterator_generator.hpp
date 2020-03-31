#ifndef ITERATOR_GENERATOR_HPP
#define ITERATOR_GENERATOR_HPP

#include "memory/builtin/iterator.h"
#include "memory/functiontool.h"
#include "scheduler/processor.h"
#include "iterator_items.hpp"
#include "system/assert.h"
#include "ast/savedstate.h"

#include <stack>

class generator_data : public items_data {
public:
	generator_data(mint::Cursor *cursor, size_t stack_size) :
		m_state(nullptr),
		m_cursor(cursor),
		m_stackSize(stack_size) {

	}

	void mark() override {
		items_data::mark();
		for (mint::SharedReference &item : m_storedStack) {
			item->data()->mark();
		}
	}

	mint::Iterator::ctx_type::type getType() override {
		return mint::Iterator::ctx_type::generator;
	}

	void emplace_back(mint::Iterator::ctx_type::value_type &value) override {

		items_data::emplace_back(value);

		switch (m_cursor->executionMode()) {
		case mint::Cursor::single_pass:
			break;

		case mint::Cursor::interruptible:
			while (m_stackSize < m_cursor->stack().size()) {
				m_storedStack.emplace_back(std::move(m_cursor->stack().back()));
				m_cursor->stack().pop_back();
			}

			m_state = m_cursor->interrupt();
			break;
		}
	}

	void pop_front() override {

		items_data::pop_front();

		if (m_state) {

			m_cursor->stack().pop_back();
			m_stackSize = m_cursor->stack().size();

			while (!m_storedStack.empty()) {
				m_cursor->stack().emplace_back(std::move(m_storedStack.back()));
				m_storedStack.pop_back();
			}

			m_cursor->restore(std::move(m_state));
			m_state = nullptr;
		}
	}

	void finalize() override {

		if (m_state) {

			m_cursor->stack().pop_back();

			while (!m_storedStack.empty()) {
				m_cursor->stack().emplace_back(std::move(m_storedStack.back()));
				m_storedStack.pop_back();
			}

			m_state->setResumeMode(mint::Cursor::single_pass);
			m_cursor->restore(std::move(m_state));
			m_state = nullptr;
		}
	}

private:
	std::unique_ptr<mint::SavedState> m_state;
	mint::Cursor *m_cursor;

	std::vector<mint::SharedReference> m_storedStack;
	size_t m_stackSize;
};

#endif // ITERATOR_GENERATOR_HPP
