#include "iterator_generator.h"
#include "memory/builtin/iterator.h"
#include "scheduler/scheduler.h"

#include <algorithm>
#include <iterator>

using namespace _mint_iterator;
using namespace mint;
using namespace std;

generator_data::generator_data(size_t stack_size) :
	m_state(nullptr),
	m_stackSize(stack_size) {

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
	const_cast<generator_data *>(this)->generator_data::finalize();
	return new items_data(*this);
}

data_iterator *generator_data::begin() {
	generator_data::finalize();
	return items_data::begin();
}

Iterator::ctx_type::value_type &generator_data::back() {
	generator_data::finalize();
	return items_data::back();
}

void generator_data::emplace_next(Iterator::ctx_type::value_type &&value) {

	items_data::emplace_next(std::forward<Reference>(value));

	switch (m_executionMode) {
	case single_pass:
		break;

	case interruptible:
		Cursor *cursor = Scheduler::instance()->currentProcess()->cursor();
		move(std::next(cursor->stack().begin(), static_cast<vector<WeakReference>::difference_type>(m_stackSize)), cursor->stack().end(), back_inserter(m_storedStack));
		cursor->stack().resize(m_stackSize);
		m_state = cursor->interrupt();
		break;
	}
}

void generator_data::pop_next() {

	items_data::pop_next();

	if (m_state) {
		Cursor *cursor = Scheduler::instance()->currentProcess()->cursor();
		m_stackSize = cursor->stack().size();
		move(m_storedStack.begin(), m_storedStack.end(), back_inserter(cursor->stack()));
		m_storedStack.clear();
		if (cursor->isInBuiltin()) {
			Scheduler::instance()->createGenerator(std::move(m_state));
		}
		else {
			cursor->restore(std::move(m_state));
		}
	}
}

void generator_data::pop_back() {
	generator_data::finalize();
	items_data::pop_back();
}

void generator_data::finalize() {
	if (m_state) {
		m_executionMode = single_pass;
		Cursor *cursor = Scheduler::instance()->currentProcess()->cursor();
		move(m_storedStack.begin(), m_storedStack.end(), back_inserter(cursor->stack()));
		m_storedStack.clear();
		Scheduler::instance()->createGenerator(std::move(m_state));
	}
}
