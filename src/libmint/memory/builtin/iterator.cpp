#include "memory/builtin/iterator.h"
#include "memory/functiontool.h"
#include "memory/memorytool.h"
#include "ast/abstractsyntaxtree.h"
#include "ast/cursor.h"
#include "system/assert.h"
#include "system/error.h"
#include "iterator_items.hpp"
#include "iterator_range.hpp"

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
	Reference *iterator = Reference::create(Reference::alloc<Iterator>(begin, begin < end ? end + 1 : end - 1));
	iterator->data<Iterator>()->construct();
	return iterator;
}

Reference *Iterator::fromExclusiveRange(double begin, double end) {
	Reference *iterator = Reference::create(Reference::alloc<Iterator>(begin, end));
	iterator->data<Iterator>()->construct();
	return iterator;
}

IteratorClass::IteratorClass() : Class("iterator", Class::iterator) {

	createBuiltinMember(":=", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							Reference &other = *cursor->stack().at(base);
							Reference &self = *cursor->stack().at(base - 1);

							SharedReference it = SharedReference::unique(Reference::create(iterator_init(other)));

							for (SharedReference &item : self.data<Iterator>()->ctx) {
								if ((item->flags() & Reference::const_address) && (item->data()->format != Data::fmt_none)) {
									error("invalid modification of constant reference");
								}
								if (SharedReference it_value = iterator_next(it->data<Iterator>())) {
									item->move(*it_value);
								}
								else {
									break;
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

	createBuiltinMember("value", 1, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							Reference &self = *cursor->stack().back();

							if (SharedReference result = iterator_get(self.data<Iterator>())) {
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
							SharedReference result = create_array({});

							for (SharedReference &item : self.data<Iterator>()->ctx) {
								array_append(result->data<Array>(), SharedReference::unique(new Reference(*item)));
							}

							cursor->stack().pop_back();
							cursor->stack().push_back(result);
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

Iterator::ctx_type::iterator::iterator(data *data) : m_data(data) {

}

Iterator::ctx_type::type Iterator::ctx_type::getType() const {
	return m_data->getType();
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
	m_data(new items_data()) {

}

Iterator::ctx_type::ctx_type(double begin, double end) :
	m_data(new range_data(begin, end)) {

}

Iterator::ctx_type::~ctx_type() {
	delete m_data;
}

Iterator::ctx_type::iterator Iterator::ctx_type::begin() const {
	return iterator(m_data->begin());
}

Iterator::ctx_type::iterator Iterator::ctx_type::end() const {
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

void Iterator::ctx_type::clear() {
	m_data->clear();
}

size_t Iterator::ctx_type::size() const {
	return m_data->size();
}

bool Iterator::ctx_type::empty() const {
	return m_data->empty();
}
