#include "memory/builtin/iterator.h"
#include "memory/functiontool.h"
#include "memory/memorytool.h"
#include "ast/abstractsyntaxtree.h"
#include "ast/cursor.h"
#include "system/assert.h"
#include "system/error.h"
#include "iterator_items.hpp"
#include "iterator_range.hpp"
#include "iterator_generator.hpp"

using namespace mint;
using namespace std;

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

Iterator::Iterator(Cursor *cursor, size_t stack_size) :
	Object((IteratorClass::instance())),
	ctx(cursor, stack_size) {

}

void Iterator::mark() {
	if (!markedBit()) {
		Object::mark();
		ctx.mark();
	}
}

Reference *Iterator::fromInclusiveRange(double begin, double end) {
	Reference *iterator = StrongReference::create(Reference::alloc<Iterator>(begin, begin < end ? end + 1 : end - 1));
	iterator->data<Iterator>()->construct();
	return iterator;
}

Reference *Iterator::fromExclusiveRange(double begin, double end) {
	Reference *iterator = StrongReference::create(Reference::alloc<Iterator>(begin, end));
	iterator->data<Iterator>()->construct();
	return iterator;
}

IteratorClass::IteratorClass() : Class("iterator", Class::iterator) {

	createBuiltinMember(":=", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							size_t base = get_stack_base(cursor);

							SharedReference &other = cursor->stack().at(base);
							SharedReference &self = cursor->stack().at(base - 1);

							SharedReference it = SharedReference::unique(StrongReference::create(iterator_init(other)));

							for (SharedReference &item : self->data<Iterator>()->ctx) {
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

							SharedReference self = move(cursor->stack().back());

							if (self->data<Iterator>()->ctx.empty()) {
								cursor->stack().pop_back();
								cursor->stack().emplace_back(SharedReference::unique(StrongReference::create<None>()));
							}
							else {
								cursor->stack().pop_back();
								cursor->stack().emplace_back(move(self->data<Iterator>()->ctx.front()));
								self->data<Iterator>()->ctx.pop_front();
							}
						}));

	createBuiltinMember("value", 1, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {

							SharedReference self = move(cursor->stack().back());

							if (SharedReference result = iterator_get(self->data<Iterator>())) {
								cursor->stack().pop_back();
								cursor->stack().emplace_back(move(result));
							}
							else {
								cursor->stack().pop_back();
								cursor->stack().emplace_back(SharedReference::unique(StrongReference::create<None>()));
							}
						}));

	createBuiltinMember("isEmpty", 1, AbstractSyntaxTree::createBuiltinMethode(metatype(), [] (Cursor *cursor) {
							SharedReference self = move(cursor->stack().back());
							cursor->stack().back() = create_boolean(self->data<Iterator>()->ctx.empty());
						}));

	createBuiltinMember("each", 2, AbstractSyntaxTree::createBuiltinMethode(metatype(),
																			"	def (self, func) {\n"
																			"		for item in self {\n"
																			"			func(item)\n"
																			"		}\n"
																			"	}\n"));

	/// \todo register operator overloads
}

Iterator::ctx_type::iterator::iterator(data *data) : m_data(data) {

}

void Iterator::ctx_type::mark() {
	m_data->mark();
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

Iterator::ctx_type::ctx_type(Cursor *cursor, size_t stack_size) :
	m_data(new generator_data(cursor, stack_size)) {

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

Iterator::ctx_type::value_type &Iterator::ctx_type::front() {
	return m_data->front();
}

Iterator::ctx_type::value_type &Iterator::ctx_type::back() {
	return m_data->back();
}

void Iterator::ctx_type::emplace_front(value_type &value) {
	m_data->emplace_front(value);
}

void Iterator::ctx_type::emplace_back(value_type &value) {
	m_data->emplace_back(value);
}

void Iterator::ctx_type::pop_front() {
	m_data->pop_front();
}

void Iterator::ctx_type::pop_back() {
	m_data->pop_back();
}

void Iterator::ctx_type::finalize() {
	m_data->finalize();
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
