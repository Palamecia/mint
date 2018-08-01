#include "memory/builtin/iterator.h"
#include "memory/functiontool.h"
#include "memory/memorytool.h"
#include "ast/abstractsyntaxtree.h"
#include "ast/cursor.h"
#include "system/assert.h"
#include "system/error.h"

using namespace mint;

IteratorClass *IteratorClass::instance() {

	static IteratorClass g_instance;
	return &g_instance;
}

Iterator::Iterator() : Object(IteratorClass::instance()) {

}

Iterator::Iterator(double begin, double end) :
	Object(IteratorClass::instance()),
	ctx(begin, end) {

}

Reference *Iterator::fromInclusiveRange(double begin, double end) {
	Reference *iterator =  new Reference(Reference::const_address | Reference::const_value, Reference::alloc<Iterator>(begin, begin < end ? end + 1 : end - 1));
	iterator->data<Iterator>()->construct();
	return iterator;
}

Reference *Iterator::fromExclusiveRange(double begin, double end) {
	Reference *iterator =  new Reference(Reference::const_address | Reference::const_value, Reference::alloc<Iterator>(begin, end));
	iterator->data<Iterator>()->construct();
	return iterator;
}

IteratorClass::IteratorClass() : Class("iterator", Class::iterator) {

	createBuiltinMember(":=", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							Reference &other = *cursor->stack().at(base);
							Reference &self = *cursor->stack().at(base - 1);

							Iterator it;
							iterator_init(&it, other);
							for (SharedReference &item : self.data<Iterator>()->ctx) {
								if ((item->flags() & Reference::const_address) && (item->data()->format != Data::fmt_none)) {
									error("invalid modification of constant reference");
								}
								if (it.ctx.empty()) {
									item->move(Reference(Reference::standard, Reference::alloc<None>()));
								}
								else {
									item->move(*it.ctx.front());
									it.ctx.pop_front();
								}
							}

							cursor->stack().pop_back();
						}));

	createBuiltinMember("next", 1, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							Reference &self = *cursor->stack().back();

							if (SharedReference result = iterator_next(self.data<Iterator>())) {
								cursor->stack().pop_back();
								cursor->stack().push_back(result);
							}
							else {
								cursor->stack().pop_back();
								cursor->stack().push_back(SharedReference::unique(Reference::create<None>()));
							}
						}));

	createBuiltinMember("values", 1, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							Reference &self = *cursor->stack().back();
							Reference *result = Reference::create<Array>();

							result->data<Object>()->construct();
							for (SharedReference &item : self.data<Iterator>()->ctx) {
								array_append(result->data<Array>(), SharedReference::unique(new Reference(*item)));
							}

							cursor->stack().pop_back();
							cursor->stack().push_back(SharedReference::unique(result));
						}));

	createBuiltinMember("size", 1, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							Reference &self = *cursor->stack().back();
							Reference *result = Reference::create<Number>();

							result->data<Number>()->value = self.data<Iterator>()->ctx.size();

							cursor->stack().pop_back();
							cursor->stack().push_back(SharedReference::unique(result));
						}));

	/// \todo register operator overloads
}

class items_data : public Iterator::ctx_type::data {
public:
	class iterator : public Iterator::ctx_type::iterator::data {
	public:
		iterator(const std::deque<SharedReference>::iterator &impl) : m_impl(impl) {

		}

		bool compare(data *other) const override {
			if (iterator *it = dynamic_cast<iterator *>(other)) {
				return m_impl != it->m_impl;
			}
			return true;
		}

		Iterator::ctx_type::value_type &get() override {
			return *m_impl;
		}

		data *next() override {
			return new iterator(++m_impl);
		}

	private:
		std::deque<SharedReference>::iterator m_impl;
	};

	Iterator::ctx_type::iterator::data *begin() override {
		return new iterator(m_items.begin());
	}

	Iterator::ctx_type::iterator::data *end() override {
		return new iterator(m_items.end());
	}

	const Iterator::ctx_type::value_type &front() const override {
		return m_items.front();
	}

	const Iterator::ctx_type::value_type &back() const override {
		return m_items.back();
	}

	void push_front(const Iterator::ctx_type::value_type &value) override {
		m_items.push_front(value);
	}

	void push_back(const Iterator::ctx_type::value_type &value) override {
		m_items.push_back(value);
	}

	void pop_front() override {
		m_items.pop_front();
	}

	void pop_back() override {
		m_items.pop_back();
	}

	void clear() override {
		m_items.clear();
	}

	size_t size() const override {
		return m_items.size();
	}

	bool empty() const override {
		return m_items.empty();
	}

private:
	std::deque<SharedReference> m_items;
};

class range_data : public Iterator::ctx_type::data {
public:
	class iterator : public Iterator::ctx_type::iterator::data {
	public:
		iterator(double value, bool ascending) :
			m_data(create_number(value)),
			m_ascending(ascending),
			m_value(value) {

		}

		bool compare(data *other) const override {
			if (iterator *it = dynamic_cast<iterator *>(other)) {
				return m_value != it->m_value;
			}
			return true;
		}

		Iterator::ctx_type::value_type &get() override {
			return m_data;
		}

		data *next() override {
			m_data = create_number(m_ascending ? ++m_value : --m_value);
			return new iterator(m_value, m_ascending);
		}

	private:
		SharedReference m_data;
		bool m_ascending;
		double m_value;
	};

	range_data(double begin, double end) :
		m_begin(begin), m_end(end),
		m_front(create_number(begin)), m_back(create_number(end)) {

	}

	Iterator::ctx_type::iterator::data *begin() override {
		return new iterator(m_begin, m_begin < m_end);
	}

	Iterator::ctx_type::iterator::data *end() override {
		return new iterator(m_end, m_begin < m_end);
	}

	const Iterator::ctx_type::value_type &front() const override {
		return m_front;
	}

	const Iterator::ctx_type::value_type &back() const override {
		return m_back;
	}

	void push_front(const Iterator::ctx_type::value_type &value) override {
		((void)value);
		assert(false);
	}

	void push_back(const Iterator::ctx_type::value_type &value) override {
		((void)value);
		assert(false);
	}

	void pop_front() override {
		m_begin < m_end ? m_begin++ : m_begin--;
		m_front = create_number(m_begin);
	}

	void pop_back() override {
		m_begin < m_end ? m_end-- : m_end++;
		m_back = create_number(m_begin < m_end ? m_end - 1 : m_end + 1);
	}

	void clear() override {
		m_begin = m_end = 0;
	}

	size_t size() const override {
		return m_begin < m_end ? static_cast<size_t>(m_end - m_begin) : static_cast<size_t>(m_begin - m_end);
	}

	bool empty() const override {
		return m_begin == m_end;
	}

private:
	double m_begin;
	double m_end;

	SharedReference m_front;
	SharedReference m_back;
};

Iterator::ctx_type::iterator::iterator(data *data) : m_data(data) {

}

bool Iterator::ctx_type::iterator::operator !=(const iterator &other) const {
	return m_data->compare(other.m_data.get());
}

Iterator::ctx_type::value_type &Iterator::ctx_type::iterator::operator *() {
	return m_data->get();
}

Iterator::ctx_type::iterator Iterator::ctx_type::iterator::operator ++() {
	return iterator(m_data->next());
}

Iterator::ctx_type::ctx_type() :
	m_data(new items_data) {

}

Iterator::ctx_type::ctx_type(double begin, double end) :
	m_data(new range_data(begin, end)) {

}

Iterator::ctx_type::~ctx_type() {
	delete m_data;
}

Iterator::ctx_type::iterator Iterator::ctx_type::begin() {
	return iterator(m_data->begin());
}

Iterator::ctx_type::iterator Iterator::ctx_type::end() {
	return iterator(m_data->end());
}

const Iterator::ctx_type::value_type &Iterator::ctx_type::front() const {
	return m_data->front();
}

const Iterator::ctx_type::value_type &Iterator::ctx_type::back() const {
	return m_data->back();
}

void Iterator::ctx_type::push_front(const value_type &value) {
	m_data->push_front(value);
}

void Iterator::ctx_type::push_back(const value_type &value) {
	m_data->push_back(value);
}

void Iterator::ctx_type::pop_front() {
	m_data->pop_front();
}

void Iterator::ctx_type::pop_back() {
	m_data->pop_back();
}

void Iterator::ctx_type::swap(ctx_type &other) {
	std::swap(m_data, other.m_data);
}

void Iterator::ctx_type::clear() {
	m_data->clear();
}

size_t Iterator::ctx_type::size() const {
	return m_data->size();
}

bool Iterator::ctx_type::empty() const {
	return m_data->empty();
}
