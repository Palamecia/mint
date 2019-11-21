#ifndef ITERATOR_GENERATOR_HPP
#define ITERATOR_GENERATOR_HPP

#include "memory/builtin/iterator.h"
#include "memory/functiontool.h"
#include "scheduler/processor.h"
#include "iterator_items.hpp"
#include "system/assert.h"

#include <stack>

class generator_data : public items_data {
public:
	generator_data(mint::Cursor *cursor, size_t stack_size) :
		m_context(nullptr),
		m_cursor(cursor),
		m_stackSize(stack_size) {

    }

	~generator_data() override {
		delete m_context;
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
				m_storedStack.emplace(m_cursor->stack().back());
				m_cursor->stack().pop_back();
			}

			m_context = m_cursor->interrupt();
			break;
		}
	}

	void pop_front() override {

		items_data::pop_front();

		if (m_context) {

			m_cursor->stack().pop_back();
			m_stackSize = m_cursor->stack().size();

			while (!m_storedStack.empty()) {
				m_cursor->stack().emplace_back(m_storedStack.top());
				m_storedStack.pop();
			}

			m_cursor->restore(m_context);
			m_context = nullptr;
		}
	}

	void finalize() override {

		if (m_context) {

			m_cursor->stack().pop_back();

			while (!m_storedStack.empty()) {
				m_cursor->stack().emplace_back(m_storedStack.top());
				m_storedStack.pop();
			}

			m_context->executionMode = mint::Cursor::single_pass;
			m_cursor->restore(m_context);
			m_context = nullptr;
		}
	}

private:
	mint::Cursor::Context *m_context;
	mint::Cursor *m_cursor;

	std::stack<mint::SharedReference> m_storedStack;
	size_t m_stackSize;
};

#endif // ITERATOR_GENERATOR_HPP
