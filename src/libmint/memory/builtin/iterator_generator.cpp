#include "iterator_generator.h"

#include "memory/builtin/iterator.h"
#include "memory/functiontool.h"
#include "scheduler/processor.h"
#include "system/assert.h"

#include <algorithm>
#include <iterator>

using namespace _mint_iterator;
using namespace mint;
using namespace std;

generator_data::generator_data(Cursor *cursor, size_t stack_size) :
	m_state(nullptr),
	m_cursor(cursor),
	m_stackSize(stack_size) {

}

generator_data::generator_data(const generator_data &other) :
	items_data(other),
	m_state(nullptr),
	m_cursor(other.m_cursor),
	m_stackSize(other.m_stackSize) {

}

void generator_data::mark() {
	items_data::mark();
	for (Reference &item : m_storedStack) {
		item.data()->mark();
	}
}

Iterator::ctx_type::type generator_data::getType() {
	return Iterator::ctx_type::generator;
}

_mint_iterator::data *generator_data::copy() const {
	return new generator_data(*this);
}

void generator_data::emplace_back(Iterator::ctx_type::value_type &&value) {

	items_data::emplace_back(move(value));

	switch (m_cursor->executionMode()) {
	case Cursor::single_pass:
		break;

	case Cursor::interruptible:
	case Cursor::resumed:
		move(next(m_cursor->stack().begin(), static_cast<vector<WeakReference>::difference_type>(m_stackSize)), m_cursor->stack().end(), back_inserter(m_storedStack));
		m_cursor->stack().resize(m_stackSize);
		m_state = m_cursor->interrupt();
		break;
	}
}

void generator_data::pop_front() {

	items_data::pop_front();

	if (m_state) {
		m_stackSize = m_cursor->stack().size();
		move(m_storedStack.begin(), m_storedStack.end(), back_inserter(m_cursor->stack()));
		m_storedStack.clear();
		m_cursor->restore(move(m_state));
	}
}

void generator_data::finalize() {

	if (m_state) {
		m_state->setResumeMode(Cursor::single_pass);
		move(m_storedStack.begin(), m_storedStack.end(), back_inserter(m_cursor->stack()));
		m_storedStack.clear();
		m_cursor->restore(move(m_state));
	}
}
