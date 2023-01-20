#include "memory/builtin/iterator.h"
#include "memory/algorithm.hpp"
#include "ast/abstractsyntaxtree.h"
#include "ast/cursor.h"
#include "system/error.h"

#include "iterator_items.h"
#include "iterator_range.h"
#include "iterator_generator.h"

using namespace mint;
using namespace std;

IteratorClass *IteratorClass::instance() {
	return GlobalData::instance()->builtin<IteratorClass>(Class::iterator);
}

Iterator::Iterator() : Object(IteratorClass::instance()),
	ctx(new _mint_iterator::items_data) {

}

Iterator::Iterator(Reference &ref) : Object(IteratorClass::instance()),
	ctx(new _mint_iterator::items_data(ref)) {

}

Iterator::Iterator(const Iterator &other) : Object(IteratorClass::instance()),
	ctx(other.ctx) {

}

Iterator::Iterator(double begin, double end) : Object(IteratorClass::instance()),
	ctx(new _mint_iterator::range_data(begin, end)) {

}

Iterator::Iterator(Cursor *cursor, size_t stack_size) : Object((IteratorClass::instance())),
	ctx(new _mint_iterator::generator_data(cursor, stack_size)) {

}

void Iterator::mark() {
	if (!markedBit()) {
		Object::mark();
		ctx.mark();
	}
}

WeakReference Iterator::fromInclusiveRange(double begin, double end) {
	WeakReference iterator = WeakReference::create(Reference::alloc<Iterator>(begin, begin <= end ? end + 1 : end - 1));
	iterator.data<Iterator>()->construct();
	return iterator;
}

WeakReference Iterator::fromExclusiveRange(double begin, double end) {
	WeakReference iterator = WeakReference::create(Reference::alloc<Iterator>(begin, end));
	iterator.data<Iterator>()->construct();
	return iterator;
}

IteratorClass::IteratorClass() : Class("iterator", Class::iterator) {

	AbstractSyntaxTree *ast = AbstractSyntaxTree::instance();

	createBuiltinMember(copy_operator, ast->createBuiltinMethode(this, 2, [] (Cursor *cursor) {

							const size_t base = get_stack_base(cursor);

							Reference &other = load_from_stack(cursor, base);
							Reference &self = load_from_stack(cursor, base - 1);
							Iterator::ctx_type::iterator it = self.data<Iterator>()->ctx.begin();
							const Iterator::ctx_type::iterator end = self.data<Iterator>()->ctx.end();

							for_each_if(other, [&it, &end] (const Reference &item) -> bool {
								if (it != end) {
									if (UNLIKELY((it->flags() & Reference::const_address) && (it->data()->format != Data::fmt_none))) {
										error("invalid modification of constant reference");
									}

									it->move(item);
									++it;
									return true;
								}

								return false;
							});

							cursor->stack().pop_back();
						}));

	createBuiltinMember("next", ast->createBuiltinMethode(this, 1, [] (Cursor *cursor) {

							WeakReference self = std::move(cursor->stack().back());

							if (!self.data<Iterator>()->ctx.empty()) {
								cursor->stack().back() = std::move(self.data<Iterator>()->ctx.front());
								// The next call can iterrupt the current context,
								// so the value must be pushed first
								self.data<Iterator>()->ctx.pop_front();
							}
							else {
								cursor->stack().back() = WeakReference::create<None>();
							}
						}));

	createBuiltinMember("value", ast->createBuiltinMethode(this, 1, [] (Cursor *cursor) {
							if (optional<WeakReference> &&result = iterator_get(cursor->stack().back().data<Iterator>())) {
								cursor->stack().back() = std::move(*result);
							}
							else {
								cursor->stack().back() = WeakReference::create<None>();
							}
						}));

	createBuiltinMember("isEmpty", ast->createBuiltinMethode(this, 1, [] (Cursor *cursor) {
							cursor->stack().back() = WeakReference::create<Boolean>(cursor->stack().back().data<Iterator>()->ctx.empty());
						}));

	createBuiltinMember("each", ast->createBuiltinMethode(this, 2,
																			"	def (const self, const func) {\n"
																			"		for item in self {\n"
																			"			func(item)\n"
																			"		}\n"
																			"	}\n"));

	/// \todo register operator overloads
}

Iterator::ctx_type::iterator::iterator(_mint_iterator::data_iterator *data) :
	m_data(data) {

}

Iterator::ctx_type::iterator::iterator(const iterator &other) :
	m_data(other.m_data->copy()) {

}

Iterator::ctx_type::iterator::iterator(iterator &&other) :
	m_data(std::forward<unique_ptr<_mint_iterator::data_iterator>>(other.m_data)) {

}

Iterator::ctx_type::iterator::~iterator() {

}

Iterator::ctx_type::iterator &Iterator::ctx_type::iterator::operator =(const iterator &other) {
	m_data.reset(other.m_data->copy());
	return *this;
}

Iterator::ctx_type::iterator &Iterator::ctx_type::iterator::operator =(iterator &&other) {
	swap(m_data, other.m_data);
	return *this;
}

void Iterator::ctx_type::mark() {
	m_data->mark();
}

Iterator::ctx_type::type Iterator::ctx_type::getType() const {
	return m_data->getType();
}

bool Iterator::ctx_type::iterator::operator ==(const iterator &other) const {
	return !m_data->compare(other.m_data.get());
}

bool Iterator::ctx_type::iterator::operator !=(const iterator &other) const {
	return m_data->compare(other.m_data.get());
}

Iterator::ctx_type::value_type &Iterator::ctx_type::iterator::operator *() {
	return m_data->get();
}

Iterator::ctx_type::value_type *Iterator::ctx_type::iterator::operator ->() {
	return &m_data->get();
}

Iterator::ctx_type::iterator Iterator::ctx_type::iterator::operator ++(int) {
	iterator tmp(m_data->copy());
	m_data->next();
	return tmp;
}

Iterator::ctx_type::iterator &Iterator::ctx_type::iterator::operator ++() {
	m_data->next();
	return *this;
}

Iterator::ctx_type::ctx_type(_mint_iterator::data *data) :
	m_data(data) {

}

Iterator::ctx_type::ctx_type(const ctx_type &other) :
	m_data(other.m_data->copy()) {

}

Iterator::ctx_type::~ctx_type() {

}

Iterator::ctx_type &Iterator::ctx_type::operator =(const ctx_type &other) {
	m_data.reset(other.m_data->copy());
	return *this;
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

void Iterator::ctx_type::emplace_front(value_type &&value) {
	m_data->emplace_front(std::forward<Reference>(value));
}

void Iterator::ctx_type::emplace_back(value_type &&value) {
	m_data->emplace_back(std::forward<Reference>(value));
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

void mint::iterator_init_from_stack(Cursor *cursor, size_t length) {

	WeakReference it(Reference::const_address, Reference::alloc<Iterator>());
	it.data<Iterator>()->construct();

	while (length--) {
		iterator_set_next(it.data<Iterator>(), std::move(cursor->stack().back()));
		cursor->stack().pop_back();
	}

	cursor->stack().emplace_back(std::forward<Reference>(it));
}

void mint::iterator_finalize(Cursor *cursor, int signature, int limit) {

	size_t base = get_stack_base(cursor);

	if (signature < 0) {

		Reference &reference = load_from_stack(cursor, base);
		signature = -signature;
		--base;

		for (Reference &item : reference.data<Iterator>()->ctx) {
			if (item.data()->format == Data::fmt_object && item.data<Object>()->metadata->metatype() == Class::iterator) {
				item.data<Iterator>()->ctx.finalize();
			}
		}
	}

	while (signature-- > limit) {
		Reference &reference = load_from_stack(cursor, base - static_cast<size_t>(signature));
		if (reference.data()->format == Data::fmt_object && reference.data<Object>()->metadata->metatype() == Class::iterator) {
			reference.data<Iterator>()->ctx.finalize();
		}
	}
}

Iterator *mint::iterator_init(Reference &ref) {

	if (ref.data()->format == Data::fmt_object && ref.data<Object>()->metadata->metatype() == Class::iterator) {
		return ref.data<Iterator>();
	}

	Iterator *iterator = new Iterator(ref);
	iterator->construct();
	return iterator;
}

Iterator *mint::iterator_init(Reference &&ref) {
	return iterator_init(static_cast<Reference &>(ref));
}

void mint::iterator_insert(Iterator *iterator, Reference &&item) {
	iterator->ctx.emplace_back(std::forward<Reference>(item));
}

void mint::iterator_set_next(Iterator *iterator, Reference &&item) {
	iterator->ctx.emplace_front(std::forward<Reference>(item));
}

optional<WeakReference> mint::iterator_get(Iterator *iterator) {

	if (!iterator->ctx.empty()) {
		return optional<WeakReference>(WeakReference::share(iterator->ctx.front()));
	}

	return nullopt;
}

optional<WeakReference> mint::iterator_next(Iterator *iterator) {

	if (!iterator->ctx.empty()) {
		optional<WeakReference> item(WeakReference::share(iterator->ctx.front()));
		iterator->ctx.pop_front();
		return item;
	}

	return nullopt;
}
